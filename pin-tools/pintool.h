#ifndef PINTOOL_H
#define PINTOOL_H

#include <stdio.h>
#include <stdlib.h>
/* Marcos used to control the sequence of the work */

#define READ_ADDR_FILE /*Read the variables address from Linux message queue */
//#define READ_TRACE_SIZE  /*Read the TRACE_SIZE to save time*/
//#define MPI

//#define DEBUG

/* ===================================================================== */
/* Macro */
/* ===================================================================== */
const int MAX_SIZE = 32768;   //// Maximum count of objects
static size_t TRACE_SIZE = 0;
//const int TRACE_SIZE = 221183;
const int PAGE_SIZE = 4096;
const int CACHE_LINE_SIZE = 64;

///// For the count of cache line in row buffer
const int CACHE_LINE_COUNT_INBUFFER = 64;

const int NEIGBOUR_SIZE = 8;
const int HASH_TABLE_SIZE = 10000000;
const int HASH_SIZE = 10000000;
/* Do not excceed the size of int / 10*/
const int64_t NEIGBOUR_INS_SIZE = 1e3; 
const int SLEEP_TIME = 50;
const uint64_t EPISODE_LEN = 1e9;

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
std::ofstream DataFile;
std::ofstream EpisodeFile;
int64_t var_size[MAX_SIZE];
int64_t var_addr[MAX_SIZE];
int64_t reference_count[MAX_SIZE];
#ifdef READ_ADDR_FILE
static int32_t in_alloc = 0;
#endif
int64_t G_var_size;
static int64_t ins_count = 0;
//static int64_t flag_modified = 0;  ///// To count dirty ratio (for selective write)
//static int64_t flag_modified_buffer = 0;  ///// To count dirty ratio (for exact selective write)
///// To count dirty cache line (for partial write)
//static int64_t flag_modified_cache_inbuffer[CACHE_LINE_COUNT_INBUFFER];

int64_t dirty_page_count = 0;
int64_t dirty_page_count_buffer[MAX_SIZE];
int64_t dirty_cacheline_count_total[MAX_SIZE];
int64_t last_access_page_total = -1;

int64_t last_access_page[MAX_SIZE];
int64_t touch_count[MAX_SIZE];
int64_t dirty_count[MAX_SIZE];
int64_t page_offsets[MAX_SIZE];
int64_t sequential_count[MAX_SIZE];
int64_t sequential_part[MAX_SIZE];
int32_t sequential_direct[MAX_SIZE];
int32_t hash_table[HASH_TABLE_SIZE];
int32_t hash_array[HASH_SIZE];
int32_t hash_index[HASH_SIZE];
int32_t dirty_flag[MAX_SIZE];
int64_t *last_access_ins[MAX_SIZE];
int64_t *last_access_ins_cache[MAX_SIZE];
size_t *references_perpage[MAX_SIZE];
int64_t spatial_count[MAX_SIZE];
int64_t temperal_count[MAX_SIZE];
int64_t spatial_count_cache[MAX_SIZE];
int64_t temperal_count_cache[MAX_SIZE]; // temporal..
int64_t access_volume[MAX_SIZE];
int64_t access_ops[MAX_SIZE];
int64_t read_volume[MAX_SIZE];
int64_t seq_volume[MAX_SIZE];
int64_t seq_byte_count[MAX_SIZE];
int64_t stride_count[MAX_SIZE];
int64_t stride_volume[MAX_SIZE];
int64_t last_access_stride[MAX_SIZE];
size_t last_access_bytes[MAX_SIZE];
size_t avg_offset_bytes[MAX_SIZE];
int64_t total_volume = 0;
int64_t total_ops = 0;
double first_access_time[MAX_SIZE];
double last_access_time[MAX_SIZE];
double malloc_time[MAX_SIZE];
double free_time[MAX_SIZE];

int64_t last_access_volume[MAX_SIZE];
int64_t last_access_ops[MAX_SIZE];
int64_t last_spatial_count_cache[MAX_SIZE];
int64_t last_temperal_count_cache[MAX_SIZE];
int64_t last_stride_volume[MAX_SIZE];
int64_t last_seq_volume[MAX_SIZE];
int64_t last_read_volume[MAX_SIZE];

static int64_t start_num_inst[MAX_SIZE];
static int64_t end_num_inst[MAX_SIZE];

int num_episode = 0;
/////// count : index of variable
size_t count = 0; 
size_t hash_count = 0;
int msg_id = -1;

struct stc_msg_addr
{
    long int msg_type;
    size_t size;
    size_t addr;
} msg_addr;

/* ===================================================================== */
/* Analysis routines                                                     */
/* ===================================================================== */

VOID AllocAfter(ADDRINT ret);

inline double GetTimeNow() {
    struct timeval stime;
    gettimeofday(&stime, NULL);
    return stime.tv_sec + 1e-6 * stime.tv_usec;
}
 
inline VOID UpdateHash(size_t start, size_t size) {
    int last_hash = -2;
    size_t hash_head = 0;
    size_t start_cnt = (start >> 3) % HASH_TABLE_SIZE;
    //cout << start << endl;
    //cout << size << endl;
    for (size_t i = start; i < (start + size); i += 8) {
        int index = hash_table[(i >> 3) % HASH_TABLE_SIZE];
        
        //cout << index << endl;
        if (index == -1 && last_hash != index) {
            hash_count ++;
            hash_table[(i >> 3) % HASH_TABLE_SIZE] = hash_count;
            cout << "In Update" << endl;
            cout << index << " " << hash_count << " " << count << endl;
            hash_index[hash_count] = -1;
            hash_array[hash_count] = count;
            last_hash = index;
            hash_head = hash_count;
        }
        else if (index != last_hash) { /*copy previous one*/
            int old_index = hash_table[(i >> 3) % HASH_TABLE_SIZE];
            int head = hash_count + 1;
            last_hash = old_index;
            while (old_index != -1) {
                hash_count ++;
                hash_index[hash_count] = hash_count + 1;
                hash_array[hash_count] = hash_array[old_index];
                //cout << "In Copy" << endl;
                //cout << index << " " << hash_count << " " << hash_array[old_index] << endl;
                old_index = hash_index[old_index];
            }
            hash_count++;
            hash_index[hash_count] = -1;
            hash_array[hash_count] = count;
            //cout << index << " " << hash_count << " " << hash_array[hash_count] << endl;
            hash_table[(i >> 3) % HASH_TABLE_SIZE] = head;
            hash_head = head;
        }
        else if (last_hash == index) 
            hash_table[(i >> 3) % HASH_TABLE_SIZE] = hash_head;
        if ((i >> 3) % HASH_TABLE_SIZE == start_cnt - 1) break;
    }
}

/*
inline VOID CleanHash(size_t start, size_t size) {
}
*/

inline int Hash(size_t addr) {
    int index = hash_table[(addr >> 3) % HASH_TABLE_SIZE];
    int var_id;
    while (index != -1) {
        //cout << "In hash" << index << endl;
        var_id = hash_array[index];
        //cout << "Var_id " << var_id << endl;
        if ((int64_t)addr >= var_addr[var_id] && 
            (int64_t)addr < var_addr[var_id] + var_size[var_id]) 
            return var_id;
        index = hash_index[index];
    }
    return -1;
}        

inline int Debug(size_t addr) {
    int index = hash_table[(addr >> 3) % HASH_TABLE_SIZE];
    int var_id;
    while (index != -1) {
        cout << "In hash" << index << endl;
        var_id = hash_array[index];
        cout << "Var_id " << var_id << endl;
        if ((int64_t)addr >= var_addr[var_id] && 
            (int64_t)addr < var_addr[var_id] + var_size[var_id]) 
            return var_id;
        index = hash_index[index];
    }
    return -1;
}
inline VOID UpdateTotalOps() {
    total_ops++;
}

inline VOID UpdateObjOps(int obj_id) {
    access_ops[obj_id]++;
}

inline VOID UpdateTotalVolume(size_t size) {
    total_volume += size;
}

inline VOID UpdateAccessVolume(int obj_id, size_t size) {
    access_volume[obj_id] += size;
}

inline VOID UpdateReadVolume(int obj_id, size_t size) {
    read_volume[obj_id] += size;
}

inline VOID UpdateTime(int obj) {
    
    double time_now = GetTimeNow();
    if (first_access_time[obj] == 0) 
        first_access_time[obj] = time_now;
    
    last_access_time[obj] = time_now;
}

inline VOID UpdateAvgOffsetBytes(int obj_id, size_t addr) {
    avg_offset_bytes[obj_id] += abs(addr - last_access_bytes[obj_id]);
    //if ( abs(addr - last_access_bytes[obj_id]) != 4 ) {
    //cout << abs(addr - last_access_bytes[obj_id]) << endl;
    //cout << access_ops[obj_id] << endl;
    //}
}
            
inline VOID UpdateSeqVolume(int obj_id, size_t addr, size_t size) {
    if (last_access_bytes[obj_id] + size == addr) {
        seq_volume[obj_id] += size;
        seq_byte_count[obj_id]++;
    }
    else if (last_access_bytes[obj_id] - size == addr) {
        seq_volume[obj_id] += size;
        seq_byte_count[obj_id]++;
    }
    
    else if (addr == last_access_stride[obj_id] + last_access_bytes[obj_id]) {
        stride_count[obj_id]++;
        stride_volume[obj_id] += size;
    }

    last_access_stride[obj_id] = addr - last_access_bytes[obj_id];
}

inline VOID UpdateTouchCount(size_t obj_id) {
    touch_count[obj_id]++;
}

inline VOID UpdateSeqCount(size_t obj_id, size_t npage) {
    if (sequential_direct[obj_id] == 1) {
        if (last_access_page[obj_id] + 1 != (int64_t) npage) {
            sequential_direct[obj_id] = 0;
        }
        else 
            sequential_count[obj_id]++;
    }
    if (sequential_direct[obj_id] == 2) { 
        if (last_access_page[obj_id] - 1 != (int64_t) npage) {
            sequential_direct[obj_id] = 0;
        }
        else 
            sequential_count[obj_id]++;
    }
    if (sequential_direct[obj_id] == 0) {
        if (last_access_page[obj_id] + 1 == (int64_t) npage) 
            sequential_direct[obj_id] = 1;
        if (last_access_page[obj_id] - 1 == (int64_t) npage)
            sequential_direct[obj_id] = 2;
    }
}

inline VOID UpdatePageOffset(size_t obj_id, size_t npage) {
    page_offsets[obj_id] += abs(npage - last_access_page[obj_id]);
    //cout << obj_id << " " << page_offsets[obj_id] << std::endl;
}

inline VOID ChangeDirtyStat(size_t obj_id) {
    dirty_flag[obj_id] = 0;
}
    
inline VOID UpdateDirtyCount(size_t obj_id) {
    if (dirty_flag[obj_id] == 0) {
        dirty_flag[obj_id] = 1;
        dirty_count[obj_id]++;
    }
}

inline VOID UpdateLocality(size_t obj_id, size_t npage) {
    for (size_t i = 1; i < NEIGBOUR_SIZE; i++) { //Change 1 to 0...
        ///// npage(=N) : N-th page of accessed address in its object
        if (npage >= i) {
            if (ins_count - last_access_ins[obj_id][npage - i] 
                < NEIGBOUR_INS_SIZE) {
                spatial_count[obj_id]++;
                break;
            }
        }
        if (npage + i <= (size_t)var_size[obj_id] / PAGE_SIZE) {
            if (ins_count - last_access_ins[obj_id][npage + i]
                < NEIGBOUR_INS_SIZE) {
                spatial_count[obj_id]++;
                break;
            }
        }
    }
    
    if (ins_count - last_access_ins[obj_id][npage]
        < NEIGBOUR_INS_SIZE)
        temperal_count[obj_id]++;
}

inline VOID UpdateLocalityCache(size_t obj_id, size_t cache_line) {
    for (size_t i = 1; i < NEIGBOUR_SIZE; i++) { //Change 1 to 0..
        ///// cache_line(=N) : N-th cache line of accessed address in its object
        if (cache_line >= i) {
            if (ins_count - last_access_ins_cache[obj_id][cache_line - i] 
                < NEIGBOUR_INS_SIZE) {
                spatial_count_cache[obj_id]++;
                break;
            }
        }

        if (cache_line + i <= (size_t)var_size[obj_id] / CACHE_LINE_SIZE) {
            if (ins_count - last_access_ins_cache[obj_id][cache_line + i]
                < NEIGBOUR_INS_SIZE) {
                spatial_count_cache[obj_id]++;
                break;
            }
        }
    }
    
    if (ins_count - last_access_ins_cache[obj_id][cache_line]
        < NEIGBOUR_INS_SIZE)
        temperal_count_cache[obj_id]++;
}

inline VOID UpdateReferencesPerpage(size_t obj_id, size_t npage) {
    references_perpage[obj_id][npage]++;
}
     
// obj_id(=N) : N-th object which contains accessed address (object index)
inline VOID GetMetrics(VOID * addr, size_t size, int obj_id, int flag) {
    /////// addr : accessed address
    /////// var_addr : start address of object
    //
    ////// npage(N) : N-th page where accessed address is belong to
    ////// cache_line(N) : N-th cache line where accessed address is belong to
    size_t npage = ((size_t)addr - var_addr[obj_id]) / PAGE_SIZE;
    size_t cache_line = ((size_t)addr - var_addr[obj_id]) / CACHE_LINE_SIZE;

    /* do cache level locality */
    UpdateLocalityCache(obj_id, cache_line);
    UpdateLocality(obj_id, npage);
    last_access_ins_cache[obj_id][cache_line] = ins_count;
    last_access_ins[obj_id][npage] = ins_count;
    UpdateAccessVolume(obj_id, size);
    UpdateObjOps(obj_id);
    UpdateAvgOffsetBytes(obj_id, (size_t) addr);
    UpdateSeqVolume(obj_id, (size_t) addr, size);
    UpdateTime(obj_id);

/*
    ///// In case of total page change, not only inside of variable (same with row buffer change)
    ///// This is for considering exact selective write and partial write
    if ((int64_t)npage != last_access_page_total) {

        last_access_page_total = npage;

        ///// This is for checking total cache line changed (for partial write)
        for (size_t i = 0; i < CACHE_LINE_COUNT_INBUFFER; i++) {
            //// Increment dirty cache line count with its flag
            if (flag_modified_cache_inbuffer[i] == 1) {
                dirty_cacheline_count_total ++;

                //// Initialize the dirty cache line flags
                flag_modified_cache_inbuffer[i] = 0;
            }

        }
        ///// This is for checking total cache line changed (for partial write)
        for (size_t i = 0; i < CACHE_LINE_COUNT_INBUFFER; i++)
            //// Initialize the dirty cache line flags
            if (flag_modified_cache_inbuffer[i] == 1)
                flag_modified_cache_inbuffer[i] = 0;

        //// Increment dirty page count
        if (flag_modified_buffer)
            flag_modified_buffer = 0;
    }
*/
    if ((int64_t) npage != last_access_page[obj_id]) { 
        
        UpdateTouchCount(obj_id);
        UpdateSeqCount(obj_id, npage);
        UpdatePageOffset(obj_id, npage);
        ChangeDirtyStat(obj_id);
        UpdateReferencesPerpage(obj_id, npage);

        ////// last_access_page[N] (M) : M-th page which N-th object access at last
        last_access_page[obj_id] = npage;

/*
        ////// If the previous page is dirty,
        ////// increment dirty page count and unset the flag_modified.
        if (flag_modified)
            flag_modified = 0;
*/

    }
    ////// flag = 1 : Case of write
    if (flag == 1) {
        UpdateDirtyCount(obj_id);

/*
        if (!flag_modified) {
            dirty_page_count ++;
            flag_modified = 1;
        }
        
        //// Increment dirty page count instead of row buffer with its flag (selective write)
        if (!flag_modified_buffer) {
            dirty_page_count_buffer[obj_id] ++;
            flag_modified_buffer = 1;
        }

        //// Increment dirty cache line count with its flag (partial write)
        if (!flag_modified_cache_inbuffer[cache_line%CACHE_LINE_COUNT_INBUFFER]) {
            dirty_cacheline_count_total[obj_id] ++;          
            flag_modified_cache_inbuffer[cache_line%CACHE_LINE_COUNT_INBUFFER] = 1;
        }
*/
    }
    
    ////// flag = 0 : Case of read
    if (flag == 0)
        UpdateReadVolume(obj_id, size);

    reference_count[obj_id]++;
    last_access_bytes[obj_id] = (size_t)addr;

}

VOID InitVariable(size_t obj_id) {
    struct timeval start;
    gettimeofday(&start, NULL);
    reference_count[obj_id] = 0;
    last_access_page[obj_id] = -1;
    dirty_page_count_buffer[obj_id] = 0;
    dirty_cacheline_count_total[obj_id] = 0;
    touch_count[obj_id] = 0;
    dirty_count[obj_id] = 0;
    page_offsets[obj_id] = 0;
    temperal_count[obj_id] = 0;
    spatial_count[obj_id] = 0;
    temperal_count_cache[obj_id] = 0;
    spatial_count_cache[obj_id] = 0;
    sequential_direct[obj_id] = 0;
    access_volume[obj_id] = 0;
    read_volume[obj_id] = 0;
    seq_volume[obj_id] = 0;
    seq_byte_count[obj_id] = 0;
    //last_access_bytes[obj_id] = 0;
    last_access_stride[obj_id] = 0;
    first_access_time[obj_id] = 0;
    stride_count[obj_id] = 0;
    avg_offset_bytes[obj_id] = 0;
    free_time[obj_id] = 0;
    malloc_time[obj_id] = start.tv_sec + 1e-6 * start.tv_usec;
}

VOID OutputPageInfo() {
    TraceFile << std::endl << "Object touch count" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << touch_count[i] << std::endl;
    
    TraceFile << std::endl << "References per touch" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) reference_count[i] / touch_count[i]
                  << std::endl;
    
    TraceFile << std::endl << "Rreference per page" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) reference_count[i]
                / ((var_size[i] + PAGE_SIZE - 1) / PAGE_SIZE) << std::endl;
    
    TraceFile << std::endl << "Dirty ratio %" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) 100 * dirty_count[i] / touch_count[i]
                  << std::endl;

    TraceFile << std::endl << "sequentail ratio %" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) 100 * sequential_count[i]
                / touch_count[i] << std::endl;

    TraceFile << std::endl << "Avg offsets " << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) page_offsets[i] / 
                (touch_count[i] - 1) << std::endl;

}

VOID Output() {
    TraceFile << "Size" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << var_size[i] << std::endl;

    //OutputPageInfo();
   
    TraceFile << std::endl << "Temporal_locality_page" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) temperal_count[i] * 100 
                / reference_count[i] << std::endl;

    TraceFile << std::endl << "Spatial_locality_page_new" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) spatial_count[i] * 100
                / reference_count[i] << std::endl;

    TraceFile << std::endl << "Temporal_locality_cache" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) temperal_count_cache[i] * 100 
                / reference_count[i] << std::endl;

    TraceFile << std::endl << "Spatial_locality_cache_new" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) spatial_count_cache[i] * 100
                / reference_count[i] << std::endl;

    //TraceFile << std::endl << "Total_memory_ops" << std::endl;
    //TraceFile << total_ops << endl;

    TraceFile << std::endl << "Obj_memory_ops" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << access_ops[i] << endl;
   
    TraceFile << std::endl << "Total_access_volume_bytes" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << access_volume[i] << endl;

    TraceFile << std::endl << "access_bytes_per_ops" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) access_volume[i] 
                / access_ops[i] << endl;

    TraceFile << std::endl << "accessed_volume_per_var_size" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) access_volume[i]
                / var_size[i] << endl;

    TraceFile << std::endl << "PerVar_access_volume_percentage" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) access_volume[i] * 100
                / total_volume << endl;

    TraceFile << std::endl << "seq_ratio_volume" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) seq_volume[i] * 100
                / access_volume[i] << std::endl;
    
    TraceFile << std::endl << "seq_ratio_count" << std::endl;
    for (size_t i = 0; i < count; i++)
        TraceFile << i << "\t" << (double) seq_byte_count[i] * 100
                / access_ops[i] << std::endl;
    
    TraceFile << std::endl << "read_ratio_volume" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) read_volume[i] * 100
                / access_volume[i] << std::endl;

    TraceFile << std::endl << "Avg_offset_bytes" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << (double) avg_offset_bytes[i]
                / access_ops[i]  << std::endl;

    TraceFile << std::endl << "stride_ratio_counts" << std::endl;
    for (size_t i = 0; i < count; i++)
        TraceFile << i << "\t" << (double) stride_count[i] * 100
                / access_ops[i] << std::endl;

    TraceFile << std::endl << "stride_ratio_volume" << std::endl;
    for (size_t i = 0; i < count; i++)
        TraceFile << i << "\t" << (double) stride_volume[i] * 100
                / access_volume[i] << std::endl;

    TraceFile << std::endl << "idle_time_first_%" << std::endl;
    for (size_t i = 0; i < count; i++)
        TraceFile << i << "\t" << 
                (double) (first_access_time[i] - malloc_time[i]) * 100 
                / (free_time[i] - malloc_time[i]) << std::endl;

    TraceFile << std::endl << "idle_time_last_%" << std::endl;
    for (size_t i = 0; i < count; i++)
        TraceFile << i << "\t" << 
                (double) (free_time[i] - last_access_time[i]) * 100 
                / (free_time[i] - malloc_time[i]) << std::endl;

    TraceFile << std::endl << "bytes_per_inst" << std::endl;
    for (size_t i = 0; i < count; i++) {
        if (end_num_inst[i] == 0)
            end_num_inst[i] = ins_count;
        TraceFile << i << "\t" <<
                (double) access_volume[i] /
                (end_num_inst[i] - start_num_inst[i]) << std::endl;
    }
    for (size_t i = 0; i < count; i++) {
        DataFile << "Object: " << i << std::endl;
        for(int j = 0; j < ((var_size[i] + PAGE_SIZE - 1) / PAGE_SIZE); j++)
            DataFile << references_perpage[i][j] << std::endl;
        DataFile << std::endl;
    }    
/*
    TraceFile << std::endl << "dirty_page_count (For selective write in case of same objects)" << std::endl;
    TraceFile << dirty_page_count << endl;

    TraceFile << std::endl << "Just dirty count" << std::endl;
    size_t temp = 0;
    for (size_t i = 0; i < count; i++)
      temp += dirty_count[i];
    TraceFile << "\t" << temp << std::endl;
   
    TraceFile << std::endl << " Total changed pages count (For exact selective write)" << std::endl;
    TraceFile << dirty_page_count_buffer << endl;

    TraceFile << std::endl << " Total dirty cache lines count in page (For partial write)" << std::endl;
    TraceFile << dirty_cacheline_count_total << endl;
*/
}

/*
VOID OutputEnergy() {

    TraceFile << count << std::endl;
    TraceFile << "Size" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << var_size[i] << std::endl;

    TraceFile << std::endl << "Total accessed volume" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << access_volume[i] << endl;
  
    TraceFile << std::endl << "Count of changed pages inside of variable (For selective write)" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << dirty_page_count_buffer[i] << endl;

    TraceFile << std::endl << " Total dirty cache lines count in page (For partial write)" << std::endl;
    for (size_t i = 0; i < count; i++) 
        TraceFile << i << "\t" << dirty_cacheline_count_total[i] << endl;

    TraceFile << std::endl << "Just \"dirty\" count (For reference)" << std::endl;
    size_t temp = 0;
    for (size_t i = 0; i < count; i++)
      temp += dirty_count[i];
    TraceFile << "\t" << temp << std::endl;
   
    //TraceFile << std::endl << " Total changed pages count (For exact selective write)" << std::endl;
    //TraceFile << dirty_page_count_buffer << endl;

}
*/

VOID ChangeAllocStat() {
#ifdef READ_ADDR_FILE
    if (G_var_size > (int64_t) TRACE_SIZE) {
        in_alloc = 1;
        //cout << "New Alloc comes " << G_var_size << endl;
    }
#endif
}

//// msg parameters : addr, size, hash value
//// Types of msg parameters
///// msg_type: long int
///// addr: size_t
///// size: size_t
///// hash value: unsigned long
VOID inline TestAndGetAddr() {
#ifdef READ_ADDR_FILE
    //size_t msg_size = sizeof(long int) + 2*sizeof(size_t) + sizeof(unsigned long);
    //int res = msgrcv(msg_id, (void*) &msg_addr, msg_size, 0, IPC_NOWAIT);
    int res = msgrcv(msg_id, (void*) &msg_addr, 16, 0, IPC_NOWAIT);

    if (res > 0) {
        #ifdef DEBUG
        printf("rcv the message %ld\n", msg_addr.addr);
        #endif
        in_alloc = 0;
        G_var_size = msg_addr.size;
        AllocAfter((ADDRINT)msg_addr.addr);
    }    
#endif
}

inline VOID DumpVarMemInfo(int obj_id) {
    if (access_volume[obj_id] == last_access_volume[obj_id]) {
        EpisodeFile << num_episode << "\t" << obj_id << "\t"
                    << "Volume: 0" << endl;
        EpisodeFile << num_episode << "\t" << obj_id << "\t"
                    << "Read: 0" << endl;
        EpisodeFile << num_episode << "\t" << obj_id << "\t"
                    << "Stride: 0" << endl;
        EpisodeFile << num_episode << "\t" << obj_id << "\t"
                    << "Seq: 0" << endl;
        EpisodeFile << num_episode << "\t" << obj_id << "\t"
                    << "Spatial: 0" << endl;
        EpisodeFile << num_episode << "\t" << obj_id << "\t"
                    << "Temporal: 0" << endl;
        return;
    }
    
    EpisodeFile << num_episode << "\t" << obj_id << "\t"
                << "Volume:\t" << access_volume[obj_id] - last_access_volume[obj_id] 
                << endl;
    EpisodeFile << num_episode << "\t" << obj_id << "\t"
                << "Read:\t" << 100.0 * (read_volume[obj_id] - last_read_volume[obj_id])
            / (access_volume[obj_id] - last_access_volume[obj_id]) << endl;
    EpisodeFile << num_episode << "\t" << obj_id << "\t"
                << "Stride:\t" << 100.0 * (stride_volume[obj_id] - last_stride_volume[obj_id]) 
            / (access_volume[obj_id] - last_access_volume[obj_id]) << endl;
    EpisodeFile << num_episode << "\t" << obj_id << "\t"
                << "Seq:\t" << 100.0 * (seq_volume[obj_id] - last_seq_volume[obj_id]) 
            / (access_volume[obj_id] - last_access_volume[obj_id]) << endl;
    EpisodeFile << num_episode << "\t" << obj_id << "\t"
                << "Spatial:\t" 
                << 100.0 * (spatial_count_cache[obj_id] - last_spatial_count_cache[obj_id]) 
            / (access_ops[obj_id] - last_access_ops[obj_id]) << endl;
    EpisodeFile << num_episode << "\t" << obj_id << "\t"
                << "Temporal:\t" 
                << 100.0 * (temperal_count_cache[obj_id] - last_temperal_count_cache[obj_id])
            / (access_ops[obj_id] - last_access_ops[obj_id]) << endl;
}

inline VOID SaveLastVarInfo(int obj_id) {
    last_access_volume[obj_id] = access_volume[obj_id];
    last_access_ops[obj_id] = access_ops[obj_id];
    last_read_volume[obj_id] = read_volume[obj_id];
    last_stride_volume[obj_id] = stride_volume[obj_id];
    last_temperal_count_cache[obj_id] = temperal_count_cache[obj_id];
    last_spatial_count_cache[obj_id] = spatial_count_cache[obj_id];
    last_seq_volume[obj_id] = seq_volume[obj_id];
}

inline VOID DumpMemoryInfo() {
    for (uint64_t i = 0; i < count; i++) {
        if (free_time[i] == 0) {
            DumpVarMemInfo(i);
            SaveLastVarInfo(i);
        }
    }
}

VOID InsCount() {
    ins_count++;
    if (ins_count % EPISODE_LEN == 0) {
	      printf("ins_count %ld\n", ins_count);
        DumpMemoryInfo();
        //cout << "Now it's border of episode\ncount is " << count << endl;
        num_episode++;
    }
}

void ReadAllocInfo() { 
    //ifstream alloc_info_file("./AllocInfo");
    //string tmp;
    FILE* alloc_info_file = fopen("./AllocInfo", "r");
    size_t var_addr = 0;
    //while(!alloc_info_file.eof()) {
    //    alloc_info_file >> G_var_size >> var_length;
    //	printf("%lu %lu %d\n", count, G_var_size, var_length);
    //
    //    AllocAfter(var_length);
    //}
    if (alloc_info_file == NULL) perror("Error: open file error!\n");
    while ( fscanf(alloc_info_file, "%lu %lu", &var_addr, &G_var_size) != EOF) {
        if (G_var_size > 0x7ffffff) printf("Error, length larger than int\n");
	      AllocAfter((int)var_addr);
    }
}

VOID MallocBefore(CHAR * name, ADDRINT size)
{
#ifdef READ_ADDR_FILE
    ///// in_alloc : A kind of lock
    if (in_alloc == 1 && G_var_size > (int64_t)TRACE_SIZE) {
        //sleep(1);
        usleep(SLEEP_TIME);
        TestAndGetAddr();
    }
#endif
    G_var_size = size;
    ChangeAllocStat();
}

VOID CallocBefore(CHAR * name, ADDRINT num, ADDRINT size) 
{
#ifdef READ_ADDR_FILE
    if (in_alloc == 1 && G_var_size > (int64_t)TRACE_SIZE) {
        //sleep(1);
        usleep(SLEEP_TIME);
        TestAndGetAddr();
    }
#endif

    G_var_size = size * num;
    ChangeAllocStat();
}

VOID ReallocBefore(CHAR * name, ADDRINT addr, ADDRINT size)
{
    struct timeval stime;
#ifdef READ_ADDR_FILE
    if (in_alloc == 1 && G_var_size > (int64_t)TRACE_SIZE) {
        usleep(SLEEP_TIME);
        TestAndGetAddr();
    }
#endif

    gettimeofday(&stime, NULL);
    
    for (size_t i = 0; i < count; i++) 
        if ((int64_t)addr == var_addr[i]) {
            var_addr[i] = -var_size[i]; 
            //var_size[i] = 0;
            free_time[i] = stime.tv_sec + 1e-6 * stime.tv_usec;
        }
    
    G_var_size = size;
    ChangeAllocStat();
}

VOID FreeBefore(CHAR * name, ADDRINT addr) 
{
    struct timeval stime;
    gettimeofday(&stime, NULL);
    
    for (size_t i = 0; i < count; i++) 
        if ((int64_t)addr == var_addr[i]) {
            var_addr[i] = -var_size[i]; 
            //var_size[i] = 0;
            free_time[i] = stime.tv_sec + 1e-6 * stime.tv_usec;

            end_num_inst[i] = ins_count;
            //Sometimes frees will cause some memory problems... Still not clear why.
            //freep(last_access_ins[i]);
            //freep(last_access_ins_cache[i]);
            //free(references_perpage[i]);
        }
}

VOID AllocAfter(ADDRINT ret)
{
    size_t page_count, cache_count;

#ifdef READ_ADDR_FILE
    in_alloc = 0;
#endif
        
    if (G_var_size > (int64_t) TRACE_SIZE) {
        
        // count : index of object
        var_size[count] = G_var_size;
        page_count = G_var_size / PAGE_SIZE + 1;
        cache_count = G_var_size / CACHE_LINE_SIZE + 1;

        last_access_ins[count] = (int64_t *) 
                malloc(sizeof(int64_t) * page_count);
        references_perpage[count] = (size_t *) 
                malloc(sizeof(size_t)* page_count);
        last_access_ins_cache[count] = (int64_t *)
                malloc(sizeof(size_t) * cache_count);
                
        for (size_t i = 0; i < page_count; i++) 
            last_access_ins[count][i] = -NEIGBOUR_INS_SIZE;
        for (size_t i = 0; i < cache_count; i++)
            last_access_ins_cache[count][i] = -NEIGBOUR_INS_SIZE;
        for (size_t i = 0; i < page_count; i++)
            references_perpage[count][i] = 0;

        var_addr[count] = ret;
        last_access_bytes[count] = ret;   ///// Why address comes here?
                                        ///// -> To collect the offsets it accessed (used in sequentiality check)
        start_num_inst[count] = ins_count;
        
        InitVariable(count);
        //UpdateHash((size_t) ret, (size_t) G_var_size);
        
        count++;

    }
}

VOID RecordMemRead(VOID * ip, VOID * addr, VOID * size)
{
    //fprintf(trace,"%p: R %p\n", ip, addr);
    //TraceFile << ip << " R " << addr << std::endl;  
    /*
    int obj = Hash((size_t) addr);
    if (obj != -1) 
        GetMetrics(addr, obj, 0);
    */
    UpdateTotalOps();
    UpdateTotalVolume((size_t) size);

#ifdef READ_ADDR_FILE
    if (in_alloc == 1)
        TestAndGetAddr();
#endif

    for (uint64_t i = 0; i < count; i++) {
        if ((int64_t)addr >= var_addr[i] && 
            (int64_t)addr < var_addr[i] + var_size[i]) {
            //int s = Hash((size_t) addr);
            //if (s != (int) i) cout << "Addr Error!" << endl << s << " " << i << endl, Debug((size_t) addr);
            GetMetrics(addr, (size_t) size, i, 0);
            break;
        }
    }
    
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, VOID * size)
{
    /*
    int obj = Hash((size_t) addr);
    if (obj != -1) 
        GetMetrics(addr, obj, 1);
    */
    UpdateTotalOps();
    UpdateTotalVolume((size_t) size);
    

#ifdef READ_ADDR_FILE
    if (in_alloc == 1) {
        TestAndGetAddr();
    }
#endif

    for (uint64_t i = 0; i < count; i++) {
        if ((int64_t)addr >= var_addr[i] &&
            (int64_t)addr < var_addr[i] + var_size[i]) {
            GetMetrics(addr, (size_t) size, i, 1);
            break;
        }
    }
}

#endif
