# Buzz Fuzz - TODO

Emulation of The 2014 breadboard fuzz: a single-transistor common-emitter amp (MPSA13
Darlington) with diode feedback clipping. The real-time circuit solver (`Source/circuit`) matches
ngspice to within 0.13 % (see `spice/`, `tools/verify.cpp`, `tools/compare.py`).

## Next
- [ ] **Speed:** the solver runs at 2x real time at 192 kHz - works but heavy. Make it several
      times faster before it goes into a plugin (stamp the linear parts once, only redo the
      diodes/transistors each Newton iteration; maybe 2x oversampling instead of 4x).
- [ ] **The plugin:** standalone + VST3 with
  - [ ] VOLUME knob
  - [ ] BATTERY knob, fresh to dying (lower voltage, more internal resistance - brings up the rail-cap mod)
  - [ ] RAIL CAP switch: the 2014 mod (1 uF across the battery) on / off
  - [ ] hidden extras: DIODES (1 to 4 in the feedback, like the breadboard experiments) and
        diode type (1N4148, germanium, LED)
- [ ] **Look** (name is Buzz Fuzz for now): a different style from Safety Pin, its own thing.
      Idea: the real 2014 perfboard - copper pads, solder joints, the parts where they really
      sit (MPSA13 round side forward, silver 2.2 uF, blue 334, grey 1 nF, 10k, diodes, the 1 uF
      on the battery leads), hook-up wire to the jacks and a 9 V battery clip; VOLUME and
      BATTERY as pots on the board, the RAIL CAP switch makes the 1 uF appear / fade.
      Open: brown (phenolic) or green (fibreglass) board? A photo to copy the layout.

## Credits
Farica☆Kimora  (c) Cursed Entertainment - in the plugin, README and LICENSE.

## Later
- [ ] Reamp the real 2014 pedal (Scarlett out -> pedal -> Scarlett in) and tune the model to it.
- [ ] The breadboard: a GUI to wire parts on a virtual breadboard and hear it live, on top of the same solver.
