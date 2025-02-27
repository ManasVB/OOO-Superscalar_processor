#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <inttypes.h>

#include <vector>

#include "sim_proc.h"
#include "frontend.h"
#include "backend.h"

using namespace std;

vector<RMT> rmt;
vector<ROB> rob;
vector<IQ> iq;

uint32_t head = 0, tail = 0;
bool is_rob_full = false;

uint8_t num_regs = 67;  // No. of registers in the ISA (r0-r66)
uint64_t total_cycle_count = 0;
uint64_t total_instruction_count = 0;
int64_t final_instruction_number = -1;

bool trace_read_complete = false;
bool is_done = false;

extern vector<pipeline_regs_e> execute_list;
extern vector<pipeline_regs_e> WB_Reg;

static bool Advance_Cycle(void);

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/
int main (int argc, char* argv[]) {
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    
    if (argc != 5) {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
    // printf("rob_size:%lu "
    //         "iq_size:%lu "
    //         "width:%lu "
    //         "tracefile:%s\n", params.rob_size, params.iq_size, params.width, trace_file);
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL) {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    // Setup ROB, RMT and IQ
    rob.resize(params.rob_size, {0});
    iq.resize(params.iq_size, {0});
    rmt.resize(num_regs,{0});
    
    execute_list.resize(params.width*5, {0});
    WB_Reg.resize(params.width*5, {0});

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // The following loop just tests reading the trace and echoing it back to the screen.
    //
    // Replace this loop with the "do { } while (Advance_Cycle());" loop indicated in the Project 3 spec.
    // Note: fscanf() calls -- to obtain a fetch bundle worth of instructions from the trace -- should be
    // inside the Fetch() function.
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    do {

        Retire(params.rob_size, params.width);

        Writeback();

        Execute();

        Issue(params.width);

        Dispatch();

        RegRead();

        Rename(params.rob_size);

        Decode();

        Fetch(FP, params.width);

    } while (Advance_Cycle());

    printf("# === Simulator Command =========\n");
    printf("# ./sim %lu %lu %lu %s\n", params.rob_size, params.iq_size, params.width, trace_file);
    printf("# === Processor Configuration ===\n");
    printf("# ROB_SIZE = %lu\n",params.rob_size);
    printf("# IQ_SIZE = %lu\n",params.iq_size);
    printf("# WIDTH    = %lu\n",params.width);
    printf("# === Simulation Results ========\n");
    printf("# Dynamic Instruction Count    = %lu\n",total_instruction_count);
    printf("# Cycles                       = %lu\n", total_cycle_count);
    printf("# Instructions Per Cycle (IPC) = %.2f\n", (float)total_instruction_count/(float)total_cycle_count);
    
    return 0;
}

static bool Advance_Cycle () {

    ++total_cycle_count;
    
    return (!trace_read_complete || !is_done);
}
