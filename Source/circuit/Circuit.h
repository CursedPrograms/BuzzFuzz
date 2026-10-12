#pragma once
#include <array>
#include <cmath>
#include <string>
#include <vector>

// A small real-time circuit solver: nodal analysis like SPICE, solved every sample.
//
// Parts are resistors, capacitors (trapezoidal companion models), diodes (Shockley), NPN
// transistors (Ebers-Moll transport model) and voltage sources with an internal resistance
// (Norton equivalents, so every unknown is a node voltage). Each sample the non-linear
// equations are solved with Newton-Raphson, starting from the previous sample's answer.
//
// Node 0 is ground; addNode() hands out the others. Built for the handful of nodes in a pedal,
// with a dense matrix - the same engine is meant to run whatever gets wired up on a breadboard.
class Circuit
{
public:
    struct DiodeModel
    {
        double is = 2.52e-9;              // saturation current (1N4148)
        double n = 1.752;                 // emission coefficient
    };

    struct BjtModel
    {
        double is = 1.0e-14;
        double bf = 120.0;                // forward beta
        double br = 0.74;                 // reverse beta
    };

    int addNode (const std::string& name);
    int node (const std::string& name) const;

    void addResistor (int a, int b, double ohms);
    void addCapacitor (int a, int b, double farads);
    void addDiode (int anode, int cathode, const DiodeModel&);
    void addNpn (int collector, int base, int emitter, const BjtModel&);
    // a source from node to ground; returns its index for setSource()
    int addSource (int node, double volts, double seriesOhms);

    void setSource (int index, double volts);
    void setResistor (int index, double ohms);
    int resistorCount() const { return (int) resistors.size(); }

    // Settle to the operating point (DC, capacitors charged up), then run at this rate.
    void prepare (double sampleRate);
    void step();                          // advance one sample
    double voltage (int node) const      { return node == 0 ? 0.0 : v[(size_t) node - 1]; }
    int lastIterations() const           { return iterations; }

private:
    struct Resistor { int a, b; double g; };
    struct Capacitor { int a, b; double c, g = 0, current = 0, lastV = 0; };
    struct Diode { int a, k; DiodeModel m; };
    struct Npn { int c, b, e; BjtModel m; };
    struct Source { int node; double volts, g; };

    void integrate (double dt, bool backwardEuler);
    bool solve (int maxIterations);
    double& at (int row, int col) { return G[(size_t) row * (size_t) n + (size_t) col]; }
    void stamp (int row, int col, double value);
    void inject (int row, double current);

    std::vector<std::string> names;
    std::vector<Resistor> resistors;
    std::vector<Capacitor> capacitors;
    std::vector<Diode> diodes;
    std::vector<Npn> npns;
    std::vector<Source> sources;

    int n = 0, iterations = 0;
    bool backwardEuler = false;
    std::vector<double> v, vNew, G, rhs;
};
