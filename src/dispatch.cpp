// This file includes the In-order fetch/dispatch engine
#include <iostream>
#include <cstdint>
#include <vector>
#include <algorithm>

#include "dispatch.h"

using namespace std;

#define DE_REG  bundle[0]
#define RN_REG  bundle[1]
#define RR_REG  bundle[2]
#define DI_REG  bundle[3]

extern vector<ROB> rob;
extern vector<RMT> rmt;
extern vector<IQ> iq;

extern vector<uint32_t> wakeup;

extern bool trace_read_complete;
extern uint64_t total_instruction_count;
extern uint64_t total_cycle_count;
extern int64_t final_instruction_number;

extern uint32_t head, tail;
extern bool is_rob_full;

/* bundle is a matrix of size 4 x width; for 4 pipeline registers in the dispatch engine
 * The no. of rows is 4 because there are 4 pipeline regs in this engine: DE, RN, RR, DI
*/
vector<vector<pipeline_regs_d>> bundle(4);

void Fetch(FILE *FP, unsigned long int width) {

  int op_type, dest, src1, src2;  // Variables are read from trace file
  uint64_t pc; // Variable holds the pc read from input file

  uint32_t line_count = 0;
  uint8_t latency = 0;

  /* If there are more instructions in the trace file and if DE is empty (can accept
   * a new decode bundle), then fetch up to WIDTH instructions from the trace file
   * into DE. Fewer than WIDTH instructions will be fetched only if the trace file
   * has fewer than WIDTH instructions left.
   */ 
  if(DE_REG.empty() && !trace_read_complete) {
    while(line_count < width) {
      if(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF) {

        // Set latency according to op_type
        if(op_type == 0)
          latency = 1;
        else if(op_type == 1)
          latency = 2;
        else
          latency = 5;

        DE_REG.push_back({total_instruction_count, op_type, latency, dest, false, src1, false, src2, {total_cycle_count,0}});

        ++line_count;
        ++total_instruction_count;
      } else {
        final_instruction_number = total_instruction_count;
        trace_read_complete = true;
        break;
      }
    }
  }
}

void Decode() {
  if(!DE_REG.empty()) {
    if(RN_REG.empty()) {
      for(auto &instr: DE_REG) {

        // Decode stage begin cycle
        instr.begin_cycle[1] = total_cycle_count;
      }

      RN_REG = DE_REG;
      DE_REG.clear();
    }
  }
}

void Rename(unsigned long int rob_size) {

  if(!RN_REG.empty()) {
    bool is_rob_free = (tail + RN_REG.size())%rob_size > head;

    if(RR_REG.empty() && is_rob_free) {
      for(auto &instr: RN_REG) {

        // Rename stage begin cycle
        instr.begin_cycle[2] = total_cycle_count;

        // allocate an entry in the ROB for the instruction
        rob[tail] = {.rdy = false, .dest = instr.dest, .metadata = &instr};

        // rename its source registers
        if(instr.src1 == -1) {
          instr.src1_rdy = true;
        } else {
          (rmt[instr.src1].valid) ? instr.src1 = rmt[instr.src1].ROB_tag : instr.src1_rdy = true;
        }

        if(instr.src2 == -1) {
          instr.src2_rdy = true;
        } else {
          (rmt[instr.src2].valid) ? instr.src2 = rmt[instr.src2].ROB_tag : instr.src2_rdy = true;
        }

        if(instr.dest != -1) {
          rmt[instr.dest] = {.valid = true, .ROB_tag = tail};
        }
        instr.dest = tail; //dst now becomes dst_tag

        tail = (tail + 1)%rob_size;

        if(head == tail) {
          is_rob_full = true;
        }
      }

      RR_REG = RN_REG;
      RN_REG.clear();
    }
  }
}

void RegRead() {
  if(!RR_REG.empty()) {
    if(DI_REG.empty()) {
      for(auto &instr: RR_REG) {
         // RegRead stage begin cycle
        instr.begin_cycle[3] = total_cycle_count;

        if(!wakeup.empty()) {
          for(auto &wakeup_itr: wakeup) {
            if(!instr.src1_rdy && instr.src1 == (int)wakeup_itr) {
              instr.src1_rdy = true;
              break;
            }
          }

          for(auto &wakeup_itr: wakeup) {
            if(!instr.src2_rdy && instr.src2 == (int)wakeup_itr) {
              instr.src2_rdy = true;
              break;
            }
          }
        }
      }

      DI_REG = RR_REG;
      RR_REG.clear();
    }
  }
}


void Dispatch() {

}
