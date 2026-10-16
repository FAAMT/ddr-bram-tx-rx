from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from pprint import pformat
from typing import Any, Dict, Optional, Sequence, Tuple

import time
import numpy as np

try:
    import xrfclk
    import xrfdc
    from pynq import Overlay
except ImportError as exc:  # Allows the file to be inspected on a non-PYNQ host.
    xrfclk = None
    xrfdc = None
    Overlay = None
    _PYNQ_IMPORT_ERROR = exc
else:
    _PYNQ_IMPORT_ERROR = None


@dataclass
class WaveformData:
    """One 1024-sample complex waveform and its BRAM representation."""

    i: np.ndarray
    q: np.ndarray
    iq: np.ndarray
    packed: np.ndarray
    tx_fd: Optional[np.ndarray] = None
    active_mask: Optional[np.ndarray] = None


@dataclass
class CaptureResult:
    """Result returned by transmit_and_capture() or capture_frame()."""

    packed: np.ndarray
    i: np.ndarray
    q: np.ndarray
    iq: np.ndarray
    captured_words: int
    frames_requested: int = 0
    frames_completed: int = 0
    capture_time_s: float = 0.0
    total_time_s: float = 0.0


class RFSoCSounder:
    """
    Controller for the RFSoC TX/RX waveform sounder overlay.

    Expected block-design instances:
        iq_bram_bridge_tx
        iq_bram_bridge_rx
        wave_player_0
        wave_capture_0
        usp_rf_data_converter_0

    Data format:
        bits 15:0  = signed int16 I
        bits 31:16 = signed int16 Q

    Typical workflow:
        sounder = RFSoCSounder(Path.cwd())
        sounder.startup()
        sounder.configure_tx(center_mhz=1000.0, duration_s=1.0)
        sounder.configure_rx(center_mhz=1000.0)
        sounder.set_zadoff_chu()
        sounder.load_waveform(verify=True)
        result = sounder.transmit_and_capture()
        sounder.plot_spectra(result, rf_axis=True)
    """

    # HLS control register
    REG_CTRL = 0x00

    # iq_bram_bridge register map
    REG_SAMPLE0 = 0x10
    REG_SAMPLE1 = 0x18
    REG_SAMPLE2 = 0x20
    REG_SAMPLE3 = 0x28
    REG_WORD_INDEX = 0x30
    REG_COMMAND = 0x38
    REG_READ0 = 0x40
    REG_READ1 = 0x48
    REG_READ2 = 0x50
    REG_READ3 = 0x58

    CMD_WRITE = 0
    CMD_READ = 1
    CMD_CLEAR = 2

    # wave_player register map
    WP_REPEAT_COUNT = 0x10
    WP_FRAMES_COMPLETED = 0x18

    # wave_capture register map
    WC_CAPTURED_WORDS = 0x10

    EXPECTED_ADDRESSES = {
        "iq_bram_bridge_tx": 0xA000_0000,
        "wave_player_0": 0xA001_0000,
        "iq_bram_bridge_rx": 0xA002_0000,
        "wave_capture_0": 0xA003_0000,
        "usp_rf_data_converter_0": 0xA004_0000,
    }

    def __init__(
        self,
        project_dir: str | Path,
        bit_name: str = "rfsoc_waveform_tx_rx.bit",
        *,
        rf_sample_rate_hz: float = 3.93216e9,
        interpolation: int = 4,
        decimation: int = 4,
        frame_samples: int = 1024,
        iq_per_word: int = 4,
        adc_tile_index: int = 0,
        adc_block_index: int = 0,
        dac_tile_index: int = 0,
        dac_block_index: int = 0,
    ) -> None:
        self.project_dir = Path(project_dir)
        self.bit_path = self.project_dir / bit_name
        self.hwh_path = self.bit_path.with_suffix(".hwh")

        self.rf_sample_rate_hz = float(rf_sample_rate_hz)
        self.interpolation = int(interpolation)
        self.decimation = int(decimation)
        self.frame_samples = int(frame_samples)
        self.iq_per_word = int(iq_per_word)
        self.bram_word_count = self.frame_samples // self.iq_per_word
        self.fs_complex_hz = self.rf_sample_rate_hz / self.decimation
        self.frame_duration_s = self.frame_samples / self.fs_complex_hz
        self.frame_rate_hz = self.fs_complex_hz / self.frame_samples
        self.fft_bin_hz = self.fs_complex_hz / self.frame_samples

        if self.frame_samples % self.iq_per_word:
            raise ValueError("frame_samples must be divisible by iq_per_word")

        self.adc_tile_index = int(adc_tile_index)
        self.adc_block_index = int(adc_block_index)
        self.dac_tile_index = int(dac_tile_index)
        self.dac_block_index = int(dac_block_index)

        self.overlay = None
        self.tx_bridge = None
        self.rx_bridge = None
        self.wave_player = None
        self.wave_capture = None
        self.rfdc = None
        self.adc_tile = None
        self.dac_tile = None
        self.adc_block = None
        self.dac_block = None

        self.waveform: Optional[WaveformData] = None
        self.waveform_loaded = False

        self.tx_center_mhz: Optional[float] = None
        self.rx_center_mhz: Optional[float] = None
        self.tx_duration_s = 1.0
        self.tx_repeat_count = int(round(self.frame_rate_hz))
        self.rx_capture_timeout_s = 0.5
        self.rx_settle_s = 0.020

    # ------------------------------------------------------------------
    # Startup and self-test
    # ------------------------------------------------------------------

    def startup(
        self,
        *,
        configure_clocks: bool = True,
        lmk_freq_mhz: float = 245.76,
        lmx_freq_mhz: float = 491.52,
        download: bool = True,
        run_self_test: bool = True,
        capture_smoke_test: bool = True,
        strict: bool = True,
    ) -> Dict[str, Any]:
        """Configure clocks, load the overlay, bind IPs, and optionally self-test."""

        self._require_pynq_runtime()
        self._check_files()

        if configure_clocks:
            xrfclk.set_ref_clks(
                lmk_freq=float(lmk_freq_mhz),
                lmx_freq=float(lmx_freq_mhz),
            )

        self.overlay = Overlay(str(self.bit_path), download=download)
        self._bind_hardware()

        if run_self_test:
            return self.self_test(
                capture_smoke_test=capture_smoke_test,
                strict=strict,
            )

        return {"overlay_loaded": bool(self.overlay.is_loaded())}

    def self_test(
        self,
        *,
        capture_smoke_test: bool = True,
        strict: bool = True,
    ) -> Dict[str, Any]:
        """
        Check overlay IPs, addresses, HLS states, PLL/FIFO status, and ADC capture.

        If strict=True, a failed required condition raises RuntimeError.
        """

        self._require_started()

        report: Dict[str, Any] = {
            "files": {
                "bit": str(self.bit_path),
                "hwh": str(self.hwh_path),
            },
            "overlay_loaded": bool(self.overlay.is_loaded()),
            "addresses": {},
            "hls": {},
            "rfdc": {},
            "capture_smoke_test": None,
        }
        failures = []

        for name, expected in self.EXPECTED_ADDRESSES.items():
            info = self.overlay.ip_dict.get(name)
            actual = None if info is None else int(info["phys_addr"])
            report["addresses"][name] = {
                "actual": actual,
                "expected": expected,
                "pass": actual == expected,
            }
            if actual != expected:
                failures.append(
                    f"{name} address is {actual!r}, expected 0x{expected:08X}"
                )

        for name, ip in (
            ("tx_bridge", self.tx_bridge),
            ("rx_bridge", self.rx_bridge),
            ("wave_player", self.wave_player),
            ("wave_capture", self.wave_capture),
        ):
            ctrl = int(ip.read(self.REG_CTRL))
            decoded = self.decode_hls_ctrl(ctrl)
            decoded["pass"] = decoded["ap_idle"] == 1
            report["hls"][name] = decoded
            if decoded["ap_idle"] != 1:
                failures.append(f"{name} is not idle: 0x{ctrl:08X}")

        adc_block_status = dict(self.adc_block.BlockStatus)
        dac_block_status = dict(self.dac_block.BlockStatus)

        report["rfdc"] = {
            "adc_pll_lock": int(self.adc_tile.PLLLockStatus),
            "dac_pll_lock": int(self.dac_tile.PLLLockStatus),
            "adc_fifo": int(self.adc_tile.FIFOStatus),
            "dac_fifo": int(self.dac_tile.FIFOStatus),
            "adc_pll_config": dict(self.adc_tile.PLLConfig),
            "dac_pll_config": dict(self.dac_tile.PLLConfig),
            "adc_block_status": adc_block_status,
            "dac_block_status": dac_block_status,
            "adc_decimation": int(self.adc_block.DecimationFactor),
            "dac_interpolation": int(self.dac_block.InterpolationFactor),
            "adc_nyquist_zone": int(self.adc_block.NyquistZone),
            "dac_nyquist_zone": int(self.dac_block.NyquistZone),
            "adc_calibration_mode": int(self.adc_block.CalibrationMode),
            "adc_dither": int(self.adc_block.Dither),
            "adc_mixer": dict(self.adc_block.MixerSettings),
            "dac_mixer": dict(self.dac_block.MixerSettings),
        }

        required_checks = {
            "ADC PLL lock": report["rfdc"]["adc_pll_lock"] == 2,
            "DAC PLL lock": report["rfdc"]["dac_pll_lock"] == 2,
            "ADC FIFO": report["rfdc"]["adc_fifo"] == 1,
            "DAC FIFO": report["rfdc"]["dac_fifo"] == 1,
            "ADC decimation": report["rfdc"]["adc_decimation"] == self.decimation,
            "DAC interpolation": (
                report["rfdc"]["dac_interpolation"] == self.interpolation
            ),
            "ADC FIFO flags": (
                int(adc_block_status.get("IsFIFOFlagsAsserted", 0)) == 0
            ),
            "DAC FIFO flags": (
                int(dac_block_status.get("IsFIFOFlagsAsserted", 0)) == 0
            ),
        }

        for label, passed in required_checks.items():
            if not passed:
                failures.append(f"{label} check failed")

        if capture_smoke_test:
            try:
                capture = self.capture_frame(timeout_s=1.0)
                passed = capture.captured_words == self.bram_word_count
                report["capture_smoke_test"] = {
                    "pass": passed,
                    "captured_words": capture.captured_words,
                    "i_rms": self._rms(capture.i),
                    "q_rms": self._rms(capture.q),
                }
                if not passed:
                    failures.append(
                        f"capture returned {capture.captured_words} words"
                    )
            except Exception as exc:
                report["capture_smoke_test"] = {
                    "pass": False,
                    "error": str(exc),
                }
                failures.append(f"capture smoke test failed: {exc}")

        report["pass"] = len(failures) == 0
        report["failures"] = failures

        if failures and strict:
            raise RuntimeError(
                "RFSoC startup self-test failed:\n- " + "\n- ".join(failures)
            )

        return report

    def print_self_test(self, report: Dict[str, Any]) -> None:
        """Print a compact self-test report."""

        print("RFSoC sounder startup self-test")
        print("  overall:", "PASS" if report.get("pass") else "FAIL")

        for name, item in report.get("addresses", {}).items():
            actual = item["actual"]
            actual_text = "missing" if actual is None else f"0x{actual:08X}"
            print(
                f"  {name:28s}: {actual_text} "
                f"({'PASS' if item['pass'] else 'FAIL'})"
            )

        print("  ADC PLL/FIFO:",
              report["rfdc"]["adc_pll_lock"],
              report["rfdc"]["adc_fifo"])
        print("  DAC PLL/FIFO:",
              report["rfdc"]["dac_pll_lock"],
              report["rfdc"]["dac_fifo"])

        smoke = report.get("capture_smoke_test")
        if smoke is not None:
            print("  capture smoke:", smoke)

        if report.get("failures"):
            print("  failures:")
            for failure in report["failures"]:
                print("   -", failure)

    # ------------------------------------------------------------------
    # RFDC TX/RX settings
    # ------------------------------------------------------------------

    def configure_tx(
        self,
        *,
        center_mhz: float = 1000.0,
        nyquist_zone: int = 1,
        duration_s: float = 1.0,
        phase_offset_deg: float = 0.0,
        fine_mixer_scale: int = 1,
        event_source: Optional[int] = None,
        verify: bool = True,
    ) -> Dict[str, Any]:
        """Configure the DAC Fine NCO and wave-player duration."""

        self._require_started()
        if duration_s <= 0:
            raise ValueError("duration_s must be positive")

        if event_source is None:
            event_source = getattr(xrfdc, "EVNT_SRC_IMMEDIATE", 0)

        self.dac_block.NyquistZone = int(nyquist_zone)

        settings = dict(self.dac_block.MixerSettings)
        settings.update({
            "CoarseMixFreq": getattr(xrfdc, "COARSE_MIX_BYPASS", 16),
            "EventSource": int(event_source),
            "FineMixerScale": int(fine_mixer_scale),
            "Freq": float(center_mhz),
            "MixerMode": getattr(xrfdc, "MIXER_MODE_C2R", 2),
            "MixerType": getattr(xrfdc, "MIXER_TYPE_FINE", 2),
            "PhaseOffset": float(phase_offset_deg),
        })

        self.dac_block.MixerSettings = settings
        self.dac_block.UpdateEvent(xrfdc.EVENT_MIXER)

        self.tx_center_mhz = float(center_mhz)
        self.tx_duration_s = float(duration_s)
        self.tx_repeat_count = self.repeat_count_for_duration(duration_s)
        self.wave_player.write(self.WP_REPEAT_COUNT, self.tx_repeat_count)

        readback = {
            "mixer": dict(self.dac_block.MixerSettings),
            "nyquist_zone": int(self.dac_block.NyquistZone),
            "duration_s": self.tx_duration_s,
            "repeat_count": self.tx_repeat_count,
            "interpolation": int(self.dac_block.InterpolationFactor),
        }

        if verify:
            self._assert_close(
                float(readback["mixer"]["Freq"]),
                float(center_mhz),
                1e-3,
                "DAC NCO",
            )
            if readback["mixer"]["MixerType"] != getattr(
                xrfdc, "MIXER_TYPE_FINE", 2
            ):
                raise RuntimeError("DAC is not using the Fine Mixer")
            if readback["mixer"]["MixerMode"] != getattr(
                xrfdc, "MIXER_MODE_C2R", 2
            ):
                raise RuntimeError("DAC mixer mode is not Complex-to-Real")
            if readback["nyquist_zone"] != int(nyquist_zone):
                raise RuntimeError("DAC NyquistZone readback mismatch")

        return readback

    def configure_rx(
        self,
        *,
        center_mhz: float = 1000.0,
        nyquist_zone: int = 1,
        calibration_mode: int = 2,
        dither: bool = True,
        phase_offset_deg: float = 0.0,
        fine_mixer_scale: int = 1,
        event_source: Optional[int] = None,
        capture_timeout_s: float = 0.5,
        settle_s: float = 0.020,
        verify: bool = True,
    ) -> Dict[str, Any]:
        """Configure the ADC Fine NCO and capture settings."""

        self._require_started()
        if calibration_mode not in (1, 2):
            raise ValueError("calibration_mode must be 1 or 2")
        if capture_timeout_s <= 0:
            raise ValueError("capture_timeout_s must be positive")
        if settle_s < 0:
            raise ValueError("settle_s cannot be negative")

        if event_source is None:
            event_source = getattr(xrfdc, "EVNT_SRC_TILE", 2)

        self.adc_block.NyquistZone = int(nyquist_zone)
        self.adc_block.CalibrationMode = int(calibration_mode)
        self.adc_block.Dither = int(bool(dither))

        settings = dict(self.adc_block.MixerSettings)
        settings.update({
            "CoarseMixFreq": getattr(xrfdc, "COARSE_MIX_BYPASS", 16),
            "EventSource": int(event_source),
            "FineMixerScale": int(fine_mixer_scale),
            "Freq": -abs(float(center_mhz)),
            "MixerMode": getattr(xrfdc, "MIXER_MODE_R2C", 3),
            "MixerType": getattr(xrfdc, "MIXER_TYPE_FINE", 2),
            "PhaseOffset": float(phase_offset_deg),
        })

        self.adc_block.MixerSettings = settings
        self.adc_block.UpdateEvent(xrfdc.EVENT_MIXER)

        self.rx_center_mhz = float(center_mhz)
        self.rx_capture_timeout_s = float(capture_timeout_s)
        self.rx_settle_s = float(settle_s)

        readback = {
            "mixer": dict(self.adc_block.MixerSettings),
            "nyquist_zone": int(self.adc_block.NyquistZone),
            "calibration_mode": int(self.adc_block.CalibrationMode),
            "dither": int(self.adc_block.Dither),
            "capture_timeout_s": self.rx_capture_timeout_s,
            "settle_s": self.rx_settle_s,
            "decimation": int(self.adc_block.DecimationFactor),
        }

        if verify:
            self._assert_close(
                float(readback["mixer"]["Freq"]),
                -abs(float(center_mhz)),
                1e-3,
                "ADC NCO",
            )
            if readback["mixer"]["MixerType"] != getattr(
                xrfdc, "MIXER_TYPE_FINE", 2
            ):
                raise RuntimeError("ADC is not using the Fine Mixer")
            if readback["mixer"]["MixerMode"] != getattr(
                xrfdc, "MIXER_MODE_R2C", 3
            ):
                raise RuntimeError("ADC mixer mode is not Real-to-Complex")
            if readback["nyquist_zone"] != int(nyquist_zone):
                raise RuntimeError("ADC NyquistZone readback mismatch")
            if readback["calibration_mode"] != int(calibration_mode):
                raise RuntimeError("ADC CalibrationMode readback mismatch")
            if readback["dither"] != int(bool(dither)):
                raise RuntimeError("ADC Dither readback mismatch")

        return readback

    # ------------------------------------------------------------------
    # Waveform generation and loading
    # ------------------------------------------------------------------

    def set_zadoff_chu(
        self,
        *,
        sc_range: Tuple[int, int] = (-260, 260),
        root: int = 3,
        zc_q: float = 0.0,
        amplitude: float = 0.8,
    ) -> WaveformData:
        """Generate and select the established band-limited Zadoff-Chu frame."""

        n_samples = self.frame_samples
        if not 0.0 < amplitude <= 1.0:
            raise ValueError("amplitude must be in (0, 1]")

        n = np.arange(n_samples, dtype=np.float64)
        cf = n_samples % 2

        tx_fd = np.exp(
            -1j
            * np.pi
            * int(root)
            * n
            * (n + cf + 2.0 * float(zc_q))
            / n_samples
        )

        zero_start = int(sc_range[1])
        zero_stop = n_samples + int(sc_range[0])
        if not (0 <= zero_start <= zero_stop <= n_samples):
            raise ValueError("invalid sc_range")

        tx_fd[zero_start:zero_stop] = 0.0
        tx_time = np.fft.ifft(tx_fd)

        peak = max(
            float(np.max(np.abs(tx_time.real))),
            float(np.max(np.abs(tx_time.imag))),
        )
        if peak <= 0:
            raise RuntimeError("generated waveform has zero amplitude")

        tx_time = tx_time / peak
        scale = float(amplitude) * 32767.0

        i_data = np.clip(
            np.round(tx_time.real * scale), -32768, 32767
        ).astype(np.int16)
        q_data = np.clip(
            np.round(tx_time.imag * scale), -32768, 32767
        ).astype(np.int16)

        active_mask = np.fft.fftshift(np.abs(tx_fd) > 0)
        return self.set_waveform(
            i=i_data,
            q=q_data,
            tx_fd=tx_fd,
            active_mask=active_mask,
        )

    def set_waveform(
        self,
        *,
        iq: Optional[Sequence[complex]] = None,
        i: Optional[Sequence[int]] = None,
        q: Optional[Sequence[int]] = None,
        packed: Optional[Sequence[int]] = None,
        normalized: bool = False,
        amplitude: float = 0.8,
        tx_fd: Optional[np.ndarray] = None,
        active_mask: Optional[np.ndarray] = None,
    ) -> WaveformData:
        """
        Select an arbitrary waveform.

        Provide exactly one representation:
            iq=<complex samples>
            i=<int16 samples>, q=<int16 samples>
            packed=<uint32 {Q,I} samples>

        If iq is floating point and normalized=True, it is peak-normalized and
        scaled by amplitude*32767 before quantization.
        """

        supplied = sum([
            iq is not None,
            packed is not None,
            i is not None or q is not None,
        ])
        if supplied != 1:
            raise ValueError(
                "provide exactly one of iq, packed, or the i/q pair"
            )

        if packed is not None:
            packed_array = np.asarray(packed, dtype=np.uint32)
            self._validate_frame_shape(packed_array)
            i_data, q_data = self.unpack_iq32(packed_array)

        elif iq is not None:
            iq_array = np.asarray(iq, dtype=np.complex128)
            self._validate_frame_shape(iq_array)

            if normalized:
                peak = max(
                    float(np.max(np.abs(iq_array.real))),
                    float(np.max(np.abs(iq_array.imag))),
                )
                if peak <= 0:
                    raise ValueError("cannot normalize a zero waveform")
                iq_array = iq_array / peak * (float(amplitude) * 32767.0)

            i_data = np.clip(
                np.round(iq_array.real), -32768, 32767
            ).astype(np.int16)
            q_data = np.clip(
                np.round(iq_array.imag), -32768, 32767
            ).astype(np.int16)
            packed_array = self.pack_iq32(i_data, q_data)

        else:
            if i is None or q is None:
                raise ValueError("both i and q must be supplied")
            i_data = np.asarray(i, dtype=np.int16)
            q_data = np.asarray(q, dtype=np.int16)
            self._validate_frame_shape(i_data)
            self._validate_frame_shape(q_data)
            packed_array = self.pack_iq32(i_data, q_data)

        iq_data = (
            i_data.astype(np.float64)
            + 1j * q_data.astype(np.float64)
        )

        if active_mask is None:
            spectrum = np.fft.fftshift(np.fft.fft(iq_data))
            threshold = 0.5 * np.max(np.abs(spectrum))
            active_mask_array = np.abs(spectrum) > threshold
        else:
            active_mask_array = np.asarray(active_mask, dtype=bool)
            self._validate_frame_shape(active_mask_array)

        if tx_fd is not None:
            tx_fd = np.asarray(tx_fd, dtype=np.complex128)
            self._validate_frame_shape(tx_fd)

        self.waveform = WaveformData(
            i=i_data.copy(),
            q=q_data.copy(),
            iq=iq_data,
            packed=packed_array.copy(),
            tx_fd=None if tx_fd is None else tx_fd.copy(),
            active_mask=active_mask_array.copy(),
        )
        self.waveform_loaded = False
        return self.waveform

    def load_waveform(self, *, verify: bool = True) -> Dict[str, int]:
        """Write the selected frame into TX BRAM and optionally read it back."""

        self._require_started()
        if self.waveform is None:
            raise RuntimeError("no waveform selected")

        self._write_bram_frame(self.tx_bridge, self.waveform.packed)
        self.waveform_loaded = True

        result = {
            "samples_written": self.frame_samples,
            "words_written": self.bram_word_count,
            "packed_mismatches": 0,
            "i_mismatches": 0,
            "q_mismatches": 0,
        }

        if verify:
            packed, i_data, q_data, _ = self._read_bram_frame(self.tx_bridge)
            result["packed_mismatches"] = int(
                np.count_nonzero(packed != self.waveform.packed)
            )
            result["i_mismatches"] = int(
                np.count_nonzero(i_data != self.waveform.i)
            )
            result["q_mismatches"] = int(
                np.count_nonzero(q_data != self.waveform.q)
            )
            if any(
                result[key]
                for key in (
                    "packed_mismatches",
                    "i_mismatches",
                    "q_mismatches",
                )
            ):
                raise RuntimeError(f"TX BRAM verification failed: {result}")

        return result

    # ------------------------------------------------------------------
    # TX/RX execution
    # ------------------------------------------------------------------

    def capture_frame(self, *, timeout_s: Optional[float] = None) -> CaptureResult:
        """Capture and read one 1024-IQ-sample ADC frame."""

        self._require_started()
        if timeout_s is None:
            timeout_s = self.rx_capture_timeout_s

        self.wave_capture.read(self.REG_CTRL)
        start = time.perf_counter()
        self.wave_capture.write(self.REG_CTRL, 0x01)
        self._wait_hls_done(self.wave_capture, timeout_s=float(timeout_s))
        elapsed = time.perf_counter() - start

        captured_words = int(
            self.wave_capture.read(self.WC_CAPTURED_WORDS)
        )
        if captured_words != self.bram_word_count:
            raise RuntimeError(
                f"captured {captured_words} words; "
                f"expected {self.bram_word_count}"
            )

        packed, i_data, q_data, iq_data = self._read_bram_frame(
            self.rx_bridge
        )
        return CaptureResult(
            packed=packed,
            i=i_data,
            q=q_data,
            iq=iq_data,
            captured_words=captured_words,
            capture_time_s=elapsed,
        )

    def transmit_and_capture(
        self,
        *,
        duration_s: Optional[float] = None,
        settle_s: Optional[float] = None,
        capture_timeout_s: Optional[float] = None,
        auto_load_waveform: bool = True,
        verify_waveform: bool = False,
        require_frame_count_match: bool = True,
    ) -> CaptureResult:
        """Repeat TX for a finite duration and capture one RX frame during TX."""

        self._require_started()
        if self.waveform is None:
            raise RuntimeError("no waveform selected")

        if auto_load_waveform and not self.waveform_loaded:
            self.load_waveform(verify=verify_waveform)

        if duration_s is None:
            duration_s = self.tx_duration_s
        if settle_s is None:
            settle_s = self.rx_settle_s
        if capture_timeout_s is None:
            capture_timeout_s = self.rx_capture_timeout_s

        repeat_count = self.repeat_count_for_duration(duration_s)
        self.wave_player.write(self.WP_REPEAT_COUNT, repeat_count)

        self.wave_player.read(self.REG_CTRL)
        self.wave_capture.read(self.REG_CTRL)

        start = time.perf_counter()
        self.wave_player.write(self.REG_CTRL, 0x01)
        time.sleep(float(settle_s))

        self.wave_capture.write(self.REG_CTRL, 0x01)
        self._wait_hls_done(
            self.wave_capture,
            timeout_s=float(capture_timeout_s),
        )
        capture_time = time.perf_counter() - start

        captured_words = int(
            self.wave_capture.read(self.WC_CAPTURED_WORDS)
        )
        if captured_words != self.bram_word_count:
            raise RuntimeError(
                f"captured {captured_words} words; "
                f"expected {self.bram_word_count}"
            )

        packed, i_data, q_data, iq_data = self._read_bram_frame(
            self.rx_bridge
        )

        player_timeout_s = max(float(duration_s) + 1.0, 2.0)
        self._wait_hls_done(self.wave_player, timeout_s=player_timeout_s)
        total_time = time.perf_counter() - start
        frames_completed = int(
            self.wave_player.read(self.WP_FRAMES_COMPLETED)
        )

        if require_frame_count_match and frames_completed != repeat_count:
            raise RuntimeError(
                f"wave_player completed {frames_completed} frames; "
                f"expected {repeat_count}"
            )

        return CaptureResult(
            packed=packed,
            i=i_data,
            q=q_data,
            iq=iq_data,
            captured_words=captured_words,
            frames_requested=repeat_count,
            frames_completed=frames_completed,
            capture_time_s=capture_time,
            total_time_s=total_time,
        )

    def repeat_count_for_duration(self, duration_s: float) -> int:
        if duration_s <= 0:
            raise ValueError("duration_s must be positive")
        repeat_count = int(round(float(duration_s) * self.frame_rate_hz))
        if repeat_count <= 0:
            raise ValueError("duration is shorter than one frame")
        return repeat_count

    # ------------------------------------------------------------------
    # Analysis and plotting
    # ------------------------------------------------------------------

    def spectrum_analysis(
        self,
        capture: CaptureResult,
        *,
        noise_referenced: bool = True,
        floor_db: float = -30.0,
    ) -> Dict[str, Any]:
        """Calculate TX/RX FFTs and spectra, optionally referenced to mean noise."""

        if self.waveform is None:
            raise RuntimeError("no waveform selected")

        tx_fft = np.fft.fftshift(np.fft.fft(self.waveform.iq))
        rx_fft = np.fft.fftshift(np.fft.fft(capture.iq))
        freq_mhz = (
            np.fft.fftshift(
                np.fft.fftfreq(
                    self.frame_samples,
                    d=1.0 / self.fs_complex_hz,
                )
            )
            / 1e6
        )

        active_mask = self.waveform.active_mask
        if active_mask is None:
            threshold = 0.5 * np.max(np.abs(tx_fft))
            active_mask = np.abs(tx_fft) > threshold
        noise_mask = ~active_mask

        result: Dict[str, Any] = {
            "tx_fft": tx_fft,
            "rx_fft": rx_fft,
            "frequency_mhz": freq_mhz,
            "active_mask": active_mask,
            "noise_mask": noise_mask,
        }

        if noise_referenced:
            tx_db, tx_noise, tx_power = self._noise_referenced_spectrum(
                tx_fft, noise_mask, floor_db
            )
            rx_db, rx_noise, rx_power = self._noise_referenced_spectrum(
                rx_fft, noise_mask, floor_db
            )
            result.update({
                "tx_spectrum_db": tx_db,
                "rx_spectrum_db": rx_db,
                "tx_mean_noise_power": tx_noise,
                "rx_mean_noise_power": rx_noise,
                "tx_mean_active_above_noise_db": 10.0 * np.log10(
                    np.mean(tx_power[active_mask]) / tx_noise
                ),
                "rx_mean_active_above_noise_db": 10.0 * np.log10(
                    np.mean(rx_power[active_mask]) / rx_noise
                ),
                "reference": "mean inactive-bin power = 0 dB",
            })
        else:
            result.update({
                "tx_spectrum_db": self._peak_normalized_spectrum(tx_fft),
                "rx_spectrum_db": self._peak_normalized_spectrum(rx_fft),
                "reference": "individual spectrum peak = 0 dB",
            })

        return result

    def plot_spectra(
        self,
        capture: CaptureResult,
        *,
        rf_axis: bool = False,
        noise_referenced: bool = True,
        floor_db: float = -30.0,
        figsize: Tuple[float, float] = (13, 5),
    ):
        """Plot TX and RX spectra."""

        import matplotlib.pyplot as plt

        analysis = self.spectrum_analysis(
            capture,
            noise_referenced=noise_referenced,
            floor_db=floor_db,
        )
        x = analysis["frequency_mhz"]
        xlabel = "Complex baseband frequency (MHz)"
        if rf_axis:
            if self.tx_center_mhz is None:
                raise RuntimeError("configure_tx() before using the RF axis")
            x = self.tx_center_mhz + x
            xlabel = "Nominal RF frequency (MHz)"

        plt.figure(figsize=figsize)
        plt.plot(x, analysis["tx_spectrum_db"], label="TX spectrum")
        plt.plot(
            x,
            analysis["rx_spectrum_db"],
            label="RX spectrum",
            alpha=0.8,
        )
        if noise_referenced:
            plt.axhline(
                0.0,
                linestyle="--",
                label="Mean inactive-band noise = 0 dB",
            )
            ylabel = "Power relative to mean noise (dB)"
        else:
            ylabel = "Normalized magnitude (dB)"

        plt.xlabel(xlabel)
        plt.ylabel(ylabel)
        plt.title("TX/RX Sounder Spectrum")
        plt.grid(True)
        plt.legend()
        plt.tight_layout()
        plt.show()
        return analysis

    def estimate_channel(
        self,
        capture: CaptureResult,
        *,
        definition: str = "standard",
        normalize_magnitude: bool = True,
    ) -> Dict[str, Any]:
        """
        Estimate frequency response on active bins.

        definition="standard":
            H = R * conj(S) / (|S|^2 + epsilon)

        definition="cross":
            H = S * conj(R)
            This is the user-requested cross-spectrum convention, not R/S.
        """

        if self.waveform is None:
            raise RuntimeError("no waveform selected")

        s_f = np.fft.fftshift(np.fft.fft(self.waveform.iq))
        r_f = np.fft.fftshift(np.fft.fft(capture.iq))
        freq_mhz = (
            np.fft.fftshift(
                np.fft.fftfreq(
                    self.frame_samples,
                    d=1.0 / self.fs_complex_hz,
                )
            )
            / 1e6
        )
        active_mask = self.waveform.active_mask
        if active_mask is None:
            active_mask = np.abs(s_f) > 0.5 * np.max(np.abs(s_f))

        if definition == "standard":
            epsilon = 1e-12 * np.max(np.abs(s_f) ** 2)
            h_f = r_f * np.conj(s_f) / (np.abs(s_f) ** 2 + epsilon)
        elif definition == "cross":
            h_f = s_f * np.conj(r_f)
        else:
            raise ValueError("definition must be 'standard' or 'cross'")

        h_active = h_f[active_mask]
        magnitude_db = 20.0 * np.log10(
            np.maximum(np.abs(h_active), 1e-12)
        )
        if normalize_magnitude:
            magnitude_db = magnitude_db - np.median(magnitude_db)

        phase = np.unwrap(np.angle(h_active))
        active_freq_mhz = freq_mhz[active_mask]

        phase_slope, phase_intercept = np.polyfit(
            active_freq_mhz * 1e6,
            phase,
            1,
        )
        if definition == "standard":
            estimated_delay_s = -phase_slope / (2.0 * np.pi)
        else:
            estimated_delay_s = phase_slope / (2.0 * np.pi)

        return {
            "definition": definition,
            "s_f": s_f,
            "r_f": r_f,
            "h_f": h_f,
            "active_mask": active_mask,
            "frequency_mhz": active_freq_mhz,
            "magnitude_db": magnitude_db,
            "phase_rad": phase,
            "phase_fit_rad": (
                phase_slope * active_freq_mhz * 1e6 + phase_intercept
            ),
            "estimated_delay_s": estimated_delay_s,
        }

    def plot_channel(
        self,
        capture: CaptureResult,
        *,
        definition: str = "standard",
        rf_axis: bool = True,
        figsize: Tuple[float, float] = (13, 5),
    ) -> Dict[str, Any]:
        """Plot channel magnitude and unwrapped phase."""

        import matplotlib.pyplot as plt

        channel = self.estimate_channel(
            capture,
            definition=definition,
        )
        x = channel["frequency_mhz"]
        xlabel = "Complex baseband frequency (MHz)"
        if rf_axis:
            if self.tx_center_mhz is None:
                raise RuntimeError("configure_tx() before using the RF axis")
            x = self.tx_center_mhz + x
            xlabel = "Nominal RF frequency (MHz)"

        title_definition = (
            r"$\hat H(f)=R(f)/S(f)$"
            if definition == "standard"
            else r"$H_{\mathrm{cross}}(f)=S(f)R^*(f)$"
        )

        plt.figure(figsize=figsize)
        plt.plot(x, channel["magnitude_db"])
        plt.axhline(0.0, linestyle="--")
        plt.xlabel(xlabel)
        plt.ylabel("|H(f)| relative to median (dB)")
        plt.title(f"{title_definition} — Magnitude")
        plt.grid(True)
        plt.tight_layout()
        plt.show()

        plt.figure(figsize=figsize)
        plt.plot(x, channel["phase_rad"], label="Measured phase")
        plt.plot(
            x,
            channel["phase_fit_rad"],
            linestyle="--",
            label=(
                f"Linear fit, delay = "
                f"{channel['estimated_delay_s'] * 1e9:.2f} ns"
            ),
        )
        plt.xlabel(xlabel)
        plt.ylabel("Unwrapped phase (rad)")
        plt.title(f"{title_definition} — Phase")
        plt.grid(True)
        plt.legend()
        plt.tight_layout()
        plt.show()

        return channel

    def capture_statistics(self, capture: CaptureResult) -> Dict[str, float]:
        """Return basic I/Q statistics and clipping indicators."""

        peak_i = int(np.max(np.abs(capture.i.astype(np.int32))))
        peak_q = int(np.max(np.abs(capture.q.astype(np.int32))))
        return {
            "i_min": int(np.min(capture.i)),
            "i_max": int(np.max(capture.i)),
            "q_min": int(np.min(capture.q)),
            "q_max": int(np.max(capture.q)),
            "i_rms": self._rms(capture.i),
            "q_rms": self._rms(capture.q),
            "complex_rms": float(np.sqrt(np.mean(np.abs(capture.iq) ** 2))),
            "peak_i": peak_i,
            "peak_q": peak_q,
            "peak_full_scale_ratio": max(peak_i, peak_q) / 32768.0,
            "nonzero_i": int(np.count_nonzero(capture.i)),
            "nonzero_q": int(np.count_nonzero(capture.q)),
        }

    def summary(self) -> str:
        """Return a readable controller summary."""

        return (
            "RFSoCSounder\n"
            f"  BIT: {self.bit_path}\n"
            f"  RF sample rate: {self.rf_sample_rate_hz / 1e9:.6f} GSPS\n"
            f"  Complex sample rate: {self.fs_complex_hz / 1e6:.2f} MS/s\n"
            f"  Frame: {self.frame_samples} IQ samples\n"
            f"  Frame duration: {self.frame_duration_s * 1e6:.6f} us\n"
            f"  TX center: {self.tx_center_mhz} MHz\n"
            f"  RX center: {self.rx_center_mhz} MHz\n"
            f"  Waveform selected: {self.waveform is not None}\n"
            f"  Waveform loaded: {self.waveform_loaded}"
        )

    # ------------------------------------------------------------------
    # Low-level helpers
    # ------------------------------------------------------------------

    @staticmethod
    def decode_hls_ctrl(value: int) -> Dict[str, Any]:
        return {
            "raw": f"0x{int(value):08X}",
            "ap_start": (int(value) >> 0) & 1,
            "ap_done": (int(value) >> 1) & 1,
            "ap_idle": (int(value) >> 2) & 1,
            "ap_ready": (int(value) >> 3) & 1,
        }

    @staticmethod
    def pack_iq32(i_data: np.ndarray, q_data: np.ndarray) -> np.ndarray:
        i_array = np.asarray(i_data, dtype=np.int16)
        q_array = np.asarray(q_data, dtype=np.int16)
        if i_array.shape != q_array.shape:
            raise ValueError("I and Q shapes must match")
        i_bits = i_array.view(np.uint16).astype(np.uint32)
        q_bits = q_array.view(np.uint16).astype(np.uint32)
        return (i_bits | (q_bits << 16)).astype(np.uint32)

    @staticmethod
    def unpack_iq32(packed: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
        packed_array = np.asarray(packed, dtype=np.uint32)
        i_bits = (packed_array & np.uint32(0xFFFF)).astype(np.uint16)
        q_bits = (
            (packed_array >> 16) & np.uint32(0xFFFF)
        ).astype(np.uint16)
        return i_bits.view(np.int16), q_bits.view(np.int16)

    def _write_bram_frame(self, bridge: Any, packed_samples: np.ndarray) -> None:
        packed_array = np.asarray(packed_samples, dtype=np.uint32)
        self._validate_frame_shape(packed_array)
        sample_registers = (
            self.REG_SAMPLE0,
            self.REG_SAMPLE1,
            self.REG_SAMPLE2,
            self.REG_SAMPLE3,
        )

        for word_index in range(self.bram_word_count):
            base = word_index * self.iq_per_word
            for lane, register in enumerate(sample_registers):
                bridge.write(register, int(packed_array[base + lane]))
            bridge.write(self.REG_WORD_INDEX, word_index)
            bridge.write(self.REG_COMMAND, self.CMD_WRITE)
            self._run_hls_transaction(bridge, timeout_s=0.2)

    def _read_bram_frame(
        self,
        bridge: Any,
    ) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
        packed = np.zeros(self.frame_samples, dtype=np.uint32)
        read_registers = (
            self.REG_READ0,
            self.REG_READ1,
            self.REG_READ2,
            self.REG_READ3,
        )

        for word_index in range(self.bram_word_count):
            bridge.write(self.REG_WORD_INDEX, word_index)
            bridge.write(self.REG_COMMAND, self.CMD_READ)
            self._run_hls_transaction(bridge, timeout_s=0.2)

            base = word_index * self.iq_per_word
            for lane, register in enumerate(read_registers):
                packed[base + lane] = bridge.read(register)

        i_data, q_data = self.unpack_iq32(packed)
        iq_data = (
            i_data.astype(np.float64)
            + 1j * q_data.astype(np.float64)
        )
        return packed, i_data, q_data, iq_data

    def _wait_hls_done(
        self,
        ip: Any,
        *,
        timeout_s: float,
        poll_interval_s: float = 1e-5,
    ) -> int:
        deadline = time.monotonic() + float(timeout_s)
        last_ctrl = 0
        while time.monotonic() < deadline:
            last_ctrl = int(ip.read(self.REG_CTRL))
            if last_ctrl & 0x02:
                return last_ctrl
            if poll_interval_s > 0:
                time.sleep(poll_interval_s)
        raise TimeoutError(
            f"HLS transaction timed out; last CTRL=0x{last_ctrl:08X}"
        )

    def _run_hls_transaction(self, ip: Any, *, timeout_s: float) -> int:
        ip.read(self.REG_CTRL)
        ip.write(self.REG_CTRL, 0x01)
        return self._wait_hls_done(ip, timeout_s=timeout_s)

    def _bind_hardware(self) -> None:
        required = set(self.EXPECTED_ADDRESSES)
        missing = required - set(self.overlay.ip_dict)
        if missing:
            raise RuntimeError(f"overlay is missing IPs: {sorted(missing)}")

        self.tx_bridge = getattr(self.overlay, "iq_bram_bridge_tx")
        self.rx_bridge = getattr(self.overlay, "iq_bram_bridge_rx")
        self.wave_player = getattr(self.overlay, "wave_player_0")
        self.wave_capture = getattr(self.overlay, "wave_capture_0")
        self.rfdc = getattr(self.overlay, "usp_rf_data_converter_0")

        self.adc_tile = self.rfdc.adc_tiles[self.adc_tile_index]
        self.dac_tile = self.rfdc.dac_tiles[self.dac_tile_index]
        self.adc_block = self.adc_tile.blocks[self.adc_block_index]
        self.dac_block = self.dac_tile.blocks[self.dac_block_index]

    def _check_files(self) -> None:
        missing = [
            str(path)
            for path in (self.bit_path, self.hwh_path)
            if not path.exists()
        ]
        if missing:
            raise FileNotFoundError(
                "missing overlay file(s): " + ", ".join(missing)
            )

    def _require_started(self) -> None:
        if self.overlay is None or self.rfdc is None:
            raise RuntimeError("call startup() first")

    @staticmethod
    def _require_pynq_runtime() -> None:
        if _PYNQ_IMPORT_ERROR is not None:
            raise ImportError(
                "This module must run in the RFSoC PYNQ environment"
            ) from _PYNQ_IMPORT_ERROR

    def _validate_frame_shape(self, array: np.ndarray) -> None:
        if np.asarray(array).shape != (self.frame_samples,):
            raise ValueError(
                f"expected shape ({self.frame_samples},), "
                f"got {np.asarray(array).shape}"
            )

    @staticmethod
    def _assert_close(
        actual: float,
        expected: float,
        tolerance: float,
        label: str,
    ) -> None:
        if abs(actual - expected) > tolerance:
            raise RuntimeError(
                f"{label} readback is {actual}; expected {expected}"
            )

    @staticmethod
    def _rms(data: np.ndarray) -> float:
        return float(
            np.sqrt(np.mean(np.asarray(data, dtype=np.float64) ** 2))
        )

    @staticmethod
    def _noise_referenced_spectrum(
        fft_data: np.ndarray,
        noise_mask: np.ndarray,
        floor_db: float,
    ) -> Tuple[np.ndarray, float, np.ndarray]:
        power = np.abs(fft_data) ** 2
        mean_noise = float(np.mean(power[noise_mask]))
        if not np.isfinite(mean_noise) or mean_noise <= 0:
            mean_noise = np.finfo(np.float64).tiny
        minimum_ratio = 10.0 ** (float(floor_db) / 10.0)
        spectrum_db = 10.0 * np.log10(
            np.maximum(power / mean_noise, minimum_ratio)
        )
        return spectrum_db, mean_noise, power

    @staticmethod
    def _peak_normalized_spectrum(fft_data: np.ndarray) -> np.ndarray:
        magnitude = np.abs(fft_data)
        peak = float(np.max(magnitude))
        if peak <= 0:
            return np.full(magnitude.shape, -300.0)
        return 20.0 * np.log10(
            np.maximum(magnitude / peak, 1e-15)
        )
