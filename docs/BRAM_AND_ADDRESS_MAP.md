# BRAM, Waveform Layout, and Address Map

## Fixed configuration of both BRAMs

TX and RX each use an independent Block Memory Generator:

| Parameter | Fixed TX/RX value |
|---|---|
| Memory type | True Dual Port RAM |
| A/B data width | 128 bit |
| Depth | 256 words |
| Capacity | 4096 bytes |
| Addressing | Byte address |
| Common clock | false; asynchronous A/B clocks |
| Operating mode A/B | `WRITE_FIRST` |
| Byte write enable | true; 8-bit bytes; 16-bit WEA/WEB |
| Physical BMG read latency | 1 cycle on both A and B |
| Extra output registers | All disabled |

The authoritative rebuild source is the complete BD and cell-XCI bundle in `vivado/source_bd/`. Both BMG XCI files explicitly preserve `Write_Depth_A=256` and the propagated latency metadata for all four interfaces. Vivado 2025.1 `write_bd_tcl` omits these read-only interface values, so `vivado/block_design.tcl` is only for review and modification and must not be used by itself as the rebuild entry point. After `create_project.tcl` copies the complete BD source, `verify_design.tcl` checks both the physical BMG parameters and the interface metadata.

## Ports, clocks, and interface latency

| BRAM | Port A | Port B |
|---|---|---|
| TX | `iq_bram_bridge_tx`, 100 MHz, interface `READ_LATENCY=0` | `wave_player_0`, 245.76 MHz, interface `READ_LATENCY=1` |
| RX | `wave_capture_0`, 245.76 MHz, interface `READ_LATENCY=1` | `iq_bram_bridge_rx`, 100 MHz, interface `READ_LATENCY=0` |

Interface `READ_LATENCY` is HLS/BD scheduling metadata and is not the same as the BMG's physical one-cycle synchronous read latency. Do not attempt to write `MEM_SIZE`, `MEM_WIDTH`, or `READ_LATENCY` on a BRAM interface pin; these are propagated read-only properties. After updating an HLS IP or reconnecting BRAM, revalidate all four interface values and save the updated BD together with every cell XCI.

## BRAM byte addressing

The HLS port uses byte addressing. The address of 128-bit word `word_index` is:

```text
byte_address = word_index << 4
word_index   = 0 ... 255
address      = 0x000, 0x010, ... 0xFF0
```

The last word covers `0xFF0...0xFFF`, giving exactly 4096 bytes. Do not use `word_index` directly as the BRAM byte address.

## IQ data layout

Each 32-bit complex sample:

```text
bits 15:0  = signed 16-bit I
bits 31:16 = signed 16-bit Q
```

Each 128-bit word:

```text
bits  31:0   = sample0 = {Q0, I0}
bits  63:32  = sample1 = {Q1, I1}
bits  95:64  = sample2 = {Q2, I2}
bits 127:96  = sample3 = {Q3, I3}

whole word = {Q3,I3,Q2,I2,Q1,I1,Q0,I0}
```

Software should pack one lane as:

```text
packed_sample = (uint16_t(Q) << 16) | uint16_t(I)
```

Each frame contains `256 × 4 = 1024` complex samples and occupies 4096 bytes.

## PS AXI-Lite address map

The BRAM blocks are not mapped directly into the PS address space; the PS accesses them through the two IQ bridges.

| Address range | Size | Peripheral |
|---|---:|---|
| `0xA0000000–0xA0000FFF` | 4 KiB | TX IQ bridge |
| `0xA0010000–0xA0010FFF` | 4 KiB | Wave player |
| `0xA0020000–0xA0020FFF` | 4 KiB | RX IQ bridge |
| `0xA0030000–0xA0030FFF` | 4 KiB | Wave capture |
| `0xA0040000–0xA007FFFF` | 256 KiB | RFDC |

## HLS register offsets

Common registers in every HLS IP: control/GIE/IER/ISR are at `0x00/0x04/0x08/0x0C`.

IQ bridge 1.0, used by RX and by the original TX design:

| Offset | Register |
|---:|---|
| `0x10/18/20/28` | `sample0/1/2/3` |
| `0x30` | `word_index`, valid range 0–255 |
| `0x38` | `command`: 0=write, 1=read, 2=clear |
| `0x40/48/50/58` | `read0/1/2/3` |
| `0x44/4C/54/5C` | `ap_vld` control for each read output |

TX `ddr_bram_bridge:1.0` retains those registers and adds:

| Offset | Register |
|---:|---|
| `0x60/0x64` | 64-bit physical DDR source address, low/high word |
| `0x70` | Destination BRAM start word, valid range 0–255 |
| `0x78` | Requested transfer length, 0–256 words |
| `0x80` | Actual words transferred |
| `0x84` | `ap_vld` control for `words_transferred` |

Bridge command `3` copies consecutive 128-bit words from DDR into TX BRAM.
If `start + length` exceeds 256, hardware truncates the transfer at the end of
BRAM.  See `docs/DDR_TO_BRAM_EXTENSION.md` for integration and cache rules.

Wave player: `repeat_count=0x10`, `frames_completed=0x18`, valid=`0x1C`.

Wave capture: `captured_words=0x10`, valid=`0x14`.

## Cross-clock access rules

The two TDP BRAMs provide a CDC storage boundary but no transaction-level snapshot or mutual exclusion. Follow these rules:

- Do not write or clear TX BRAM through the bridge while `wave_player` is reading it.
- Do not read, write, or clear RX BRAM through the bridge while `wave_capture` is writing it.
- Wait for capture `ap_done` before reading RX BRAM.
- For live updates or continuous capture, extend the design with ping-pong BRAM or an explicit hardware ownership handshake.
