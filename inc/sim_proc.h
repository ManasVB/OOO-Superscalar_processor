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
    int op_type;
    uint8_t latency;
    int dest;

    bool src1_rdy;
    int src1; 
    
    bool src2_rdy;
    int src2;

    uint64_t begin_cycle[9]; //Begin cycle for all the stages

} Payload;

typedef struct {
    bool rdy;
    int dest;    // R0-R66, -1 if no dest register
    Payload metadata;
} ROB;

typedef struct {
    bool valid;
    uint64_t age;
    Payload payload;
} IQ;

typedef struct {
    bool valid;
    uint32_t ROB_tag;
} RMT;
