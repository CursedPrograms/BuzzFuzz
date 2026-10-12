#include "BuzzFuzz.h"

BuzzFuzz::BuzzFuzz (const Settings& s)
{
    auto& c = circuit;
    in = c.addNode ("in");
    base = c.addNode ("b");
    collector = c.addNode ("c");
    rail = c.addNode ("vcc");
    const auto darlington = c.addNode ("e1");     // emitter of the first transistor inside the MPSA13
    const auto between = c.addNode ("d12");       // between the two bias diodes
    const auto hiPass = c.addNode ("x");          // between the 1 nF and its diode

    const Circuit::DiodeModel d1n4148;
    const Circuit::BjtModel half;                 // each half of the Darlington

    input = c.addSource (in, 0.0, s.sourceOhms);
    battery = c.addSource (rail, s.batteryVolts, s.batteryOhms);
    if (s.railCap)
        c.addCapacitor (rail, 0, 1e-6);

    c.addResistor (rail, collector, 10e3);
    c.addCapacitor (in, base, 2.2e-6);
    c.addCapacitor (base, 0, 330e-9);
    c.addNpn (collector, base, darlington, half);
    c.addNpn (collector, darlington, 0, half);

    c.addDiode (collector, between, d1n4148);
    c.addDiode (between, base, d1n4148);
    c.addCapacitor (collector, hiPass, 1e-9);
    c.addDiode (hiPass, base, d1n4148);

    c.addResistor (collector, 0, 1e6);            // the amp's input
}
