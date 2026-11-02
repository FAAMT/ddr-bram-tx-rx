# plan.md: Sionna RT channel emulator on one NVIDIA DGX Spark

The diagram is the source of truth: the slide at `/spark/brief-uav` (`web/spark/brief-uav.html`).
This file says how to build exactly that, on the Spark, one checked step at a time.
If this file and the diagram disagree, the diagram wins and this file is corrected.

Status: nothing below has been built or run yet. Every number comes from a source listed in section 9,
or is marked [estimate]. Every command is untested on the Spark until its check has passed there.


## 0. Rules for whoever follows this file

1. Work top to bottom. Every step ends with a **Check**. Do not start the next step until it passes.
2. Paste what you ran and what you saw into `spark/LOG.md` (date, command, output). "Should work" is not a pass.
3. Change one thing at a time. Every component must still run on its own after each step.
4. If the build needs something the diagram does not show, stop and update the diagram first, then this file.
5. Credentials (SIM keys, core passwords) never go into a tracked file. They stay in the environment.


## 1. What the diagram shows

One NVIDIA DGX Spark stands in for the radio channel between two 5G cells and two UEs. Two OCUDU DUs and
two OAI UEs exchange IQ samples over ZMQ instead of over the air, and an IQ broker between them hands every
block to a channel emulator running Sionna PHY on the Spark's Blackwell GPU. For each 0.5 ms slot (30,720
samples at 61.44 Msps, 40 MHz, both cells, downlink and uplink) the emulator pulls the block, takes the
current channel taps, filters the samples with them, adds noise and returns a block of the same length.
The taps come from the 3GPP TDL model by default, and Sionna RT replaces them: outside the slot loop it
ray-traces a 3D scene, here a UAV flying through an urban street canyon between two gNBs, and sends the
path gains and delays as live taps, so the emulated channel follows the UAV. On the UE side, a UE broker
joins the two UEs' links with MPTCP.


## 2. Components and where they run

| Diagram box            | Runs on              | Software                                   | Talks to                         |
|------------------------|----------------------|--------------------------------------------|----------------------------------|
| DU 1, 2 (OCUDU)        | Arm CPU              | OCUDU `odu`, stock `zmq` radio driver      | IQ broker (ZMQ IQ), CU (F1)      |
| IQ broker (ZMQ)        | Arm CPU, own core    | `spark/emulator/iq_broker.py`              | DUs, UEs (ZMQ), channel emulator |
| Channel emulator       | GPU (Blackwell)      | `spark/emulator/channel.py`, Sionna PHY    | IQ broker (unified memory)       |
| Live taps (PUB / SUB)  | CPU                  | `uav_rt.publish()` → `TapSwitch` in broker | emulator, `tcp://127.0.0.1:5566` |
| Sionna RT              | GPU (RT cores)       | `spark/rt/uav_rt.py`, Sionna RT 2.2.0      | live taps publisher              |
| OAI UE 1, 2 (ue1, ue2) | Arm CPU, netns       | OAI `nr-uesoftmodem`, `oai_zmqdevif`       | IQ broker (ZMQ), UE broker (IP)  |
| UE broker (MPTCP)      | Arm CPU, netns `mp`  | Linux MPTCP + `spark/ue_broker/setup.sh`   | UE tunnels, MPTCP server on DN   |
| 10 GbE to core         | RJ-45 port           | CU `ocu` on the Spark, 5G core elsewhere   | AMF / UPF                        |

How to read two boxes:

- **UE SIDE** is drawn apart from the DGX Spark box because it is the far end of the emulated radio link.
  It runs on the same Spark: four IQ streams need 4 × 61.44 Msps × 8 B ≈ 15.7 Gb/s [estimate], more than
  the 10 GbE port, which carries the core. ue1 and ue2 are network namespaces, which are host-local.
- **No CU is drawn.** Two DUs need one CU. It runs on the Spark (`ocu`) and reaches the core over 10 GbE.


## 3. Fixed numbers

| Quantity                 | Value                                   | Note                                  |
|--------------------------|-----------------------------------------|---------------------------------------|
| Bandwidth, SCS, PRB      | 40 MHz, 30 kHz, 106 PRB                 | band n78                              |
| Sample rate              | 61.44 Msps                              | 2048 × 30 kHz                         |
| Slot                     | 0.5 ms = 30,720 samples                 | 14 × 2048 + 176 + 13 × 144            |
| Wire format              | complex float32 (cf32), 1.0 = full scale| both stacks map 1.0 ↔ int16 32767     |
| Streams                  | 4: 2 cells × DL, UL                     | ≈ 492 MB/s each [estimate]            |
| Tap span (first version) | 16 taps = 260 ns ≈ 78 m excess path     | TOSSI default; raise later (step 9)   |
| Filter lags              | l_min = −6 → fixed 6-sample delay       | `time_lag_discrete_time_channel`      |
| Live taps socket         | XPUB binds `tcp://127.0.0.1:5566`       | TOSSI convention; avoid 5555/5556     |
| Carrier (Doppler only)   | 3.5 GHz, λ = 8.57 cm                    | 15 m/s → 175 Hz max Doppler [estimate]|


## 4. The loop, step by step, as the diagram draws it

Each slot, per stream (DL and UL of each cell), inside `spark/emulator/`:

1. **Pull (REQ, 30,720 IQ samples).** The broker asks the sender with a ZMQ REQ. The sender's REP answers
   with whatever it has queued (OCUDU: 1 to 614,400 samples), so the broker keeps a FIFO per stream and cuts
   it into 30,720-sample slots. Never drop or reorder a sample: one lost sample shifts every later SSB.
2. **New live taps?** Checked once per slot, never mid-slot.
   - **yes → Swap taps.** Take the newest valid message from the SUB socket, sample it (step 3) and install
     it for this slot. A malformed message is dropped and counted; the old taps stay.
   - **no → 2 Taps.** Use the current tap source for this slot.
3. **2 Taps (3GPP TDL: gains a, delays τ).** TDL mode: slot k reads entry k of one long, continuous TDL
   realization made once at start-up. Calling `TDL` again every slot draws a fresh random channel each
   time, so it is called once. RT mode: "scene-derived gains and delays replace the 3GPP TDL model", so
   this step returns the last swapped-in RT taps, held until the next swap.
4. **3 Sample (`cir_to_time_channel`).** Turn (a, τ) into 16 discrete taps at 61.44 Msps. Cached: it only
   runs when the taps changed. Steps 2, 3 and Swap taps together are the "channel generation and update" stage.
5. **4 Filter (`ApplyTimeChannel`).** Filter the slot with its taps. The output is N + L − 1 samples: add
   the previous slot's last L − 1 samples to the start (overlap-add) and keep this slot's tail for the next.
6. **5 Noise (AWGN + bias).** `AWGN` at a fixed noise power in dBFS (`noise_dbfs`), then add a constant
   complex DC offset (`bias`, default 0). The emulator never adds net gain: OAI needs about 12 dB of
   headroom below full scale.
7. **6 Return (REP, same 30,720 samples).** Answer the receiver's REQ with this slot. The stream leaves
   exactly as many samples as it came in with, 6 samples later. That delay is fixed and the same in both
   directions, so the UE sees it as about 29 m of extra path.
8. **Next slot.**


## 5. Decisions the diagram leaves open (defaults used below, to confirm)

| #  | Question                                              | Default here                                         |
|----|-------------------------------------------------------|------------------------------------------------------|
| Q1 | Same carrier for both cells, or different?            | Different: the broker runs two independent pipes     |
| Q2 | Who rides the UAV?                                    | Both UEs: UE 1 on gNB 1, UE 2 on gNB 2               |
| Q3 | Upstream OCUDU + OAI, or the TOSSI forks?             | Upstream OCUDU `dev` + Duranta OAI ≥ 2026.w25        |
| Q4 | Live message: (a, τ) or sampled taps?                 | (a, τ), as the diagram labels it (section 6)         |
| Q5 | Scene, UAV speed, altitude, gNB heights?              | `munich`, 10 m/s, 8 to 10 m in the canyon            |
| Q6 | Taps per slot or per OFDM symbol? How many?           | Per slot, 16 taps                                    |
| Q7 | Where do the core and the MPTCP server run?           | Off the Spark, over 10 GbE; server on the DN         |
| Q8 | Must it run in real time?                             | No: measure the real-time factor, don't target it    |

Notes on the defaults:

- Q3: OCUDU's own tutorial connects OCUDU to OAI directly over ZMQ (proven at 20 MHz). The TOSSI forks are
  proven at 40 MHz but differ in gain handling and ZMQ threading. Do not mix configs between the two trees.
- Q6: at 10 m/s the phase moves about 0.37 rad per slot [estimate], so per-slot taps step at slot edges.
  Acceptable for the first version. Per-symbol taps (14 per slot) are the refinement.
- Q8: ZMQ has no clock. A slow pipe slows the whole link instead of breaking it, so all timing below is
  measured, and trajectory time follows the stream's sample count, never the wall clock.


## 6. Interfaces

Port map (TOSSI conventions; UE addresses are inside each namespace):

| Link       | DU side (REP binds TX, REQ connects RX) | IQ broker                                        | UE side (netns)                          |
|------------|-----------------------------------------|--------------------------------------------------|------------------------------------------|
| cell 1 DL  | DU1 `tx_port=tcp://127.0.0.1:4556`      | REQ → 4556; REP binds `tcp://10.201.1.100:5001`  | UE1 `rx_channels tcp://10.201.1.100:5001`|
| cell 1 UL  | DU1 `rx_port=tcp://127.0.0.1:4557`      | REP binds 4557; REQ → `tcp://10.201.1.1:5000`    | UE1 `tx_channels tcp://0.0.0.0:5000`     |
| cell 2 DL  | DU2 `tx_port=tcp://127.0.0.1:4558`      | REQ → 4558; REP binds `tcp://10.202.1.100:5001`  | UE2 `rx_channels tcp://10.202.1.100:5001`|
| cell 2 UL  | DU2 `rx_port=tcp://127.0.0.1:4559`      | REP binds 4559; REQ → `tcp://10.202.1.2:5000`    | UE2 `tx_channels tcp://0.0.0.0:5000`     |
| live taps  | (none)                                  | SUB connects `tcp://127.0.0.1:5566`              | XPUB binds 5566 (RT publisher)           |

Live taps message, version 1 (ZMQ multipart, little-endian):

    frame 0  JSON  {"v": 1, "seq": n, "sample_index": s, "fs_hz": 61440000,
                    "links": [{"cell": 1, "num_paths": P1}, {"cell": 2, "num_paths": P2}]}
    frame 1  a     complex64, P1 + P2 values, link order as in frame 0
    frame 2  tau   float32 seconds, P1 + P2 values

`sample_index` is the stream sample at which the taps take effect: the broker swaps them in at the first
slot that starts at or after it. Reject a message whose `fs_hz` differs, or whose sizes do not match.

Files this plan creates on the Spark, under `spark/` in this repo:

    spark/LOG.md                  every check, with its output
    spark/configs/cu.yml          OCUDU CU
    spark/configs/du1.yml         OCUDU DU, cell 1
    spark/configs/du2.yml         OCUDU DU, cell 2
    spark/scripts/netns.sh        ue1, ue2, mp namespaces and veths
    spark/scripts/run.sh          start order: publisher, CU, DUs, broker, UEs
    spark/scripts/stop.sh         stop everything (restart DU before UE after any failure)
    spark/emulator/iq_broker.py   ZMQ sockets, FIFOs, slot cutting, lock-step
    spark/emulator/channel.py     tap sources, sampling, filter, overlap-add, noise
    spark/rt/uav_rt.py            scene, route, trace, live-taps message, publisher, TapSwitch, sample_taps
    spark/rt/verify_uav_rt.ipynb  21 checks of uav_rt.py, on Colab or any CPU
    spark/ue_broker/setup.sh      MPTCP endpoints, limits, routing
    spark/tests/test_channel.py   offline checks for step 9


## 7. Build steps

### Step 1. Platform baseline

    nvidia-smi                                   # driver 580.x, CUDA 13.0
    cat /etc/dgx-release; uname -r; python3 -V   # DGX OS 7.x, Ubuntu 24.04, Python 3.12
    lscpu -e                                     # find the 10 X925 cores (higher max MHz)
    sysctl net.mptcp.enabled                     # 1

Check: all four answer as commented. Record which CPU numbers are X925 cores; steps 7 and 8 pin to them.

### Step 2. Python environment with Sionna PHY and RT

    sudo apt install -y python3-venv libzmq3-dev cmake build-essential
    python3 -m venv .venv && . .venv/bin/activate
    pip install --upgrade pip
    pip install sionna==2.2.0 pyzmq         # pulls sionna-rt 2.2.0, mitsuba 3.9.1, drjit 1.5.0, torch (CUDA 13)
    python -c "import torch; print(torch.__version__, torch.cuda.is_available(), torch.cuda.get_device_capability(0), torch.cuda.get_arch_list())"
    python -c "import mitsuba as mi, sionna.rt as rt; print(rt.__version__, mi.variant())"
    python -c "import drjit as dr; from drjit.cuda import Float; x = Float(1., 2., 3.); dr.eval(x * 2.); print(x * 2.)"

Check: torch sees CUDA with capability (12, 1); the Mitsuba variant is `cuda_ad_mono_polarized`
(any `llvm_*` variant means Sionna RT silently fell back to the CPU); Dr.Jit prints [2, 4, 6].

### Step 3. Sionna RT on the GB10

The RT side is one module, `spark/rt/uav_rt.py`: the scene, the route, the trace, the live-taps message, the
publisher, and the emulator's Swap taps and Sample steps. `spark/rt/verify_uav_rt.ipynb` checks it with 21 checks;
it passes on a CPU, and is run on Colab before the Spark.

    cd spark/rt && pip install nbconvert ipykernel && jupyter nbconvert --to notebook --execute verify_uav_rt.ipynb

Check: the summary says 21 of 21 checks passed, with the variant `cuda_ad_mono_polarized`. Record the trace time
from `uav_rt_report.json` (TOSSI measured about 40 to 63 ms per trace on a Turing GPU; the 327 traces of the route
took 42 s on a 16-core CPU).

The scenario is `Scenario` in the module: the `munich` scene, gNB 1 at (55, 338, 18.83) and gNB 2 at
(74, 339, 25.28) on the roof edges either side of the canyon's far end, and the UAV at 8 to 10 m on 7 waypoints from
the square into the canyon, at 10 m/s for 16.3 s. TOSSI's ground route, (20, 40) to (20, 330), runs through a
building near y = 150, so it is not used.

### Step 4. CU, core reachability and the OCUDU build

    git clone https://gitlab.com/ocudu/ocudu.git && cd ocudu && git checkout dev
    mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_ZEROMQ=ON .. && make -j"$(nproc)" ocu odu

`spark/configs/cu.yml`: `cu_cp.amf.addrs` = the core's AMF address, `bind_addrs` = the Spark's address on
the 10 GbE port, `cu_cp.f1ap.bind_addrs: 127.0.10.1`, `cu_up.f1u.socket.bind_addr: 127.0.10.1`.
Check key names against the config reference of the checked-out tree; the TOSSI fork uses older keys.

Check: CMake's output says ZeroMQ was found (without `libzmq3-dev` it silently builds without ZMQ);
`sudo ./apps/cu/ocu -c spark/configs/cu.yml` connects to the AMF (`sudo ss -np --sctp | grep :38412`
shows ESTAB).

### Step 5. OAI UE build

    git clone https://github.com/duranta-project/openairinterface5g.git oai && cd oai   # tag 2026.w25 or later
    cd cmake_targets && ./build_oai --nrUE -w SIMU --ninja      # or: cmake .. -DOAI_ZMQ=ON
    grep OAI_ZMQ ran_build/build/CMakeCache.txt                 # must be ON

Check: `nr-uesoftmodem` and `liboai_zmqdevif` exist and `OAI_ZMQ` is ON.

### Step 6. One cell, one UE, direct over ZMQ (no broker yet)

`spark/configs/du1.yml` (upstream keys):

    gnb_id: 411
    gnb_du_id: 0
    f1ap: {addrs: 127.0.10.1, bind_addrs: 127.0.10.2}
    f1u: {socket: [{bind_addr: 127.0.10.2}]}
    ru_sdr:
      device_driver: zmq
      device_args: tx_port=tcp://127.0.0.1:4556,rx_port=tcp://127.0.0.1:4557
      srate: 61.44
      tx_gain: -12          # headroom for OAI's fixed-point FFT; upstream ZMQ rejects gain > 0 dB
      rx_gain: 0
    cell_cfg: {dl_arfcn: 632628, band: 78, channel_bandwidth_MHz: 40, common_scs: 30, pci: 1, plmn: "00101", tac: 1}

UE 1, on the host for this step only (the direct link inverts the ports):

    sudo ./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3489420000 --ue-scan-carrier \
      --device.name oai_zmqdevif \
      "--zmq.[0].tx_channels" tcp://127.0.0.1:4557 "--zmq.[0].rx_channels" tcp://127.0.0.1:4556 \
      --uicc0.imsi "$UE1_IMSI" --uicc0.key "$UE1_KEY" --uicc0.opc "$UE1_OPC" \
      --uecap_file ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/uecap_ports1.xml

No `-E`: without it the UE samples at 61.44 Msps for 106 PRB. The SIM values come from the core's
subscriber database, through the environment.

Check: the UE log shows `TUN Interface oaitun_ue1 successfully configured` and registration accepted;
`ping -I oaitun_ue1` reaches the data network. This is the baseline every later step is compared with.
If 40 MHz fails on the upstream pair but works at 20 MHz, record it and decide Q3 before going on.

### Step 7. IQ broker, pass-through, one cell

`spark/scripts/netns.sh` creates ue1 and ue2 (TOSSI's layout), and later mp:

    ip netns add ue$i
    ip link add v-eth$i type veth peer name v-ue$i; ip link set v-ue$i netns ue$i
    ip addr add 10.$((200+i)).1.100/24 dev v-eth$i; ip link set v-eth$i up
    ip netns exec ue$i ip link set lo up
    ip netns exec ue$i ip addr add 10.$((200+i)).1.$i/24 dev v-ue$i; ip netns exec ue$i ip link set v-ue$i up
    ip netns exec ue$i ip route add default via 10.$((200+i)).1.100

`spark/emulator/iq_broker.py`, pass-through only (filter = identity). Rules taken from TOSSI's proxy:

- One thread, driven by the DU's uplink requests; downlink handled non-blocking in the same loop.
- Exactly one request in flight per socket. Never set `ZMQ_REQ_RELAXED`; on a timeout, stay waiting.
- Downlink is lossless and contiguous. Keep remainders, apply back-pressure instead of dropping.
  Split a downlink reply above 240,000 samples (the UE's receive buffer is 300,000).
- Pin the broker to one dedicated X925 core. Start the DU and the broker together, the UE last.
- Count samples in and out per stream and print them every second.

Run UE 1 inside ue1 with the section 6 ports.

Check: the same registration and ping as step 6; samples in = samples out on every stream; no drops.

### Step 8. Second cell and second UE

`spark/configs/du2.yml` = du1.yml with `gnb_du_id: 1`, F1 bind address `127.0.10.3`, `pci: 2`, ports
4558 / 4559, and the cell 2 `dl_arfcn` (Q1). UE 2 runs in ue2 with its own SIM. The broker runs the two
cells as two independent pipes.

Check: both UEs registered at once, each pinging through its own tunnel, counters equal on all 4 streams.

### Step 9. Channel emulator on the GPU, TDL mode

Write `spark/emulator/channel.py` and `spark/tests/test_channel.py`. The core of one slot (sketch, untested):

    import torch
    from sionna.phy.channel import tr38901, cir_to_time_channel, time_lag_discrete_time_channel, ApplyTimeChannel, AWGN
    FS, N = 61.44e6, 30720
    l_min, l_max = -6, 9                       # 16 taps; time_lag_discrete_time_channel(FS, 3e-6) gives 198
    L = l_max - l_min + 1
    apply, awgn = ApplyTimeChannel(N, L), AWGN()

    # TDL mode: one continuous realization at the slot rate, sampled once into a table of taps
    tdl = tr38901.TDL("C", delay_spread=100e-9, carrier_frequency=3.5e9, min_speed=10., max_speed=10.)
    a, tau = tdl(batch_size=1, num_time_steps=K, sampling_frequency=2000.)   # K slots, 2000 slots/s
    table = cir_to_time_channel(FS, a, tau, l_min, l_max, normalize=True)[0, 0, 0, 0, 0]   # [K, L]

    def slot(x, taps, tail, no, bias):         # x: [1, 1, 1, N] complex64 on cuda; taps: [L]
        h = taps.view(1, 1, 1, 1, 1, 1, L).expand(1, 1, 1, 1, 1, N + L - 1, L)
        y = apply(x, h)[0, 0]                   # [1, N + L - 1]
        y[..., :L - 1] += tail                  # overlap-add the previous slot's tail
        out, tail = y[..., :N], y[..., N:].clone()
        return awgn(out, no) + bias, tail

Offline checks in `test_channel.py`, all on the GPU:

- One tap of 1 at lag 0: the output equals the input delayed by 6 samples, to float precision.
- One tap at lag d: the output is the input shifted by d + 6.
- Filtering a long signal slot by slot equals filtering it in one piece (overlap-add is right).
- With `no` set, the measured SNR matches the setting within 0.2 dB.
- No jump between slot k and k + 1: the tap correlation across slot edges matches inside a slot.
- Per-slot GPU time for 4 streams, as a histogram (median, 99th percentile), against the 0.5 ms slot.

Then put the filter in the broker and run both cells.

Check: the offline checks pass; both UEs stay registered at the set noise level; the UE logs' SINR and CQI
move when `noise_dbfs` changes; per-slot latency and the real-time factor are in `LOG.md`.
Only if `ApplyTimeChannel` is too slow for 4 streams: replace it with a direct FIR
(`torch.nn.functional.conv1d`) that matches it within 1e-6 on the same checks. At 16 taps it moves about
94 GB/s for 4 streams, 35 % of the Spark's 273 GB/s; at 64 taps and above it exceeds it [estimate].

### Step 10. Live taps over PUB/SUB, synthetic first

The broker takes them from `uav_rt`: `subscriber()` connects `tcp://127.0.0.1:5566`, `drain()` runs once per
slot without blocking, `decode()` validates the section 6 message, and `TapSwitch.at_slot()` gives the slot's taps.
A test publisher sends a two-path channel that changes every second. The notebook's section 7 already runs these
checks over loopback; this step runs them inside the broker.

Check: each message is swapped in at the first slot at or after its `sample_index`, and nowhere else;
the swap counter equals the messages sent; a malformed message is counted and the old taps stay; when
messages stop, the last taps are held (they do not fall back to TDL).

### Step 11. Sionna RT UAV scenario

`uav_rt.trace_route()` traces the route, `Table.save()` writes it, `messages()` and `publish()` send it.

1. Trajectory: Q5's waypoints at Q5's speed. Position is a function of stream time t = sample_index / 61.44e6.
2. Precompute: one trace every Δt = 50 ms (0.5 m at 10 m/s, every 100 slots), (t, a, τ) stored for both links.
3. Publish: XPUB sends the whole route to each subscriber as it joins, every entry with its `sample_index`.
   Precomputing keeps the run deterministic and independent of trace time.

Path cap: past `max_num_paths_per_src`, Sionna drops candidate paths without a warning, and the candidates grow
with the receivers in one call: 64 receivers per call lost up to 4.6 dB out of line of sight. So the route is traced
one receiver per call, which at the default cap loses nothing (the notebook checks it against 4 times the cap).

Path loss: scale each link's gains relative to the strongest point on the route, clamped to a maximum drop
(TOSSI's publisher: `--max-drop-db`, 45 dB by default), so the UEs stay attached along the route.

GPU sharing: if a route is traced while the emulator runs, run both under NVIDIA MPS, or tracing stalls the
slot loop:

    export CUDA_MPS_PIPE_DIRECTORY=/tmp/nvidia-mps CUDA_MPS_LOG_DIRECTORY=/tmp/nvidia-log
    nvidia-cuda-mps-control -d

Check: taps change along the route; each UE's SINR follows its link's path gain; both links update from one
trace; replaying the saved table gives the same tap sequence, byte for byte.

### Step 12. UE broker, MPTCP

MPTCP endpoints are per network namespace, so one client cannot open subflows over both UE tunnels
directly. `spark/ue_broker/setup.sh` builds a third namespace `mp` for the client:

- veth mp ↔ ue1 and mp ↔ ue2; inside each UE namespace, forward and NAT from its veth into `oaitun_ue<i>`.
- In mp: `ip mptcp limits set subflows 2`, one `ip mptcp endpoint add <ip> dev <veth_i> subflow` per UE,
  and a policy route per source address (`ip rule add from <ip_i> table <100+i>`).
- The MPTCP server sits on the data network behind the UPF: `mptcpize run iperf3 -s`.

Check: `ip netns exec mp mptcpize run iperf3 -c <server>` shows one connection with two subflows, one per UE
(`ss -M`, `ip mptcp monitor`); the throughput is close to the sum of the two single-UE runs.

### Step 13. End-to-end run and what to report

Start order: MPS, publisher, CU, DU 1 and 2 with the broker, UE 1 and 2, UE broker, traffic.
Report in `LOG.md`: per-slot GPU latency, real-time factor, SINR and CQI against UAV position for both UEs,
MPTCP throughput along the route, and swap and drop counters.


## 8. Refinements once it works

- Per-OFDM-symbol taps (14 per slot) from `paths.cir(num_time_steps=..., sampling_frequency=...)` with UAV
  velocity set, so Doppler is interpolated between traces.
- More taps (64, then the 198 that 3 µs needs) with a direct or FFT FIR.
- A 2 × 2 superposition broker, if Q1 puts both cells on one carrier.


## 9. Sources

- TOSSI, Sionna RT + OCUDU RAN: https://tossi.org/docs/ran-integration/digital-twins/sionna-rt-ocudu-ran/
- TOSSI, multi-UE simulation (the dual-UE example): https://tossi.org/docs/ran-integration/multi-ue-sim/
- TOSSI code: https://github.com/TOSSI-Foundation/OCUDU-RAN (`dev`), https://github.com/TOSSI-Foundation/OAI-RAN (`develop`)
- Sionna 2.2.0: https://nvlabs.github.io/sionna/ ; RT source https://github.com/NVlabs/sionna-rt (`v2.2.0`)
- Sionna Research Kit channel emulator on DGX Spark: https://nvlabs.github.io/sionna/rk/tutorials/channel_emulation/channel_emulation.html
- NVIDIA DGX Spark hardware and release notes: https://docs.nvidia.com/dgx/dgx-spark/hardware.html
- OCUDU: https://gitlab.com/ocudu/ocudu , docs https://docs.ocudu.org/ (OAI UE over ZMQ tutorial)
- OAI ZMQ radio: https://github.com/OPENAIRINTERFACE/openairinterface5g/tree/develop/radio/zmq ; Duranta OAI https://github.com/duranta-project/openairinterface5g
- MPTCP: https://www.mptcp.dev/
- Earlier fact check of the design: `web/spark/ERRORs.md`
