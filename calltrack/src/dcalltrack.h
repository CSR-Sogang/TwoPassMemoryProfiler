#ifndef _DCALLTRACK_H
#define _DCALLTRACK_H

#include <dlfcn.h>
#include <execinfo.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
#include <cxxabi.h>
#include <unistd.h>
#include <sys/msg.h>
#include <iostream>
#include <algorithm>
#include <map>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

//#define DEBUG
//#define DEBUG_EXTRA

//#define _GNU_SOURCE
#define _TRACE
/* empty showstack function*/
//#define _NO_SHOWSTACK

//#define READ_MALLOC_SIZE_FILE
//#define SHOW_FREE
/* if you want to use summary output, notice that MAX_ARRAY_SIZE should be large enoough. */
//#define SUMMARY
/*show the size output */
//#define SHOW_SIZE

//#define READ_VAR_HASH_VALUE
//#define PER_VAR_SIZE // get per variable serival objects' size depend on TRACE_COUNT
//#define IDENT_TRANSFER  // work with READ_VAR_HASH_VALUE to transfer addr & size to PIN
//#define GET_TIME_INFO // This is for getting the lifetime
//#define READ_VAR_ID
//#define BLAST
//#define SUMMARY
//#define MPI

static FILE* trace_file = NULL;

//Used in the identifying transfer processing output the coressponding variable id
static FILE* var_outfile = NULL;

/* for getting the var size*/
/*
#define MAX_ARRAY_SIZE 500000000
#define MAX_STRING_LENGTH 8192
#define MAX_HASH_SIZE 5000000
*/
/* for get the top10 calltrack */
///*
#define MAX_ARRAY_SIZE 500000000
#define MAX_STRING_LENGTH 8192
#define MAX_HASH_SIZE 500000
//*/

#define MAX_UNFREE 100000
#define BIG_SIZE 1048576
#define _32K_SIZE 32768
#define _128K_SIZE 131072
#define MID_SIZE 8192
#define TRACE_COUNT 1
#define MIN_TRACE_SIZE 32768
#define VAR_HASH_VALUE_SIZE 333333
#define VAR_ID_TABLE_SIZE 100

static const int MALLOC_SIZE = 0;
static const int CALLTRACK_SIZE = MALLOC_SIZE;

//static int CALLTRACK_SIZE = MALLOC_SIZE;

int malloc_count = 0; // total malloc object count
int calloc_count = 0; // total calloc object count
int realloc_count = 0; // total realloc object count
int total_count = 0; // total object count;
int big_count = 0;
int _32k_count = 0;
int _128k_count = 0;
int mid_count = 0;
int msg_id = -1;
int var_time_count = 0;
size_t max_size = 0;
double program_start_time = 0.0;
//std::map<size_t, size_t> allocmap;

long int unfree_list[MAX_UNFREE];
size_t unfree_count = 0;
size_t footprint = 0;
size_t max_footprint = 0;
size_t max_obj_count = 0;
size_t obj_count = 0; // Current living object count;
size_t total_obj_size = 0;
size_t var_count = 0; // Total variable count;
size_t var_id_size = 0;

size_t trace_count_array[TRACE_COUNT];

// only used in the train and test size to get the variable hash values
// the variable hash values are the variable we are going to trace
size_t var_hash_value_size = 0;
size_t var_hash_value_table[VAR_HASH_VALUE_SIZE];
size_t var_hash_value_size_array[VAR_HASH_VALUE_SIZE][10];
//In fact, it store the variable size regarding to the variable in the var hash value file
size_t var_count_array[1000][20];
size_t size_array[MAX_ARRAY_SIZE];
size_t var_ptr_for_time[20];
double variable_start_time[20];
double variable_end_time[20];
int var_id_table[VAR_ID_TABLE_SIZE];

struct hash_table
{
    char calltrack_str[MAX_STRING_LENGTH];
    unsigned int hash_value;
    int exist;
    int id;
}calltrack_hash_table[MAX_HASH_SIZE];

struct stc_msg_addr
{
    long int msg_type;
    size_t size;
    size_t addr;
    //unsigned long hash;
} msg_addr;

void write_down_addr(size_t addr, size_t size, unsigned long hash) {
    //size_t msg_size = sizeof(long int) + 2*sizeof(size_t) + sizeof(unsigned long);

    msg_addr.msg_type = 1;
    msg_addr.addr = addr;
    msg_addr.size = size;
    //msg_addr.hash = hash;

    //if (msgsnd(msg_id, (void*) &msg_addr, msg_size, 0) == -1) {
    if (msgsnd(msg_id, (void*) &msg_addr, 16, 0) == -1) {
      printf("Error in send the message!\n");
    }
    #ifdef DEBUG
    printf("Send the message %ld\n", msg_addr.addr);
    #endif
}

double mysecond()
{
    struct timeval tp;
    struct timezone tzp;
    int i;
    
    i = gettimeofday(&tp,&tzp);
    return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

void write_addr_init() {

    FILE* size_file = fopen("./VarMinSize", "r");
    int queue_key;

    if (size_file == NULL) 
        perror("write_addr_init() : VarMinSize not exist!\n");

    // Use the pid to init the queue, this is more safe
    queue_key = getpid();

    for (int i = 0; i < var_hash_value_size; i++)
      for (int j = 0; j < TRACE_COUNT; j++) 
        fscanf(size_file, "%lu", &var_hash_value_size_array[i][j]);

    //////////////
    #ifdef DEBUG_EXTRA
    printf("(write_addr_init function)\n");
    for (int i = 0; i < var_hash_value_size; i++)
      for ( int j = 0 ; j < TRACE_COUNT; j++)
        printf("%lu ", var_hash_value_size_array[i][j]);
    printf("\n");
    #endif
    //////////////


    msg_id = msgget((key_t) queue_key, 0666 | IPC_CREAT);
    if (msg_id == -1) 
        perror("msg queue create error!\n");
    
}

void var_time_init() {
    for ( int i = 0; i < 20; i++) 
        var_ptr_for_time[i] = 0;
    
    FILE* size_file = fopen("./VarMinSize", "r");
    
    if (size_file == NULL) 
        perror("var_time_init() : VarMinSize not exist!\n");

    for (int i = 0; i < var_hash_value_size; i++)
        for (int j = 0; j < TRACE_COUNT; j++) 
            fscanf(size_file, "%lu", &var_hash_value_size_array[i][j]);

}

void update_var_kth_count(size_t size, int id) {
    size_t min_num = 0x7fffffff;
    int min_id;
    
    for (int i = 0; i < TRACE_COUNT; i++) {
      if (var_count_array[id][i] < min_num) {
        min_num = var_count_array[id][i];
        min_id = i;
      }
    }
    
    #ifdef DEBUG_EXTRA
    printf("(update_var_kth_count) size : %d, min_num : %d\n", size, min_num);
    #endif
    if (size > min_num) {
      #ifdef DEBUG_EXTRA
      printf("[update_var_kth_count] var_count_array[%d][%d] = %d\n",id,min_id,size);
      #endif
      var_count_array[id][min_id] = size;
    }
}

size_t OutputVarKthSize() {
  FILE* out = fopen("./VarMinSize", "w");
  size_t min_num; 
    /*
    for (int j = 0; j < var_hash_value_size; j++) {
        min_num = 0x7fffffff;
        
        for (int i = 0; i < TRACE_COUNT; i++) {
        
            if (var_count_array[j][i] < min_num && var_count_array[j][i] != 0) {
                min_num = var_count_array[j][i];
            }
        }
        fprintf(out, "%lu\n", min_num);
        }*/
  #ifdef DEBUG_EXTRA
  printf("[OutputVarKthSize] var_hash_value_size : %d\n", var_hash_value_size);
  #endif
  for (int j = 0; j < var_hash_value_size; j++) { 
    std::sort(var_count_array[j], var_count_array[j] + TRACE_COUNT);
    for ( int i = 0; i < TRACE_COUNT; i++) { 
      fprintf(out, "%lu ", var_count_array[j][i]);
    }
    fprintf(out, "\n");
  }
  
  fclose(out);
}

void update_kth_count(size_t size) {
    size_t min_num = 0x7fffffff;
    int min_id;

    for (int i = 0; i < TRACE_COUNT; i++) {
        if (trace_count_array[i] < min_num) {
            min_num = trace_count_array[i];
            min_id = i;
        }
    }
    if (size > min_num) 
        trace_count_array[min_id] = size;
}

size_t get_kth_count() {
    size_t min_num = 0x7fffffff;

    for (int i = 0; i < TRACE_COUNT; i++) {
        if (trace_count_array[i] < min_num && trace_count_array[i] != 0) { 
           min_num = trace_count_array[i];
        }
    }
    return min_num;
}

unsigned long StringHash(char *str)
{
    unsigned long seed = 31; // 31 131 1313 13131 131313 etc..
    unsigned long hash = 0;

    while (*str)
    {
        hash = hash * seed + (*str++);
    }

    return hash;
}

/*
void ReadMallocSize() {
    FILE* malloc_size_file = fopen("./TraceSize", "r");
  
    if (malloc_size_file == NULL) perror ("No TraceSzie file!\n");
    fscanf(malloc_size_file, "%d\n", &MALLOC_SIZE);

    if (MALLOC_SIZE != 0 ) 
        MALLOC_SIZE = MALLOC_SIZE - 1;
}
*/

void ReadHashValues() {
    FILE* hash_value_file = fopen("./HashValues", "r");
    
    if (hash_value_file == NULL) 
        perror("HashValues not exits!\n");
    size_t hash;

    if (var_hash_value_size != 0) 
      return;

    while (fscanf(hash_value_file, "%lu\n", &hash) != EOF) {
      //printf("!!!!!!!var_hash_value_size increases!!!!!!!!!\n");
      var_hash_value_table[var_hash_value_size++] = hash;
    }
    ///////////
    #ifdef DEBUG_EXTRA
    printf("(ReadHashValues function)\n");
    for (int i=0; i < var_hash_value_size; i++)
      printf("%lu ", var_hash_value_table[i]);
    printf("\n");
    #endif
    //////////

    for (int i = 0; i < var_hash_value_size; i++) 
      for (int j = 0; j <= TRACE_COUNT; j++) {
        var_count_array[i][j] = 0;
      }
}

int InitVarIDTable() {
  #ifdef DEBUG_EXTRA
  printf("(InitVarIDTable) came here!\n");
  #endif

  FILE* var_id_file = fopen("./VarIDs", "r");
	if (var_id_file == NULL) perror("VarIDs file not exist!\n");
    while (fscanf(var_id_file, "%d\n", &var_id_table[var_id_size]) != EOF) 
        var_id_size++;
}

int LookVarID(int var_id){
    for (int i = 0; i < var_id_size; i++) 
        if (var_id == var_id_table[i]) {
            fprintf(trace_file, "[Var_ID & Index] %d, %d\n", var_id, i+1);
            //printf("[Var_ID & Index] %d, %d\n", var_id, i+1);
            var_id_table[i] = -1;
            return 1;
        }
    return 0;
}

//FIXME
static int var_count_trans[100]; //temp add.

int LookIntoHashValue(char* str, size_t addr, size_t size) {
  size_t hash = StringHash(str);
    
  #ifdef DEBUG_EXTRA
  printf("(In LookIntoHashValue) var_hash_value_size is %u\n", var_hash_value_size);
  #endif

  for (int i = 0; i < var_hash_value_size; i++) {
    #ifdef DEBUG_EXTRA
    printf("For reference, size_t size : %u, unsigned long size : %u\n", sizeof(size_t), sizeof(unsigned long));

    printf("hash function value : %lu\n", StringHash(str));
    printf("hash : %lu and var_hash_value_table[%d] : %lu\n", hash, i, var_hash_value_table[i]);
    #endif

    if (hash == var_hash_value_table[i]) {

      var_count_trans[i]++;
      #ifdef DEBUG_EXTRA
      printf("(LookIntoHashValue) var_outfile is %s, and the addr is %u, size is %u\n",var_outfile, addr, size);
      #endif
      fprintf(var_outfile, "%d_%d\n", i+1, var_count_trans[i]);
      write_down_addr(addr, size, hash);

#ifdef IDENT_TRANSFER
      //FIXME
      /*
      for (int j = 0; j < TRACE_COUNT; j++)
          FIXME
          if (size == var_hash_value_size_array[i][j] ) { // change this to get variable all information
              fprintf(var_outfile, "%d_%d\n", i + 1, TRACE_COUNT - j);
              var_hash_value_size_array[i][j] = 0;
              write_down_addr(addr, size);
    return i;
          }
      */
#endif

#ifdef GET_TIME_INFO
      for (int j = 0; j < TRACE_COUNT; j++)
        if (size == var_hash_value_size_array[i][j] ) {
          fprintf(var_outfile, "%d_%d\n", i + 1, TRACE_COUNT - j);
          var_hash_value_size_array[i][j] = 0;
          variable_start_time[var_time_count] = mysecond();
          var_ptr_for_time[var_time_count++] = addr;
          return i;
        }
#endif

      return i;
    }
  }
  return -1;
}

#ifdef _NO_SHOWSTACK
static inline int show_stackframe(void) {
    return 1;
}
#else
static inline int show_stackframe1(void) {
    void *trace[16];
    char **messages = (char **)NULL;
    int i, trace_size = 0;
    
    trace_size = backtrace(trace, 16);
    messages = backtrace_symbols(trace, trace_size);
    fprintf(trace_file, "[Call] Execution path:\n");
    for (i=0; i < trace_size; ++i)
        fprintf(trace_file, "[Call] %s\n", messages[i]);
    
    return 1;
}

int LookHashTable(char *str) {
    if (strlen(str) >= MAX_STRING_LENGTH) 
        perror("String length too long!\n");

    unsigned int hash_value = StringHash(str);
    int index = hash_value % MAX_HASH_SIZE;
    int found = 0;
    while (calltrack_hash_table[index].exist == 1) {
        if (hash_value == calltrack_hash_table[index].hash_value) {
            //if (strcmp(calltrack_hash_table[index].calltrack_str, str) == 0 ) 
            found = 1;
            //else 
            //    printf ("Different call track map to same hash value\n");
        }
        if(found == 1) break;
        index = (index + 1) % MAX_HASH_SIZE;
    }

    if(found == 0) {
        var_count++;
        //strcpy(calltrack_hash_table[index].calltrack_str, str);
        calltrack_hash_table[index].exist = 1;
        calltrack_hash_table[index].hash_value = hash_value;
        calltrack_hash_table[index].id = var_count;
    }

    if (var_count >= MAX_HASH_SIZE) 
        perror("Hash table exceed!\n");

    return calltrack_hash_table[index].id;
}

void InitHashTable() {
    for (int i = 0; i < MAX_HASH_SIZE; i++) 
        calltrack_hash_table[i].exist = 0;
}

//void Hash

////// malloc, calloc, realloc functions call this function.
////// This function finds the object's hash value with its call-stack in
//// hash table, which is constructed in first fast pass.
////// After finding hash value, it sends a 'addr' and 'size' to 
//// custom Pin code via message. (LookIntoHashValue)
////// Custom Pin instrumentation code gets the 'addr' and 'size' when
//// malloc, calloc, realloc functions are called (TestAndGetAddr) and
//// calculates the page and cache line offsets with 'addr' and 'size.
//
///// addr : the virtual address which mallocp function returns
///// size : object's allocated size
static inline int show_stackframe(size_t addr, size_t size)
{
  unsigned int max_frames = 63;
  FILE *out = trace_file;
  int return_value = 1;
  
  char calltrack[MAX_STRING_LENGTH] = "";
  int calltrack_strlen = 0;
    //CHNAGEME 
    //if (!size == 65536) 
    //    return 0;
    //else 
    //    return 0;
    //printf("(masaka3) Did it come here?\n");

#ifdef READ_VAR_HASH_VALUE
  return_value = 0;
#endif
  
    // storage array for stack trace address data
  void* addrlist[max_frames+1];

  // retrieve current stack addresses
  int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

  if (addrlen == 0) {
    fprintf(out, "  <empty, possibly corrupt>\n");
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
              return 0;
          }
      } else {
          strcat(calltrack, symbollist[i]);
      }
  }
  #ifdef DEBUG_EXTRA
  printf("(In show_stackframe) \"calltrack\" variable is : %s\n", calltrack);
  #endif

#ifdef READ_VAR_HASH_VALUE
  if (LookIntoHashValue(calltrack, addr, size) == -1) {

      free(funcname);
      free(symbollist);
      return return_value;
  } 
#endif

#ifdef IDENT_TRANSFER
  free(funcname);
  free(symbollist);
  return return_value;
#endif

#ifdef GET_TIME_INFO
  free(funcname);
  free(symbollist);
  return return_value;
#endif
    
  return_value = LookHashTable(calltrack);

    //CHANGEME
#ifdef SUMMARY
  free(funcname);
  free(symbollist);
  return return_value;
#endif

#ifdef SHOW_SIZE
  fprintf(trace_file, "[Hash] %lu\n", StringHash(calltrack));
  free(funcname);
  free(symbollist);
  return return_value;
#endif   

#ifdef READ_VAR_ID
  if ( LookVarID(return_value) == 0) {
      free(funcname);
      free(symbollist);
      return return_value;
  }
#endif

#ifdef PER_VAR_SIZE
  update_var_kth_count(size, LookIntoHashValue(calltrack, addr, size));
#endif
   
#ifdef BLAST
  #ifdef DEBUG
  printf("[Stack trace:]\n");
  #endif

#else
  fprintf(out, "[Stack trace:]\n");
#endif
    
  for (int i = 1; i < addrlen; i++)
  {
      char *begin_name = 0, *begin_offset = 0, *end_offset = 0;
#ifdef BLAST
  #ifdef DEBUG
    printf("[Call]");
  #endif

#else 
      fprintf(out, "[Call]");
#endif
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
#ifdef BLAST
  #ifdef DEBUG_EXTRA
              printf("%s : %s+%s\n",
                      symbollist[i], funcname, begin_offset);
  #endif

#else            
              fprintf(out, "%s : %s+%s\n",
                      symbollist[i], funcname, begin_offset);
#endif
          }
          else {
              // demangling failed. Output function name as a C function with
              // no arguments.
#ifdef BLAST
  #ifdef DEBUG
              printf("%s : %s()+%s\n",
                      symbollist[i], begin_name, begin_offset);
  #endif

#else
              fprintf(out, "%s : %s()+%s\n",
                      symbollist[i], begin_name, begin_offset);
#endif
          }
      }
      else
      {
          // couldn't parse the line? print the whole line.
#ifdef BLAST
  #ifdef DEBUG
          printf(" %s\n", symbollist[i]);
  #endif

#else
          fprintf(out, " %s\n", symbollist[i]);
#endif
      }
  }
    
#ifdef BLAST
  #ifdef DEBUG
  printf("[Hash] %lu\n", StringHash(calltrack));
  #endif

#else
  fprintf(out, "[Hash] %lu\n", StringHash(calltrack));
#endif

  free(funcname);
  free(symbollist);
  // NOTICE here
  return return_value;
}
#endif

#endif               
