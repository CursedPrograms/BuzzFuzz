// Runs the circuit through the real-time solver with the same input as spice/buzz_fuzz.cir
// and writes solver_out.csv (time, v(in), v(b), v(c)) for tools/compare.py.
//   verify <amplitude volts> <frequency Hz> <out.csv>
#include "../Source/circuit/BuzzFuzz.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>

int main (int argc, char* argv[])
{
    const auto amp = argc > 1 ? std::atof (argv[1]) : 0.1;
    const auto freq = argc > 2 ? std::atof (argv[2]) : 196.0;
    const char* path = argc > 3 ? argv[3] : "solver_out.csv";
    const auto rate = 192000.0, seconds = 0.5;

    BuzzFuzz fuzz;
    auto& c = fuzz.circuit;
    c.prepare (rate);
    std::printf ("op: v(b)=%.6f v(c)=%.6f v(e1)=%.6f v(vcc)=%.6f\n",
                 c.voltage (fuzz.base), c.voltage (fuzz.collector), c.voltage (c.node ("e1")), c.voltage (fuzz.rail));

    auto* f = std::fopen (path, "w");
    const auto start = std::chrono::steady_clock::now();
    const auto steps = (int) (rate * seconds);
    long totalIterations = 0;
    int worst = 0;
    for (int i = 1; i <= steps; ++i)
    {
        const auto t = i / rate;
        c.setSource (fuzz.input, amp * std::sin (2.0 * 3.141592653589793 * freq * t));
        c.step();
        totalIterations += c.lastIterations();
        worst = std::max (worst, c.lastIterations());
        std::fprintf (f, "%.9e %.9e %.9e %.9e\n", t, c.voltage (fuzz.in), c.voltage (fuzz.base), c.voltage (fuzz.collector));
    }
    std::fclose (f);
    const auto secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count();
    std::printf ("%d steps in %.3f s (%.1fx real time at 192 kHz), avg %.2f / max %d Newton iterations\n",
                 steps, secs, seconds / secs, (double) totalIterations / steps, worst);
}
