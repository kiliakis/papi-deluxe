#pragma once

#include <papi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include <stack>

int papi_init(int *eventSet);
int papi_add_events(int eventSet, std::vector<std::string> eventNames);
int papi_start(int eventSet);
int papi_stop(int eventSet, long long * values);
int papi_reset(int eventSet);
int papi_read(int eventSet, long long * values);
int papi_destroy(int *eventSet);
int papi_remove_events(int eventSet, std::vector<std::string> eventNames);
int papi_cleanup(int eventset);
int papi_destroy(int * eventset);
bool papi_is_running(int eventSet);
int papi_multithread_init(unsigned long (*func) (void));
void papi_shutdown();


class PAPIProf
{
private:
    std::set<std::string> _events_set;
    std::vector<std::string> _events_names;
    std::set<std::string> _metrics;
    std::unordered_map<std::string, std::vector<double>> _counters;
    int _eventSet;
    std::stack<long long> _ts_stack;
    std::stack<std::string> _key_stack;
    std::stack<long long *> _eventValues;
public:
    PAPIProf(std::vector<std::string> metrics = std::vector<std::string>(),
             std::vector<std::string> events = std::vector<std::string>());
    ~PAPIProf();
    void add_events(std::vector<std::string> events);
    void start_counters(std::string funcname,
                        std::vector<std::string> metrics = std::vector<std::string>(),
                        std::vector<std::string> events = std::vector<std::string>());
    void stop_counters();
    // void clear_counters();
    void remove_events(std::vector<std::string> events);
    void add_metrics(std::vector<std::string> metrics, bool helper = false);
    void report_metrics();
    void report_counters();
    void report_timing();
};

class PAPIProfMultiThread
{
private:
    std::set<std::string> _events_set;
    std::vector<std::string> _events_names;
    std::set<std::string> _metrics;
    std::unordered_map<std::string, std::vector<double>> _counters;
    int _eventSet = PAPI_NULL;
    std::stack<long long> _ts_stack;
    std::stack<std::string> _key_stack;
    std::stack<long long *> _eventValues;
public:
    PAPIProfMultiThread(std::vector<std::string> metrics = std::vector<std::string>(),
                        std::vector<std::string> events = std::vector<std::string>());
    ~PAPIProfMultiThread();
    void add_events(std::vector<std::string> events);
    void start_counters(std::string funcname,
                        std::vector<std::string> metrics = std::vector<std::string>(),
                        std::vector<std::string> events = std::vector<std::string>());
    void stop_counters();
    void clear_counters();
    void cleanup();
    void remove_events(std::vector<std::string> events);
    void add_metrics(std::vector<std::string> metrics, bool helper = false);
    void report_metrics();
    void report_counters();
    void report_timing();
};