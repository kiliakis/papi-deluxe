#include "PAPIProf.h"
#include <algorithm>
#include <cmath>
#include <stack>
#include <limits>
#include "metrics.h"
#include <unordered_map>

using namespace std;
using namespace metrics;

PAPIProfMultiThread::PAPIProfMultiThread(unsigned int threads,
        unsigned long (*func) (void))
{
    int retval = PAPI_library_init(PAPI_VER_CURRENT);

    retval = PAPI_thread_init(func);
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI thread init error\n");
        PAPI_perror(PAPI_strerror(retval));
    }

    _events_set_arr.resize(threads);
    _events_names_arr.resize(threads);
    _metrics_arr.resize(threads);
    _counters_arr.resize(threads);
    _eventSet_arr.resize(threads, PAPI_NULL);
    _ts_stack_arr.resize(threads);
    _key_stack_arr.resize(threads);
    _eventValues_arr.resize(threads);
}


PAPIProfMultiThread::~PAPIProfMultiThread() {
    PAPI_shutdown();
}


void PAPIProfMultiThread::init(vector<string> metrics, vector<string> events)
{
    unique_lock<mutex> lock(_mutex);

    int retval = PAPI_register_thread();
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI register thread error\n");
        PAPI_perror(PAPI_strerror(retval));
    }

    auto tid = PAPI_thread_id();
    _tids[tid] = _thread_count;
    _thread_count++;
    tid = _tids[tid];
    // printf("[%d] Hi!\n", (int) tid);

    retval = PAPI_multiplex_init();
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI multiplex init error\n");
        PAPI_perror(PAPI_strerror(retval));
    }
    /* Create the Event Set */
    retval = PAPI_create_eventset(&_eventSet_arr[tid]);
    if ( retval != PAPI_OK) {
        fprintf(stderr, "PAPI create_eventset error\n");
        PAPI_perror(PAPI_strerror(retval));
    }

    /* Convert the eventSet */
    retval = PAPI_assign_eventset_component(_eventSet_arr[tid], 0);
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI assign eventSet error\n");
        PAPI_perror(PAPI_strerror(retval));
    }

    /* Convert the eventSet */
    retval = PAPI_set_multiplex(_eventSet_arr[tid]);
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI set multiplex error\n");
        PAPI_perror(PAPI_strerror(retval));
    }
    lock.unlock();

    if (metrics.size() != 0)
        add_metrics(metrics);
    add_events(events);
}


// called by each of the slave threads
void PAPIProfMultiThread::cleanup() {

    auto tid = _tids[PAPI_thread_id()];

    if (papi_is_running(_eventSet_arr[tid])) {
        long long *eventValues = new long long[_events_set_arr[tid].size()];
        papi_stop(_eventSet_arr[tid], eventValues);
        delete[] eventValues;
    }

    papi_cleanup(_eventSet_arr[tid]);
    papi_destroy(&_eventSet_arr[tid]);
    int retval = PAPI_unregister_thread();
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI unregister thread error\n");
        PAPI_perror(PAPI_strerror(retval));
    }

}


// Called by the master thread only
void PAPIProfMultiThread::clear_counters() {
    // auto tid = _tids[PAPI_thread_id()];

    _events_names_arr.clear();
    _events_set_arr.clear();
    _counters_arr.clear();
    _metrics_arr.clear();
    _ts_stack_arr.clear();
    _key_stack_arr.clear();
    _eventValues_arr.clear();
    // while (!_ts_stack_arr.empty()) _ts_stack_arr.pop();
    // while (!_key_stack_arr.empty()) _key_stack_arr.pop();
    // while (!_eventValues_arr.empty()) _eventValues_arr.pop();
}

void PAPIProfMultiThread::add_events(vector<string> events)
{

    auto tid = _tids[PAPI_thread_id()];

    vector<string> new_events;
    for (auto e : events) {
        auto pair = _events_set_arr[tid].insert(e);
        if (pair.second) {
            new_events.push_back(e);
            _events_names_arr[tid].push_back(e);
        }
    }
    if (new_events.size() > 0) {
        if (papi_is_running(_eventSet_arr[tid])) {
            long long *eventValues = new long long[_events_set_arr[tid].size()];
            papi_stop(_eventSet_arr[tid], eventValues);
            papi_add_events(_eventSet_arr[tid], new_events);
            papi_start(_eventSet_arr[tid]);
            delete[] eventValues;
        } else {
            papi_add_events(_eventSet_arr[tid], new_events);
            papi_start(_eventSet_arr[tid]);
        }
    }
}

void PAPIProfMultiThread::remove_events(vector<string> events)
{
    auto tid = _tids[PAPI_thread_id()];

    vector<string> removed_events;
    for (auto e : events) {
        if (_events_set_arr[tid].find(e) != _events_set_arr[tid].end()) {
            _events_set_arr[tid].erase(e);
            auto it = find(_events_names_arr[tid].begin(), _events_names_arr[tid].end(), e);
            _events_names_arr[tid].erase(it);
            removed_events.push_back(e);
        }
    }
    papi_remove_events(_eventSet_arr[tid], removed_events);

    if (removed_events.size() > 0) {
        long long *eventValues = new long long[_events_set_arr[tid].size()];
        if (papi_is_running(_eventSet_arr[tid])) {
            papi_stop(_eventSet_arr[tid], eventValues);
            papi_remove_events(_eventSet_arr[tid], removed_events);
            papi_start(_eventSet_arr[tid]);
        } else {
            papi_add_events(_eventSet_arr[tid], removed_events);
            papi_start(_eventSet_arr[tid]);
        }
    }
}


void PAPIProfMultiThread::start_counters(string funcname,
        vector<string> metrics,
        vector<string> events)
{

    auto tid = _tids[PAPI_thread_id()];

    if (metrics.size()) add_metrics(metrics);
    add_events(events);
    _key_stack_arr[tid].push(funcname);

    if (_events_set_arr[tid].size()) {
        if (papi_is_running(_eventSet_arr[tid])) {
            long long *eventValues = new long long[_events_set_arr[tid].size()];
            papi_stop(_eventSet_arr[tid], eventValues);
            papi_reset(_eventSet_arr[tid]);
        }
        papi_start(_eventSet_arr[tid]);
    }

    _ts_stack_arr[tid].push(PAPI_get_real_usec());


}


void PAPIProfMultiThread::stop_counters()
{

    auto tid = _tids[PAPI_thread_id()];

    long long *eventValues = new long long[_events_set_arr[tid].size()];

    if (_events_set_arr[tid].size()) {
        papi_stop(_eventSet_arr[tid], eventValues);
        for (int i = 0; i < (int)_events_set_arr[tid].size(); i++) {
            if (eventValues[i] < 0)
                printf("EventName: %s, Value: %Ld\n", _events_names_arr[tid][i].c_str(), eventValues[i]);
        }
    }

    long long te = PAPI_get_real_usec();
    double msec = (double) (te - _ts_stack_arr[tid].top()) / 1000.0; _ts_stack_arr[tid].pop();

    auto key = _key_stack_arr[tid].top(); _key_stack_arr[tid].pop();
    _counters_arr[tid][key + "\ttime(ms)"].push_back(msec);

    for (int i = 0; i < (int)_events_names_arr[tid].size(); ++i) {
        _counters_arr[tid][key + "\t" + _events_names_arr[tid][i]].push_back((double)eventValues[i]);
    }
}


void PAPIProfMultiThread::add_metrics(vector<string> metrics, bool helper)
{
    auto tid = _tids[PAPI_thread_id()];

    vector<string> new_metrics;
    for (auto m : metrics) {
        if (_metrics_arr[tid].find(m) == _metrics_arr[tid].end())
            new_metrics.push_back(m);
        // auto pair = _metrics_arr[tid].insert(m);
        // if (pair.second) new_metrics_arr[tid].push_back(m);
    }
    vector<string> events;
    set<string> operators = {"/", "*", "-", "+"};
    for (auto m : new_metrics) {
        auto equation = gPresetMetrics[m];
        for (auto symbol : equation) {
            try {
                stod(symbol);
            } catch (exception& e) {
                // printf("SYmbol %s\n", symbol.c_str());
                if (gPresetMetrics.find(symbol) != gPresetMetrics.end()) {
                    add_metrics({symbol}, true);
                } else if (operators.find(symbol) == operators.end()) {
                    events.push_back(symbol);
                }
            }
        }
    }
    if (!helper)
        _metrics_arr[tid].insert(new_metrics.begin(), new_metrics.end());

    add_events(events);

}


void PAPIProfMultiThread::report_metrics()
{
    auto tid = _tids[PAPI_thread_id()];

    fprintf(stderr, "\nCounters Report Start\n");
    fprintf(stderr, "function\tcounter\taverage_value\tstd(%%)\tcalls\n");
    set<string> checkedFunctions;
    for (auto &kv : _counters_arr[tid]) {
        auto funcname = kv.first.substr(0, kv.first.find('\t'));
        if (checkedFunctions.insert(funcname).second == false)
            continue;
        for (auto &metric_name : _metrics_arr[tid]) {
            auto equation = gPresetMetrics[metric_name];
            auto result = evaluate(equation, funcname,
                                   _counters_arr[tid], gPresetMetrics);
            fprintf(stderr, "%s\t%s\t%.3lf\t%d\t%d\n",
                    funcname.c_str(), metric_name.c_str(), result, 0, 0);
        }
    }

    fprintf(stderr, "\nCounters Report End\n");
}

void PAPIProfMultiThread::report_counters()
{
    auto tid = _tids[PAPI_thread_id()];

    fprintf(stderr, "\nCounters Report Start\n");
    fprintf(stderr, "function\tcounter\taverage_value\tstd(%%)\tcalls\n");

    for (auto &kv : _counters_arr[tid]) {
        if (kv.first.find("time(ms)") == string::npos) {
            auto mean = get_mean(kv.second);
            long double sum = mean * kv.second.size();
            auto std = get_std(kv.second);
            fprintf(stderr, "%s\t%.0Lf\t%.0Lf\t%lu\n",
                    kv.first.c_str(), sum, 100. * kv.second.size() * std / sum,
                    kv.second.size());

        }
    }
    fprintf(stderr, "\nCounters Report End\n");
}


void PAPIProfMultiThread::report_timing()
{
    auto tid = _tids[PAPI_thread_id()];

    fprintf(stderr, "\nCounters Report Start\n");
    fprintf(stderr, "function\tcounter\taverage_value\tstd(%%)\tcalls\n");

    for (auto &kv : _counters_arr[tid]) {
        if (kv.first.find("time(ms)") != string::npos) {
            auto mean = get_mean(kv.second);
            auto std = get_std(kv.second);
            fprintf(stderr, "%s\t%.3lf\t%.2lf\t%lu\n",
                    kv.first.c_str(), mean, 100. * std / mean, kv.second.size());

        }
    }
    fprintf(stderr, "\nCounters Report End\n");

}


unsigned int is_in(vector<set<string>> &setV, string key) {
    int acc = 0;
    for (auto &s : setV) {
        if (s.find(key) != s.end()) acc++;
    }
    return acc;
}

unsigned int is_in(vector<unordered_map<string, vector<double>>> mapV, string key) {
    int acc = 0;
    for (auto &s : mapV) {
        if (s.find(key) != s.end()) acc++;
    }
    return acc;
}


void PAPIProfMultiThread::compute_global()
{
    // SUM the timings, SUM the counters, then compute the metrics array
    unsigned int num_threads = _counters_arr.size();

    // Sum all counters
    for (auto &kv : _counters_arr[0]) {
        if (is_in(_counters_arr, kv.first) == num_threads) {
            _counters_global[kv.first].push_back(0.0);
            for (auto &c : _counters_arr) {
                _counters_global[kv.first].push_back(get_mean(c[kv.first]));
            }
        }
    }


    // calculate all metrics
    set<string> checkedFunctions;
    for (auto &kv : _counters_global) {
        auto funcname = kv.first.substr(0, kv.first.find('\t'));
        if (checkedFunctions.insert(funcname).second == false)
            continue;
        for (auto &m : _metrics_arr[0]) {
            if (is_in(_metrics_arr, m) == num_threads) {
                auto equation = gPresetMetrics[m];
                auto result = evaluate(equation, funcname, _counters_global,
                                       gPresetMetrics);
                _metrics_global[m] = result;
            }

        }
    }
}



void PAPIProfMultiThread::report_global_timing()
{

    fprintf(stderr, "\nCounters Report Start\n");
    fprintf(stderr, "function\tcounter\taverage_value\tstd(%%)\tcalls\n");

    for (auto &kv : _counters_global) {
        if (kv.first.find("time(ms)") != string::npos) {
            long double mean = get_mean(kv.second);
            long double sum = mean * kv.second.size();
            long double std = get_std(kv.second);

            fprintf(stderr, "%s\t%.3Lf\t%.1Lf\t%u\n",
                    kv.first.c_str(), sum, 100.0 * std / mean, 1);

        }
    }
    fprintf(stderr, "\nCounters Report End\n");

}



void PAPIProfMultiThread::report_global_counters()
{

    fprintf(stderr, "\nCounters Report Start\n");
    fprintf(stderr, "function\tcounter\taverage_value\tstd(%%)\tcalls\n");

    for (auto &kv : _counters_global) {
        if (kv.first.find("time(ms)") == string::npos) {
            long double mean = get_mean(kv.second);
            long double sum = mean * kv.second.size();
            long double std = get_std(kv.second);

            fprintf(stderr, "%s\t%.0Lf\t%.1Lf\t%u\n",
                    kv.first.c_str(), sum, 100.0 * std / mean, 1);

        }
    }
    fprintf(stderr, "\nCounters Report End\n");

}


void PAPIProfMultiThread::report_global_metrics()
{

    fprintf(stderr, "\nCounters Report Start\n");
    fprintf(stderr, "function\tcounter\taverage_value\tstd(%%)\tcalls\n");

    for (auto &kv : _metrics_global) {
        fprintf(stderr, "%s\t%.3lf\t%d\t%u\n",
                kv.first.c_str(), kv.second, 0, 1);

    }
    fprintf(stderr, "\nCounters Report End\n");

}