#pragma once
#include "Circuit.h"

// The 2014 breadboard fuzz: a single-transistor common-emitter amp (MPSA13 Darlington)
// with diode feedback clipping, built by adding diodes until it sounded right.
//
//                          +9V (battery, internal resistance) ──┬── 1uF ── GND   (rail cap mod)
//                                                               10k
//      OUT ─────────────────────────────────────────────────────┼── collector
//                                                    ┌── 1N4148 ── 1N4148 ──┐
//                                                    └── 1nF ──── 1N4148 ───┤
//      IN ── (source R) ── 2.2uF ───────────────────────────────────────────┼── base
//                                                                         330nF
//                                                                          GND ── emitter
struct BuzzFuzz
{
    struct Settings
    {
        double batteryVolts = 9.0;
        double batteryOhms = 2.0;         // fresh 9 V; old ones climb to tens of ohms
        bool railCap = true;              // the 1 uF across the battery
        double sourceOhms = 10000.0;      // what's driving the input (pickup)
    };

    explicit BuzzFuzz (const Settings& s = {});

    Circuit circuit;
    int input = 0, battery = 0;
    int in = 0, base = 0, collector = 0, rail = 0;
};
