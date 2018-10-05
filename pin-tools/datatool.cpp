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
#include <stdlib.h>
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

std::ofstream TraceFile;

const size_t max_stack_size = 8 * 1024 * 1024;

static size_t global_volume = 0;
static size_t other_volume = 0;
static size_t stack_volume = 0;


size_t stack_address = 0;
size_t global_start_address = 0;
size_t global_end_address = 0;

void GetGlobalData() { 
    char line[200];
    char start_address[200];
    char end_address[200];
    FILE* status = fopen("/proc/self/smaps", "r");
    
    while (fgets(line, 200, status) ) {
       printf("%s", line);
        if (strstr(line, "[stack]") != NULL) {
            const char* pos1 = strstr(line, "-");
            //printf("here\n");
            strncpy(start_address, line, pos1 - line);
                        
            start_address[pos1 - line] = '\0';
            
            //printf("%s\n", start_address);
            //printf("%s\n", end_address);
            stack_address = strtol(start_address, NULL, 16);
        }

        if (strstr(line, "rw-p") != NULL) {
            const char* pos1 = strstr(line, "-");
            const char* pos2 = strstr(line, "rw-p");
            //printf("here\n");
            strncpy(start_address, line, pos1 - line);
            strncpy(end_address, pos1 + 1, pos2 - pos1);
            
            start_address[pos1 - line] = '\0';
            end_address[pos2 - pos1] = '\0';
            //printf("%s\n", start_address);
            //printf("%s\n", end_address);
            if (global_start_address == 0) {
                global_start_address = strtol(start_address, NULL, 16);
                global_end_address = strtol(end_address, NULL, 16);
            }
            else if (global_end_address == (size_t)strtol(start_address, NULL, 16)) {
                global_end_address = strtol(end_address, NULL, 16);
            }
        }
    }
    fclose(status);
    if (stack_address == 0) {
        printf("Stack address is 0 !!!\n");
    }
    //printf("%lx\n", stack_address);
}

inline void FindAddr(size_t addr, size_t size) {
    //return Find(addr, (size_t) 0, (size_t) LONGEST_ADDR, 0);
    
    if (addr > global_start_address && addr < global_end_address) 
        global_volume += size;
    
    else if (addr > stack_address && addr < stack_address + max_stack_size) 
        stack_volume += size;
    
    else 
        other_volume += size;
    //if (addr > LONGEST_ADDR)
    //    printf("Too long Address!\n");
}

void init() {
    //    memset(seg_nodes, 0, sizeof(seg_nodes));
    GetGlobalData();
}

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
    
    FindAddr((size_t) addr, (size_t) size);

}

VOID RecordMemRead(VOID * ip, VOID * addr, VOID * size)
{

    FindAddr((size_t) addr, (size_t) size);
     
}

/* ===================================================================== */
/* Instrumentation routines                                              */
/* ===================================================================== */
   
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
    TraceFile << "Global Volume:" << global_volume << endl;
    TraceFile << "Percentage: " << 100 * global_volume / 
            (double) (global_volume + stack_volume + other_volume) << "%" << endl;
    TraceFile << "Stack Volume:" << stack_volume << endl;
    TraceFile << "Percentage:  " << 100 * stack_volume / 
            (double) (global_volume + stack_volume + other_volume) << "%" << endl;
    
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
    

    TraceFile.open("category_volume_res_bytes", ios::app);
    TraceFile.setf(ios::showbase);

    // Register Image to be called to instrument functions.
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
