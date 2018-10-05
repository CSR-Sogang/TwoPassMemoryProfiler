#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <execinfo.h>
extern "C" {
#include "nvm.h"
}

#define _DEBUG

#ifdef _DEBUG
#define PrintMessage(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PrintMessage(fmt, ...) fprintf(log_fd, fmt, ##__VA_ARGS__)
//#define PrintMessage(fmt, ...) 
#endif

static void* (*callocp) (size_t, size_t) = NULL;
static void* (*mallocp) (size_t) = NULL;
static void* (*reallocp)(void*, size_t) = NULL;
static void (*freep)( void* ) = NULL;

char tmp_array[500000000]; // for the bug with the pthread program

static bool trace_flag = true;
static bool inited = false;

static void __attribute__ ((constructor)) init() 
{
    PrintMessage("Init Begin\n");

    trace_flag = true;

    mallocp = ( void* (*)(size_t)) dlsym(RTLD_NEXT, "malloc");
    callocp = ( void* (*)(size_t, size_t)) dlsym(RTLD_NEXT, "calloc");
    reallocp = ( void* (*)(void*, size_t)) dlsym(RTLD_NEXT, "realloc");
    freep = ( void (*)(void*) ) dlsym(RTLD_NEXT, "free");

    PrintMessage("Function pointer init done.\n");

    nvm_init();

    trace_flag = false;
    inited = true;
    PrintMessage("Init End\n");
}


static void __attribute__ ((destructor)) final()
{
    nvm_exit();
}

////////////////////////////////////////////////////////////////////////////////
/* Malloc*/
/////////////////////////////////////////////////////////////////////////////////////////

void* malloc(size_t size)
{
    void *ptr;

    PrintMessage("Trace one malloc...\n");

    if (!trace_flag && size) {
        trace_flag = true;

        ptr = nvm_malloc(size);
        PrintMessage("On the ssd: %x...\n", ptr);

        trace_flag = false;
    } else {
        ptr = mallocp(size);
    }
    
    PrintMessage("Trace malloc end...\n");
    return ptr;
}

//////////////////////////////////////////////////////////////////////////////////////////
/* Calloc*/
/////////////////////////////////////////////////////////////////////////////////////////
void* calloc(size_t nmemb, size_t size)
{
    void *ptr;
    PrintMessage("Trace one calloc...\n");

    if (inited == false) {
        if (nmemb * size > 50000000)
            perror("calloc size before init is not enough\n");
        return (void*) tmp_array;
    }

    if (!trace_flag && size) {
        trace_flag = true;

        ptr = nvm_calloc(nmemb, size);
        PrintMessage("On the ssd: %x...\n", ptr);

        trace_flag = false;
    } else {
        ptr = callocp(nmemb, size);
    }
    
    return ptr;
}

//////////////////////////////////////////////////////////////////////////////////////////
/* Realloc */
/////////////////////////////////////////////////////////////////////////////////////////

void* realloc(void *var, size_t size)
{
    void *ptr;
    PrintMessage("Trace one realloc...\n");

    if (!trace_flag && size) {
        trace_flag = true;
        
        nvm_realloc(var, size);
        PrintMessage("On the ssd: %x...\n", ptr);

        trace_flag = false;
    } else {
        ptr = realloc(var, size);
    }

    return ptr;
}

void free(void* ptr)
{
    int flag = 0;
    if (ptr == tmp_array) return;

    if (!trace_flag && ptr) {
        trace_flag = true;

        flag = nvm_free(ptr);
        if (flag == 0) free(ptr);
        PrintMessage("Free: %x...\n", ptr);

        trace_flag = false;
    }
}
