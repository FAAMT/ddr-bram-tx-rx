# CLAUDE.md — ddr-bram-tx-rx

Inherits `~/.claude/CLAUDE.md` (NOMADIC OS) and `~/CLAUDE/projects/CLAUDE.md`.

## What this is

Fahad's own copy of Ruibin Chen's RFSoC 4x2 waveform TX/RX design: PS → TX BRAM → DAC and ADC → RX BRAM → PS, plus
an optional PS DDR → TX BRAM extension. Four Vitis HLS IPs, a Vivado 2025.1 block design, and PYNQ drivers and
notebooks for a Zadoff-Chu channel sounder. `README.md` is Ruibin's and covers the build.

| | |
|---|---|
| source | `github.com/Ruibin2000/DDR_BRAM_tx_rx_v1` at `84275a9e1dd1` ("version 1", 2026-08-11): one commit, no licence file |
| here | `github.com/FAAMT/ddr-bram-tx-rx`, public. Branch `ruibin-source` is the untouched import, kept as the reference for testing: Ruibin's files byte-identical to the source, plus an empty `test.txt` |

## A side project of Falcon, kept apart from it

Project Falcon is Fahad's research position at NYU Wireless, opened 2026-10-10. It is a private 5G SA network on an
NVIDIA DGX Spark, with OCUDU DUs on USRP B210s → FlexRIC → xApps, built by Ruibin, who also wrote this repo. Fahad's
Falcon tasks are wider bandwidth than 20 MHz per B210, an alternative to NVIDIA Aerial, uplink IQ to a GPU and out
over E2, and reuse on a higher-performance radio platform.

This repo is related but separate. The `falcon` session (`~/CLAUDE/projects/falcon`) owns Falcon; this session owns
only this directory.

- Read Falcon for context when it helps: `~/CLAUDE/projects/falcon/CLAUDE.md` and `docs/SCOPE.md`. Never write
  there, and never touch its clones under `upstream/`.
- How this repo feeds Falcon is not written down anywhere yet. Ask Fahad; don't assume.
- Anything that belongs to Falcon goes to Fahad as a note for the falcon session, not into this repo.

## Constraints

- **Public repo.** Nothing machine-specific goes in a tracked file: no tailnet hostnames, no session ids, no
  absolute home paths.
- **Commit as `FAAMT <82693292+FAAMT@users.noreply.github.com>`,** set in this repo's local git config. It is the
  identity Fahad's other public repos use. Never the gmail address.
- **Ruibin's code has no licence.** Keep his name on it: don't drop the source line above, and don't add a licence
  of our own without Fahad's word.
- **No build products in git.** `.gitignore` covers what the Makefile and Vivado generate. The README calls those
  ignored, but Ruibin's repo had no `.gitignore`. `reference/known_good/` is the one deliberate binary (a 34 MB
  `.bit`); GitHub refuses any file over 100 MB.
- **Nothing here builds on this machine.** Vivado/Vitis 2025.1 are not on PATH and `~/Xilinx` has no 2025.1
  install. Say so rather than claim a build passed.
