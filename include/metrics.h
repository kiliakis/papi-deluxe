#pragma once

#include <vector>
#include <string>
#include <unordered_map>

namespace metrics {

std::unordered_map<std::string, std::vector<std::string>>
gPresetMetrics = {
    {"IPC", {"INSTRUCTIONS_RETIRED", "CPU_CLK_UNHALTED", "/"} },
    {"CPI", {"CPU_CLK_UNHALTED", "INSTRUCTIONS_RETIRED", "/"} },
    {   "FRONT_BOUND%", {"IDQ_UOPS_NOT_DELIVERED:CORE", "4.",
            "CPU_CLK_UNHALTED", "*", "/", "100.0", "*"
        }
    },
    {   "BAD_SPECULATION%", {"UOPS_ISSUED:ANY", "UOPS_RETIRED:RETIRE_SLOTS", "-",
            "4.0", "INT_MISC:RECOVERY_CYCLES", "*", "+",
            "4.0", "CPU_CLK_UNHALTED", "*", "/", "100.0", "*"
        }
    },
    {   "RETIRING%", {"UOPS_RETIRED:RETIRE_SLOTS",
            "4.0", "CPU_CLK_UNHALTED", "*", "/", "100.0", "*"
        }
    },
    {   "BACK_BOUND%", {
            "1.0",
            "FRONTEND_BOUND", "RETIRING", "+",
            "BAD_SPECULATION", "+", "-", "100.0", "*"
        }
    },
    {   "BACK_BOUND_AT_EXE%", {"CYCLE_ACTIVITY:CYCLES_NO_EXECUTE",
            "UOPS_EXECUTED:CYCLES_GE_1_UOP_EXEC", "+",
            "UOPS_EXECUTED:CYCLES_GE_2_UOPS_EXEC", "-",
            "CPU_CLK_UNHALTED", "/", "100.0", "*"
        }
    },
    {   "MEMORY_BOUND%", {"CYCLE_ACTIVITY:STALLS_LDM_PENDING", "CPU_CLK_UNHALTED",
            "/", "100.0", "*"
        }
    },
    {   "L2_COST%", {
            "12.0", "MEM_LOAD_UOPS_RETIRED:L2_HIT", "*",
            "CPU_CLK_UNHALTED", "/", "100.0", "*"
        }
    },
    {   "L3_COST%", {
            "26.0", "MEM_LOAD_UOPS_RETIRED:L3_HIT", "*",
            "CPU_CLK_UNHALTED", "/", "100.0", "*"
        }
    },
    {   "MEMORY_COST%", {
            "200.0", "MEM_LOAD_UOPS_LLC_HIT_RETIRED:XNSP_HIT", "*",
            "CPU_CLK_UNHALTED", "/", "100.0", "*"
        }
    },
    {   "L1_BOUND%", {"CYCLE_ACTIVITY:STALLS_LDM_PENDING",
            "CYCLE_ACTIVITY:STALLS_L1D_PENDING", "-",
            "CPU_CLK_UNHALTED", "/", "100.0", "*"
        }
    },
    {   "L2_BOUND%", {"CYCLE_ACTIVITY:STALLS_L1D_PENDING",
            "CYCLE_ACTIVITY:STALLS_L2_PENDING", "-",
            "CPU_CLK_UNHALTED", "/", "100.0", "*"
        }
    },
    {   "L3_BOUND%", {"CYCLE_ACTIVITY:STALLS_L2_PENDING", "L3_Hit_fraction", "*",
            "CPU_CLK_UNHALTED", "/", "100.0", "*"
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
    {"MEM_L3_WEIGHT", {"7.0"}},
    {   "L3_MISS_FRACTION", {"MEM_LOAD_UOPS_RETIRED:L3_MISS",
            "MEM_LOAD_UOPS_RETIRED:L3_MISS",
            "MEM_LOAD_UOPS_RETIRED:L3_HIT",
            "+",
            "/"
        }
    },
    {   "MEM_BOUND%", {"CYCLE_ACTIVITY:STALLS_L2_PENDING", "L3_MISS_FRACTION", "*",
            "CPU_CLK_UNHALTED", "/", "100.0", "*"
        }
    },
    {   "UNCORE_BOUND%", {"CYCLE_ACTIVITY:STALLS_L2_PENDING", "CPU_CLK_UNHALTED",
            "/", "100.0", "*"
        }
    },
    {"CORE_BOUND%", {"BACK_BOUND_AT_EXE%", "MEM_BOUND%", "-", "100.0", "*"}},
    {"RESOURCE_STALLS_COST%", {"RESOURCE_STALLS:ALL", "CPU_CLK_UNHALTED", "/"}},
    {   "LOCK_CONTENTION%", {"MEM_LOAD_UOPS_L3_HIT_RETIRED:XSNP_HITM",
            "MEM_UOPS_RETIRED:LOCK_LOADS", "/", "100.00", "*"
        }
    },
    {   "BR_MISP_COST%", {
            "20.0", "BR_MISP_RETIRED:ALL_BRANCHES", "*",
            "CPU_CLK_UNHALTED", "/", "100.0", "*"
        }
    }
};

}