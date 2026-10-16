"""DDR-backed extension of the verified :mod:`rfsoc_sounder` controller.

The original controller and plotting code stay unchanged.  This subclass binds
the upgraded ``ddr_bram_bridge_tx`` instance and overrides ``load_waveform`` so
that a complete 4096-byte frame reaches TX BRAM only through the DDR AXI master.
"""

from __future__ import annotations

from typing import Any, Dict

import time
import numpy as np

from rfsoc_sounder import RFSoCSounder as _BaseRFSoCSounder

try:
    from pynq import allocate
except ImportError as exc:  # Allows offline inspection and syntax checking.
    allocate = None
    _ALLOCATE_IMPORT_ERROR = exc
else:
    _ALLOCATE_IMPORT_ERROR = None


class RFSoCSounder(_BaseRFSoCSounder):
    """RFSoC sounder whose TX frame is loaded through DDR and PS HP0."""

    TX_BRIDGE_NAME = "ddr_bram_bridge_tx"

    EXPECTED_ADDRESSES = {
        TX_BRIDGE_NAME: 0xA000_0000,
        "wave_player_0": 0xA001_0000,
        "iq_bram_bridge_rx": 0xA002_0000,
        "wave_capture_0": 0xA003_0000,
        "usp_rf_data_converter_0": 0xA004_0000,
    }

    CMD_DDR_TO_BRAM = 3

    REG_DDR_ADDRESS_LOW = 0x60
    REG_DDR_ADDRESS_HIGH = 0x64
    REG_BRAM_START_INDEX = 0x70
    REG_TRANSFER_WORD_COUNT = 0x78
    REG_WORDS_TRANSFERRED = 0x80
    REG_WORDS_TRANSFERRED_CTRL = 0x84

    # Address apertures assigned to ddr_bram_bridge_tx/Data_m_axi_gmem in the
    # implemented block design.  A CMA allocation must fit wholly in one.
    DDR_APERTURES = (
        (0x0000_0000, 0x7FFF_FFFF),
        (0x8_0000_0000, 0x8_7FFF_FFFF),
    )

    def __init__(self, *args: Any, **kwargs: Any) -> None:
        super().__init__(*args, **kwargs)
        self._tx_ddr_buffer = None

    def _bind_hardware(self) -> None:
        """Bind the upgraded TX bridge while retaining all original IPs."""

        required = set(self.EXPECTED_ADDRESSES)
        missing = required - set(self.overlay.ip_dict)
        if missing:
            raise RuntimeError(f"overlay is missing IPs: {sorted(missing)}")

        self.tx_bridge = getattr(self.overlay, self.TX_BRIDGE_NAME)
        self.rx_bridge = getattr(self.overlay, "iq_bram_bridge_rx")
        self.wave_player = getattr(self.overlay, "wave_player_0")
        self.wave_capture = getattr(self.overlay, "wave_capture_0")
        self.rfdc = getattr(self.overlay, "usp_rf_data_converter_0")

        self.adc_tile = self.rfdc.adc_tiles[self.adc_tile_index]
        self.dac_tile = self.rfdc.dac_tiles[self.dac_tile_index]
        self.adc_block = self.adc_tile.blocks[self.adc_block_index]
        self.dac_block = self.dac_tile.blocks[self.dac_block_index]

    def load_waveform(self, *, verify: bool = True) -> Dict[str, Any]:
        """Copy the selected 4096-byte frame from contiguous DDR to TX BRAM."""

        total_start = time.perf_counter()
        self._require_started()
        if self.waveform is None:
            raise RuntimeError("no waveform selected")
        if allocate is None:
            raise ImportError(
                "pynq.allocate is required for DDR waveform loading"
            ) from _ALLOCATE_IMPORT_ERROR

        player_ctrl = int(self.wave_player.read(self.REG_CTRL))
        if not (player_ctrl & 0x04):
            raise RuntimeError(
                "wave_player must be idle before updating TX BRAM; "
                f"CTRL=0x{player_ctrl:08X}"
            )

        allocation_time_s = 0.0
        if self._tx_ddr_buffer is None:
            allocation_start = time.perf_counter()
            self._tx_ddr_buffer = allocate(
                shape=(self.frame_samples,),
                dtype=np.uint32,
            )
            allocation_time_s = time.perf_counter() - allocation_start

        if int(self._tx_ddr_buffer.nbytes) != self.frame_samples * 4:
            raise RuntimeError("unexpected DDR buffer size")

        prepare_start = time.perf_counter()
        self._tx_ddr_buffer[:] = self.waveform.packed
        self._tx_ddr_buffer.flush()

        physical_address = self._buffer_physical_address(
            self._tx_ddr_buffer
        )
        byte_count = int(self._tx_ddr_buffer.nbytes)
        self._validate_ddr_buffer(physical_address, byte_count)
        buffer_prepare_time_s = time.perf_counter() - prepare_start

        write_start = time.perf_counter()
        self.tx_bridge.write(
            self.REG_DDR_ADDRESS_LOW,
            physical_address & 0xFFFF_FFFF,
        )
        self.tx_bridge.write(
            self.REG_DDR_ADDRESS_HIGH,
            (physical_address >> 32) & 0xFFFF_FFFF,
        )
        self.tx_bridge.write(self.REG_BRAM_START_INDEX, 0)
        self.tx_bridge.write(
            self.REG_TRANSFER_WORD_COUNT,
            self.bram_word_count,
        )
        self.tx_bridge.write(self.REG_COMMAND, self.CMD_DDR_TO_BRAM)
        self._run_hls_transaction(self.tx_bridge, timeout_s=1.0)

        words_transferred = int(
            self.tx_bridge.read(self.REG_WORDS_TRANSFERRED)
        ) & 0x1FF
        words_valid = int(
            self.tx_bridge.read(self.REG_WORDS_TRANSFERRED_CTRL)
        ) & 0x01

        if not words_valid:
            raise RuntimeError("DDR bridge did not assert words_transferred valid")
        if words_transferred != self.bram_word_count:
            raise RuntimeError(
                f"DDR bridge copied {words_transferred} words; "
                f"expected {self.bram_word_count}"
            )
        write_time_s = time.perf_counter() - write_start

        result: Dict[str, Any] = {
            "transport": "DDR -> S_AXI_HP0_FPD -> TX BRAM",
            "physical_address": physical_address,
            "bytes_copied": byte_count,
            "samples_written": self.frame_samples,
            "words_written": words_transferred,
            "allocation_time_s": allocation_time_s,
            "buffer_prepare_time_s": buffer_prepare_time_s,
            "write_time_s": write_time_s,
            "load_time_s": (
                allocation_time_s + buffer_prepare_time_s + write_time_s
            ),
            "verify_time_s": 0.0,
            "packed_mismatches": 0,
            "i_mismatches": 0,
            "q_mismatches": 0,
        }

        if verify:
            verify_start = time.perf_counter()
            packed, i_data, q_data, _ = self._read_bram_frame(
                self.tx_bridge
            )
            result["verify_time_s"] = time.perf_counter() - verify_start
            result["packed_mismatches"] = int(
                np.count_nonzero(packed != self.waveform.packed)
            )
            result["i_mismatches"] = int(
                np.count_nonzero(i_data != self.waveform.i)
            )
            result["q_mismatches"] = int(
                np.count_nonzero(q_data != self.waveform.q)
            )

            mismatch_keys = (
                "packed_mismatches",
                "i_mismatches",
                "q_mismatches",
            )
            if any(result[key] for key in mismatch_keys):
                raise RuntimeError(f"DDR TX BRAM verification failed: {result}")

        self.waveform_loaded = True
        result["total_time_s"] = time.perf_counter() - total_start
        return result

    def load_waveform_ddr(self, *, verify: bool = True) -> Dict[str, Any]:
        """Explicitly named alias for the DDR-backed loading path."""

        return self.load_waveform(verify=verify)

    def load_waveform_direct(self, *, verify: bool = True) -> Dict[str, Any]:
        """Load TX BRAM through the original AXI-Lite single-word path."""

        total_start = time.perf_counter()
        self._require_started()
        if self.waveform is None:
            raise RuntimeError("no waveform selected")
        player_ctrl = int(self.wave_player.read(self.REG_CTRL))
        if not (player_ctrl & 0x04):
            raise RuntimeError(
                "wave_player must be idle before updating TX BRAM; "
                f"CTRL=0x{player_ctrl:08X}"
            )

        write_start = time.perf_counter()
        result: Dict[str, Any] = dict(
            _BaseRFSoCSounder.load_waveform(self, verify=False)
        )
        write_time_s = time.perf_counter() - write_start
        result["transport"] = "AXI-Lite command 0 -> TX BRAM"
        result["bytes_copied"] = self.frame_samples * 4
        result["allocation_time_s"] = 0.0
        result["buffer_prepare_time_s"] = 0.0
        result["write_time_s"] = write_time_s
        result["load_time_s"] = write_time_s
        result["verify_time_s"] = 0.0

        if verify:
            verify_start = time.perf_counter()
            packed, i_data, q_data, _ = self._read_bram_frame(
                self.tx_bridge
            )
            result["verify_time_s"] = time.perf_counter() - verify_start
            result["packed_mismatches"] = int(
                np.count_nonzero(packed != self.waveform.packed)
            )
            result["i_mismatches"] = int(
                np.count_nonzero(i_data != self.waveform.i)
            )
            result["q_mismatches"] = int(
                np.count_nonzero(q_data != self.waveform.q)
            )

            mismatch_keys = (
                "packed_mismatches",
                "i_mismatches",
                "q_mismatches",
            )
            if any(result[key] for key in mismatch_keys):
                raise RuntimeError(
                    f"direct TX BRAM verification failed: {result}"
                )

        result["total_time_s"] = time.perf_counter() - total_start
        return result

    def release_ddr_buffer(self) -> None:
        """Release the owned CMA buffer after all DDR loads are complete."""

        if self._tx_ddr_buffer is not None:
            self._tx_ddr_buffer.freebuffer()
            self._tx_ddr_buffer = None

    @staticmethod
    def _buffer_physical_address(buffer: Any) -> int:
        address = getattr(buffer, "device_address", None)
        if address is None:
            address = getattr(buffer, "physical_address", None)
        if address is None:
            raise RuntimeError("allocated buffer has no physical address")
        return int(address)

    def _validate_ddr_buffer(self, address: int, byte_count: int) -> None:
        if address & 0x0F:
            raise RuntimeError(
                f"DDR buffer is not 16-byte aligned: 0x{address:X}"
            )
        if byte_count != self.bram_word_count * 16:
            raise RuntimeError(
                f"DDR frame is {byte_count} bytes; expected "
                f"{self.bram_word_count * 16}"
            )

        last_address = address + byte_count - 1
        in_aperture = any(
            low <= address and last_address <= high
            for low, high in self.DDR_APERTURES
        )
        if not in_aperture:
            raise RuntimeError(
                "DDR buffer lies outside the bridge address apertures: "
                f"0x{address:X}..0x{last_address:X}"
            )
