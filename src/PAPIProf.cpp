#include "PAPIProf.h"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <stack>
#include <limits>
#include "metrics.h"

using namespace std;
using namespace metrics;
// unordered_map<string, vector<string>> gPresetMetrics;


int papi_init(int *eventSet)
{
    /* Initialize the PAPI library */
    int retval = PAPI_library_init(PAPI_VER_CURRENT);
    *eventSet = PAPI_NULL;

    if (retval != PAPI_VER_CURRENT && retval > 0) {
        fprintf(stderr, "PAPI version error\n");
        PAPI_perror(PAPI_strerror(retval));
        return retval;
    }

    if (retval < 0) {
        fprintf(stderr, "PAPI version error\n");

        PAPI_perror(PAPI_strerror(retval));
        return retval;
    }
    /* Initialize the multiplexed events */
    retval = PAPI_multiplex_init();
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI multiplex init error\n");
        PAPI_perror(PAPI_strerror(retval));
        return retval;
    }

    /* Create the Event Set */
    retval = PAPI_create_eventset(eventSet);
    if ( retval != PAPI_OK) {
        fprintf(stderr, "PAPI create_eventset error\n");
        PAPI_perror(PAPI_strerror(retval));
        return retval;
    }

    /* Convert the eventSet */
    retval = PAPI_assign_eventset_component(*eventSet, 0);
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI assign eventSet error\n");
        PAPI_perror(PAPI_strerror(retval));
        return retval;
    }

    /* Convert the eventSet */
    retval = PAPI_set_multiplex(*eventSet);
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI set multiplex error\n");
        PAPI_perror(PAPI_strerror(retval));
        return retval;
    }



    return 0;

}


int papi_add_events(int eventSet, vector<std::string> eventNames)
{
    int retval;
    for (auto e : eventNames) {
        char* c_string = const_cast<char*>(e.c_str());
        retval = PAPI_add_named_event(eventSet, c_string);
        if (retval != PAPI_OK) {
            // fprintf(stderr, "PAPI add_named_event error\n");
            fprintf(stderr, "PAPI event %s was not added!\n", c_string);
            PAPI_perror(PAPI_strerror(retval));
            return retval;
        }
    }
    return 0;
}


int papi_start(int eventSet)
{

    /* Start counting events in the Event Set */
    int retval = PAPI_start(eventSet);
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI start error\n");
        PAPI_perror(PAPI_strerror(retval));
        return retval;
    }
    return 0;
}


int papi_stop(int eventSet, long long * values)
{
    /* Stop the counting of events in the Event Set */
    int retval = PAPI_stop(eventSet, values);
    if ( retval != PAPI_OK) {
        fprintf(stderr, "PAPI stop error\n");
        PAPI_perror(PAPI_strerror(retval));
        return retval;
    }
    return 0;
}


int papi_reset(int eventSet) {
    /* Reset the counting events in the Event Set */
    int retval = PAPI_reset(eventSet);
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI reset error\n");
        PAPI_perror(PAPI_strerror(retval));
        return retval;
    }
    return 0;

}


int papi_read(int eventSet, long long * values)
{
    int retval = PAPI_read(eventSet, values);
    /* Read the counting events in the Event Set */
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI read error\n");
        PAPI_perror(PAPI_strerror(retval));
        return retval;
    }
    return 0;
}

int papi_destroy(int *eventSet)
{
    int retval = PAPI_destroy_eventset(eventSet);
    /* Read the counting events in the Event Set */
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI destroy error\n");
        PAPI_perror(PAPI_strerror(retval));
        return retval;
    }
    return 0;
}


double get_mean(vector<long long> &v) {
    return accumulate(v.begin(), v.end(), 0.0) / v.size();
}

double get_std(vector<long long> &v) {
    auto m = get_mean(v);
    double acc = 0;
    for (auto i : v) {
        acc += (i - m) * (i - m);
    }
    return std::sqrt(acc / (v.size()));
}


double calculate(vector<string> equation,
                 unordered_map<string, vector<long long>> &counters,
                 unordered_map<string, vector<string>> &preset_metrics)
{
    stack<double> stack;
    set<string> operators = {"/", "*", "-", "+"};

    while (equation.size()) {
        auto symbol = equation.front();
        equation.erase(equation.begin());
        try {
            double num = stod(symbol);
            stack.push(num);
        } catch (std::exception& e) {
            if (operators.find(symbol) != operators.end()) {
                auto op2 = stack.top(); stack.pop();
                auto op1 = stack.top(); stack.pop();
                if (symbol == "+") stack.push(op1 + op2);
                else if (symbol == "-") stack.push(op1 - op2);
                else if (symbol == "*") stack.push(op1 * op2);
                else if (symbol == "/") stack.push(op1 / op2);
            } else if (counters.find(symbol) != counters.end()) {
                auto a = counters[symbol];
                stack.push(accumulate(a.begin(), a.end(), 0));
            } else if (preset_metrics.find(symbol) != preset_metrics.end()) {
                auto a = preset_metrics[symbol];
                equation.insert(equation.begin(), a.begin(), a.end());
            } else {
                fprintf(stderr, "Symbol %s could not be recognized\n", symbol.c_str());
                return numeric_limits<double>::quiet_NaN();
            }
        }

    }
    if (stack.size() > 1) {
        fprintf(stderr, "Something went wrong with the metric evaluation\n");
        return numeric_limits<double>::quiet_NaN();
    }
    return stack.top();
}

PAPIProf::PAPIProf(vector<string> metrics,
                   vector<string> events)
{
    papi_init(&_eventSet);
    if (metrics.size() != 0)
        add_metrics(metrics);
    if (_events_set.size() != 0)
        add_events(events);
}

void PAPIProf::add_events(std::vector<std::string> events)
{
    vector<string> new_events;
    for (auto e : events) {
        auto pair = _events_set.insert(e);
        if (pair.second) {
            new_events.push_back(e);
            _events_names.push_back(e);
        }
    }
    papi_add_events(_eventSet, new_events);
}


void PAPIProf::start_counters(std::string funcname,
                              std::vector<std::string> metrics,
                              std::vector<std::string> events)
{
    if (metrics.size()) add_metrics(metrics);
    if (events.size()) add_events(events);

    _key = funcname;

    if (_events_set.size())
        papi_start(_eventSet);

    _ts = chrono::system_clock::now();

}


void PAPIProf::stop_counters()
{
    long long *eventValues = new long long[_events_set.size()];
    if (_events_set.size())
        papi_stop(_eventSet, eventValues);

    auto elapsed_time = chrono::system_clock::now() - _ts;

    _counters[_key + "\ttime(ms)"].push_back(1000 * elapsed_time.count());

    for (int i = 0; i < (int)_events_names.size(); ++i) {
        _counters[_key + "\t" + _events_names[i]].push_back(eventValues[i]);
    }
}


void PAPIProf::clear_counters() {}


void PAPIProf::add_metrics(std::vector<std::string> metrics, bool helper)
{
    vector<string> new_metrics;
    for (auto m : metrics) {
        auto pair = _metrics.insert(m);
        if (pair.second) new_metrics.push_back(m);
    }
    vector<string> events;
    set<string> operators = {"/", "*", "-", "+"};
    for (auto m : new_metrics) {
        auto equation = gPresetMetrics[m];
        for (auto symbol : equation) {
            auto it = gPresetMetrics.find(symbol);
            if (it != gPresetMetrics.end()) {
                add_metrics({symbol}, true);
            } else if (operators.find(symbol) != operators.end()) {
                events.push_back(symbol);
            }
        }
    }
    if (!helper)
        _metrics.insert(new_metrics.begin(), new_metrics.end());

    add_events(events);
    // vector<string> new_events;

    // for (auto e : events) {
    //     auto pair = _events.insert(e);
    //     if (pair.second) new_events.push_back(e);
    // }
    // if (new_events.size() != 0)
    //     papi_add_events(_eventSet, new_events);

}


void PAPIProf::report_metrics() {}

void PAPIProf::report_counters()
{
    fprintf(stderr, "\n Counters Report Start\n");
    fprintf(stderr, "function\tcounter\taverage_value\tstd(%%)\tcalls\n");

    for (auto &kv : _counters) {
        if (kv.first.find("time(ms)") == string::npos) {
            auto mean = get_mean(kv.second);
            auto sum = mean * kv.second.size();
            auto std = get_std(kv.second);
            fprintf(stderr, "%s\t%.3f\t%.2f\t%lu\n",
                    kv.first.c_str(), sum, 100 * kv.second.size() * std / sum,
                    kv.second.size());

        }
    }


    fprintf(stderr, "\n Counters Report End\n");
}


void PAPIProf::report_timing() {
    fprintf(stderr, "\n Timing Report Start\n");
    fprintf(stderr, "function\tcounter\taverage_value\tstd(%%)\tcalls\n");

    for (auto &kv : _counters) {
        if (kv.first.find("time(ms)") != string::npos) {
            auto mean = get_mean(kv.second);
            auto std = get_std(kv.second);
            fprintf(stderr, "%s\t%.3f\t%.2f\t%lu\n",
                    kv.first.c_str(), mean, 100 * std / mean, kv.second.size());

        }
    }


    fprintf(stderr, "\n Timing Report End\n");

}