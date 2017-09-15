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
    std::string _key = ""; // could be even better with a stack of strings
    // initialize timer start
    std::chrono::time_point<std::chrono::high_resolution_clock> _ts;

public:
    std::unordered_map<std::string, std::vector<std::string>>
    _preset_metrics = {
        { "IPC", {"INSTRUCTIONS_RETIRED", "CPU_CLK_UNHALTED", "/"} },
        { "CPI", {"CPU_CLK_UNHALTED", "INSTRUCTIONS_RETIRED", "/"} },
        {   "FRONT_BOUND%", {"IDQ_UOPS_NOT_DELIVERED:CORE",
                "4.", "CPU_CLK_UNHALTED", "*", "/", "100.", "*"
            }
        },
        {   "BAD_SPECULATION%", {"UOPS_ISSUED:ANY", "UOPS_RETIRED:RETIRE_SLOTS", "-",
                "4.", "INT_MISC:RECOVERY_CYCLES", "*", "+",
                "4.", "CPU_CLK_UNHALTED", "*", "/", "100.", "*"
            }
        },
        {   "RETIRING%", {"UOPS_RETIRED:RETIRE_SLOTS",
                "4.", "CPU_CLK_UNHALTED", "*", "/", "100.", "*"
            }
        },
        {   "BACK_BOUND%", {
                "1.",
                "FRONTEND_BOUND", "RETIRING", "+",
                "BAD_SPECULATION", "+", "-", "100.", "*"
            }
        },
        {   "BACK_BOUND_AT_EXE%", {"CYCLE_ACTIVITY:CYCLES_NO_EXECUTE",
                "UOPS_EXECUTED:CYCLES_GE_1_UOP_EXEC", "+",
                "UOPS_EXECUTED:CYCLES_GE_2_UOPS_EXEC", "-",
                "CPU_CLK_UNHALTED", "/", "100.", "*"
            }
        },
        {   "MEMORY_BOUND%", {"CYCLE_ACTIVITY:STALLS_LDM_PENDING", "CPU_CLK_UNHALTED",
                "/", "100.", "*"
            }
        },
        {   "L2_COST%", {
                "12.", "MEM_LOAD_UOPS_RETIRED:L2_HIT", "*",
                "CPU_CLK_UNHALTED", "/", "100.", "*"
            }
        },
        {   "L3_COST%", {
                "26.", "MEM_LOAD_UOPS_RETIRED:L3_HIT", "*",
                "CPU_CLK_UNHALTED", "/", "100.", "*"
            }
        },
        {   "MEMORY_COST%", {
                "200.", "MEM_LOAD_UOPS_LLC_HIT_RETIRED:XNSP_HIT", "*",
                "CPU_CLK_UNHALTED", "/", "100.", "*"
            }
        },
        {   "L1_BOUND%", {"CYCLE_ACTIVITY:STALLS_LDM_PENDING",
                "CYCLE_ACTIVITY:STALLS_L1D_PENDING", "-",
                "CPU_CLK_UNHALTED", "/", "100.", "*"
            }
        },
        {   "L2_BOUND%", {"CYCLE_ACTIVITY:STALLS_L1D_PENDING",
                "CYCLE_ACTIVITY:STALLS_L2_PENDING", "-",
                "CPU_CLK_UNHALTED", "/", "100.", "*"
            }
        },
        {   "L3_BOUND%", {"CYCLE_ACTIVITY:STALLS_L2_PENDING", "L3_Hit_fraction", "*",
                "CPU_CLK_UNHALTED", "/", "100.", "*"
            }
        },
        {   "L3_HIT_FRACTION", {"MEM_LOAD_UOPS_RETIRED:L3_HIT",
                "MEM_LOAD_UOPS_RETIRED:L3_HIT",
                "MEM_L3_WEIGHT", "MEM_LOAD_UOPS_RETIRED:L3_MISS", "*",
                "+",
                "/"
            }
        },
// MEM_L3_WEIGHT: External memory to L3 cache latency ratio, 7 can be used
//for 3rd generation intel
        {"MEM_L3_WEIGHT", {"7."}},
        {   "L3_MISS_FRACTION", {"MEM_LOAD_UOPS_RETIRED:L3_MISS",
                "MEM_LOAD_UOPS_RETIRED:L3_MISS",
                "MEM_LOAD_UOPS_RETIRED:L3_HIT",
                "+",
                "/"
            }
        },
        {   "MEM_BOUND%", {"CYCLE_ACTIVITY:STALLS_L2_PENDING", "L3_MISS_FRACTION", "*",
                "CPU_CLK_UNHALTED", "/", "100.", "*"
            }
        },
        {   "UNCORE_BOUND%", {"CYCLE_ACTIVITY:STALLS_L2_PENDING", "CPU_CLK_UNHALTED",
                "/", "100.", "*"
            }
        },
        {"CORE_BOUND%", {"BACK_BOUND_AT_EXE", "MEM_BOUND", "-", "100.", "*"}},
        {"RESOURCE_STALLS_COST%", {"RESOURCE_STALLS:ALL", "CPU_CLK_UNHALTED", "/"}},
        {   "LOCK_CONTENTION%", {"MEM_LOAD_UOPS_L3_HIT_RETIRED:XSNP_HITM",
                "MEM_UOPS_RETIRED:LOCK_LOADS", "/", "100.", "*"
            }
        },
        {   "BR_MISP_COST%", {
                "20.", "BR_MISP_RETIRED:ALL_BRANCHES", "*",
                "CPU_CLK_UNHALTED", "/", "100.", "*"
            }
        }
    };
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