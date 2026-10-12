"""Compare the real-time solver against ngspice: same circuit, same input.
usage: compare.py <spice_out.csv> <solver_out.csv>"""
import sys

import numpy as np

spice = np.loadtxt(sys.argv[1])          # wrdata: t v(in) t v(b) t v(c)
solver = np.loadtxt(sys.argv[2])         # t v(in) v(b) v(c)
t = solver[:, 0]
names = ["v(in)", "v(b)", "v(c)"]
cols = [(0, 1), (2, 3), (4, 5)]

skip = t > 0.1                            # let both settle
for name, (tc, vc), k in zip(names, cols, (1, 2, 3)):
    ref = np.interp(t, spice[:, tc], spice[:, vc])
    ours = solver[:, k]
    swing = ref[skip] - ref[skip].mean()
    err = ours[skip] - ref[skip]
    print(f"{name}: spice mean {ref[skip].mean():.4f} V, swing {np.ptp(ref[skip]):.4f} V | "
          f"ours mean {ours[skip].mean():.4f} V, swing {np.ptp(ours[skip]):.4f} V | "
          f"rms error {np.sqrt(np.mean(err ** 2)) * 1000:.3f} mV "
          f"({100 * np.sqrt(np.mean(err ** 2)) / max(np.sqrt(np.mean(swing ** 2)), 1e-12):.2f} % of signal)")
