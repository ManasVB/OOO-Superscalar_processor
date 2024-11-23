// This file includes the Out-of-order issue/execute engine
#include <iostream>
#include <stdint.h>
#include <vector>

#include "execute.h"

using namespace std;

extern vector<ROB> rob;
extern vector<RMT> rmt;
extern vector<IQ> iq;

extern vector<uint32_t> wakeup;

vector<pipeline_regs_e> execute_list;
vector<pipeline_regs_e> WB_Reg;

extern uint32_t head, tail;
extern bool is_rob_full;

// Issue up to WIDTH oldest instructions from the IQ
void Issue(unsigned long int width) {

}

void Execute() {

}

// Process the writeback bundle in WBFor each instruction in WB, mark the instruction as “ready” in its entry in the ROB.
void Writeback() {

}

void Retire(unsigned long int rob_size, unsigned long int width) {

}
