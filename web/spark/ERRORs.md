# ERRORs: the v2 brief, checked claim by claim

Checked 2026-10-26 against `web/spark/brief.html` (served at `/spark/brief`). Every claim on the diagram was checked
against primary sources: the OCUDU source, the OAI
`radio/zmq` source, the Sionna 2.2.0 docs and source, NVIDIA's DGX Spark hardware page, and 3GPP TS 38.211 numerology.
Nothing was changed. **[A]** marks my own arithmetic or inference, not a sourced fact.

## Which version this covers

Nothing listed here has been fixed yet, in either version.

- `/spark/brief` is the original layout, and every item applies to it.
- `/spark/brief-ruibin` is Ruibin's layout: the same System and loop, a compact Sionna RT flow, and no OFDM panel.
  - Items 1–4, 10, 13–17, 22–24, 29 and 31 still apply.
  - Items 5–9, 11, 12, 18–21, 26, 27 and 30 do not, because that layout drops the parts they concern: the OFDM panel,
    the FFT → Hₖ·Sₖ → IFFT chain, and the Server and t₀ → tₙ labels. Item 12's sample‑alignment requirement still holds
    for the emulator itself.
  - Items 25 and 28 apply in part: the RT cards are not teal, and only one zone is a focus zone, but zone tints are still
    missing from the legend.

## 1. Wrong or illogical as drawn

| # | Where | Claim on the diagram | Problem | Evidence |
|---|---|---|---|---|
| 1 | Loop, decision | "New taps on SUB :5566?" no → 2 Taps `tr38901.TDL` | Every slot without a new message rebuilds TDL taps, so the live taps that were swapped in last only **one slot** (0.5 ms). The "no" branch should keep the current taps. As drawn, live mode and TDL mode alternate. | Flowchart logic |
| 2 | Loop, step 2 | `tr38901.TDL` called once per slot | Each call draws fresh random Doppler, angles and phases, and restarts time at t = 0. The channel therefore **jumps to a new independent realization every 0.5 ms**, with no fading continuity. | Sionna `tdl.py` `__call__`: `sample_times = arange(num_time_steps)`, `theta = rand(...)`, `phi = rand(...)` on every call |
| 3 | Loop, step 1 | "Pull `REQ` 30,720 IQ samples" | The emulator does not choose the block size. OCUDU's TX `REP` answers each request with **whatever is queued**, from 1 sample up to 614,400. OAI behaves the same way. The emulator has to buffer and re-chunk; "30,720" is our choice, not what the socket delivers. | OCUDU `radio_zmq_tx_channel.cpp` `send_response()`: `count = circular_buffer.try_pop(...)`; `DEFAULT_STREAM_BUFFER_SIZE = 614400` |
| 4 | System, UE side | OAI UEs drawn **outside** the Spark | They cannot be on another machine. The UE side carries 4 streams × 61.44 Msps × 8 B ≈ 1.97 GB/s ≈ 15.7 Gb/s, which does not fit the 10 GbE port, and the CX‑7 link was removed as "later". "ns ue1, ue2" are network namespaces, which by definition are on the same host. The UEs run on the Spark's Arm CPU. | [A] arithmetic; NVIDIA lists one 10 GbE RJ‑45 port |
| 5 | Sionna RT band vs loop | Two different ways of applying the channel on one page | The loop filters in time (`ApplyTimeChannel`). The band uses FFT → Hₖ·Sₖ → IFFT. The brief never says which one the emulator runs, or that they are alternatives. Sionna RT can produce taps directly (`Paths.taps`, or `Paths.cir` → `cir_to_time_channel`). Those taps could feed the existing loop or the live‑taps PUB, which is what TOSSI does, with no FFT at all. The frequency‑domain path adds items 6 and 7 for no stated gain. | Sionna RT `Paths.taps(bandwidth, l_min, l_max, …)`; TOSSI applies RT as a time‑domain FIR |
| 6 | Sionna RT band, FFT | `OFDMDemodulator` on the IQ stream | It needs the OFDM symbol and slot boundaries: the input must start at symbol 0, with the per‑symbol CP pattern 176/144. The ZMQ stream is **raw samples with no timestamp or header**, so the emulator does not know where slots start. The time‑domain loop does not need this. | Sionna `OFDMDemodulator` input length `num_ofdm_symbols*fft_size+sum(cp)+n`; OCUDU sends a bare `cf_t` buffer |
| 7 | Sionna RT band, Hₖ·Sₖ | Per‑subcarrier product | This is only valid when every path delay is ≤ CP (144 samples = 2.34 µs at 30 kHz) and Doppler within a symbol is negligible. The loop is sized for 3 µs (`maximum_delay_spread=3e-6` → 198 taps), and the extra delay would wrap around instead of causing ISI. The two halves of the brief assume different channels. | Sionna `ApplyOFDMChannel`: "y = h·x + w" per subcarrier; NR normal CP = 144 Ts at 61.44 Msps [A] |
| 8 | OFDM grid | f axis points **up**, S₀ drawn at the **top** | In NR, subcarrier k = 0 is the lowest frequency. With the arrow pointing up, S₀ … S_{N−1} run downward, which contradicts the axis. Either flip the arrow or put S₀ at the bottom. | TS 38.211 §4.4.4: k numbered upward from point A |
| 9 | OFDM zone vs Sionna RT band | The same symbol means two things | In the grid, Sₖ is the **sequence index**: S_N sits at subcarrier 0 of symbol 1. The axis, and "Hₖ · Sₖ" below, use k as the **subcarrier index**. Both cannot hold. Use S_{k,l} (subcarrier k, symbol l) for the grid, and a separate index such as d(i) for the QAM sequence. | Internal consistency |

## 2. Unstated assumptions and conflicts

| # | Where | Issue |
|---|---|---|
| 10 | Sionna RT box | `PathSolver()(scene)` needs a 3D scene. This contradicts the earlier decision: "no 3D scene, plain IQ pipe for now". Removing "needs a scene" in the last edit hid that conflict, it did not resolve it. If only "now" belongs on the brief, either the RT band or the decision is wrong. |
| 11 | Sionna RT band | "Server / IQ at t₀" is undefined. Nothing called "Server" appears in the System zone; there, the IQ source is the DU, through the IQ broker. |
| 12 | Sionna RT band | t₀ → tₙ suggests the emulator adds delay. In ZMQ mode time is the **sample count**, so the emulator must keep sample n at index n, apart from the filter's fixed 6‑sample lookahead (`l_min = −6`). Any extra sample delay shows up as propagation delay in both directions (timing advance, PRACH window). Both pages miss this requirement. [A] |
| 13 | System zone | The RAN chain has no CU and no 5G core. Two DUs need a CU; only "10 GbE: core" hints that the core is off the box. Where the core runs is not established. |
| 14 | Loop, step 4 | `ApplyTimeChannel` needs `h_time` with **N + l_tot − 1 time steps × l_tot taps**: 30,917 × 198 complex64 ≈ 49 MB per stream per slot. Written and read every 0.5 ms for 4 streams, that is ≈ 780 GB/s, about 3× the Spark's 273 GB/s memory bandwidth. Per‑sample time‑varying taps cannot run in real time. Taps held constant per slot can, and are physically fine: at 3.6 GHz and walking speed, coherence time is about 40 ms. [A], from the documented shapes |
| 15 | Loop, step 4 | The filter output is N + l_tot − 1 samples, so streaming needs overlap‑add, carrying 197 samples into the next block. "Return same 30,720" is right only if that is done. The diagram does not show it. That is acceptable at this level, but note it. |
| 16 | Live taps | Keeping TOSSI's OCIR header for compatibility caps updates at **64 taps** (≈ 1.04 µs at 61.44 Msps). The loop's 3 µs sizing (198 taps) cannot travel in that format unchanged. |
| 17 | Sionna RT on GB10 | `sionna-rt 2.2.0` pins `mitsuba==3.9.1` and `drjit==1.5.0`. Both now ship `manylinux aarch64` wheels on PyPI, so it should install. A DGX Spark lab repo reported Sionna RT **failing on aarch64** with mitsuba 3.7.1. Running RT on the GB10 GPU (CUDA/OptiX on sm_121) is **not verified**. TOSSI ran on x86 with a Turing Quadro. |

## 3. Imprecise wording

| # | Where | Issue |
|---|---|---|
| 18 | OFDM sentence | "Each subcarrier carries one QAM symbol": that is per OFDM symbol, not overall. Some resource elements carry DMRS, PTRS, PDCCH or SSB, and unused bins carry zeros. |
| 19 | OFDM sentence | "an IFFT sums all N": the IFFT size is N_FFT = 2048, while the used N is 1,272 at 40 MHz. The figure's N = 12 and the "14 × 2048" line use different meanings of N, and the brief never separates N from N_FFT. |
| 20 | QAM | "4‑QAM" is mathematically right, but 3GPP names it QPSK. A wireless audience will expect "QPSK". |
| 21 | UL | The UL is drawn as plain OFDM. With transform precoding on, NR UL uses DFT‑s‑OFDM and subcarriers no longer carry QAM symbols directly. It is off by default in both stacks, so this is minor. |
| 22 | Loop, step 5 | "+ bias" is unexplained on an outside brief. It carries over from the v1 equation, where it means DC offset. |
| 23 | System zone | "NVLink‑C2C · 128 GB unified" puts the die‑to‑die link and the LPDDR5x memory in one bar. They are two different things. |
| 24 | GPU lane | Live taps placed **on the GPU**. That is a design choice, not a fact: TDL or parameter publishing could run on the CPU. It is only true if the source is Sionna RT. |

## 4. Legend and house style

| # | Issue |
|---|---|
| 25 | Teal means "New: our emulator", but it also colours the Sionna RT box (a third‑party library), the two note cards, the QAM points, the highlighted grid column and the bar tints. |
| 26 | The Hₖ arrow uses the teal "Loop step" style, but it is not a loop step. |
| 27 | Purple means "Spark hardware", but it also colours the RB bracket and the "RB" label. |
| 28 | There are two teal focus zones (Emulator loop, Sionna RT); the schema allows one. Zone tints are not in the legend. |
| 29 | IQ links are drawn double‑headed; the legend sample is single‑headed. |
| 30 | Subscripts are 20 px, under the schema's 22 px floor. |
| 31 | The banner reserve is 139 px (7.8 %); the schema says 10 % (179 px). The legend row (y 1594–1622) sits inside that 10 % band. |

## 5. Checked and correct

- **Socket roles.** DU `REP·REQ` ↔ broker `REQ·REP` ↔ broker `REP·REQ` ↔ UE `REQ·REP`. Both stacks bind a `REP` for TX and connect a `REQ` for RX, with cf32 on the wire (OCUDU `radio_zmq_*_channel.cpp`; OAI `zmq_radio.cpp` lines 326–352). This also **settles the dashboard's open item** on which side binds on the UE links: the UE's TX binds, so the broker connects a `REQ` to it; the broker binds a `REP` for the UE's RX.
- **Live publisher.** PUB `tcp://127.0.0.1:5566` (TOSSI).
- **Hardware.** GB10 has 20 Arm cores (10 X925 + 10 A725), a Blackwell GPU with 6,144 CUDA cores, and 128 GB of LPDDR5x at 273 GB/s on a 256‑bit bus (NVIDIA docs). NVLink‑C2C runs at 300 GB/s each way (Hot Chips reporting). There is one 10 GbE RJ‑45 port.
- **Cell numbers.** 40 MHz at 30 kHz gives N_RB = 106, which is 1,272 subcarriers. 61.44 Msps = 2048 × 30 kHz.
- **Slot length.** 30,720 samples = 14 × 2048 + 176 + 13 × 144. The long CP falls once per 0.5 ms (TS 38.211 §5.3.1). A slot is 14 symbols and 0.5 ms; a frame is 10 ms and 20 slots (μ = 1).
- **QPSK labels.** 00 (+,+), 01 (+,−), 10 (−,+), 11 (−,−), matching TS 38.211 §5.1.3.
- **Mapping and RB size.** Mapping is frequency‑first (PDSCH RE mapping, k before l), and an RB is 12 subcarriers.
- **Sionna names and signatures.** `tr38901.TDL`, `cir_to_time_channel`, `ApplyTimeChannel`, `AWGN`, `OFDMDemodulator(fft_size, l_min, cyclic_prefix_length)` and `OFDMModulator(cyclic_prefix_length)` all exist as written. Both OFDM classes accept a per‑symbol CP vector, so 176/144 can be expressed. Sionna 2.2.0 requires `torch>=2.9.1`, so PHY is PyTorch, and torch ships aarch64 CUDA 13 wheels.
- **Sionna RT calls.** `PathSolver()(scene)` and `Paths.cfr(frequencies, …)` exist, and `out_type="torch"` is allowed. `frequencies` are baseband offsets, since the documented coefficients already include the carrier phase. Sionna RT never takes IQ.
- **OAI on Arm.** OAI's ZMQ driver uses SIMDe, so it is portable to Arm.

## 6. Found on the dashboard while checking (not on the brief)

- "4× USB4 (40 Gb/s)" is probably wrong. NVIDIA lists 4× USB Type‑C, one of them for power; reviewers measure 20 Gb/s (USB 3.2 Gen 2x2).
- "24 MB L2" and "31 TFLOPS FP32" are not confirmed by any source found.
- The load estimate ("6.1 M MACs, tiny; launch overhead sets latency") ignores the `h_time` memory traffic in item 14.
- OAI ZMQ cannot reconnect once running, so restarting the broker means restarting every DU and UE. This is not written down.

## Sources

- OCUDU: `lib/radio/zmq/radio_zmq_tx_channel.cpp`, `radio_zmq_rx_channel.cpp`, `radio_session_zmq_impl.cpp`
- OAI `radio/zmq`: https://github.com/OPENAIRINTERFACE/openairinterface5g/tree/develop/radio/zmq
- Sionna 2.2.0: `phy/api/ofdm/sionna.phy.ofdm.OFDMModulator.html`, `…OFDMDemodulator.html`, `phy/api/channel/wireless/…ApplyOFDMChannel.html`, `…ApplyTimeChannel.html`, `…cir_to_time_channel.html`, `…tr38901.TDL.html`, `…time_lag_discrete_time_channel.html`, `rt/api/paths.html` at https://nvlabs.github.io/sionna/
- Sionna TDL source: https://github.com/NVlabs/sionna/blob/main/src/sionna/phy/channel/tr38901/tdl.py
- PyPI metadata for `sionna`, `sionna-rt`, `mitsuba`, `drjit`, `torch` (wheel tags)
- TOSSI, Sionna RT + OCUDU: https://tossi.org/docs/ran-integration/digital-twins/sionna-rt-ocudu-ran/
- NVIDIA DGX Spark hardware: https://docs.nvidia.com/dgx/dgx-spark/hardware.html
- GB10 at Hot Chips 2025: https://heise.de/-10624970 , https://www.hwcooling.net/en/gb10-or-n1x-nvidias-arm-superchip-packs-rtx-5070-class-gpu/
- USB speed: https://storagereview.com/review/nvidia-dgx-spark-review-the-ai-appliance-bringing-datacenter-capabilities-to-desktops
- Sionna RT on aarch64 report: https://github.com/rcbarke/ai-ran-dgx-spark
- 3GPP TS 38.211 §4.3.2, §4.4.4, §5.1.3, §5.3.1; TS 38.101‑1 Table 5.3.2‑1
