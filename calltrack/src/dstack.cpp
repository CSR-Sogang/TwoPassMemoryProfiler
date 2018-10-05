#ifndef _DCALLTRACK_H
#define _DCALLTRACK_H

//#define _GNU_SOURCE
#define _TRACE

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <map>

static void* (*callocp) (size_t, size_t) = NULL;
static void* (*mallocp) (size_t) = NULL;
static void* (*reallocp) (void*, size_t) = NULL;
static void (*freep) (void*) = NULL;
//static void* (*mmapp) (void*, size_t, int, int, int, off_t) = NULL;
std::map<size_t, size_t> allocmap;

static bool malloc_flag = true;
static bool calloc_flag = true;
static bool realloc_flag = true;
static bool free_flag = true;
static bool trace_flag = true;

static int64_t footprint = 0;
static int64_t max_stack_size = 0;
static int64_t total_size = 0;
static int64_t vm_data_size = 0;
static int init_flag = 0;
//static int64_t max_footrpi

int64_t pstack = 0;
int64_t pfootprint = 0;
double cstack = 0;
double cfootprint = 0;

const int TMP_ARRAY_SIZE = 1024 * 1024 * 20;
char tmp_array[TMP_ARRAY_SIZE]; // for the bug with the pthread 
double start_time = 0.0;
double last_time = 0.0;

double mysecond()
{
    struct timeval tp;
    struct timezone tzp;
    int i;
    
    i = gettimeofday(&tp,&tzp);
    return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

int64_t get_mem(int flag){
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
    fclose(status);
    if (flag == 1)
        return vm_data;
    if (flag == 2)
        return vm_stk;

}

int64_t get_stack_rss() { 
    char line[200];
    FILE* status = fopen("/proc/self/smaps", "r");
    
    int rss_data;
    int flag = 0;
    while (fscanf(status, "%s", line) != EOF) {
        //printf("%s !!!\n", line);
        if (flag == 2) {
            rss_data = atoi(line);
            fclose(status);
            //printf("%lf %d %ld\n", mysecond(), rss_data, footprint / 1024);
            //printf("%lf %lf\n", cfootprint, cstack);
            double time_now = mysecond();
            cfootprint += (footprint / 1024) * ( time_now - last_time);
            cstack += rss_data * (time_now - last_time);
            pfootprint = (pfootprint > footprint / 1024) ? pfootprint : footprint / 1024;
            pstack = (pstack > rss_data) ? pstack : rss_data;
            last_time = time_now;
            return rss_data;
        }
        if (strcmp("[stack]", line) == 0)
            flag = 1;
        if (flag == 1 && ( strcmp("Rss:", line) == 0 ) )
            flag = 2;  
    }
    fclose(status);

}

/*
int64_t get_mem() {
    return 0;
}
*/
static void __attribute__ ((constructor)) init() 
{
    
    mallocp = ( void* (*)(size_t)) dlsym(RTLD_NEXT, "malloc");
    callocp = ( void* (*)(size_t, size_t)) dlsym(RTLD_NEXT, "calloc");
    reallocp = ( void* (*)(void*, size_t)) dlsym(RTLD_NEXT, "realloc");
    freep = ( void (*)(void*) ) dlsym(RTLD_NEXT, "free");
    //mmapp = ( void* (*)(void*, size_t, int, int, int, off_t) ) dlsym(RTLD_NEXT, "mmap");
    total_size = 0;
    //get_stack_rss();
    last_time = mysecond();

    malloc_flag = false;
    calloc_flag = false;
    realloc_flag = false;
    free_flag = false;
    trace_flag = false;

}

static void __attribute__ ((destructor)) final()
{
    //printf("vm data: %ld\n", vm_data_size - TMP_ARRAY_SIZE / 1024 );
    //printf("stack size: %ld\n", max_stack_size);
    //printf("total data size: %ld\n", vm_data_size + max_stack_size - TMP_ARRAY_SIZE / 1024 );
    printf("peak heap vs peak stack %lf\n", pfootprint * 1.0 / pstack);
    printf("avg heap vs avg stack %lf\n", cfootprint * 1.0 / cstack);
}

void* malloc(size_t size) 
{
    int64_t stack_size;
    //printf("Here\n");
    //printf("Malloc size:%d\n", size);
    total_size += size;
    //__sync_fetch_and_add(&total_size, size);
    
    if (init_flag == 0) {
        init_flag = 1;
        vm_data_size = get_mem(1);
    }

    void* ptr = mallocp(size);
    if (!trace_flag) {
        trace_flag = true;
        footprint += size;
        allocmap[(size_t)ptr] = size;
        stack_size = get_mem(2);
        get_stack_rss();
        //printf ("%ld %ld\n", get_mem(), footprint / 1024);
        if (stack_size > max_stack_size) 
            max_stack_size = stack_size;
        
        trace_flag = false;
    }

    return ptr;
}

void* calloc(size_t nmemb, size_t size)
{
    //printf("1235\n");
    //printf("calloc Size:%d\n", nmemb * size);
    if (callocp == NULL) 
        return tmp_array;
    
    int64_t stack_size;
    void* ptr = callocp(nmemb, size);
    if (!trace_flag) {
        trace_flag = true;
        footprint += size * nmemb;
	allocmap[(size_t)ptr] = size * nmemb;
	stack_size = get_mem(2);
    get_stack_rss();
	if (stack_size > max_stack_size) 
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
    //printf("realloc: %d\n", size);
    //printf("flag: %d\n", trace_flag);
    int64_t stack_size;
    void* ptr = reallocp(var, size);
    if (!trace_flag) {
        trace_flag = true;
       
        if (allocmap.find((size_t)ptr) != allocmap.end()) {
            footprint -= allocmap[(size_t)var];
            if (footprint < 0) footprint = 0;
            allocmap[(size_t)var] = 0;
            allocmap[(size_t)ptr] = size;
        }

        footprint += size;
        get_stack_rss();
        stack_size = get_mem(2);
        if (stack_size > max_stack_size) 
            max_stack_size = stack_size;
        
        trace_flag = false;
    }

    return ptr;
}

void free(void *ptr)
{
    //printf("Free!\n");
    if (ptr == tmp_array)
        return;
    freep(ptr);
    if (!trace_flag) { 
        trace_flag = true;
        if (allocmap.find((size_t)ptr) != allocmap.end()) {
            footprint -= allocmap[(size_t)ptr];
            if (footprint < 0) footprint = 0;
            allocmap[(size_t)ptr] = 0;
           
        }
        trace_flag = false;
    }
    //printf("Free end!\n");
}

#endif
