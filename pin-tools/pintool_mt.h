#include "pin.H"
#include "pintool.h"

PIN_LOCK g_lock;
#define TRACED_ID 0

VOID RecordMemReadMT(VOID * ip, VOID * addr, VOID * size)
{
    //fprintf(trace,"%p: R %p\n", ip, addr);
    //TraceFile << ip << " R " << addr << std::endl;  
    PIN_GetLock(&g_lock, 0);
    
    UpdateTotalOps();
    UpdateTotalVolume((size_t) size);

    for (uint64_t i = 0; i < count; i++) {
        if ((int64_t)addr >= var_addr[i] && 
            (int64_t)addr < var_addr[i] + var_size[i]) {
            GetMetrics(addr, (size_t) size, i, 0);
            break;
        }
    }
    
    PIN_ReleaseLock(&g_lock);
}

// Print a memory write record
VOID RecordMemWriteMT(VOID * ip, VOID * addr, VOID * size)
{
    PIN_GetLock(&g_lock, 0);
    UpdateTotalOps();
    UpdateTotalVolume((size_t) size);
    
    for (uint64_t i = 0; i < count; i++) {
        if ((int64_t)addr >= var_addr[i] &&
            (int64_t)addr < var_addr[i] + var_size[i]) {
            GetMetrics(addr, (size_t) size, i, 1);
            break;
        }
    }
    PIN_ReleaseLock(&g_lock);
}

VOID RecordMemReadMTsep(VOID* ip, VOID* addr, VOID* size, THREADID threadid)
{
    //fprintf(trace,"%p: R %p\n", ip, addr);
    //TraceFile << ip << " R " << addr << std::endl; 
    //printf("threadid %d\n", threadid);
    if (threadid != TRACED_ID) return;
    
    PIN_GetLock(&g_lock, 0);
    
    UpdateTotalOps();
    UpdateTotalVolume((size_t) size);

#ifdef READ_ADDR_FILE
    if (in_alloc == 1)
        TestAndGetAddr();
#endif

    for (uint64_t i = 0; i < count; i++) {
        if ((int64_t)addr >= var_addr[i] && 
            (int64_t)addr < var_addr[i] + var_size[i]) {
            GetMetrics(addr, (size_t) size, i, 0);
            break;
        }
    }
    
    PIN_ReleaseLock(&g_lock);
}

// Print a memory write record
VOID RecordMemWriteMTsep(VOID* ip, VOID* addr, VOID* size, THREADID threadid)
{
    if (threadid != TRACED_ID) return;
    
    PIN_GetLock(&g_lock, 0);
    UpdateTotalOps();
    UpdateTotalVolume((size_t) size);

#ifdef READ_ADDR_FILE
    if (in_alloc == 1)
        TestAndGetAddr();
#endif

    for (uint64_t i = 0; i < count; i++) {
        if ((int64_t)addr >= var_addr[i] &&
            (int64_t)addr < var_addr[i] + var_size[i]) {
            GetMetrics(addr, (size_t) size, i, 1);
            break;
        }
    }
    PIN_ReleaseLock(&g_lock);
}
