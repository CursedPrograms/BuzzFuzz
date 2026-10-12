#pragma once
#include "Circuit.h"

// The 2014 breadboard fuzz: a single-transistor common-emitter amp (MPSA13 Darlington)
// with diode feedback clipping, built by adding diodes until it sounded right.
//
//      +9V (battery, internal resistance) ── 10k
//                                           ├── collector ──┬── 1N4148 ──┐   two diodes side by side,
//                                           │               └── 1N4148 ──┤   stripes at the base
//      OUT ──────────── 1nF (102K63) ───────┘                            │
//       └── 1N4148 (stripe at the base) ─────────────────────────────────┤
//      IN ── (source R) ── 2.2uF ────────────────────────────────────────┼── base
//                                                                      330nF
//                                                                       GND ── emitter

struct Fuzz14
{
    struct Settings
    {
        double batteryVolts = 9.0;
        double batteryOhms = 2.0;         // fresh 9 V; old ones climb to tens of ohms
        double sourceOhms = 10000.0;      // what's driving the input (pickup)
    };

    explicit Fuzz14 (const Settings& s = {});

    Circuit circuit;
    int input = 0, battery = 0;
    int in = 0, base = 0, collector = 0, rail = 0, output = 0;
    int d1 = 0, d2 = 0, d3 = 0;           // diode indices, for Circuit::setDiode
};
