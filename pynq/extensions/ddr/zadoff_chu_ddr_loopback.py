"""Zadoff-Chu DDR -> DAC -> ADC loopback test for the RFSoC overlay.

Run this from a PYNQ notebook only after confirming that the physical DAC-to-ADC
loopback includes suitable attenuation.  Importing this module does not program
the FPGA or start the DAC.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any, Dict, Optional

import numpy as np

from rfsoc_sounder_ddr import RFSoCSounder
from zadoff_chu_bram_signal import ZadoffChuBramSignal


def run_loopback(
    *,
    attenuation_confirmed: bool = False,
    project_dir: Optional[str | Path] = None,
    center_mhz: float = 1000.0,
    amplitude: float = 0.9,
    duration_s: float = 0.040,
    plot_channel: bool = False,
) -> Dict[str, Any]:
    """Run and plot one complete DDR-backed RF loopback measurement."""

    if not attenuation_confirmed:
        raise RuntimeError(
            "DAC-to-ADC attenuation has not been confirmed. "
            "Do not start the loopback into an unprotected ADC input."
        )
    if not 0.0 < float(amplitude) <= 1.0:
        raise ValueError("amplitude must be in (0, 1]")
    if duration_s <= 0:
        raise ValueError("duration_s must be positive")

    if project_dir is None:
        project_path = Path(__file__).resolve().parent
    else:
        project_path = Path(project_dir)

    sounder = RFSoCSounder(
        project_dir=project_path,
        bit_name="rfsoc_waveform_tx_rx.bit",
    )

    startup_report = sounder.startup(
        configure_clocks=True,
        lmk_freq_mhz=245.76,
        lmx_freq_mhz=491.52,
        download=True,
        run_self_test=True,
        capture_smoke_test=True,
    )
    sounder.print_self_test(startup_report)
    print()
    print(sounder.summary())

    tx_settings = sounder.configure_tx(
        center_mhz=center_mhz,
        nyquist_zone=1,
        duration_s=max(float(duration_s), 0.05),
        phase_offset_deg=0.0,
    )
    rx_settings = sounder.configure_rx(
        center_mhz=center_mhz,
        nyquist_zone=1,
        calibration_mode=2,
        dither=True,
        capture_timeout_s=0.5,
        settle_s=0.010,
    )

    print("TX settings:", tx_settings)
    print("RX settings:", rx_settings)

    signal = ZadoffChuBramSignal(
        n_samples=sounder.frame_samples,
        sc_range=(-250, 250),
        root=3,
        zc_q=0.0,
        amplitude=float(amplitude),
    )
    if not signal.verify_packing():
        raise RuntimeError("Zadoff-Chu IQ packing self-check failed")

    active_unshifted = np.ones(sounder.frame_samples, dtype=bool)
    active_unshifted[
        signal.sc_range[1]:sounder.frame_samples + signal.sc_range[0]
    ] = False
    active_mask = np.fft.fftshift(active_unshifted)

    waveform = sounder.set_waveform(
        packed=signal.get_bram_data(copy=True),
        active_mask=active_mask,
    )

    # This overridden method performs no AXI-Lite single-word writes.  It
    # allocates one contiguous 4096-byte CMA buffer, flushes it for the
    # non-coherent HP0 path, launches command 3, and reads the full BRAM back.
    load_report = sounder.load_waveform(verify=True)
    print("Waveform I range:", int(waveform.i.min()), "to", int(waveform.i.max()))
    print("Waveform Q range:", int(waveform.q.min()), "to", int(waveform.q.max()))
    print("DDR load report:", load_report)

    if load_report["bytes_copied"] != 4096:
        raise RuntimeError("DDR bridge did not copy one complete 4096-byte frame")

    # Explicitly disable auto-loading: after the verified DDR transfer, nothing
    # may overwrite TX BRAM before wave_player starts.
    capture = sounder.transmit_and_capture(
        duration_s=float(duration_s),
        settle_s=0.010,
        capture_timeout_s=0.5,
        auto_load_waveform=False,
        require_frame_count_match=True,
    )

    print("Captured words:", capture.captured_words)
    print("Frames requested:", capture.frames_requested)
    print("Frames completed:", capture.frames_completed)
    print("Capture time:", capture.capture_time_s, "s")
    print("Total time:", capture.total_time_s, "s")

    stats = sounder.capture_statistics(capture)
    for key, value in stats.items():
        print(f"{key:24s}: {value}")

    if stats["nonzero_i"] == 0 or stats["nonzero_q"] == 0:
        raise RuntimeError("ADC capture is zero; check RF loopback and RFDC setup")
    if stats["peak_full_scale_ratio"] >= 0.95:
        print("WARNING: ADC is close to clipping; increase loopback attenuation")

    spectrum = sounder.plot_spectra(
        capture,
        rf_axis=True,
        noise_referenced=True,
        floor_db=-30.0,
    )
    print(
        "TX mean active/noise:",
        spectrum["tx_mean_active_above_noise_db"],
        "dB",
    )
    print(
        "RX mean active/noise:",
        spectrum["rx_mean_active_above_noise_db"],
        "dB",
    )

    tx_iq = waveform.iq.astype(np.complex128)
    rx_iq = capture.iq.astype(np.complex128)
    circular_correlation = np.fft.ifft(
        np.fft.fft(rx_iq) * np.conj(np.fft.fft(tx_iq))
    )
    correlation_peak_index = int(np.argmax(np.abs(circular_correlation)))
    correlation_peak = float(
        np.abs(circular_correlation[correlation_peak_index])
        / np.sqrt(np.sum(np.abs(tx_iq) ** 2) * np.sum(np.abs(rx_iq) ** 2))
    )
    print("Circular correlation peak index:", correlation_peak_index)
    print("Normalized correlation peak:", correlation_peak)

    channel = None
    if plot_channel:
        channel = sounder.plot_channel(
            capture,
            definition="standard",
            rf_axis=True,
        )
        print(
            "Estimated channel delay:",
            channel["estimated_delay_s"] * 1e9,
            "ns",
        )

    return {
        "sounder": sounder,
        "startup_report": startup_report,
        "tx_settings": tx_settings,
        "rx_settings": rx_settings,
        "signal": signal,
        "waveform": waveform,
        "load_report": load_report,
        "capture": capture,
        "statistics": stats,
        "spectrum": spectrum,
        "correlation_peak_index": correlation_peak_index,
        "correlation_peak": correlation_peak,
        "channel": channel,
    }


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description="Run the attenuated RFSoC DDR loopback test"
    )
    parser.add_argument(
        "--confirm-attenuated-loopback",
        action="store_true",
        help="confirm that the DAC-to-ADC cable path includes attenuation",
    )
    parser.add_argument("--amplitude", type=float, default=0.9)
    parser.add_argument("--duration", type=float, default=0.040)
    parser.add_argument("--plot-channel", action="store_true")
    args = parser.parse_args()

    RESULTS = run_loopback(
        attenuation_confirmed=args.confirm_attenuated_loopback,
        amplitude=args.amplitude,
        duration_s=args.duration,
        plot_channel=args.plot_channel,
    )
