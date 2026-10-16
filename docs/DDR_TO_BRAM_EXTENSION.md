# DDR-to-TX-BRAM Extension

## Data path

```text
PS software -> DDR buffer
                  |
                  v
       PS S_AXI_HP0_FPD (128 bit, non-coherent)
                  |
                  v
          DDR SmartConnect
                  |
                  v
       ddr_bram_bridge_tx 1.0 -> TX BRAM Port A
                                      |
                                      v
                             wave_player -> RFDC DAC
```

Only the TX `iq_bram_bridge` is replaced by `ddr_bram_bridge:1.0`.  The RX
`iq_bram_bridge:1.0` remains unchanged and does not expose an unused AXI master.

## HLS operation

DDR BRAM bridge 1.0 retains commands 0–2 and adds command 3:

| Command | Operation |
|---:|---|
| 0 | Write one 128-bit BRAM word from `sample0..3` |
| 1 | Read one 128-bit BRAM word into `read0..3` |
| 2 | Clear all 256 BRAM words |
| 3 | Copy consecutive 128-bit words from DDR into BRAM |

The DDR source address is a byte-addressed 64-bit physical address.  The
address must be 16-byte aligned because each AXI beat and BRAM word is 128 bits.
`transfer_word_count` is expressed in 128-bit words, not bytes.  The bridge
returns `words_transferred`; a request that would cross BRAM word 255 is
truncated.

## DDR BRAM bridge AXI-Lite registers

The TX control base remains `0xA0000000`.

| Offset | Access | Meaning |
|---:|---|---|
| `0x60` | W | DDR physical address bits 31:0 |
| `0x64` | W | DDR physical address bits 63:32 |
| `0x70` | W | Destination BRAM start word, 0–255 |
| `0x78` | W | Transfer length, 0–256 words |
| `0x80` | R | Actual transfer length |
| `0x84` | R/COR | Valid flag for `words_transferred` |

The standard HLS control register remains at offset `0x00`: write `ap_start`
after programming the address, destination, length, and command `3` at offset
`0x38`.  Wait for `ap_done` or `ap_idle` before consuming
`words_transferred` or starting the player.

## DDR allocation and cache coherency

`S_AXI_HP0_FPD` is deliberately used as a high-throughput, non-coherent path.
The current address map exposes physical DDR addresses
`0x00000000..0x7fffffff` to the bridge.

- Use a physically contiguous buffer wholly inside that range.
- Do not pass a Linux virtual address to the IP.
- On Linux, allocate/map the buffer with a proper DMA driver and DMA API.  Map
  or synchronize it for device access before starting the bridge.
- On bare metal, flush the written buffer from the CPU cache before starting
  the bridge, for example with the platform cache-maintenance API.
- Do not modify or free the DDR buffer until the bridge reports completion.
- Do not run `wave_player` while the bridge is updating TX BRAM.

## Integration boundary

The portable folder includes a known packaged `ddr_bram_bridge:1.0` under
`ip_repo/`, so the checked-in extension can be applied with `make ddr-project`.
After changing the HLS source, run `make hls` and `make stage-ip`, review the
new package, then select it explicitly:

```bash
make ddr-project DDR_IP_REPO=generated_ip_repo/ddr_bram_bridge_1_0
```

The integration script performs these BD changes:

1. Replace `iq_bram_bridge_tx` with `ddr_bram_bridge_tx:1.0` only.
2. Enable PS `S_AXI_HP0_FPD` at 128-bit width.
3. Insert a one-input/one-output DDR SmartConnect between `m_axi_gmem` and HP0
   to adapt AXI address, ID, and protocol details.
4. Clock the bridge, SmartConnect, and HP0 port from PS `pl_clk0` at 100 MHz.
5. Map the low 2 GiB DDR segment into `Data_m_axi_gmem`.
6. Validate, save, regenerate, and run the project assertions.
