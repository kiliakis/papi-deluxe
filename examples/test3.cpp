#include <iostream>
#include "PAPIProf.h"
#include <stdlib.h>     /* atoi */
#include <random>


PAPIProf profiler = PAPIProf({"IPC"});

double foo(const int *a, const int *b, const int points)
{
    profiler.start_counters("foo");
    double acc = 0.0;
    for (int i = 0; i < points; ++i)
    {
        acc += (double) (a[i] - b[i]) / (a[i] + b[i]);
    }
    profiler.stop_counters();

    return acc;
}



double bar(const int *a, const int *b, const int loops, const int points)
{
    double acc = 0.0;
    profiler.start_counters("bar");
    for (int i = 0; i < loops; ++i)
    {
        acc += foo(a, b, points);
    }
    profiler.stop_counters();

    return acc;
}


int main(int argc, char const *argv[])
{
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

    profiler.start_counters("rand");
    for (int i = 0; i < points; ++i)
    {
        a[i] = rand() % 100;
        b[i] = rand() % 100;
    }
    profiler.stop_counters();

    double acc = bar(a, b, loops, points);

    printf("Acc is: %lf\n", acc);

    profiler.stop_counters();

    profiler.report_timing();
    profiler.report_counters();
    profiler.report_metrics();


    return 0;
}