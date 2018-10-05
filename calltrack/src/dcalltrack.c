#ifndef _DCALLTRACK_H
#define _DCALLTRACK_H

#define _GNU_SOURCE
#define _TRACE

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
#include <stirng.h>
#include <map>

static void* (*callocp) (size_t, size_t) = NULL;
static void* (*mallocp) (size_t) = NULL;
static void* (*reallocp) (void*, size_t) = NULL;
static void (*freep) (void*) = NULL;
static void* (*mmapp) (void*, size_t, int, int, int, off_t) = NULL;
set::map<size_t, size_t> allocmap;

static bool malloc_flag = true;
static bool calloc_flag = true;
static bool realloc_flag = true;
static bool free_flag = true;
static bool trace_flag = true;

static long long footprint = 0;
static long long max_stack_size = 0;
static long long total_size = 0;
//char tmp_array[500000000]; // for the bug with the pthread 

void getmem(){
    char line[200];
    FILE* status = fopen( "/proc/self/status", "r" );
    //while( fgets(line, 200, status) != NULL) {
    //  printf("%s\n", line);
    //}
    int vm_data, vm_stk;
    int flag_data = 0, flag_stk = 0;
    while (fscanf(status, "%s", line) != EOF) { 
        if (flag_data == 1) {
            vm_data = atoi(line);
            flag_data = 0;
    
        }
        if (flag_stk == 1) {
            vm_stk = atoi(line);
            flag_stk = 0;
    
        }
        if (strcmp(line, "VmData:") == 0) {
            flag_data = 1;
        }
        if (strcmp(line, "VmStk:") == 0) {
            flag_stk = 1;
        }
    }
}

static void __attribute__ ((constructor)) init() 
{
    //printf("here1\n");
    mallocp = ( void* (*)(size_t)) dlsym(RTLD_NEXT, "malloc");
    callocp = ( void* (*)(size_t, size_t)) dlsym(RTLD_NEXT, "calloc");
    reallocp = ( void* (*)(void*, size_t)) dlsym(RTLD_NEXT, "realloc");
    freep = ( void (*)(void*) ) dlsym(RTLD_NEXT, "free");
    //mmapp = ( void* (*)(void*, size_t, int, int, int, off_t) ) dlsym(RTLD_NEXT, "mmap");
    total_size = 0;
}

static void __attribute__ ((destructor)) final()
{
    printf("stack size: %lld\n", stack_size);
}

void* malloc(size_t size) 
{
    total_size += size;
    //__sync_fetch_and_add(&total_size, size);
    void* ptr = mallocp(size);
    if (!trace_flag) {
        trace_flag = true;
        footprint += size;
	allocmap[(size_t)ptr] = size;
	stack_size = footprint - get_mem();
	if (stack_size < max_stack_size) 
	    max_stack_size = stack_size;
	trace_flag = false;
    }

    return ptr;
}

void* calloc(size_t nmemb, size_t size)
{
    if (callocp == NULL) {
        return (void*) tmp_array;
    }
    total_size += size * nmemb;
    //__sync_fetch_and_add(&total_size, size * nmemb);
    void* ptr = callocp(nmemb, size);
    if (!trace_flag) {
        trace_flag = true;
        footprint += size * nmemb;
	allocmap[(size_t)ptr] = size * nmemb;
	stack_size = footprint - get_mem();
	if (stack_size < max_stack_size) 
	    max_stack_size = stack_size;
	trace_flag = false;
    }

    return ptr;
}

/*void* mmap(void *addr, size_t length, int prot, int flags,
           int fd, off_t offset)
{
    __sync_fetch_and_add(&total_size, length);
    void *ptr = mmapp(addr, length, prot, flags, fd, offset);
    return ptr;
    }*/

void* realloc(void *var, size_t size)
{
    total_size += size;
    void* ptr = realloc(var, size);
    if (!trace_flag) {
        trace_flag = true;
	
	if (allocmap.find((size_t)ptr) != allocmap.end()) {
	    footprint -= allocmap[(size_t)ptr];
	    if (footprint < 0) footprint = 0;
	    allocmap[(size_t)var] = 0;
	    allocMap[(size_t)ptr] = size;
	}

        footprint += size;
	stack_size = footprint - get_mem();
	if (stack_size < max_stack_size) 
	    max_stack_size = stack_size;
	trace_flag = false;
    }

    return ptr;
}

void free(void *ptr)
{
  if (allocmap.find((size_t)ptr) != allocmap.end()) {
      footprint -= allocmap[(size_t)ptr];
      if (footprint < 0) footprint = 0;
      allocmap[(size_t)ptr] = 0;
  }
}

#endif
