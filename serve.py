#!/usr/bin/env python3
"""Serves the project page on 127.0.0.1:8667, and nothing else.

Only the paths listed below go out. This tree also holds a 34 MB bitstream, an
.xsa, vendor IP zips and generated HDL; a plain `http.server` here would hand all
of it to the tailnet. So the page, its state, the source files named in SOURCES,
the three reference reports and the validation record are served, and every
other path is a 404. A tailnet proxy fronts this; it is not configured here.

Everything is read from disk per request, so the page cannot show a file, a
commit or a timing figure that is not there now.
"""
import json
import os
import re
import shutil
import subprocess
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

HERE = Path(__file__).resolve().parent
PORT = 8667
SLUG = "ddr-bram-tx-rx"
# The page's name, description and tile colour come from the services register,
# not from this repo; without it the page falls back to the slug.
REGISTER = Path(os.environ.get("NOMAD_PAGES_TSV", Path.home() / "CLAUDE/shotcheck/pages.tsv"))

# Two versions of the demo, one folder each, so their pages never mix:
# v1 is the RFSoC 4x2 FPGA data path, v2 the DGX Spark virtual channel emulator.
PAGES = {
    "/rfsoc/": HERE / "web/rfsoc/index.html",
    "/spark/": HERE / "web/spark/index.html",
    # The dashboard as it was before the final slide, kept for reference.
    "/spark/notes": HERE / "web/spark/notes.html",
    # The outside-audience version: no source, no internal plan, prints as a PDF.
    "/spark/brief": HERE / "web/spark/brief.html",
    # The same brief in Ruibin's layout, kept beside the original rather than replacing it.
    "/spark/brief-ruibin": HERE / "web/spark/brief-ruibin.html",
    # The final slide, with the UAV scenario: what the dashboard leads with.
    "/spark/brief-uav": HERE / "web/spark/brief-uav.html",
}
# Scripts the brief pages share. The .pptx export builds the deck in the browser, from the page's own SVG.
SCRIPTS = {
    "/spark/slide-pptx.js": HERE / "web/spark/slide-pptx.js",
}
# Files the pages show or offer, each with its type.
FILES = {
    "/spark/uav-scenario.jpg": (HERE / "web/spark/uav-scenario.jpg", "image/jpeg"),
    "/spark/plan.md": (HERE / "web/spark/plan.md", "text/markdown; charset=utf-8"),
}
# Old and bare addresses land on the current version rather than on a 404.
REDIRECTS = {
    "/": "/spark/",
    "/index.html": "/spark/",
    "/rfsoc": "/rfsoc/",
    "/spark": "/spark/",
    "/brief": "/spark/brief",
}

REPORTS = {
    "timing_summary_routed.rpt": HERE / "reference/known_good/reports/timing_summary_routed.rpt",
    "drc_routed.rpt": HERE / "reference/known_good/reports/drc_routed.rpt",
    "route_status.rpt": HERE / "reference/known_good/reports/route_status.rpt",
}
VALIDATION = HERE / "docs/VALIDATION.md"

# (group, path). Explicit on purpose: a glob would publish whatever lands here next.
SOURCES = [
    ("HLS · ddr_bram_bridge", "hls/ddr_bram_bridge/src/ddr_bram_bridge.hpp"),
    ("HLS · ddr_bram_bridge", "hls/ddr_bram_bridge/src/ddr_bram_bridge.cpp"),
    ("HLS · ddr_bram_bridge", "hls/ddr_bram_bridge/tb/tb_ddr_bram_bridge.cpp"),
    ("HLS · ddr_bram_bridge", "hls/ddr_bram_bridge/hls_config.cfg"),
    ("HLS · channel_emulator", "hls/channel_emulator/src/channel_emulator.hpp"),
    ("HLS · channel_emulator", "hls/channel_emulator/src/channel_emulator.cpp"),
    ("HLS · channel_emulator", "hls/channel_emulator/tb/tb_channel_emulator.cpp"),
    ("HLS · channel_emulator", "hls/channel_emulator/hls_config.cfg"),
    ("HLS · other IP", "hls/iq_bram_bridge/src/iq_bram_bridge.hpp"),
    ("HLS · other IP", "hls/iq_bram_bridge/src/iq_bram_bridge.cpp"),
    ("HLS · other IP", "hls/iq_bram_bridge/tb/tb_iq_bram_bridge.cpp"),
    ("HLS · other IP", "hls/wave_player_128/src/wave_player.hpp"),
    ("HLS · other IP", "hls/wave_player_128/src/wave_player.cpp"),
    ("HLS · other IP", "hls/wave_player_128/tb/tb_wave_player.cpp"),
    ("HLS · other IP", "hls/wave_capture_128/src/wave_capture.hpp"),
    ("HLS · other IP", "hls/wave_capture_128/src/wave_capture.cpp"),
    ("HLS · other IP", "hls/wave_capture_128/tb/tb_wave_capture.cpp"),
    ("ip_repo HDL · ddr_bram_bridge", "ip_repo/ddr_bram_bridge_1_0/hdl/verilog/ddr_bram_bridge.v"),
    ("ip_repo HDL · ddr_bram_bridge", "ip_repo/ddr_bram_bridge_1_0/hdl/verilog/ddr_bram_bridge_control_s_axi.v"),
    ("ip_repo HDL · ddr_bram_bridge", "ip_repo/ddr_bram_bridge_1_0/hdl/verilog/ddr_bram_bridge_ddr_bram_bridge_Pipeline_ddr_to_bram.v"),
    ("ip_repo HDL · ddr_bram_bridge", "ip_repo/ddr_bram_bridge_1_0/hdl/verilog/ddr_bram_bridge_ddr_bram_bridge_Pipeline_clear_bram.v"),
    ("ip_repo HDL · ddr_bram_bridge", "ip_repo/ddr_bram_bridge_1_0/hdl/verilog/ddr_bram_bridge_gmem_m_axi.v"),
    ("ip_repo HDL · ddr_bram_bridge", "ip_repo/ddr_bram_bridge_1_0/hdl/verilog/ddr_bram_bridge_flow_control_loop_pipe_sequential_init.v"),
    ("ip_repo HDL · iq_bram_bridge", "ip_repo/iq_bram_bridge_1_0/hdl/verilog/iq_bram_bridge.v"),
    ("ip_repo HDL · iq_bram_bridge", "ip_repo/iq_bram_bridge_1_0/hdl/verilog/iq_bram_bridge_control_s_axi.v"),
    ("ip_repo HDL · iq_bram_bridge", "ip_repo/iq_bram_bridge_1_0/hdl/verilog/iq_bram_bridge_iq_bram_bridge_Pipeline_clear_bram.v"),
    ("ip_repo HDL · iq_bram_bridge", "ip_repo/iq_bram_bridge_1_0/hdl/verilog/iq_bram_bridge_flow_control_loop_pipe_sequential_init.v"),
    ("ip_repo HDL · wave_player", "ip_repo/wave_player_128_1_0/hdl/verilog/wave_player.v"),
    ("ip_repo HDL · wave_player", "ip_repo/wave_player_128_1_0/hdl/verilog/wave_player_control_s_axi.v"),
    ("ip_repo HDL · wave_player", "ip_repo/wave_player_128_1_0/hdl/verilog/wave_player_wave_player_Pipeline_stream_loop.v"),
    ("ip_repo HDL · wave_player", "ip_repo/wave_player_128_1_0/hdl/verilog/wave_player_regslice_both.v"),
    ("ip_repo HDL · wave_capture", "ip_repo/wave_capture_128_1_1/hdl/verilog/wave_capture.v"),
    ("ip_repo HDL · wave_capture", "ip_repo/wave_capture_128_1_1/hdl/verilog/wave_capture_control_s_axi.v"),
    ("ip_repo HDL · wave_capture", "ip_repo/wave_capture_128_1_1/hdl/verilog/wave_capture_regslice_both.v"),
    ("PYNQ", "pynq/src/rfsoc_sounder.py"),
    ("PYNQ", "pynq/src/zadoff_chu_bram_signal.py"),
    ("PYNQ", "pynq/extensions/ddr/rfsoc_sounder_ddr.py"),
    ("PYNQ", "pynq/extensions/ddr/zadoff_chu_ddr_loopback.py"),
    ("Vivado", "vivado/create_project.tcl"),
    ("Vivado", "vivado/block_design.tcl"),
    ("Vivado", "vivado/apply_ddr_to_bram.tcl"),
    ("Vivado", "vivado/build_bitstream.tcl"),
    ("Vivado", "vivado/verify_design.tcl"),
    ("Vivado", "vivado/verify_project.tcl"),
    ("Vivado", "vivado/stage_source_bd.tcl"),
    ("Vivado", "vivado/export_hw_platform.tcl"),
    ("Vivado", "vivado/export_bd_snapshot.tcl"),
    ("Vivado", "vivado/dump_bd_properties.tcl"),
    ("Vivado", "vivado/inspect_bram_interfaces.tcl"),
    ("Vivado", "vivado/constraints.xdc"),
]
SOURCE_PATHS = {"/src/" + p: HERE / p for _, p in SOURCES}

# The HLS designs with a self-checking testbench, and what VALIDATION.md records for each.
HLS = [
    ("ddr_bram_bridge", "tb/tb_ddr_bram_bridge.cpp", "csim passed, zero mismatches, 2026-08-11"),
    ("iq_bram_bridge", "tb/tb_iq_bram_bridge.cpp", "csim passed, zero mismatches, 2026-08-11"),
    ("wave_player_128", "tb/tb_wave_player.cpp", "csim passed, zero mismatches, 2026-08-11"),
    ("wave_capture_128", "tb/tb_wave_capture.cpp", "csim passed, zero mismatches, 2026-08-11"),
    ("channel_emulator", "tb/tb_channel_emulator.cpp", None),
]


def git(*args: str) -> str | None:
    try:
        r = subprocess.run(["git", "-C", str(HERE), *args], capture_output=True, text=True, timeout=10)
    except (OSError, subprocess.TimeoutExpired):
        return None
    return r.stdout.strip() if r.returncode == 0 else None


def register() -> dict:
    try:
        for line in REGISTER.read_text(encoding="utf-8").splitlines():
            cols = line.split("\t")
            if cols and cols[0] == SLUG and len(cols) >= 8:
                return {"display": cols[1], "desc": cols[5], "color": cols[7]}
    except OSError:
        pass
    return {"display": SLUG, "desc": "", "color": None}


def read(path: Path) -> str | None:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None


def header_field(text: str, name: str) -> str | None:
    m = re.search(rf"^\|\s*{name}\s*:\s*(.+?)\s*$", text, re.M)
    return m.group(1) if m else None


def timing() -> dict | None:
    text = read(REPORTS["timing_summary_routed.rpt"])
    if text is None:
        return None
    out = {"date": header_field(text, "Date"), "tool": header_field(text, "Tool Version"),
           "state": header_field(text, "Design State"), "met": "All user specified timing constraints are met." in text}
    # The Design Timing Summary is one header row and one row of twelve numbers.
    sec = text.split("| Design Timing Summary", 1)
    if len(sec) == 2:
        rows = [ln for ln in sec[1].splitlines() if re.match(r"^\s*-?\d", ln)]
        if rows:
            v = rows[0].split()
            keys = ["wns", "tns", "tns_fail", "tns_total", "whs", "ths", "ths_fail", "ths_total",
                    "wpws", "tpws", "tpws_fail", "tpws_total"]
            if len(v) == 12:
                out.update(dict(zip(keys, v)))
    # Clock Summary and the per-clock table, verbatim.
    for key, start, stop in (("clocks", "| Clock Summary", "| Intra Clock Table"),
                             ("intra", "| Intra Clock Table", "| Inter Clock Table"),
                             ("inter", "| Inter Clock Table", "| Other Path Groups Table")):
        if start in text and stop in text:
            body = text.split(start, 1)[1].split(stop, 1)[0]
            lines = [ln.rstrip() for ln in body.splitlines()[3:] if ln.strip() and not ln.startswith("---")]
            out[key] = "\n".join(lines)
    return out


def route() -> dict | None:
    text = read(REPORTS["route_status.rpt"])
    if text is None:
        return None
    def num(label):
        m = re.search(rf"# of {label}\.*\s*:\s*(\d+)", text)
        return int(m.group(1)) if m else None
    return {"logical": num("logical nets"), "routable": num("routable nets"),
            "routed": num("fully routed nets"), "errors": num("nets with routing errors"), "raw": text}


def drc() -> dict | None:
    text = read(REPORTS["drc_routed.rpt"])
    if text is None:
        return None
    m = re.search(r"Checks found:\s*(\d+)", text)
    rows = re.findall(r"^\|\s*([A-Z]+-\d+)\s*\|\s*(\w+)\s*\|\s*(.+?)\s*\|\s*(\d+)\s*\|", text, re.M)
    return {"date": header_field(text, "Date"), "found": int(m.group(1)) if m else None,
            "rows": [{"rule": r, "severity": s, "desc": d.strip(), "checks": int(c)} for r, s, d, c in rows]}


def hls_results() -> list:
    out = []
    for design, tb, recorded in HLS:
        d = HERE / "hls" / design
        # A Vitis csim run leaves its log under build/hls/<design>/; none is committed.
        logs = sorted((HERE / "build/hls" / design).glob("**/csim/report/*_csim.log"))
        local = None
        if logs:
            log = read(logs[-1]) or ""
            hit = re.findall(r"^.*(?:PASSED|FAILED|mismatch).*$", log, re.M | re.I)
            local = hit[-1].strip() if hit else "log present, no pass/fail line"
        out.append({"design": design, "tb": f"hls/{design}/{tb}", "present": (d / tb).exists(),
                    "recorded": recorded, "local": local})
    return out


def state() -> dict:
    head = git("log", "-1", "--format=%h%x09%s%x09%cs")
    head_sha, head_msg, head_date = (head.split("\t") + ["", "", ""])[:3] if head else (None, None, None)
    upstream = git("rev-parse", "--short", "origin/main")
    counts = git("rev-list", "--left-right", "--count", "HEAD...origin/main")
    ahead, behind = (int(x) for x in counts.split()) if counts else (None, None)
    dirty = [ln for ln in (git("status", "--porcelain") or "").splitlines() if ln.strip()]
    tracked = set((git("ls-files") or "").splitlines())
    files = []
    for group, p in SOURCES:
        f = HERE / p
        if f.is_file():
            files.append({"group": group, "path": p, "bytes": f.stat().st_size, "tracked": p in tracked})
    return {
        "register": register(),
        "git": {"branch": git("rev-parse", "--abbrev-ref", "HEAD"), "head": head_sha, "subject": head_msg,
                "date": head_date, "origin_main": upstream, "ahead": ahead, "behind": behind,
                "dirty": dirty, "has_ruibin_source": git("rev-parse", "--verify", "-q", "ruibin-source") is not None,
                "context": {"CLAUDE.md", ".gitignore"} <= tracked,
                "emulator": (HERE / "hls/channel_emulator/src/channel_emulator.cpp").is_file(),
                "emulator_tracked": "hls/channel_emulator/src/channel_emulator.cpp" in tracked},
        "tools": {t: shutil.which(t) is not None for t in ("vitis-run", "v++", "vivado")},
        "timing": timing(), "route": route(), "drc": drc(),
        "hls": hls_results(), "files": files,
        "validation": read(VALIDATION),
    }


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        path = self.path.split("?", 1)[0].split("#", 1)[0]
        if path in REDIRECTS:
            self.send_response(302)
            self.send_header("Location", REDIRECTS[path])
            self.send_header("Content-Length", "0")
            self.end_headers()
            return
        if path in PAGES:
            body, kind = PAGES[path].read_bytes(), "text/html; charset=utf-8"
        elif path in SCRIPTS:
            body, kind = SCRIPTS[path].read_bytes(), "text/javascript; charset=utf-8"
        elif path in FILES and FILES[path][0].is_file():
            body, kind = FILES[path][0].read_bytes(), FILES[path][1]
        elif path == "/state.json":
            body, kind = json.dumps(state(), ensure_ascii=False).encode(), "application/json"
        elif path in SOURCE_PATHS and SOURCE_PATHS[path].is_file():
            body, kind = SOURCE_PATHS[path].read_bytes(), "text/plain; charset=utf-8"
        elif path.startswith("/report/") and path[8:] in REPORTS:
            body, kind = REPORTS[path[8:]].read_bytes(), "text/plain; charset=utf-8"
        else:
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header("Content-Type", kind)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-cache")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(body)

    def do_HEAD(self):
        self.do_GET()

    def log_message(self, *args):
        pass


if __name__ == "__main__":
    ThreadingHTTPServer(("127.0.0.1", PORT), Handler).serve_forever()
