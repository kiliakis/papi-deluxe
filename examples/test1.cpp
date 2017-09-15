#include <iostream>
#include "PAPIProf.h"
#include <stdlib.h>     /* atoi */
#include <random>

int main(int argc, char const *argv[])
{
    auto profiler = PAPIProf({"IPC", "CPI", "MEMORY_BOUND%", "CORE_BOUND%"});
    profiler.start_counters("main");

    int loops = 1000;
    int points = 1000;
    int threads = 1;
    if (argc > 1) {
        loops = atoi(argv[1]);
    }
    if (argc > 2) {
        points = atoi(argv[2]);
    }
    if (argc > 3) {
        threads = atoi(argv[3]);
    }

    int *a = new int[points];
    int *b = new int[points];

    for (int i = 0; i < points; ++i)
    {
        a[i] = rand() % 100;
        b[i] = rand() % 100;
    }
    // profiler.stop_counters();

    double acc = 0;

    // profiler.start_counters("loop");
    for (int i = 0; i < loops; ++i)
    {
        for (int j = 0; j < points; ++j)
        {
            acc += (double)(a[j] - b[j]) / (a[j] + b[j]);
        }
    }

    profiler.stop_counters();

    printf("Acc is: %lf\n", acc);
    profiler.report_timing();
    profiler.report_counters();
    profiler.report_metrics();

    return 0;
}