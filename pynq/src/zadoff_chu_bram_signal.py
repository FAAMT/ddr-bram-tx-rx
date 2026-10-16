import math
from typing import Optional, Tuple

import numpy as np


class ZadoffChuBramSignal:
    """
    Generate a single-antenna, single-frame Zadoff-Chu IQ waveform and
    convert it to the uint32 format used by iq_bram_bridge.

    Waveform generation pipeline:
        Frequency-domain Zadoff-Chu
        -> Band limiting
        -> IFFT
        -> I/Q peak normalization
        -> int16 quantization
        -> uint32 IQ packing

    Each uint32 word uses this format:
        bits [15:0]  = I, signed int16
        bits [31:16] = Q, signed int16

    Primary outputs:
        txtd:
            shape = (1024,)
            dtype = complex128

        txtd_frame:
            shape = (1, 1, 1024)
            dtype = complex128

        i_int16:
            shape = (1024,)
            dtype = int16

        q_int16:
            shape = (1024,)
            dtype = int16

        bram_data:
            shape = (1024,)
            dtype = uint32
    """

    def __init__(
        self,
        n_samples: int = 1024,
        sc_range: Tuple[int, int] = (-260, 260),
        root: int = 3,
        zc_q: float = 0.0,
        amplitude: float = 1.0,
    ):
        self.n_samples = int(n_samples)
        self.sc_range = sc_range
        self.root = int(root)
        self.zc_q = float(zc_q)
        self.amplitude = float(amplitude)

        self._validate_parameters()

        # One-dimensional complex IQ waveform.
        self.txtd = self._generate_txtd()

        # Preserve the frame/antenna/sample representation used upstream.
        self.txtd_frame = self.txtd.reshape(1, 1, self.n_samples)

        # int16 I/Q
        self.i_int16, self.q_int16 = self._quantize_int16()

        # uint32 array accepted by iq_bram_bridge.
        self.bram_data = self._pack_uint32()

    def _validate_parameters(self) -> None:
        if self.n_samples <= 0:
            raise ValueError("n_samples must be greater than zero")

        if math.gcd(self.root, self.n_samples) != 1:
            raise ValueError(
                "Zadoff-Chu root must be coprime with n_samples"
            )

        if not 0.0 < self.amplitude <= 1.0:
            raise ValueError("amplitude must be in the interval (0, 1]")

        sc_low, sc_high = self.sc_range

        if not (
            -self.n_samples <= sc_low < 0
            and 0 < sc_high <= self.n_samples
        ):
            raise ValueError(
                "sc_range must satisfy "
                "-n_samples <= low < 0 < high <= n_samples"
            )

        zero_start = sc_high
        zero_stop = self.n_samples + sc_low

        if zero_start > zero_stop:
            raise ValueError("sc_range exceeds the valid FFT range")

    def _generate_txtd(self) -> np.ndarray:
        """
        Generate the normalized complex time-domain waveform.
        """

        n_fft = self.n_samples
        n = np.arange(n_fft, dtype=np.float64)

        # cf=0 for even lengths and cf=1 for odd lengths.
        cf = n_fft % 2

        # Frequency-domain Zadoff-Chu sequence.
        txfd = np.exp(
            -1j
            * np.pi
            * self.root
            * n
            * (n + cf + 2.0 * self.zc_q)
            / n_fft
        )

        # For sc_range=(-260, 260), clear indices 260:764.
        zero_start = self.sc_range[1]
        zero_stop = n_fft + self.sc_range[0]

        txfd[zero_start:zero_stop] = 0.0

        # Convert to complex time-domain IQ samples.
        txtd = np.fft.ifft(txfd)

        # Normalize against the largest absolute I or Q component.
        peak = max(
            np.max(np.abs(txtd.real)),
            np.max(np.abs(txtd.imag)),
        )

        if peak == 0:
            raise RuntimeError("generated IQ waveform has zero amplitude")

        return txtd / peak

    def _quantize_int16(
        self,
    ) -> Tuple[np.ndarray, np.ndarray]:
        """
        Quantize the normalized IQ samples to signed int16.
        """

        scale = self.amplitude * 32767.0

        i_int16 = np.clip(
            np.round(self.txtd.real * scale),
            -32768,
            32767,
        ).astype(np.int16)

        q_int16 = np.clip(
            np.round(self.txtd.imag * scale),
            -32768,
            32767,
        ).astype(np.int16)

        return i_int16, q_int16

    def _pack_uint32(self) -> np.ndarray:
        """
        Pack each complex IQ sample into one uint32 value.

        bits [15:0]  = I
        bits [31:16] = Q
        """

        # view(np.uint16) preserves the int16 two's-complement bit pattern.
        i_bits = self.i_int16.view(np.uint16).astype(np.uint32)
        q_bits = self.q_int16.view(np.uint16).astype(np.uint32)

        return (i_bits | (q_bits << 16)).astype(np.uint32)

    def unpack_uint32(
        self,
        packed: Optional[np.ndarray] = None,
    ) -> Tuple[np.ndarray, np.ndarray]:
        """
        Unpack uint32 data into signed int16 I/Q samples.
        This is primarily used for readback verification after a write.
        """

        if packed is None:
            packed = self.bram_data

        packed = np.asarray(packed, dtype=np.uint32)

        i_bits = (packed & np.uint32(0xFFFF)).astype(np.uint16)
        q_bits = ((packed >> 16) & np.uint32(0xFFFF)).astype(
            np.uint16
        )

        i_data = i_bits.view(np.int16)
        q_data = q_bits.view(np.int16)

        return i_data, q_data

    def verify_packing(self) -> bool:
        """
        Check that uint32 packing and unpacking are exactly reversible.
        """

        i_check, q_check = self.unpack_uint32()

        return bool(
            np.array_equal(i_check, self.i_int16)
            and np.array_equal(q_check, self.q_int16)
        )

    def get_bram_data(self, copy: bool = True) -> np.ndarray:
        """
        Return uint32 data in the format accepted by iq_bram_bridge.
        """

        if copy:
            return self.bram_data.copy()

        return self.bram_data

    def print_summary(self) -> None:
        print("Zadoff-Chu BRAM signal")
        print(f"  samples       : {self.n_samples}")
        print(f"  sc_range      : {self.sc_range}")
        print(f"  root          : {self.root}")
        print(f"  amplitude     : {self.amplitude}")
        print(f"  txtd shape    : {self.txtd.shape}")
        print(f"  txtd dtype    : {self.txtd.dtype}")
        print(f"  BRAM shape    : {self.bram_data.shape}")
        print(f"  BRAM dtype    : {self.bram_data.dtype}")
        print(f"  size          : {self.bram_data.nbytes} bytes")
        print(f"  packing valid : {self.verify_packing()}")


if __name__ == "__main__":
    signal = ZadoffChuBramSignal()
    signal.print_summary()

    print("\nFirst 8 IQ samples:")

    for index in range(8):
        print(
            f"{index:4d}: "
            f"I={int(signal.i_int16[index]):6d}, "
            f"Q={int(signal.q_int16[index]):6d}, "
            f"packed=0x{int(signal.bram_data[index]):08X}"
        )
