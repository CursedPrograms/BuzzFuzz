# Fuzz14 - TODO

The 2014 breadboard fuzz: a single-transistor common-emitter amp (MPSA13 Darlington) with diode
feedback clipping. The real-time circuit solver (`Source/circuit`) matches ngspice to within
0.24 % (see `spice/`, `tools/verify.cpp`, `tools/compare.py`).

## Done (v0.1.0)
- [x] Plugin: VST3 + standalone with ASIO; INPUT, BATTERY (fresh to dying), VOLUME
- [x] D1 / D3 diode pickers (silicon, germanium, Schottky, LEDs, pulled out); D2 stays a 1N4148
- [x] The schematic is the skin; diodes glow while they conduct
- [x] pluginval strictness 10

## Next
- [ ] **Speed:** about 12 % of a core at 48 kHz (2x oversampled). Stamp the linear parts once
      and only redo the diodes / transistors each Newton iteration.
- [ ] Reamp the real 2014 pedal (Scarlett out -> pedal -> Scarlett in) and tune the model to it
      (the 102K63 is +-10 %, the 2u2 +-20 %, the MPSA13's gain varies a lot).

## Later
- [ ] A perfboard look as a second skin: copper pads, solder joints, the parts where they
      really sit (MPSA13 round side forward, silver 2.2 uF, blue 334, grey 102K63, 10k, diodes).
- [ ] Solderless: wire your own parts on a virtual breadboard, on top of the same solver.

## Credits
Farica☆Kimora  (c) Cursed Entertainment - in the plugin, README and LICENSE.
