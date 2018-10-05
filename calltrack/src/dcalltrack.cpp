#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <execinfo.h>

#include "dcalltrack.h"

static bool trace_flag = true;
static bool in_init = false;

static int count = 0;

static void* (*callocp) (size_t, size_t) = NULL;
static void* (*mallocp) (size_t) = NULL;
static void* (*reallocp)(void*, size_t) = NULL;
static void (*freep)( void* ) = NULL;

char tmp_array[500]; // for the bug with the pthread program

static void __attribute__ ((constructor)) init() 
{
    double times;

    //printf("program start\n");

    //trace_flag = true;
    mallocp = ( void* (*)(size_t)) dlsym(RTLD_NEXT, "malloc");
    callocp = ( void* (*)(size_t, size_t)) dlsym(RTLD_NEXT, "calloc");
    reallocp = ( void* (*)(void*, size_t)) dlsym(RTLD_NEXT, "realloc");
    freep = ( void (*)(void*) ) dlsym(RTLD_NEXT, "free");
    
    InitHashTable();
    
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
    //trace_file = fopen("var_size_file", "w");
    //trace_file = fopen("top10-calltrack-new", "w");
    //trace_file = fopen("test-output", "w");
    //var_outfile = fopen("variable_out_test1", "w");
    trace_file = fopen("var_time_info", "w");
    var_outfile = fopen("variable_out-all", "w");
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

    int i;
    for (i = 0; i < TRACE_COUNT; i++) 
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

int cmp(size_t a, size_t b) {
    return a > b ? 1 : 0;
}

static void __attribute__ ((destructor)) final()
{
	  double times = mysecond();
    size_t total_size = 0;
    size_t cul_size = 0;

    trace_flag = true;
    
#ifdef GET_TIME_INFO

    for (int i = 0; i < var_time_count; i++) 
        if (var_ptr_for_time[i] != 0)
            variable_end_time[i] = times;
    fprintf(trace_file, "Start_time\n");
    for (int i = 0; i < var_time_count; i++)
        fprintf(trace_file, "%d\t%f\n", i, variable_start_time[i]);

    fprintf(trace_file, "\nEnd_time\n");
    for (int i = 0; i < var_time_count; i++)
        fprintf(trace_file, "%d\t%f\n", i, variable_end_time[i]);

    fprintf(trace_file, "\nNormalized_time\n");
    for (int i = 0; i < var_time_count; i++)
        fprintf(trace_file, "%d\t%f\n", i, (variable_end_time[i] - variable_start_time[i]) / (times - program_start_time));
    
    fprintf(trace_file, "\n");
                
#else
    fprintf(trace_file, "[Summary] Total var count: \t%lu\n", var_count);
    fprintf(trace_file, "[Summary] Total obj count: \t%d\n", total_count);
    fprintf(trace_file, "[End] time:%f\n", times);
#endif
  #ifdef DEBUG_EXTRA
    printf("count:%d\n", count);
  #endif

#ifdef SUMMARY

    fprintf(trace_file, "[Summary] %d th objects size: %lu\n", 
            TRACE_COUNT, get_kth_count());
    /*
    
    //fclose(trace_file);
    */

    // for get the percentage of the size of variables
    std::sort(size_array, size_array + total_count, cmp);
    //for (int i = 0; i < total_count; i++)
    //    total_size += size_array[i];
    fprintf(trace_file, "[Summary] Min:\t%lu\n", size_array[total_count - 1]);
    fprintf(trace_file, "[Summary] Max:\t%lu\n", size_array[0]);
    fprintf(trace_file, "[Summary] Med:\t%lu\n", size_array[total_count / 2]);
    fprintf(trace_file, "[Summary] Total obj size: \t%lu\n", total_obj_size);

    fprintf(trace_file, "[Summary] MaxCount:\t%lu\n", max_obj_count);
    fprintf(trace_file, "[Summary] Footprint:\t%lu\n", max_footprint);
#endif
  //printf("Here1!!\n");
    fclose(trace_file);
    fclose(var_outfile);
  //printf("Here2!!\n");

#ifdef PER_VAR_SIZE
    OutputVarKthSize();
#endif
  //printf("Here3!!\n");

}

int find_in_unfree_list(long int ptr) 
{
	int i;
	if (!ptr) return 0;

	for(i = 0; i < unfree_count; i++ ) 
		if ( ptr == unfree_list[i] ) {
			unfree_list[i] = 0;
			return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
/* Malloc*/
/////////////////////////////////////////////////////////////////////////////////////////

void* malloc(size_t size)
{
  void* ptr;
  double times;
  int var_id;
    
  //printf("Malloc is called!\n");
  if (mallocp == NULL) { 
    init();
    printf("mallocp not exsit! re-init\n");
    //perror("mallocp not exsit!\n");
  }

  ptr = mallocp(size);

  count++;
  
  if (ptr == 0) perror("Malloc return NULL!\n");
   
	if (!trace_flag && size > CALLTRACK_SIZE) {
    trace_flag = true;
        
    times = mysecond();

    if ((var_id = show_stackframe((size_t)ptr, size))) {
#ifdef SHOW_SIZE
      fprintf(trace_file, "[Trace] malloc:%d var_id:%d ptr:%p size:%lu times:%f\n", ++malloc_count, var_id, ptr, size, times);
#endif

#ifdef BLAST
      printf("[Trace] malloc:%d var_id:%d ptr:%p size:%lu times:%f\n", ++malloc_count, var_id, ptr, size, times);
#endif      
#ifdef SUMMARY
        // for get the footprint of the program
      footprint += size;
      obj_count ++;
      if (footprint < 0 )
        footprint = 0;
      if (footprint > max_footprint) 
        max_footprint = footprint;
      if (obj_count > max_obj_count)
        max_obj_count = obj_count;

      //allocmap[(size_t)ptr] = size;
      total_obj_size += size;
      
      size_array[total_count] = size;

      if (size > BIG_SIZE) big_count++;
      if (size > MID_SIZE) mid_count++;
      if (size > _32K_SIZE) _32k_count++;
      if (size > _128K_SIZE) _128k_count++;
      if (size > max_size) max_size = size;
      if (size > MIN_TRACE_SIZE) update_kth_count(size);
#endif

      total_count++;
    }
      
    trace_flag = false;
  }
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
    count ++;
    
  //printf("calloc is called!\n");
    if (callocp == NULL && !in_init) {
        in_init = true;
        
        init();
        in_init = false;
        //printf("callocp not exsit!\n");
        //perror("mallocp not exsit!\n");
    }
    if (callocp == NULL && in_init) {
        if ( nmemb * size > 50000000 )
            perror("Calloc Size not enough!\n");
        //init();
        
        return (void*) tmp_array;
    }
    else 
        ptr = callocp(nmemb, size);
   
    if (!trace_flag && nmemb * size > CALLTRACK_SIZE) {
        trace_flag = true;
        times = mysecond();
        
        if ((var_id = show_stackframe((size_t)ptr, nmemb * size))) {
#ifdef SHOW_SIZE
            fprintf(trace_file, "[Trace] calloc:%d var_id:%d ptr:%p size:%lu times:%f\n",++calloc_count, var_id, ptr, size * nmemb, times);
#endif

#ifdef BLAST
            printf("[Trace] calloc:%d var_id:%d ptr:%p size:%lu times:%f\n",++calloc_count, var_id, ptr, size * nmemb, times);
#endif      

#ifdef SUMMARY
            // for get the footprint of the program
            footprint += nmemb * size;
            obj_count ++;
            if (footprint > max_footprint) 
                max_footprint = footprint;
            if (obj_count > max_obj_count)
                max_obj_count = obj_count;
            //allocmap[(size_t)ptr] = nmemb * size;
            //allocmap.insert(std::map<size_t, size_t>::value_type((size_t)ptr, nmemb * size));
            total_obj_size += nmemb * size;

            size_array[total_count] = nmemb * size;

            if (nmemb * size > BIG_SIZE) big_count++;
            if (nmemb * size > _32K_SIZE) _32k_count++;
            if (nmemb * size > _128K_SIZE) _128k_count++;
            if (nmemb * size > MID_SIZE) mid_count++;
            if (nmemb * size > max_size) max_size = nmemb * size;
            if (nmemb * size > MIN_TRACE_SIZE) update_kth_count(nmemb * size);
#endif
            total_count++;
        }
        
        trace_flag = false;
    }
     
    return ptr;
}

void* realloc(void *var, size_t size)
{
    void* ptr;
    double times;
    int unfree_flag = find_in_unfree_list((long int) var);
    int var_id;

    ptr = reallocp(var, size);
    
    if (!trace_flag && (!unfree_flag) && size > CALLTRACK_SIZE) {
        trace_flag = true;
        
        times = mysecond();
        if ((var_id = show_stackframe((size_t)ptr, size))) {
            
#ifdef SHOW_SIZE
            fprintf(trace_file, "[Trace] realloc:%d var_id:%d ptr:%p before:%p size:%lu times:%f\n",++realloc_count, var_id, ptr, var, size, times);
#endif

#ifdef BLAST
            printf("[Trace] realloc:%d var_id:%d ptr:%p before:%p size:%lu times:%f\n",++realloc_count, var_id, ptr, var, size, times);
#endif      
      
#ifdef SUMMARY
            // for get the footprint of the program
            //if (allocmap.find(size_t(var)) == allocmap.end()) 
            //    obj_count ++;
            //else 
            //    footprint -= allocmap[(size_t)var];
            //if (footprint < 0) footprint = 0;
            //footprint += size;

            if (footprint > max_footprint) 
                max_footprint = footprint;
            //allocmap[(size_t)var] = 0;
            //allocmap[(size_t)ptr] = size;
            total_obj_size += size;

            size_array[total_count] = size;

            if (size > BIG_SIZE) big_count++;
            if (size > _32K_SIZE) _32k_count++;
            if (size > _128K_SIZE) _128k_count++;
            if (size > MID_SIZE) mid_count++;
            if (size > max_size) max_size = size;
            if (size > MIN_TRACE_SIZE) update_kth_count(size);
#endif
            total_count++;
        }

        
        trace_flag = false;
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
        
/* This function doesn't change the flag in the unfree_list*/
int get_flag_unfree_list(long int ptr) 
{
	int i;
	if (!ptr) return 0;

	for(i = 0; i < unfree_count; i++ ) 
		if ( ptr == unfree_list[i] ) {
			return 1;
	}
	return 0;
}

void free(void* ptr)
{
	if (ptr == tmp_array) 
		return;

    //show_stackframe((size_t) ptr, 0);

    double times;
	int unfree_flag = find_in_unfree_list((long int) ptr);

    if (!trace_flag && ptr && (!unfree_flag)) { 
        trace_flag = true;
        
        times = mysecond();
#ifdef GET_TIME_INFO
        int id = -1;
        if ( (size_t) ptr != 0 && (id = find_ptr((size_t)ptr)) != -1 )
            variable_end_time[id] = times;
#endif

#ifdef SHOW_FREE
        fprintf(trace_file, "[Trace] free ptr:%p times:%f\n", ptr, times);
        //show_stackframe1();
#endif

#ifdef BLAST
        //printf("[Trace] free ptr:%p times:%f\n", ptr, times);
        //show_stackframe1();
#endif

        
#ifdef SUMMARY
        // for get the footprint of the program
        /*
        if (allocmap.find((size_t)ptr) != allocmap.end()) {
            obj_count--;
            footprint -= allocmap[(size_t)ptr];
            if (footprint < 0) footprint = 0;
            allocmap[(size_t)ptr] = 0;
            }*/

#endif
               
        trace_flag = false;
    }

    freep(ptr);

}
