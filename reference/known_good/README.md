# Known-good Implementation Reference

The `.bit` and `.hwh` files in this directory are outputs from the final implementation dated 2026-07-31:

- bit SHA-256: `6e351e81fe2637429bd5b9f9350c680cc2dc45b583b9ace4fb724e62ccf28461`
- HWH SHA-256: `bc1d8a7d2eeba953cf72e08df630bb708d429acee04244992719dd1f72b16aff`

The original project's `build/rfsoc_waveform_tx_rx.xsa` predates the final RFDC changes and was therefore not copied. The `.xsa` here was generated when this template was created by rerunning `write_hw_platform -fixed -include_bit` against the final live implementation. Its embedded bit and HWH hashes match the two standalone files above.

`reports/` contains the timing summary, DRC, and route-status reports from the same routed implementation for future regression comparisons.

These files support regression comparison and rapid recovery; they are not editable HLS/BD inputs. The editable sources of truth are `hls/`, `ip_repo/`, and `vivado/source_bd/`. `vivado/block_design.tcl` is only a parameter-review and modification reference.
