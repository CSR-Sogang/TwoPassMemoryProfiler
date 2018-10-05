/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2015 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */

#include <stdio.h>
#include <map>
#include <string.h>
#include <iostream>
#include <fstream>
#include "pin.H"


/* ===================================================================== */
/* Marco */
/* ===================================================================== */

/* ===================================================================== */
/* Names of malloc and free */
/* ===================================================================== */
#if defined(TARGET_MAC)
#define MALLOC "_malloc"
#define FREE "_free"
#else
#define MALLOC "malloc"
#define FREE "free"
#define CALLOC "calloc"
#define REALLOC "realloc"
#endif

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

//const size_t MEM_ARRAYS = 0x100000000;
//const int ELEMEMT_SIZE = sizeof(int) << 3;
//int bitmap[MEM_ARRAYS / ELEMEMT_SIZE];
const int SEG_NODES = 2000000000;
const size_t LONGEST_ADDR = 0x1000000000000; // must be format 0x1000... not 0xfff...

typedef struct {
    int right, left;
    int stat;  // stat 0: in stack, 1: in heap, 2, part in heap, part in stack;
    //size_t addr; // the start addr
} seg_node;

seg_node seg_nodes[SEG_NODES];

static size_t heap_volume = 0;
static size_t stack_volume = 0;
static int node_index = 0;


std::ofstream TraceFile;
std::map<size_t, size_t> allocmap;
size_t alloc_size;


/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "top10-malloctrace.out-obj", "specify trace file name");

//KNOB<string> RppOutputFile(KNOB_MODE_WRITEONCE, "pintool", 
//    "o", "References_perpage.data", "References per page");

/* ===================================================================== */
void Update(int node_id, size_t addr, size_t len, 
            size_t level_addr, size_t level_len, int type) {

    //printf("ID %d %lu %lu\n", node_id, addr, len);
    //printf("TY %d %lu %lu\n", type, level_addr, level_len);
    
    //if (node_id > 15) return;
    if (level_addr == addr && len == level_len) {
        seg_nodes[node_id].stat = type;
        return;
    }

    seg_nodes[node_id].stat = 2; // mix type
    
    const size_t start = level_addr;
    const size_t mid = level_addr + level_len / 2;
    //const size_t end = level_addr + level_len;
    
    size_t addr_l, addr_r;
    size_t len_l, len_r;
    size_t type_l, type_r;
    if (addr < mid) {
        len_l = min(len, mid - addr);
        addr_l = addr;
        type_l = type;
    } 
    else {
        
        //len_l = mid - start;
        //addr_l = start;
        //type_l = !type;
        
        type_l = 3;  //only update one side
    }
    
    if (addr + len > mid) {
        addr_r = max(addr, mid);
        len_r = addr + len - addr_r;
        type_r = type;
    }
    else {
        
        //addr_r = mid;
        //len_r = end - mid;
        //type_r = !type;
        
        type_r = 3; // only update one side
    }

    //printf("SME %lu %lu %lu\n", start, mid, end);
    //printf("L %lu %lu\n", addr_l, len_l);
    //printf("R %lu %lu\n", addr_r, len_r);
    if (type_l != 3) {
        if (seg_nodes[node_id].left)
            Update(seg_nodes[node_id].left, addr_l, len_l, 
                   start, level_len / 2, type_l);
        else {
            node_index++;
            seg_nodes[node_id].left = node_index;
            Update(seg_nodes[node_id].left, addr_l, len_l, 
                   start, level_len / 2, type_l);
        }
    }

    if (type_r != 3) {
        if (seg_nodes[node_id].right)
            Update(seg_nodes[node_id].right, addr_r, len_r, 
                   mid, level_len / 2, type_r);
        else {
            node_index++;
            seg_nodes[node_id].right = node_index;
            Update(seg_nodes[node_id].right, addr_r, len_r, 
                   mid, level_len / 2, type_r);
        }
    }

    if (node_index > SEG_NODES) 
        printf("Too much points!\n");
    
}


void UpdateAlloc(size_t addr, size_t len) {
    Update(0, addr, len, (size_t) 0, (size_t) LONGEST_ADDR, 1);
}

void UpdateFree(size_t addr, size_t len) {
    Update(0, addr, len, (size_t) 0, (size_t) LONGEST_ADDR, 0);
}

int Find(size_t addr, size_t level_addr, size_t level_len, int node_id) {
    //printf("%d %d\n", node_id, seg_nodes[node_id].stat);
    if (seg_nodes[node_id].stat != 2) 
        return seg_nodes[node_id].stat;
    
    if (addr < level_addr + level_len / 2) {
        //printf("left\n");
        return seg_nodes[node_id].left == 0 ? 0 :
                Find(addr, level_addr, level_len / 2, seg_nodes[node_id].left);
    }
    else {
        //printf("right\n");
        return seg_nodes[node_id].right == 0 ? 0 :
                Find(addr, level_addr + level_len / 2, level_len / 2, 
                     seg_nodes[node_id].right);
    }
}

inline int FindAddr(size_t addr) {
    return Find(addr, (size_t) 0, (size_t) LONGEST_ADDR, 0);
    //if (addr > LONGEST_ADDR)
    //    printf("Too long Address!\n");
}

void init() {
    memset(seg_nodes, 0, sizeof(seg_nodes));
}


/*
void UpdateAlloc(size_t addr, size_t len){
    
    size_t start = addr / ELEMEMT_SIZE;
    size_t end = (addr + len) / ELEMEMT_SIZE;

    //printf ("%d\n", ELEMEMT_SIZE);
    //printf ("%lu %lu\n", start, end);
    //memset( &bitmap[start+1], -1, (end - start - 1) * ELEMEMT_SIZE);
    if (addr + len > MEM_ARRAYS)
        printf("Error! heap address larger than 32bit\n");
    
    int start_bit = addr % ELEMEMT_SIZE;
    int end_bit = (addr + len) % ELEMEMT_SIZE;
    
    if (start == end) {
        for (int i = start_bit; i < end_bit; i++)
            bitmap[start] |= (0x1 << i);
    } else {
        // Handle the start element
        memset( &bitmap[start+1], -1, (end - start - 1) * ELEMEMT_SIZE >> 3);
    
        for (int i = start_bit; i < ELEMEMT_SIZE; i++)
            bitmap[start] |= (0x1 << i);
    
        // Handle the last element
        for (int i = 0; i < end_bit; i++)
            bitmap[end] |= (0x1 << i);
    }
}

void UpdateFree(size_t addr, size_t len) {
    size_t start = addr / ELEMEMT_SIZE;
    size_t end = (addr + len) / ELEMEMT_SIZE;

    int start_bit = addr % ELEMEMT_SIZE;
    int end_bit = (addr + len) % ELEMEMT_SIZE;
    
    if (start == end) {
        for (int i = start_bit; i < end_bit; i++)
            bitmap[start]  &= ~(0x1 << i);
    } else {
        // Handle the start element
        memset( &bitmap[start+1], 0, (end - start - 1) * ELEMEMT_SIZE >> 3);
    
        for (int i = start_bit; i < ELEMEMT_SIZE; i++)
            bitmap[start]  &= ~(0x1 << i);
    
        // Handle the last element
        for (int i = 0; i < end_bit; i++)
            bitmap[end]  &= ~(0x1 << i);
    }

}


int FindAddr(size_t addr) {
    size_t loc = addr / ELEMEMT_SIZE;
    size_t loc_bit = addr % ELEMEMT_SIZE;
    //printf("here3\n");
    //printf("%lu %lu\n", addr, loc_bit);
    if (addr > MEM_ARRAYS) return 0;
    return 1 && (bitmap[loc] & (0x1 << loc_bit));
}

void init() {
    memset(bitmap, 0, sizeof(bitmap));
    } */

/* ===================================================================== */
/* Analysis routines                                                     */
/* ===================================================================== */

/*
inline double GetTimeNow() {
    struct timeval stime;
    gettimeofday(&stime, NULL);
    return stime.tv_sec + 1e-6 * stime.tv_usec;
    }*/

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, VOID * size)
{
    if (FindAddr((size_t) addr))
        heap_volume += (size_t) size;
    else 
        stack_volume += (size_t) size;    

}

VOID RecordMemRead(VOID * ip, VOID * addr, VOID * size)
{

    if (FindAddr((size_t) addr))
        heap_volume += (size_t) size;
    else 
        stack_volume += (size_t) size;  
}

VOID MallocBefore(CHAR * name, ADDRINT size)
{
    alloc_size = size;
}

VOID CallocBefore(CHAR * name, ADDRINT num, ADDRINT size) 
{
    alloc_size = size * num;
}

VOID ReallocBefore(CHAR * name, ADDRINT addr, ADDRINT size)
{
    UpdateFree(addr, allocmap[(size_t) addr]);
    alloc_size = size;
}

VOID FreeBefore(CHAR * name, ADDRINT addr) 
{
    UpdateFree(addr, allocmap[(size_t) addr]);
}

VOID AllocAfter(ADDRINT ret) {
    UpdateAlloc((size_t) ret, (size_t) alloc_size);
    allocmap[(size_t)ret] = alloc_size;
}

/* ===================================================================== */
/* Instrumentation routines                                              */
/* ===================================================================== */
   
VOID Image(IMG img, VOID *v)
{
    // Instrument the malloc() and free() functions.  Print the input argument
    // of each malloc() or free(), and the return value of malloc().
    //
    //  Find the malloc() function.
    RTN mallocRtn = RTN_FindByName(img, MALLOC);
    
    if (RTN_Valid(mallocRtn))
    {
        RTN_Open(mallocRtn);

        // Instrument malloc() to print the input argument value and the return value.
        RTN_InsertCall(mallocRtn, IPOINT_BEFORE, (AFUNPTR)MallocBefore,
                       IARG_ADDRINT, MALLOC,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        RTN_InsertCall(mallocRtn, IPOINT_AFTER, (AFUNPTR)AllocAfter,
                       IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);

        RTN_Close(mallocRtn);
    }

    RTN callocRtn = RTN_FindByName(img, CALLOC);
    
    if (RTN_Valid(callocRtn))
    {
        RTN_Open(callocRtn);

        // Instrument calloc() to print the input argument value and the return value.
        RTN_InsertCall(callocRtn, IPOINT_BEFORE, (AFUNPTR)CallocBefore,
                       IARG_ADDRINT, CALLOC,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                       IARG_END);
        RTN_InsertCall(callocRtn, IPOINT_AFTER, (AFUNPTR)AllocAfter,
                       IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);


        RTN_Close(callocRtn);
    }

    RTN reallocRtn = RTN_FindByName(img, REALLOC);
    
    if (RTN_Valid(reallocRtn))
    {
        RTN_Open(reallocRtn);

        // Instrument realloc() to print the input argument value and the return value.
        RTN_InsertCall(reallocRtn, IPOINT_BEFORE, (AFUNPTR)ReallocBefore,
                       IARG_ADDRINT, REALLOC,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                       IARG_END);
        RTN_InsertCall(reallocRtn, IPOINT_AFTER, (AFUNPTR)AllocAfter,
                       IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);

        RTN_Close(reallocRtn);
    }

    // Find the free() function.
    RTN freeRtn = RTN_FindByName(img, FREE);
    if (RTN_Valid(freeRtn))
    {
        RTN_Open(freeRtn);
        // Instrument free() to print the input argument value.
        RTN_InsertCall(freeRtn, IPOINT_BEFORE, (AFUNPTR)FreeBefore,
                       IARG_ADDRINT, FREE,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        RTN_Close(freeRtn);
    }
}

VOID Instruction(INS ins, VOID *v)
{
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                IARG_INST_PTR, 
                IARG_MEMORYOP_EA, memOp,
                IARG_MEMORYREAD_SIZE,
                IARG_END);
        }
        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                IARG_INST_PTR, 
                IARG_MEMORYOP_EA, memOp,
                IARG_MEMORYWRITE_SIZE,
                IARG_END);
        }
    }
    //INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)InsCount, IARG_END);
}

/* ===================================================================== */

VOID Fini(INT32 code, VOID *v)
{
    TraceFile << "Heap Volume:" << heap_volume << endl;
    TraceFile << "Percentage: " << 100 * heap_volume / 
            (double) (heap_volume + stack_volume) << "%" << endl;
    TraceFile << "Stack Volume:" << stack_volume << endl;
    TraceFile << "Percentage:  " << 100 * stack_volume / 
            (double) (heap_volume + stack_volume) << "%" << endl;
    
    TraceFile.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
    cerr << "This tool produces a trace of calls to malloc." << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}


/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    // Initialize pin & symbol manager
    PIN_InitSymbols();

    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    init();
    

    TraceFile.open("heap_volume_res", ios::app);
    TraceFile.setf(ios::showbase);

    // Register Image to be called to instrument functions.
    IMG_AddInstrumentFunction(Image, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);
    
    // Never returns
    PIN_StartProgram();
    
    // Del the message queue
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
