# Buzz Fuzz

A real-time circuit emulation of a fuzz pedal first built on a breadboard in 2014: one MPSA13
Darlington transistor as a common-emitter amp, with 1N4148 diodes in the feedback clipping it
(the Bazz Fuss family, plus the diodes added "till it sounded right" and a 3.3 µF cap that made it
louder and steadier).

Farica☆Kimora  (c) Cursed Entertainment

**Status: work in progress.** The circuit solver works; the plugin (VST3 + standalone) is next.
See [TODO.md](TODO.md).

## The circuit
![Buzz Fuzz schematic](images/schematic.svg)

(PNG version: [images/schematic.png](images/schematic.png))

Wired like this on the board (MPSA13 pins seen from the round side):

| | |
|---|---|
| **VCC** (battery +) | 10k |
| **Pin 1** (collector) | 10k, D1, D2 (plain ends), 102K63 |
| **Output jack** | 102K63, D3 (plain end) |
| **Pin 2** (base) | D1, D2, D3 (stripe ends), 334 (330 nF), 2.2 µF − |
| **Pin 3** (emitter) | 334, 3.3 µF +, wire to ground |
| **Ground** (battery −) | 3.3 µF −, jack sleeves |
| **Input jack** | 2.2 µF + |

- **D1 and D2 side by side** from collector to base set the bias: the collector sits one diode
  drop (about 0.1-0.6 V depending on the current) above the base.
- **The 102K63 is the output cap**: 1 nF (10 x 10^2 pF), ±10 % (K), 63 V. Into an amp's 1 MΩ
  input it only passes what's above about 150 Hz, so the low end is gone and what's left buzzes.
- **D3 from the output back to the base** clips the big peaks.
- **The 2.2 µF ("m 2u2 50v": 2.2 µF, ±20 %, 50 V)** is in with its − on the base. The base sits at
  about +1.2 V and the jack at 0 V, so + should face the base. It has survived because 1.2 V of
  reverse voltage is gentle; flip it on a new build.
- **The 3.3 µF** runs from pin 3 to battery −, beside the wire that already joins them, so on paper
  both legs are on ground and it does nothing. It did make a difference on the board, though. The
  likely reason is the ground wire or a joint not being perfect: then the cap is what really
  carries the signal current from the emitter to battery −, and the pedal gets louder and steadier.
  Across the battery (+ on VCC, − on ground) it would do the other job, smoothing the 9 V like a
  brownout cap on a robot. The model can do that as a what-if (`railCap`).

## The solver
`Source/circuit` is a small modified-nodal-analysis solver (Newton iterations, trapezoidal
capacitors, Shockley diodes, Ebers-Moll transistors) that runs the whole circuit sample by
sample. Against ngspice 47 with the same netlist (`spice/buzz_fuzz.cir`) it matches within
0.24 % of the signal over several levels and pitches.

```
cmake -S . -B build && cmake --build build --config Release
ngspice -b spice/buzz_fuzz.cir          # writes spice_out.csv
build/Release/verify.exe                  # writes the solver's output
python tools/compare.py spice_out.csv solver_out.csv
```

## License
MIT, see [LICENSE](LICENSE).
