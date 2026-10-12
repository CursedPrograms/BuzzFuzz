#pragma once
#include "Circuit.h"
#include <array>

// Diodes you could push into the breadboard in place of a 1N4148. Shockley models (saturation
// current, emission coefficient) fitted to each type's usual forward voltage around 1 mA; no
// series resistance or capacitance, so they're close, not datasheet-exact.
struct DiodeType
{
    const char* name;
    const char* kind;
    Circuit::DiodeModel model;
};

inline const std::array<DiodeType, 16>& diodeTypes()
{
    static const std::array<DiodeType, 16> types { {
        { "1N4148",        "silicon",   { 2.52e-9, 1.752 } },     // 0.58 V - the original
        { "1N914",         "silicon",   { 2.67e-9, 1.84 } },      // 0.61 V
        { "1N4001",        "rectifier", { 14.11e-9, 1.984 } },    // 0.57 V, soft knee
        { "1N34A",         "germanium", { 2.0e-7, 1.3 } },        // 0.29 V
        { "OA90",          "germanium", { 4.0e-7, 1.6 } },        // 0.32 V
        { "1N270",         "germanium", { 1.2e-6, 1.6 } },        // 0.28 V
        { "BAT41",         "Schottky",  { 1.0e-9, 1.25 } },       // 0.45 V
        { "BAT85",         "Schottky",  { 2.0e-7, 1.04 } },       // 0.23 V
        { "1N5817",        "Schottky",  { 1.0e-6, 1.1 } },        // 0.20 V
        { "Infrared LED",  "LED",       { 1.0e-14, 1.7 } },       // 1.1 V
        { "Red LED",       "LED",       { 1.0e-18, 1.9 } },       // 1.7 V
        { "Yellow LED",    "LED",       { 1.0e-19, 1.9 } },       // 1.8 V
        { "Green LED",     "LED",       { 1.0e-21, 1.9 } },       // 2.0 V
        { "Blue LED",      "LED",       { 1.0e-28, 1.8 } },       // 2.7 V
        { "White LED",     "LED",       { 1.0e-29, 1.8 } },       // 2.8 V
        { "None",          "pulled out",{ 0.0, 1.0 } },           // an empty hole
    } };
    return types;
}
