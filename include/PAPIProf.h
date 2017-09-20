#pragma once

#include <papi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include <stack>


int papi_singlethread_init();
int papi_multithread_init(unsigned long (*func) (void));
int papi_eventset_init(int *eventSet);
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
void papi_shutdown();

// int papi_multithread_init(unsigned long (*func) (void));
double get_mean(std::vector<double> &v);
double get_std(std::vector<double> &v);
double evaluate(std::vector<std::string> equation,
                std::string &funcname,
                std::unordered_map<std::string, std::vector<double>> &counters,
                std::unordered_map<std::string, std::vector<std::string>> &preset_metrics);

class PAPIProfBase
{
protected:
    std::set<std::string> _events_set;
    std::vector<std::string> _events_names;
    int _eventSet = PAPI_NULL;
    std::stack<long long> _ts_stack;
    std::stack<std::string> _key_stack;
    std::stack<long long *> _eventValues;
public:
    std::unordered_map<std::string, std::vector<double>> _counters;
    std::set<std::string> _metrics;

    PAPIProfBase(std::vector<std::string> metrics = std::vector<std::string>(),
                 std::vector<std::string> events = std::vector<std::string>()) {};
    ~PAPIProfBase() {};
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



class PAPIProf : public PAPIProfBase
{
public:
    PAPIProf(std::vector<std::string> metrics = std::vector<std::string>(),
             std::vector<std::string> events = std::vector<std::string>());
    ~PAPIProf();
};




class PAPIProfMultiThread : public PAPIProfBase
{
public:
    PAPIProfMultiThread();
    ~PAPIProfMultiThread();
    void init(std::vector<std::string> metrics = std::vector<std::string>(),
              std::vector<std::string> events = std::vector<std::string>());
};


std::unordered_map<std::string, std::vector<double>> compute_global_counters(std::vector<PAPIProfMultiThread *> &profilers);
std::unordered_map<std::string, double> compute_global_metrics(std::vector<PAPIProfMultiThread *> &profilers);
void report_global_counters(std::vector<PAPIProfMultiThread *> &profilers);
void report_global_metrics(std::vector<PAPIProfMultiThread *> &profilers);
