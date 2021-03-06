#include "PAPIProf.h"
#include <algorithm>
#include <numeric>      // std::accumulate
#include <cmath>
#include <stack>
#include <limits>
#include "metrics.h"
#include <unordered_map>

using namespace std;
using namespace metrics;

int handle_error(int retval, const char *string) {
    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI %s error\n", string);
        PAPI_strerror(retval);
        return retval;
    }
    return 0;
}

int papi_singlethread_init()
{
    /* Initialize the PAPI library */
    int retval = PAPI_library_init(PAPI_VER_CURRENT);

    if (retval != PAPI_VER_CURRENT && retval > 0) {
        fprintf(stderr, "PAPI version error\n");
        PAPI_strerror(retval);
        return retval;
    }

    if (retval < 0) {
        fprintf(stderr, "PAPI version error\n");

        PAPI_strerror(retval);
        return retval;
    }

    return 0;
}




int papi_eventset_init(int *eventSet) {
    /* Initialize the multiplexed events */
    *eventSet = PAPI_NULL;
    int retval = PAPI_multiplex_init();
    handle_error(retval, "multiplex init");

    /* Create the Event Set */
    retval = PAPI_create_eventset(eventSet);
    handle_error(retval, "create eventset");

    /* Convert the eventSet */
    retval = PAPI_assign_eventset_component(*eventSet, 0);
    handle_error(retval, "assingn eventset");

    if (retval != PAPI_OK) {
        fprintf(stderr, "PAPI assign eventSet error\n");
        PAPI_strerror(retval);
        return retval;
    }

    /* Convert the eventSet */
    retval = PAPI_set_multiplex(*eventSet);
    handle_error(retval, "set multiplex");

    return 0;
}

int papi_add_events(int eventSet, vector<string> eventNames)
{
    int retval;
    // fprintf(stderr, "In papi_add_events\n");
    // int tempEventSet = eventSet;
    for (auto &e : eventNames) {
        char* c_string = const_cast<char*>(e.c_str());
        int code;

        retval = PAPI_event_name_to_code(c_string, &code);
        handle_error(retval, "event_name_to_code");

        retval = PAPI_add_event(eventSet, code);
        if ( handle_error(retval, "add_event") != 0)
            fprintf(stderr, "Event %s was not added\n", c_string);

    }
    // eventSet = tempEventSet;
    return 0;
}


int papi_remove_events(int eventSet, vector<string> eventNames)
{
    int retval;

    for (auto &e : eventNames) {
        char *c_string = const_cast<char *>(e.c_str());
        retval = PAPI_remove_named_event(eventSet, c_string);
        handle_error(retval, "remove_event");
    }
    return 0;

}


int papi_start(int eventSet)
{

    /* Start counting events in the Event Set */
    int retval = PAPI_start(eventSet);
    handle_error(retval, "papi start");
    return 0;
}


bool papi_is_running(int eventSet)
{
    int status;
    int retval = PAPI_state(eventSet, &status);
    handle_error(retval, "state");
    return ((status & 0x3) == PAPI_RUNNING);
}


int papi_stop(int eventSet, long long * values)
{
    /* Stop the counting of events in the Event Set */
    int retval = PAPI_stop(eventSet, values);
    handle_error(retval, "papi stop");
    return 0;
}


int papi_reset(int eventSet) {
    /* Reset the counting events in the Event Set */
    int retval = PAPI_reset(eventSet);
    handle_error(retval, "papi reset");
    return 0;

}


int papi_read(int eventSet, long long * values)
{
    int retval = PAPI_read(eventSet, values);
    handle_error(retval, "papi read");
    return 0;
}

int papi_destroy(int *eventSet)
{
    int retval = PAPI_destroy_eventset(eventSet);
    handle_error(retval, "papi destroy");
    return 0;
}


int papi_cleanup(int eventSet) {
    int retval = PAPI_cleanup_eventset(eventSet);
    handle_error(retval, "papi cleanup");
    return 0;
}

void papi_shutdown() {
    PAPI_shutdown();
}


double get_mean(vector<double> &v) {
    return accumulate(v.begin(), v.end(), (long double)0.0) / v.size();
}

double get_std(vector<double> &v) {
    auto m = get_mean(v);
    long double acc = 0;
    for (auto i : v) {
        acc += (i - m) * (i - m);
    }
    return sqrt(acc / (v.size()));
}


double evaluate(vector<string> equation,
                string &funcname,
                unordered_map<string, vector<double>> &counters,
                unordered_map<string, vector<string>> &preset_metrics)
{
    stack<long double> stack;
    set<string> operators = {"/", "*", "-", "+"};

    while (equation.size()) {
        auto symbol = equation.front();
        equation.erase(equation.begin());
        try {
            double num = stod(symbol);
            stack.push(num);
        } catch (exception& e) {
            if (operators.find(symbol) != operators.end()) {
                auto op2 = stack.top(); stack.pop();
                auto op1 = stack.top(); stack.pop();
                if (symbol == "+") stack.push(op1 + op2);
                else if (symbol == "-") stack.push(op1 - op2);
                else if (symbol == "*") stack.push(op1 * op2);
                else if (symbol == "/") stack.push(op1 / op2);
            } else if (counters.find(funcname + '\t' + symbol) != counters.end()) {
                auto a = counters[funcname + '\t' + symbol];
                stack.push(accumulate(a.begin(), a.end(), (long double)0.0));
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
    return (double) stack.top();
}



void PAPIProfBase::clear_counters() {
    _events_names.clear();
    _events_set.clear();
    _counters.clear();
    _metrics.clear();
    while (!_ts_stack.empty()) _ts_stack.pop();
    while (!_key_stack.empty()) _key_stack.pop();
    while (!_eventValues.empty()) _eventValues.pop();
}


void PAPIProfBase::cleanup() {
    if (papi_is_running(_eventSet)) {
        long long *eventValues = new long long[_events_set.size()];
        papi_stop(_eventSet, eventValues);
        delete[] eventValues;
    }

    papi_cleanup(_eventSet);
    papi_destroy(&_eventSet);
    int retval = PAPI_unregister_thread();
    handle_error(retval, "unregiter thread");
}

void PAPIProfBase::add_events(vector<string> events)
{
    vector<string> new_events;
    for (auto e : events) {
        auto pair = _events_set.insert(e);
        if (pair.second) {
            new_events.push_back(e);
            _events_names.push_back(e);
        }
    }
    if (new_events.size() > 0) {
        if (papi_is_running(_eventSet)) {
            long long *eventValues = new long long[_events_set.size()];
            papi_stop(_eventSet, eventValues);
            papi_add_events(_eventSet, new_events);
            papi_start(_eventSet);
            delete[] eventValues;
        } else {
            papi_add_events(_eventSet, new_events);
            papi_start(_eventSet);
        }
    }
}

void PAPIProfBase::remove_events(std::vector<std::string> events)
{
    vector<string> removed_events;
    for (auto e : events) {
        if (_events_set.find(e) != _events_set.end()) {
            _events_set.erase(e);
            auto it = find(_events_names.begin(), _events_names.end(), e);
            _events_names.erase(it);
            removed_events.push_back(e);
        }
    }
    papi_remove_events(_eventSet, removed_events);

    if (removed_events.size() > 0) {
        long long *eventValues = new long long[_events_set.size()];
        if (papi_is_running(_eventSet)) {
            papi_stop(_eventSet, eventValues);
            papi_remove_events(_eventSet, removed_events);
            papi_start(_eventSet);
        } else {
            papi_add_events(_eventSet, removed_events);
            papi_start(_eventSet);
        }
    }
}


void PAPIProfBase::start_counters(string funcname,
                                  vector<string> metrics,
                                  vector<string> events)
{
    if (metrics.size()) add_metrics(metrics);
    add_events(events);
    _key_stack.push(funcname);

    if (_events_set.size()) {
        if (papi_is_running(_eventSet)) {
            long long *eventValues = new long long[_events_set.size()];
            papi_stop(_eventSet, eventValues);
            papi_reset(_eventSet);
        }
        papi_start(_eventSet);
    }

    _ts_stack.push(PAPI_get_real_usec());

}


void PAPIProfBase::stop_counters()
{
    long long *eventValues = new long long[_events_set.size()];

    if (_events_set.size()) {
        papi_stop(_eventSet, eventValues);
        for (int i = 0; i < (int)_events_set.size(); i++) {
            if (eventValues[i] < 0)
                printf("EventName: %s, Value: %Ld\n", _events_names[i].c_str(), eventValues[i]);
        }
    }

    long long te = PAPI_get_real_usec();
    double msec = (double) (te - _ts_stack.top()) / 1000.0; _ts_stack.pop();

    auto key = _key_stack.top(); _key_stack.pop();
    _counters[key + "\ttime(ms)"].push_back(msec);

    for (int i = 0; i < (int)_events_names.size(); ++i) {
        _counters[key + "\t" + _events_names[i]].push_back((double)eventValues[i]);
    }
}



void PAPIProfBase::add_metrics(vector<string> metrics, bool helper)
{
    vector<string> new_metrics;
    for (auto m : metrics) {
        if (_metrics.find(m) == _metrics.end())
            new_metrics.push_back(m);
    }
    vector<string> events;
    set<string> operators = {"/", "*", "-", "+"};
    for (auto m : new_metrics) {
        auto equation = gPresetMetrics[m];
        for (auto symbol : equation) {
            try {
                stod(symbol);
            } catch (exception& e) {
                if (gPresetMetrics.find(symbol) != gPresetMetrics.end()) {
                    add_metrics({symbol}, true);
                } else if (operators.find(symbol) == operators.end()) {
                    events.push_back(symbol);
                }
            }
        }
    }
    if (!helper)
        _metrics.insert(new_metrics.begin(), new_metrics.end());

    add_events(events);

}


void PAPIProfBase::report_metrics()
{
    fprintf(stderr, "\nCounters Report Start\n");
    fprintf(stderr, "function\tcounter\taverage_value\tstd(%%)\tcalls\n");
    set<string> checkedFunctions;
    for (auto &kv : _counters) {
        auto funcname = kv.first.substr(0, kv.first.find('\t'));
        if (checkedFunctions.insert(funcname).second == false)
            continue;
        for (auto &metric_name : _metrics) {
            auto equation = gPresetMetrics[metric_name];
            auto result = evaluate(equation, funcname,
                                   _counters, gPresetMetrics);
            fprintf(stderr, "%s\t%s\t%.3lf\t%d\t%d\n",
                    funcname.c_str(), metric_name.c_str(), result, 0, 0);
        }
    }

    fprintf(stderr, "\nCounters Report End\n");
}

void PAPIProfBase::report_counters()
{
    fprintf(stderr, "\nCounters Report Start\n");
    fprintf(stderr, "function\tcounter\taverage_value\tstd(%%)\tcalls\n");

    for (auto &kv : _counters) {
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


void PAPIProfBase::report_timing() {
    fprintf(stderr, "\nCounters Report Start\n");
    fprintf(stderr, "function\tcounter\taverage_value\tstd(%%)\tcalls\n");

    for (auto &kv : _counters) {
        if (kv.first.find("time(ms)") != string::npos) {
            auto mean = get_mean(kv.second);
            auto std = get_std(kv.second);
            fprintf(stderr, "%s\t%.3lf\t%.2lf\t%lu\n",
                    kv.first.c_str(), mean, 100. * std / mean, kv.second.size());

        }
    }
    fprintf(stderr, "\nCounters Report End\n");

}



PAPIProf::PAPIProf(vector<string> metrics,
                   vector<string> events)
{
    papi_singlethread_init();
    papi_eventset_init(&_eventSet);
    if (metrics.size() != 0)
        add_metrics(metrics);
    add_events(events);
}


PAPIProf::~PAPIProf() {
    cleanup();
    papi_shutdown();
    clear_counters();
}