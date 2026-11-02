"""Sionna RT side of the channel emulator drawn in web/spark/brief-uav.html.

Ray-traces the UAV scenario (a UAV flying through an urban street canyon between two rooftop gNBs), turns the path
gains a and delays tau into the live-taps message of plan.md section 6, and samples them into the discrete taps the
emulator filters with. One module for every user, so what was verified is what runs:

- spark/rt/verify_uav_rt.ipynb checks it, on a Colab GPU or on any CPU;
- the live-taps publisher on the Spark calls trace_route(), messages() and publish();
- the emulator's Swap taps and Sample steps use subscriber(), drain(), TapSwitch and sample_taps().

Link i is gNB i -> UAV, which is cell i's channel. The emulator applies the same taps to the uplink.

The route is traced once, saved with Table.save(), and replayed from the file: the emulator gets the same tap
sequence on every run, and the Spark's slot loop never waits on a trace.
"""
from __future__ import annotations

import heapq
import json
import time
from dataclasses import dataclass, field

import numpy as np
import torch

FS_HZ = 61_440_000        # 40 MHz at 30 kHz SCS: a 2048-point FFT
SLOT = 30_720             # samples per 0.5 ms slot
L_MIN, L_MAX = -6, 9      # 16 taps; l_min = -6 keeps the sinc precursor, at a fixed 6-sample delay
C = 299_792_458.0
VERSION = 1               # of the live-taps message


@dataclass(frozen=True)
class Scenario:
    """The UAV scenario, in the coordinates of Sionna RT's built-in munich scene (metres)."""
    scene: str = "munich"
    carrier_hz: float = 3.5e9
    # On the roof edges either side of the canyon's far end, 3 m above the roof, as the slide draws them.
    gnbs: tuple = ((55.0, 338.0, 18.83), (74.0, 339.0, 25.28))
    # In from the square, then weaving up the canyon below the roofs on both sides (12 to 28 m) towards the gNBs.
    waypoints: tuple = ((4.0, 186.0, 8.0), (14.0, 200.0, 8.0), (26.4, 227.2, 9.0), (30.6, 252.8, 8.0),
                        (45.4, 275.2, 10.0), (48.6, 300.8, 9.0), (62.0, 336.0, 8.0))
    speed_mps: float = 10.0
    dt_s: float = 0.05            # one trace per 50 ms of stream time: 0.5 m at 10 m/s, every 100 slots
    max_depth: int = 5
    samples_per_src: int = 1_000_000
    max_paths_per_src: int = 1_000_000    # per receiver in the call; the notebook checks nothing is dropped at 4x
    seed: int = 42
    max_drop_db: float = 45.0     # deepest fade the published taps show, as in TOSSI's publisher


def build_scene(sc: Scenario = Scenario()):
    """The scene with both gNBs in it and no UAV yet."""
    import sionna.rt as rt
    from sionna.rt import PlanarArray, Transmitter

    scene = rt.load_scene(getattr(rt.scene, sc.scene), merge_shapes=True)
    scene.frequency = sc.carrier_hz
    # One isotropic, vertically polarised antenna at each end, so the gains are the channel's and not an array's.
    scene.tx_array = PlanarArray(num_rows=1, num_cols=1, pattern="iso", polarization="V")
    scene.rx_array = PlanarArray(num_rows=1, num_cols=1, pattern="iso", polarization="V")
    for i, p in enumerate(sc.gnbs, 1):
        scene.add(Transmitter(f"gnb{i}", position=[float(v) for v in p]))
    return scene


def trajectory(sc: Scenario = Scenario()):
    """(t, sample_index, pos) for every trace along the route, at constant speed.

    Time is stream time, sample_index / FS_HZ, never the wall clock: ZMQ has no clock, so the emulator may run slower
    than real time, and the UAV must still be where the samples say it is.
    """
    w = np.asarray(sc.waypoints, float)
    arc = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(w, axis=0), axis=1))])
    step = round(sc.dt_s * FS_HZ)
    sample_index = np.arange(int(arc[-1] / sc.speed_mps * FS_HZ / step) + 1, dtype=np.int64) * step
    t = sample_index / FS_HZ
    s = np.minimum(t * sc.speed_mps, arc[-1])
    pos = np.stack([np.interp(s, arc, w[:, j]) for j in range(3)], axis=1)
    return t, sample_index, pos


@dataclass
class Table:
    """Every trace along the route. P is the most paths any link had; shorter links are padded and marked invalid."""
    t: np.ndarray             # [K] s
    sample_index: np.ndarray  # [K] int64
    pos: np.ndarray           # [K, 3] m
    a: np.ndarray             # [K, 2, P] complex64, raw path coefficients
    tau: np.ndarray           # [K, 2, P] float32, absolute delays in s
    valid: np.ndarray         # [K, 2, P] bool
    los: np.ndarray           # [K, 2, P] bool, the path has no interaction
    meta: dict = field(default_factory=dict)

    def gain_db(self):
        """[K, 2] total path gain per link; -inf where a link has no path."""
        with np.errstate(divide="ignore"):
            return 10 * np.log10((np.abs(self.a) ** 2 * self.valid).sum(-1))

    def save(self, path):
        np.savez_compressed(path, t=self.t, sample_index=self.sample_index, pos=self.pos, a=self.a, tau=self.tau,
                            valid=self.valid, los=self.los, meta=json.dumps(self.meta))

    @classmethod
    def load(cls, path):
        z = np.load(path)
        return cls(z["t"], z["sample_index"], z["pos"], z["a"], z["tau"], z["valid"], z["los"],
                   json.loads(str(z["meta"])))


def trace_paths(scene, pos, sc: Scenario = Scenario(), solver=None):
    """One PathSolver call with a receiver at each position: (a, tau, valid, los), each [N, 2, P] numpy.

    Past max_num_paths_per_src, Sionna drops candidate paths without a warning, and the candidates grow with the
    receivers in the call: 64 receivers at the default cap lost up to 4.6 dB out of line of sight. So the cap scales
    with the receivers, and its preallocated memory with it (29 GB for 64), which is why trace_route() traces one
    position at a time, as the live publisher would.
    """
    from sionna.rt import PathSolver, Receiver

    names = [f"uav{i}" for i in range(len(pos))]
    for n, p in zip(names, pos):
        scene.add(Receiver(n, position=[float(v) for v in p]))
    try:
        paths = (solver or PathSolver())(scene, max_depth=sc.max_depth, samples_per_src=sc.samples_per_src,
                                         max_num_paths_per_src=sc.max_paths_per_src * len(pos), seed=sc.seed)
        # The results are indexed by receiver and transmitter in the order they were added; check it, not trust it.
        assert np.allclose(np.asarray(paths.targets.numpy()).reshape(3, -1).T, pos, atol=1e-3)
        a, tau = paths.cir(normalize_delays=False, out_type="numpy")
        valid = np.asarray(paths.valid.numpy(), bool)
        inter = np.asarray(paths.interactions.numpy()).astype(np.int64)     # [depth, rx, tx, path]
    finally:
        for n in names:
            scene.remove(n)
    a = a[:, 0, :, 0, :, 0].astype(np.complex64)                            # one antenna each end, one time step
    return a, tau.astype(np.float32), valid, valid & (inter == 0).all(0)


def trace_route(scene, sc: Scenario = Scenario(), chunk: int = 1) -> Table:
    import mitsuba as mi
    import sionna.rt as rt
    from sionna.rt import PathSolver

    t, sample_index, pos = trajectory(sc)
    solver, parts, t0 = PathSolver(), [], time.perf_counter()
    for c in range(0, len(pos), chunk):
        parts.append(trace_paths(scene, pos[c:c + chunk], sc, solver))
    seconds = time.perf_counter() - t0
    P = max(p[0].shape[-1] for p in parts)
    pad = lambda x, v: np.concatenate([np.pad(p, ((0, 0), (0, 0), (0, P - p.shape[-1])), constant_values=v)
                                       for p in x])
    a, tau, valid, los = (pad([p[i] for p in parts], v) for i, v in enumerate((0, -1, False, False)))
    meta = {"scenario": {k: getattr(sc, k) for k in sc.__dataclass_fields__}, "trace_seconds": seconds,
            "variant": mi.variant(), "sionna_rt": rt.__version__}
    return Table(t, sample_index, pos, a, tau, valid, los, meta)


def scaled(table: Table, sc: Scenario = Scenario()):
    """[K, 2, P] the gains the emulator applies: unit-energy taps times the loss relative to the best point on the
    route, clamped to max_drop_db (TOSSI's convention).

    The raw gains sit 60 to 120 dB down while the emulator's samples are full-scale cf32 at a fixed noise power, so
    the published gains carry the route's relative fading, not its absolute path loss.
    """
    p = (np.abs(table.a) ** 2 * table.valid).sum(-1)
    with np.errstate(divide="ignore", invalid="ignore"):
        g = 10 * np.log10(p)
        drop = np.clip(g.max(0) - g, 0, sc.max_drop_db)
        s = np.where(p > 0, 10 ** (-drop / 20) / np.sqrt(np.where(p > 0, p, 1)), 0)
    return (table.a * s[..., None]).astype(np.complex64)


def encode(seq, sample_index, links, fs_hz=FS_HZ):
    """The live-taps message (plan.md section 6): JSON header, then all gains, then all delays, links in order.

    links: per cell, (a, tau) of its valid paths only; tau in seconds, the earliest path at 0.
    """
    header = {"v": VERSION, "seq": int(seq), "sample_index": int(sample_index), "fs_hz": int(fs_hz),
              "links": [{"cell": i + 1, "num_paths": int(len(a))} for i, (a, _) in enumerate(links)]}
    a = np.concatenate([np.asarray(a, np.complex64) for a, _ in links]).astype("<c8")
    tau = np.concatenate([np.asarray(t, np.float32) for _, t in links]).astype("<f4")
    return [json.dumps(header, separators=(",", ":")).encode(), a.tobytes(), tau.tobytes()]


def decode(frames, fs_hz=FS_HZ):
    """(header, links) from a message, or ValueError: a bad message is dropped and the taps in use stay."""
    if len(frames) != 3:
        raise ValueError(f"expected 3 frames, got {len(frames)}")
    try:
        h = json.loads(bytes(frames[0]))
        n = [int(link["num_paths"]) for link in h["links"]]
        seq, si = int(h["seq"]), int(h["sample_index"])
    except (ValueError, KeyError, TypeError) as e:
        raise ValueError(f"bad header: {e}") from None
    if h.get("v") != VERSION:
        raise ValueError(f"version {h.get('v')}, expected {VERSION}")
    if h.get("fs_hz") != fs_hz:
        raise ValueError(f"fs_hz {h.get('fs_hz')}, expected {fs_hz}")
    if seq < 0 or si < 0 or min(n, default=0) < 0:
        raise ValueError("negative seq, sample_index or num_paths")
    if len(frames[1]) != 8 * sum(n) or len(frames[2]) != 4 * sum(n):
        raise ValueError(f"frame sizes {len(frames[1])}, {len(frames[2])} do not match {sum(n)} paths")
    a = np.frombuffer(bytes(frames[1]), "<c8").astype(np.complex64)
    tau = np.frombuffer(bytes(frames[2]), "<f4").astype(np.float32)
    if not (np.isfinite(a).all() and np.isfinite(tau).all() and (tau >= 0).all()):
        raise ValueError("non-finite gain or delay, or a negative delay")
    cut = np.cumsum([0] + n)
    return h, [(a[i:j], tau[i:j]) for i, j in zip(cut[:-1], cut[1:])]


def messages(table: Table, sc: Scenario = Scenario()):
    """One message per trace, in order. Each link's delays start at 0 (Sionna's normalize_delays); the gains keep
    their full propagation phase."""
    a = scaled(table, sc)
    out = []
    for k in range(len(table.t)):
        links = []
        for li in range(a.shape[1]):
            m = table.valid[k, li]
            tau = table.tau[k, li][m]
            links.append((a[k, li][m], tau - tau.min() if m.any() else tau))
        out.append(encode(k, table.sample_index[k], links))
    return out


def sample_taps(a, tau, l_min=L_MIN, l_max=L_MAX, fs_hz=FS_HZ):
    """Step 3, Sample: h[l] = sum_m a_m sinc(l - fs tau_m) for l = l_min..l_max, over the last axis of a and tau.

    The formula of Sionna's cir_to_time_channel and Paths.taps (normalize=False). Runs wherever a lives, so the
    emulator calls it on the GPU.
    """
    a = torch.as_tensor(a)
    tau = torch.as_tensor(tau, device=a.device).to(torch.float64)
    lag = torch.arange(l_min, l_max + 1, device=a.device, dtype=torch.float64)
    s = torch.sinc(lag[:, None] - fs_hz * tau[..., None, :])               # [..., L, P]
    return (s.to(a.dtype) * a[..., None, :]).sum(-1)


class TapSwitch:
    """Step 2's Swap taps. Messages arrive early and in any number; each one takes effect at the first slot that
    starts at or after its sample_index. Before the first, at_slot() returns None (the emulator stays on TDL); after
    the last, the taps hold."""

    def __init__(self, num_links=2, fs_hz=FS_HZ, l_min=L_MIN, l_max=L_MAX, device="cpu"):
        self.num_links, self.fs_hz, self.lags, self.device = num_links, fs_hz, (l_min, l_max), device
        self.pending, self.seen = [], set()
        self.taps, self.seq = None, None
        self.swaps, self.superseded, self.rejected, self.duplicates = 0, 0, 0, 0
        self.log = []                                     # (seq, slot_start) of every swap

    def offer(self, frames):
        try:
            h, links = decode(frames, self.fs_hz)
            if len(links) != self.num_links:
                raise ValueError(f"{len(links)} links, expected {self.num_links}")
        except ValueError:
            self.rejected += 1
            return False
        if h["seq"] in self.seen:                         # a second subscriber joined and the route was re-sent
            self.duplicates += 1
            return False
        self.seen.add(h["seq"])
        heapq.heappush(self.pending, (h["sample_index"], h["seq"], links))
        return True

    def at_slot(self, slot_start):
        due = None
        while self.pending and self.pending[0][0] <= slot_start:
            if due is not None:
                self.superseded += 1
            due = heapq.heappop(self.pending)
        if due is not None:
            # Sampled once per swap, not per slot: the taps only change here.
            self.taps = torch.stack([sample_taps(torch.as_tensor(a, device=self.device), tau, *self.lags,
                                                 fs_hz=self.fs_hz) for a, tau in due[2]])
            self.seq = due[1]
            self.swaps += 1
            self.log.append((due[1], slot_start))
        return self.taps


def publish(frames_list, bind="tcp://127.0.0.1:5566", stop=None, ready=None):
    """Send the whole route to each subscriber as it joins, until stop is set.

    XPUB, not PUB, so the publisher sees a subscription arrive: nothing is lost to ZMQ's slow-joiner window, and an
    emulator restarted from sample 0 gets the route again from the start.
    """
    import zmq

    sock = zmq.Context.instance().socket(zmq.XPUB)
    sock.setsockopt(zmq.SNDHWM, 0)
    sock.setsockopt(zmq.XPUB_VERBOSE, 1)
    sock.setsockopt(zmq.LINGER, 1000)
    sock.bind(bind)
    if ready is not None:
        ready.set()
    sent = 0
    try:
        while stop is None or not stop.is_set():
            if sock.poll(100) and sock.recv()[:1] == b"\x01":
                for frames in frames_list:
                    sock.send_multipart(frames)
                sent += len(frames_list)
    finally:
        sock.close()
    return sent


def subscriber(connect="tcp://127.0.0.1:5566"):
    import zmq

    sock = zmq.Context.instance().socket(zmq.SUB)
    sock.setsockopt(zmq.RCVHWM, 0)
    sock.setsockopt(zmq.SUBSCRIBE, b"")
    sock.connect(connect)
    return sock


def drain(sock, switch: TapSwitch):
    """Hand every message waiting on the socket to the switch, without blocking. Called once per slot."""
    import zmq

    n = 0
    while True:
        try:
            switch.offer(sock.recv_multipart(zmq.NOBLOCK))
        except zmq.Again:
            return n
        n += 1
