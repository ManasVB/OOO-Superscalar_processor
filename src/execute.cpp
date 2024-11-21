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

// Issue up to WIDTH oldest instructions from the IQ
void Issue(unsigned long int width) {
  uint8_t instrs_removed = 0;

  for(auto &instr: iq) {
     
    if(instr.valid && instr.rs1_rdy && instr.rs2_rdy) {

      // Remove the instruction from the IQ.
      instr.valid = false;

      //Add the instruction to the execute_list
      for(auto &itr: execute_list) {
        if(!itr.valid) {  // not valid means free entry
          itr = { .dst_tag = instr.dst_tag, .src1 = instr.rs1_tag, \
            .src2 = instr.rs2_tag, .latency = instr.latency, .valid = true};
          
          break;
        }
      }
      ++instrs_removed;
    }
    if(instrs_removed == width) {
      break;
    }
  }
}

void Execute() {
  
  uint8_t WB_Reg_Counter = 0;
  wakeup.clear();

  // Remove the instruction from the execute_list, which is finishing and add to WB_Reg
  for(auto &instr: execute_list) {
    if(instr.valid && instr.latency == 1) {

      // Add to WB_Reg
      WB_Reg[WB_Reg_Counter] = instr;
      ++WB_Reg_Counter;

      // Remove from execute_list
      instr.valid = false;

      // Wakeup dependent sources in the Issue Queue
      for(auto &iq_itr: iq) {
        if(iq_itr.valid && !iq_itr.rs1_rdy && iq_itr.rs1_tag == instr.dst_tag) {
          iq_itr.rs1_rdy = true;
        }

        if(iq_itr.valid && !iq_itr.rs2_rdy && iq_itr.rs2_tag == instr.dst_tag) {
          iq_itr.rs2_rdy = true;
        }
      }

      // Send a wakeup signal to RR and DI
      wakeup.push_back(instr.dst_tag);

    } else if(instr.valid && instr.latency > 1) {
      --(instr.latency);
    }
  }
}

void Writeback() {}

void Retire() {}
