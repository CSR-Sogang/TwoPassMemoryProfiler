#ifndef _NVMALLOC_H
#define _NVMALLOC_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define MAX_OPEN_FILE_CNT 2048576
#define MAX_FILE_INDEX_LIMIT 2048576 // 1024*1024

#define BUFFER_SIZE 4096

extern void nvm_init();
extern void nvm_exit();

extern void *nvm_malloc(size_t size);
extern void *nvm_calloc(size_t nmemb, size_t size);
extern void *nvm_realloc(void *ptr, size_t size);
extern int nvm_free(void *ptr);

// This is for the file added the include file in the source code
#ifdef USE_NVM
#define malloc nvm_malloc
#define calloc nvm_calloc
#define realloc nvm_realloc
#define free nvm_free
#endif

// for namespace
typedef struct{
	long int addr;
	int fd;	
	//char *fname;
	char fname[80];
	int flag;
	size_t size;
}var_info;

extern var_info var_info_table[MAX_OPEN_FILE_CNT];
extern void create_name_space();
extern void destory_name_space();

void register_name_space( long int addr, int fd, char *fname, size_t size );
var_info *unregister_name_space( long int addr );
var_info *check_reuse_file( size_t var_size);
void return_file_name_to_addr( long int addr, char *fname );
size_t return_variable_size_to_addr( long int addr );
void update_name_space( char *file_name, size_t file_size, long int addr);
void print_all_name_space_info();

// interface for ssd information: start

/*
 * Each SSD device has a section in the config file as follow:
 *
 * title SSD
 *         Name            : Fusion IO
 *         Device          : /dev/fioa
 *         Mounted on      : /tmp/ssd1
 *         File System     : ext4
 *
 */

#define CONFIG_FILE "/home/jixu/DeepMap/NVMallocLibrary/nvm_config"
#define PLACEMENT_FILE "/home/jixu/files_deepmap/placement_file"

struct ssd_info {
	char name[64];
	char device[64];
	char mountdir[64];
	char fs[48]; // length of "fs" could be smaller
	struct ssd_info *prev;
	struct ssd_info *next;
} ssd_info;           // but try to make the size of the struct be 2^x

struct ssd_info *ssd_head;
struct ssd_info *ssd_tail;

extern void read_ssd_info();
extern void release_ssd_info();
// interface for ssd information: end

struct variable_info {
    size_t id;
    size_t hashvalues;
    size_t layer;
} var_layers
[MAX_FILE_INDEX_LIMIT];

extern void read_placement_info();

#endif
