#include "calltrack.h"
#include <execinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

void show_stackframe(void) {
    void *trace[16];
    char **messages = (char **)NULL;
    int i, trace_size = 0;
    
    trace_size = backtrace(trace, 16);
    messages = backtrace_symbols(trace, trace_size);
    printf("[Call] Execution path:\n");
    for (i=0; i<trace_size; ++i)
        printf("[Call] %s\n", messages[i]);
}

double mysecond()
{
    struct timeval tp;
    struct timezone tzp;
    int i;
    
    i = gettimeofday(&tp,&tzp);
    return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

#undef malloc
void *calltrack_malloc(size_t size)
{
    void *addr = malloc(size);
	
    double times = mysecond();
    printf("[Call] malloc:%p:%llu:%f\n", addr, size, times);
    show_stackframe();
    return (void *) (addr);
}
#define malloc calltrack_malloc

#undef calloc
void *calltrack_calloc(size_t nmemb, size_t size)
{
    void *addr = calloc(nmemb, size);
    double times = mysecond();
    printf("[Call] calloc:%p:%llu:%f\n", addr, nmemb*size, times);
    show_stackframe();
    return (void *) (addr);
}
#define calloc calltrack_calloc

#undef malloc 
#undef realloc
void *calltrack_realloc(void *var, size_t size)
{
    void *addr = realloc(var, size);
	
    double times = mysecond();
    printf("[Call] realloc:%p:%p:%llu:%f\n", var, addr, size, times);
    show_stackframe();
    return (void *) (addr);
}
#define realloc calltrack_realloc
#define malloc calltrack_malloc


#undef free
void calltrack_free(void *ptr)
{
    double times = mysecond();
    printf("[Call] free:%p:%f\n", ptr, times);
    show_stackframe();
    free (ptr);
    return;
}
#define free calltrack_free
