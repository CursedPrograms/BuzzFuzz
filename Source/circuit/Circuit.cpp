#include "Circuit.h"
#include <algorithm>
#include <stdexcept>

namespace
{
constexpr double thermalVoltage = 0.0258648;  // kT/q at 27 C (300.15 K), as ngspice
constexpr double gmin = 1e-12;                // tiny conductance across junctions, as SPICE
constexpr double tolerance = 1e-7;            // volts

// exp() that turns into a straight line past a limit, so Newton can't overflow on a bad guess
inline double safeExp (double x, double& slope)
{
    constexpr double limit = 40.0;
    if (x < limit)
    {
        const auto e = std::exp (x);
        slope = e;
        return e;
    }
    const auto e = std::exp (limit);
    slope = e;
    return e * (1.0 + x - limit);
}
}

int Circuit::addNode (const std::string& name)
{
    names.push_back (name);
    return (int) names.size();
}

int Circuit::node (const std::string& name) const
{
    if (name == "0" || name == "gnd")
        return 0;
    const auto it = std::find (names.begin(), names.end(), name);
    if (it == names.end())
        throw std::runtime_error ("no node " + name);
    return (int) (it - names.begin()) + 1;
}

void Circuit::addResistor (int a, int b, double ohms)        { resistors.push_back ({ a, b, 1.0 / ohms }); }
void Circuit::addCapacitor (int a, int b, double farads)     { capacitors.push_back ({ a, b, farads }); }
int Circuit::addDiode (int a, int k, const DiodeModel& m)
{
    diodes.push_back ({ a, k, m });
    return (int) diodes.size() - 1;
}

double Circuit::diodeCurrent (int index) const
{
    const auto& d = diodes[(size_t) index];
    const auto vd = voltage (d.a) - voltage (d.k);
    return d.m.is * (std::exp (std::min (vd / (d.m.n * thermalVoltage), 80.0)) - 1.0);
}
void Circuit::addNpn (int c, int b, int e, const BjtModel& m) { npns.push_back ({ c, b, e, m }); }

int Circuit::addSource (int node, double volts, double seriesOhms)
{
    sources.push_back ({ node, volts, 1.0 / std::max (seriesOhms, 1e-3) });
    return (int) sources.size() - 1;
}

void Circuit::setSource (int index, double volts)   { sources[(size_t) index].volts = volts; }
void Circuit::setSourceOhms (int index, double ohms) { sources[(size_t) index].g = 1.0 / ohms; }
void Circuit::setResistor (int index, double ohms)  { resistors[(size_t) index].g = 1.0 / ohms; }

void Circuit::stamp (int row, int col, double value)
{
    if (row > 0 && col > 0)
        at (row - 1, col - 1) += value;
}

void Circuit::inject (int row, double current)
{
    if (row > 0)
        rhs[(size_t) row - 1] += current;
}

void Circuit::integrate (double dt, bool be)
{
    backwardEuler = be;
    for (auto& c : capacitors)
        c.g = (be ? 1.0 : 2.0) * c.c / dt;
}

void Circuit::prepare (double sampleRate)
{
    n = (int) names.size();
    v.assign ((size_t) n, 0.0);
    vNew.assign ((size_t) n, 0.0);
    G.assign ((size_t) n * (size_t) n, 0.0);
    rhs.assign ((size_t) n, 0.0);
    for (auto& c : capacitors)
        c.current = c.lastV = 0.0;

    // Operating point: power up gently (supply ramped in) with long backward-Euler steps until
    // every capacitor has charged and nothing moves any more.
    std::vector<double> full;
    for (auto& s : sources)
        full.push_back (s.volts);
    integrate (1e-3, true);
    for (int i = 1; i <= 2000; ++i)
    {
        const auto ramp = std::min (1.0, i / 200.0);
        for (size_t s = 0; s < sources.size(); ++s)
            sources[s].volts = full[s] * ramp;
        step();
    }
    integrate (1.0 / sampleRate, false);
}

// One Newton solve of the circuit at the next time point.
bool Circuit::solve (int maxIterations)
{
    vNew = v;
    for (iterations = 1; iterations <= maxIterations; ++iterations)
    {
        std::fill (G.begin(), G.end(), 0.0);
        std::fill (rhs.begin(), rhs.end(), 0.0);
        const auto volt = [this] (int node) { return node == 0 ? 0.0 : vNew[(size_t) node - 1]; };

        for (const auto& r : resistors)
        {
            stamp (r.a, r.a, r.g);  stamp (r.b, r.b, r.g);
            stamp (r.a, r.b, -r.g); stamp (r.b, r.a, -r.g);
        }
        for (const auto& s : sources)
        {
            stamp (s.node, s.node, s.g);
            inject (s.node, s.volts * s.g);
        }
        for (const auto& c : capacitors)
        {
            // i = g v + j  (j from last sample's voltage and current)
            const auto j = backwardEuler ? -c.g * c.lastV : -c.g * c.lastV - c.current;
            stamp (c.a, c.a, c.g);  stamp (c.b, c.b, c.g);
            stamp (c.a, c.b, -c.g); stamp (c.b, c.a, -c.g);
            inject (c.a, -j);
            inject (c.b, j);
        }
        for (const auto& d : diodes)
        {
            const auto nvt = d.m.n * thermalVoltage;
            const auto vd = volt (d.a) - volt (d.k);
            double slope;
            const auto e = safeExp (vd / nvt, slope);
            const auto id = d.m.is * (e - 1.0) + gmin * vd;
            const auto gd = d.m.is * slope / nvt + gmin;
            const auto ieq = id - gd * vd;
            stamp (d.a, d.a, gd);  stamp (d.k, d.k, gd);
            stamp (d.a, d.k, -gd); stamp (d.k, d.a, -gd);
            inject (d.a, -ieq);
            inject (d.k, ieq);
        }
        for (const auto& q : npns)
        {
            // Ebers-Moll transport model; currents flowing into the collector and base pins
            const auto vbe = volt (q.b) - volt (q.e), vbc = volt (q.b) - volt (q.c);
            double sf, sr;
            const auto ef = safeExp (vbe / thermalVoltage, sf), er = safeExp (vbc / thermalVoltage, sr);
            const auto is = q.m.is;
            const auto ic = is * (ef - er) - is / q.m.br * (er - 1.0) - gmin * vbc;
            const auto ib = is / q.m.bf * (ef - 1.0) + is / q.m.br * (er - 1.0) + gmin * (vbe + vbc);
            const auto dIcdVbe = is * sf / thermalVoltage;
            const auto dIcdVbc = -is * sr / thermalVoltage - is / q.m.br * sr / thermalVoltage - gmin;
            const auto dIbdVbe = is / q.m.bf * sf / thermalVoltage + gmin;
            const auto dIbdVbc = is / q.m.br * sr / thermalVoltage + gmin;

            const int rows[3] = { q.c, q.b, q.e };
            const double cur[3] = { ic, ib, -(ic + ib) };
            const double a[3] = { dIcdVbe, dIbdVbe, -(dIcdVbe + dIbdVbe) };
            const double k[3] = { dIcdVbc, dIbdVbc, -(dIcdVbc + dIbdVbc) };
            for (int r = 0; r < 3; ++r)
            {
                stamp (rows[r], q.b, a[r] + k[r]);
                stamp (rows[r], q.e, -a[r]);
                stamp (rows[r], q.c, -k[r]);
                inject (rows[r], -(cur[r] - a[r] * vbe - k[r] * vbc));
            }
        }

        // Gaussian elimination with partial pivoting (tiny matrix, dense is fine)
        auto x = rhs;
        for (int col = 0; col < n; ++col)
        {
            int pivot = col;
            for (int r = col + 1; r < n; ++r)
                if (std::abs (at (r, col)) > std::abs (at (pivot, col)))
                    pivot = r;
            if (pivot != col)
            {
                for (int c2 = 0; c2 < n; ++c2)
                    std::swap (at (col, c2), at (pivot, c2));
                std::swap (x[(size_t) col], x[(size_t) pivot]);
            }
            const auto d = at (col, col);
            if (std::abs (d) < 1e-30)
                return false;
            for (int r = col + 1; r < n; ++r)
            {
                const auto f = at (r, col) / d;
                if (f == 0.0)
                    continue;
                for (int c2 = col; c2 < n; ++c2)
                    at (r, c2) -= f * at (col, c2);
                x[(size_t) r] -= f * x[(size_t) col];
            }
        }
        for (int r = n - 1; r >= 0; --r)
        {
            auto sum = x[(size_t) r];
            for (int c2 = r + 1; c2 < n; ++c2)
                sum -= at (r, c2) * x[(size_t) c2];
            x[(size_t) r] = sum / at (r, r);
        }

        // damped update: junctions never jump more than a few tenths of a volt per iteration
        auto biggest = 0.0;
        for (int i = 0; i < n; ++i)
        {
            auto dv = x[(size_t) i] - vNew[(size_t) i];
            dv = std::clamp (dv, -0.3, 0.3);
            vNew[(size_t) i] += dv;
            biggest = std::max (biggest, std::abs (dv));
        }
        if (biggest < tolerance)
            return true;
    }
    return false;
}

void Circuit::step()
{
    solve (60);
    v = vNew;
    const auto volt = [this] (int node) { return node == 0 ? 0.0 : v[(size_t) node - 1]; };
    for (auto& c : capacitors)
    {
        const auto vc = volt (c.a) - volt (c.b);
        c.current = backwardEuler ? c.g * (vc - c.lastV) : c.g * (vc - c.lastV) - c.current;
        c.lastV = vc;
    }
}
