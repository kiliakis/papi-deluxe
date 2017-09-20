#include "PAPIProf.h"
#include <algorithm>
#include <cmath>
#include <stack>
#include <limits>
#include "metrics.h"
#include <unordered_map>

using namespace std;
using namespace metrics;

int papi_multithread_init(unsigned long (*func) (void)) {
    int retval = PAPI_library_init(PAPI_VER_CURRENT);

    retval = PAPI_thread_init(func);
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI thread init error\n");
        PAPI_perror(PAPI_strerror(retval));
        return retval;
    }

    return 0;
}



PAPIProfMultiThread::PAPIProfMultiThread() {}


void PAPIProfMultiThread::init(vector<string> metrics,
                               vector<string> events)
{
    int retval = PAPI_register_thread();
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI register thread error\n");
        PAPI_perror(PAPI_strerror(retval));
    }

    retval = papi_eventset_init(&_eventSet);

    if (metrics.size() != 0)
        add_metrics(metrics);
    add_events(events);

}

PAPIProfMultiThread::~PAPIProfMultiThread()
{
    cleanup();
    clear_counters();
}


unordered_map<string, vector<double>> compute_global_counters(vector<PAPIProfMultiThread *> &profilers)
{
    // SUM the timings, SUM the counters, then compute the metrics array
    int num_threads = profilers.size();

    unordered_map<string, vector<double>> counters_global;
    for (auto &kv : profilers[0]->_counters) {
        int acc = 0;
        for (auto &p : profilers) {
            if (p->_counters.find(kv.first) != p->_counters.end())
                acc++;
        }
        if (acc == num_threads) {
            for (auto &p : profilers) {
                counters_global[kv.first].push_back(get_mean(p->_counters[kv.first]));
            }
        }
    }
    return counters_global;
}


unordered_map<string, double> compute_global_metrics(vector<PAPIProfMultiThread *> &profilers)
{
    // SUM the timings, SUM the counters, then compute the metrics array
    int num_threads = profilers.size();

    auto counters_global = compute_global_counters(profilers);
    unordered_map<string, double> metrics_global;
    // calculate all metrics
    set<string> checkedFunctions;
    for (auto &kv : counters_global) {
        auto funcname = kv.first.substr(0, kv.first.find('\t'));
        if (checkedFunctions.insert(funcname).second == false)
            continue;
        for (auto &m : profilers[0]->_metrics) {
            int acc = 0;
            for (auto &p : profilers) {
                if (p->_metrics.find(m) != p->_metrics.end())
                    acc++;
            }
            if (acc == num_threads) {
                auto equation = gPresetMetrics[m];
                auto result = evaluate(equation, funcname, counters_global,
                                       gPresetMetrics);
                metrics_global[m] = result;
            }

        }
    }
    return metrics_global;
}


void report_global_counters(vector<PAPIProfMultiThread *> &profilers)
{
    // SUM the timings, SUM the counters, then compute the metrics array
    auto counters_global = compute_global_counters(profilers);
    fprintf(stderr, "\nCounters Report Start\n");
    fprintf(stderr, "function\tcounter\taverage_value\tstd(%%)\tcalls\n");

    for (auto &kv : counters_global) {
        long double mean = get_mean(kv.second);
        long double sum = mean * kv.second.size();
        long double std = get_std(kv.second);
        if (kv.first.find("time(ms)") != string::npos) {
            fprintf(stderr, "%s\t%.3Lf\t%.1Lf\t%lu\n",
                    kv.first.c_str(), sum, 100.0 * std / mean, kv.second.size());
        } else {
            fprintf(stderr, "%s\t%.0Lf\t%.1Lf\t%lu\n",
                    kv.first.c_str(), sum, 100.0 * std / mean, kv.second.size());
        }
    }
    fprintf(stderr, "\nCounters Report End\n");

}

void report_global_metrics(vector<PAPIProfMultiThread *> &profilers)
{
    // SUM the timings, SUM the counters, then compute the metrics array
    auto metrics_global = compute_global_metrics(profilers);

    fprintf(stderr, "\nCounters Report Start\n");
    fprintf(stderr, "function\tcounter\taverage_value\tstd(%%)\tcalls\n");

    for (auto &kv : metrics_global) {
        fprintf(stderr, "%s\t%.3lf\t%d\t%u\n",
                kv.first.c_str(), kv.second, 0, 1);

    }
    fprintf(stderr, "\nCounters Report End\n");
}

