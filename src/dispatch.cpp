// This file includes the In-order fetch/dispatch engine
#include <iostream>
#include <cstdint>
#include <vector>

#include "dispatch.h"

using namespace std;

#define DE_REG  bundle[0]
#define RN_REG  bundle[1]
#define RR_REG  bundle[2]

extern vector<ROB> rob;
extern vector<RMT> rmt;

extern bool trace_read_complete;
extern uint64_t total_instruction_count;
extern uint32_t head, tail;

/* bundle is a matrix of size 4 x width; for 4 pipeline registers in the dispatch engine
 * The no. of rows is 4 because there are 4 pipeline regs in this engine: DE, RN, RR, DI
*/
extern vector<vector<pipeline_regs_d>> bundle;

bool Fetch(FILE *FP, unsigned long int width) {

  int op_type, dest, src1, src2;  // Variables are read from trace file
  uint64_t pc; // Variable holds the pc read from input file

  uint32_t line_count = 0;

  /* If there are more instructions in the trace file and if DE is empty (can accept
   * a new decode bundle), then fetch up to WIDTH instructions from the trace file
   * into DE. Fewer than WIDTH instructions will be fetched only if the trace file
   * has fewer than WIDTH instructions left.
   */ 
  if(DE_REG.empty() && !trace_read_complete) {
    while(line_count < width) {
      if(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF) {
        printf("%lx %d %d %d %d\n", pc, op_type, dest, src1, src2);
        DE_REG.push_back({pc, op_type, dest, src1, src2});
        ++line_count;
        ++total_instruction_count;
      } else {
        trace_read_complete = true;
        break;
      }
    }
  }

  return trace_read_complete;
}

void Decode() {

  /* If RN is empty (can accept a new rename bundle), then advance the decode bundle
   * from DE to RN. Else do nothing.
  */ 
  if(RN_REG.empty()) {
      RN_REG = DE_REG;
      DE_REG.clear();
  }
}

void Rename(unsigned long int rob_size) {

  // Check if ROB has enough free entries to accept the entire rename bundle
  bool is_enough_entries = (tail + RN_REG.size())%rob_size > head;
  
  /* If RR is empty and enough entries exist, process the rename bundle and advance
   * if from RN to RR. Else do nothing.
  */
  if(RR_REG.empty() && is_enough_entries) {

    for(auto &instr: RN_REG) {

      // allocate an entry in the ROB for the instruction
      rob[tail] = {.rdy = false, .dst = instr.dst};

      // rename its source registers
      if(rmt[instr.src1].valid)
        instr.src1 = rmt[instr.src1].ROB_tag;

      if(rmt[instr.src2].valid)
        instr.src2 = rmt[instr.src2].ROB_tag;
      
      // if destination register exist, update RMT and update instr dst with ROB_Tag
      if(instr.dst != -1) {
        rmt[instr.dst] = {.valid = true, .ROB_tag = tail};
      }
      instr.dst = tail; //dst now becomes dst_tag

      tail = (tail + 1)%rob_size;
    }
    
    RR_REG = RN_REG;
    RN_REG.clear();
  }
}

void RegRead() {}

void Dispatch() {}
