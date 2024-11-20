#pragma once

#include <cstdint>
#include <map>

#pragma pack(1)
typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
}proc_params;

typedef struct {
    bool rdy;
    uint8_t dst;    // R0-R66
} ROB;

typedef struct {
    bool valid;
    uint32_t dst_tag;

    bool rs1_rdy;
    uint32_t rs1_tag;

    bool rs2_rdy;
    uint32_t rs2_tag;
} IQ;

typedef struct {
    bool valid;
    uint32_t ROB_tag;
} RMT;

typedef struct {
    uint64_t pc;
    int op_type;
    int dest;
    int src1; 
    int src2;
} Bundle;
