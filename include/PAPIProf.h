#pragma once

#include <papi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include <chrono>

int papi_init(int *eventSet);
int papi_add_events(int eventSet, std::vector<std::string> eventNames);
int papi_start(int eventSet);
int papi_stop(int eventSet, long long * values);
int papi_reset(int eventSet);
int papi_read(int eventSet, long long * values);
int papi_destroy(int *eventSet);


class PAPIProf
{
private:
    std::set<std::string> _events_set;
    std::vector<std::string> _events_names;
    std::set<std::string> _metrics;
    std::unordered_map<std::string, std::vector<long long>> _counters;
    int _eventSet;
    std::string _key; // could be even better with a stack of strings
    std::chrono::time_point<std::chrono::high_resolution_clock> _ts;

public:
    PAPIProf(std::vector<std::string> metrics = std::vector<std::string>(),
             std::vector<std::string> events = std::vector<std::string>());
    ~PAPIProf();
    void add_events(std::vector<std::string> events);
    void start_counters(std::string funcname,
                        std::vector<std::string> metrics = std::vector<std::string>(),
                        std::vector<std::string> events = std::vector<std::string>());
    void stop_counters();
    void clear_counters();
    void add_metrics(std::vector<std::string> metrics, bool helper = false);
    void report_metrics();
    void report_counters();
    void report_timing();
};