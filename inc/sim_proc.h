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
    uint64_t age;
    uint8_t latency;
    int op_type;
    int dst;
    int src1; 
    int src2;
    uint64_t begincycle[9]; // begin-cycle of a given pipeline stage 
} Payload;

typedef struct {
    bool rdy;
    int dst;    // R0-R66, -1 if no dest register
    Payload payload;
} ROB;

typedef struct {
    bool valid;
    uint32_t dst_tag;

    bool rs1_rdy;
    uint32_t rs1_tag;

    bool rs2_rdy;
    uint32_t rs2_tag;

    uint64_t age;

    uint8_t latency;
} IQ;

typedef struct {
    bool valid;
    uint32_t ROB_tag;
} RMT;

typedef struct {
    uint64_t pc;
    int op_type;
    int dst;
    int src1; 
    int src2;
    bool src1_rdy;
    bool src2_rdy;
    uint64_t age;
    uint8_t latency;
    uint64_t fetch_begin;
    uint64_t decode_begin;
} Bundle;
