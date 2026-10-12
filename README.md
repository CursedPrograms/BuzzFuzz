# Buzz Fuzz

A real-time circuit emulation of a fuzz pedal first built on a breadboard in 2014: one MPSA13
Darlington transistor as a common-emitter amp, with 1N4148 diodes in the feedback clipping it
(the Bazz Fuss family, plus the diodes added "till it sounded right" and a 1 µF cap across the
battery that made it louder and steadier).

Farica☆Kimora  (c) Cursed Entertainment

**Status: work in progress.** The circuit solver works; the plugin (VST3 + standalone) is next.
See [TODO.md](TODO.md).

## The circuit
- +9 V → 10k → collector; emitter to ground
- input → 2.2 µF electrolytic → base; 330 nF (334) from base to ground
- 2 × 1N4148 from collector to base, plus 1 nF in series with another 1N4148
- output straight from the collector; 1 µF across the battery

## The solver
`Source/circuit` is a small modified-nodal-analysis solver (Newton iterations, trapezoidal
capacitors, Shockley diodes, Ebers-Moll transistors) that runs the whole circuit sample by
sample. Against ngspice 47 with the same netlist (`spice/buzz_fuzz.cir`) it matches within
0.13 % of the signal over several levels and pitches.

```
cmake -S . -B build && cmake --build build --config Release
ngspice -b spice/buzz_fuzz.cir          # writes spice_out.csv
build/Release/verify.exe                  # writes the solver's output
python tools/compare.py spice_out.csv solver_out.csv
```

## License
MIT, see [LICENSE](LICENSE).
