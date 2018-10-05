#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <mutex>
#include <sys/syscall.h>
#include <inttypes.h>

#include "dcalltrack.h"

#define MAX_THREADS 65536

static bool malloc_flag = true;
static bool calloc_flag = true;
static bool realloc_flag = true;
static bool free_flag = true;
static bool trace_flag = true;
static bool in_init = false;

static FILE* trace_file_fds[MAX_THREADS];
static uint64_t obj_counts[MAX_THREADS];
static bool t_trace_flags[MAX_THREADS];
static uint64_t t_malloc_count[MAX_THREADS];
static uint64_t t_calloc_count[MAX_THREADS];
static uint64_t t_relloc_count[MAX_THREADS];

static void* (*callocp) (size_t, size_t) = NULL;
static void* (*mallocp) (size_t) = NULL;
static void* (*reallocp)(void*, size_t) = NULL;
static void (*freep)( void* ) = NULL;

char tmp_array[500000000]; // for the bug with the pthread program

std::mutex g_lock;

inline void InitThreadInfo(int tid) {
    char trace_file_name[128];
    if (trace_file_fds[tid] != NULL) 
        return;
    sprintf(trace_file_name, "var_size_file_tid_%d", tid);

    trace_file_fds[tid] = fopen(trace_file_name, "w");

    if (trace_file_fds[tid] == NULL)
        perror("[Error] trace file open error\n");

    obj_counts[tid] = 0;
}

static inline int ShowStackframeMT(size_t addr, size_t size)
{
    
    unsigned int max_frames = 63;
    FILE *out = trace_file;
    int return_value = 1;
    
    char calltrack[MAX_STRING_LENGTH] = "";
    
    g_lock.lock();
    //printf("get the lock\n");

#ifdef READ_VAR_HASH_VALUE
    return_value = 0;
#endif
  
    // storage array for stack trace address data
    void* addrlist[max_frames+1];

    // retrieve current stack addresses
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

    if (addrlen == 0) {
        fprintf(out, "  <empty, possibly corrupt>\n");
        g_lock.unlock();
        return 0;
    }

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char** symbollist = backtrace_symbols(addrlist, addrlen);
    
    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 2048;
    char* funcname = (char*) malloc(funcnamesize);

    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.

    for (int i = 2; i < addrlen; i++)
    {
        char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

        // find parentheses and +address offset surrounding the mangled name:
        // ./module(function+0x15c) [0x8048a6d]
        for (char *p = symbollist[i]; *p; ++p)
        {
            if (*p == '(')
                begin_name = p;
            else if (*p == '+')
                begin_offset = p;
            else if (*p == ')' && begin_offset) {
                end_offset = p;
                break;
            }
        }
    
        if (begin_name && begin_offset && end_offset
            && begin_name < begin_offset)
        {
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';

            /* For the hash table string */
            strcat(calltrack, begin_name);
            strcat(calltrack, begin_offset);
            //calltrack_strlen += strlen(begin_name) + strlen(begin_offset);
            //if (calltrack_strlen >= MAX_STRING_LENGTH) 
            //    printf("Error: calltrack string too long!\n");

            // ignore the glibc variable
            
            *(--begin_name) = '(';
            *(--begin_offset) = '+';
            *end_offset = ')';
            if (strstr(symbollist[i], "libc.so.6") != NULL && i == 2) {
                free(funcname);
                free(symbollist);
                g_lock.unlock();              
                return 0;
            }
        } else {
            strcat(calltrack, symbollist[i]);
        }
    }

#ifdef READ_VAR_HASH_VALUE
    if (LookIntoHashValue(calltrack, addr, size) == -1) {
        free(funcname);
        free(symbollist);
        g_lock.unlock();
        return return_value;
    } 
#endif

#ifdef IDENT_TRANSFER
    free(funcname);
    free(symbollist);
    g_lock.unlock();
    return return_value;
#endif

#ifdef GET_TIME_INFO
    free(funcname);
    free(symbollist);
    g_lock.unlock();
    return return_value;
#endif
    return_value = LookHashTable(calltrack);

#ifdef SHOW_SIZE
    free(funcname);
    free(symbollist);
    g_lock.unlock();
    return return_value;
#endif   

#ifdef READ_VAR_ID
    if ( LookVarID(return_value) == 0) {
        free(funcname);
        free(symbollist);
        g_lock.unlock();
        return return_value;
    }
#endif

    //return 1; // temporay return 1;
   
    fprintf(out, "[Stack trace:]\n");
    
    for (int i = 1; i < addrlen; i++)
    {
        char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

        fprintf(out, "[Call]");

        // find parentheses and +address offset surrounding the mangled name:
        // ./module(function+0x15c) [0x8048a6d]
        for (char *p = symbollist[i]; *p; ++p)
        {
            
            if (*p == '(')
                begin_name = p;
            else if (*p == '+')
                begin_offset = p;
            else if (*p == ')' && begin_offset) {
                end_offset = p;
                break;
            }
        }
    
        if (begin_name && begin_offset && end_offset
            && begin_name < begin_offset)
        {
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';

            // mangled name is now in [begin_name, begin_offset) and caller
            // offset in [begin_offset, end_offset). now apply
            // __cxa_demangle():
            
            int status;
            char* ret = abi::__cxa_demangle(begin_name,
                                            funcname, &funcnamesize, &status);

            if (status == 0) {
                funcname = ret; // use possibly realloc()-ed string

                fprintf(out, "%s : %s+%s\n",
                        symbollist[i], funcname, begin_offset);
            }
            else {
                // demangling failed. Output function name as a C function with
                // no arguments.

                fprintf(out, "%s : %s()+%s\n",
                        symbollist[i], begin_name, begin_offset);
            }
        }
        else
        {
            // couldn't parse the line? print the whole line.
            fprintf(out, " %s\n", symbollist[i]);
        }
    }
    
    fprintf(out, "[Hash] %lu\n", StringHash(calltrack));

    free(funcname);
    free(symbollist);
    //printf("free the lock\n");
    g_lock.unlock();

    return return_value;
}


static void __attribute__ ((constructor)) init() 
{
    double times;
    int my_tid = syscall(__NR_gettid);
    //memset(t_trace_flags, 1, sizeof(t_trace_flags));
    InitHashTable();

    for (int i = 0; i < MAX_THREADS; i++)
        t_trace_flags[i] = false;

    trace_flag = true;
    //printf("init start\n");
    
    mallocp = ( void* (*)(size_t)) dlsym(RTLD_NEXT, "malloc");
    callocp = ( void* (*)(size_t, size_t)) dlsym(RTLD_NEXT, "calloc");
    reallocp = ( void* (*)(void*, size_t)) dlsym(RTLD_NEXT, "realloc");
    freep = ( void (*)(void*) ) dlsym(RTLD_NEXT, "free");
    
    
    //    printf("hash done\n");

#ifdef READ_MALLOC_SIZE_FILE
    ReadMallocSize();
#endif

#ifdef READ_VAR_HASH_VALUE
    ReadHashValues();
#endif

#ifdef READ_VAR_ID
    InitVarIDTable();
#endif

#ifdef MPI
    char trace_file_name[64];
    char var_outfile_name[64];
    int id = getpid();
    sprintf(trace_file_name, "var_time_info_%d", id);
    sprintf(var_outfile_name, "variable_out-time_%d", id);
    trace_file = fopen(trace_file_name, "w");
    var_outfile = fopen(var_outfile_name, "w");
#else
    //sprintf(trace_file_name, "var_time_info_%d", m
    trace_file = fopen("summary-output-file", "w");              
    //printf("my tid is %d\n", my_tid);
#endif

    if ( trace_file == NULL ) {
		perror("Error in open file!\n");
    }
	times = mysecond();
#ifdef GET_TIME_INFO
    program_start_time = times;
#else 
	fprintf(trace_file, "[Start] time:%f\n", times);
#endif


    for (int i = 0; i < TRACE_COUNT; i++) 
        trace_count_array[i] = 0;

    /* init the message queue */
#ifdef IDENT_TRANSFER
    write_addr_init();
#endif

#ifdef GET_TIME_INFO
    var_time_init();
#endif

#ifdef _TRACE
    trace_flag = false;
#endif
}

inline void DumpAllHashValues() { 
    for (int i = 0; i < MAX_HASH_SIZE; i++)
        if (calltrack_hash_table[i].exist) 
            fprintf(trace_file, "id: %d Hash: %d\n", 
            calltrack_hash_table[i].id, calltrack_hash_table[i].hash_value);
}

static void __attribute__ ((destructor)) final()
{
	double times = mysecond();
    size_t total_size = 0;
    size_t cul_size = 0;
    
    trace_flag = true;
   
    //printf("final start\n");

    fprintf(trace_file, "[End] time:%f\n", times);

    DumpAllHashValues();

    fclose(trace_file);
    //    fclose(var_outfile);
     
}

//////////
////////////////////////////////////////////////////////////////////////////////
/* Malloc*/
/////////////////////////////////////////////////////////////////////////////////////////

void* malloc(size_t size)
{
    void* ptr;
    double times;
    int var_id;
    int my_tid = syscall(__NR_gettid);
    //printf("in malloc\n");
    
    if (mallocp == NULL) {
        init();
        printf("mallocp not exsit!\n");
        //perror("mallocp not exsit!\n");
    }
   
    ptr = mallocp(size);

    if (ptr == 0) perror("Malloc return NULL!\n");
   
	if (!t_trace_flags[my_tid] && !trace_flag && size > CALLTRACK_SIZE) {
        t_trace_flags[my_tid] = true;
        
        //printf("in malloc here\n");
        times = mysecond();

        InitThreadInfo(my_tid);
        //printf("my tid: %d\n", my_tid);
        if ((var_id = ShowStackframeMT((size_t)ptr, size))) {

#ifdef SHOW_SIZE
            fprintf(trace_file_fds[my_tid], 
                   "[Trace] malloc:%d my_tid:%d var_id:%d ptr:%p size:%lu times:%f\n", 
                   ++t_malloc_count[my_tid], my_tid, var_id, ptr, size, times);
#endif
            obj_counts[my_tid]++;
        }
        
        t_trace_flags[my_tid] = false;
    }
    //printf("malloc end.\n");
    return ptr;
}

//////////////////////////////////////////////////////////////////////////////////////////
/* Calloc*/
/////////////////////////////////////////////////////////////////////////////////////////
void* calloc(size_t nmemb, size_t size)
{
    void* ptr;
    double times;
    int var_id; 
    int my_tid = syscall(__NR_gettid);
    //printf("in calloc\n");

    if (callocp == NULL && !in_init) {
        in_init = true;
        init();
        in_init = false;
    }

    if (callocp == NULL && in_init) {
        if ( nmemb * size > 50000000 )
            perror("Calloc Size not enough!\n");
        
        return (void*) tmp_array;
    }
    else 
        ptr = callocp(nmemb, size);


    if (!t_trace_flags[my_tid] && nmemb * size > CALLTRACK_SIZE) {
        t_trace_flags[my_tid] = true;
        times = mysecond();
        InitThreadInfo(my_tid);
        //printf("my tid: %d\n", my_tid);
        if ((var_id = ShowStackframeMT((size_t)ptr, nmemb * size))) {

#ifdef SHOW_SIZE
            fprintf(trace_file_fds[my_tid], 
                    "[Trace] calloc:%d tid:%d var_id:%d ptr:%p size:%lu times:%f\n", 
                    ++t_calloc_count[my_tid], my_tid, var_id, ptr, size * nmemb, times);
#endif

            obj_counts[my_tid]++;
        }
        
        t_trace_flags[my_tid] = false;
    }
    //printf("calloc end.\n");
    return ptr;
}

void* realloc(void *var, size_t size)
{
    void* ptr;
    double times;
    
    int var_id;
    int my_tid = syscall(__NR_gettid);
    //printf("in realloc\n");

    ptr = reallocp(var, size);
    
    if (!t_trace_flags[my_tid] && !trace_flag && size > CALLTRACK_SIZE) {
        t_trace_flags[my_tid] = true;
        
        InitThreadInfo(my_tid);
    
        times = mysecond();
        
        if ((var_id = ShowStackframeMT((size_t)ptr, size))) {
            
#ifdef SHOW_SIZE
            fprintf(trace_file_fds[my_tid],
                    "[Trace] realloc:%d var_id:%d ptr:%p before:%p size:%lu times:%f\n",
                    ++realloc_count, var_id, ptr, var, size, times);
#endif
            obj_counts[my_tid]++;
        }

        t_trace_flags[my_tid] = false;
    }
     
    return ptr;
}


int find_ptr(size_t ptr) {
    for ( int i = 0; i < var_time_count; i++) 
        if (ptr == var_ptr_for_time[i]) {
            var_ptr_for_time[i] = 0;
            return i;
        }
    return -1;
}

void free(void* ptr)
{
    int my_tid = syscall(__NR_gettid);
    //int my_tid = 1;
    //printf("in free??\n");
    
    //if(t_trace_flags[my_tid] == true) printf("yes\n");
    //else printf("no\n");
   
    //printf("my tid:%d, ptr:%p\n", my_tid);
    
    //printf("trace_flag %d\n", t_trace_flags[my_tid]);

	if (ptr == tmp_array) 
		return;

    //show_stackframe((size_t) ptr, 0);

    double times;

    if (!t_trace_flags[my_tid] && !trace_flag && ptr) { 
        t_trace_flags[my_tid] = true;
        
        times = mysecond();
#ifdef GET_TIME_INFO
        int id = -1;
        if ( (size_t) ptr != 0 && (id = find_ptr((size_t)ptr)) != -1 )
            variable_end_time[id] = times;
#endif

#ifdef SHOW_FREE
        fprintf(trace_file_fds[my_tid], "[Trace] free ptr:%p times:%f\n", ptr, times);
        //show_stackframe1();
#endif
        
        t_trace_flags[my_tid] = false;
    }
    //printf("freed\n");
    freep(ptr);

}

