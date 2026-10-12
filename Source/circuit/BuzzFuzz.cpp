#include "BuzzFuzz.h"

BuzzFuzz::BuzzFuzz (const Settings& s)
{
    auto& c = circuit;
    in = c.addNode ("in");
    base = c.addNode ("b");
    collector = c.addNode ("c");
    rail = c.addNode ("vcc");
    const auto darlington = c.addNode ("e1");     // emitter of the first transistor inside the MPSA13
    output = c.addNode ("out");                   // after the 1 nF: the output jack

    const Circuit::DiodeModel d1n4148;
    const Circuit::BjtModel half;                 // each half of the Darlington

    input = c.addSource (in, 0.0, s.sourceOhms);
    battery = c.addSource (rail, s.batteryVolts, s.batteryOhms);
    if (s.railCap)
        c.addCapacitor (rail, 0, 3.3e-6);

    c.addResistor (rail, collector, 10e3);
    c.addCapacitor (in, base, 2.2e-6);
    c.addCapacitor (base, 0, 330e-9);
    c.addNpn (collector, base, darlington, half);
    c.addNpn (collector, darlington, 0, half);

    c.addDiode (collector, base, d1n4148);        // two side by side
    c.addDiode (collector, base, d1n4148);
    c.addCapacitor (collector, output, 1e-9);     // 102K63
    c.addDiode (output, base, d1n4148);

    c.addResistor (output, 0, 1e6);               // the amp's input
}
