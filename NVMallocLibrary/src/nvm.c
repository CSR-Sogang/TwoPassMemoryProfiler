#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

#include <fcntl.h>
#include "nvm.h"

bool var_hashvalue_flag;
size_t global_hashvalue_flag;

void check_nvm_device(char *str)
{
	struct stat st;
	char message[256];
	if (stat(str, &st) != 0 ) {
		sprintf (message, "NVM sandbox %s(for memory mapped files) does not exist\n", str);
		perror (message);
	}
		
}

void nvm_init() {
	read_ssd_info();
    read_placement_info();

	create_name_space();

    var_hashvalue_flag = false;
}

void nvm_exit(){
	release_ssd_info();

	destory_name_space();
}


///////////////////////////////////////////////////////////////////
// nvm_malloc
///////////////////////////////////////////////////////////////////

#define size_align 4096

#define	roundsize(x)	((size_t)( (size_align - 1 + (x)) & (~((size_t)(size_align - 1))) ))

//#undef malloc
void *nvm_malloc(size_t size)
{
    if ( size == 0 ) return NULL;
    
    size_t variable_size = roundsize(size);
   
    (void) check_nvm_device(ssd_head->mountdir); 
    (void) check_nvm_device(ssd_tail->mountdir);
    
    unsigned int idx = get_file_index();
    
    //At most, we have 2 layers now.
	char f_name[80];

    if (var_layers[idx].layer == 1) {
        printf("variable %d map on the layer 1 %s\n", idx, ssd_head->mountdir);
        sprintf(f_name, "%s%s%d", ssd_head->mountdir, "/file_", idx); 
    }
    else {
        printf("variables %d map on the layer 2 %s\n", idx, ssd_tail->mountdir);
        sprintf(f_name, "%s%s%d", ssd_tail->mountdir, "/file_", idx); 
    }
    
    // check if a file exists 
    if ( access ( f_name, F_OK ) != -1 ) { perror ("File exist !!!"); printf("%s\n", f_name); exit(0); }
    
    int fd = open (f_name, O_RDWR | O_CREAT, 0666);
    if (fd == -1){
		perror("File open failed\n");
		exit(0);
	}
	
    posix_fallocate(fd, 0, variable_size);
    
	ftruncate(fd, variable_size);
    
	close(fd);
    
	fd = open(f_name, O_RDWR);
    
	int prot = PROT_WRITE | PROT_READ;
    //prot |= ~((int) 7);
    
	void *addr = mmap (NULL, variable_size, prot, MAP_SHARED, fd, 0);
    
	if (addr == MAP_FAILED) { perror("Failed mmap"); exit(0); }
    
	close(fd);
    
	// register variable in name space 
	register_name_space( (long int) addr, fd, f_name, variable_size);
    
	return (void *) (addr);
}
//#define malloc nvm_malloc


///////////////////////////////////////////////////////////////////
// nvm_calloc
///////////////////////////////////////////////////////////////////

//#undef calloc
void *nvm_calloc(size_t nmemb, size_t size)
{
	void *addr = nvm_malloc(nmemb * size);
	memset(addr, 0, nmemb * size);

	if (addr == NULL) {
		printf("(nvm_calloc()), addr = NULL\n");
		exit(0);
	}

	return addr;
}
//#define calloc nvm_calloc

///////////////////////////////////////////////////////////////////
// nvm_realloc 
///////////////////////////////////////////////////////////////////

//#undef malloc 
void *nvm_realloc(void *var, size_t size)
{
	if ( var == NULL ) {

		void *addr = nvm_malloc(size);

		if (addr == NULL) {
			printf("(nvm_malloc()), addr = NULL\n");
			exit(0);
		}

		return addr;
	}
	else if ((var != NULL) && (size == 0)) {
		nvm_free(var);
		return;
	}

	size_t variable_size = roundsize(size);

	// check nvm sandbox	
	(void) check_nvm_device(ssd_head->mountdir); // by default, use the first SSD


	char *file_name = (char *) malloc ( sizeof(char) * 80 );

	(void) return_file_name_to_addr ( (long int) var, file_name);

	int fd = open (file_name, O_RDWR, 0666);
	off_t file_size = lseek (fd, 0, SEEK_END);

	munmap ( (void *) var, file_size );

	if ( variable_size > file_size) {

		posix_fallocate(fd, file_size, (variable_size - file_size));

		file_size = lseek (fd, 0, SEEK_END);

	}
	else {
		ftruncate(fd, (variable_size));

		file_size = variable_size;
	}
	
	close(fd);

	fd = open(file_name, O_RDWR);

	void* var_addr = mmap(NULL, file_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);

	if (var_addr == MAP_FAILED) {
		printf("mmap failed. file_name:%s\n", file_name);
		perror ("mmap failed.\n"); exit(0);
	}

	close(fd);

	update_name_space( file_name, file_size, (long int) var_addr );

	return var_addr;
}
//#define malloc nvm_malloc

///////////////////////////////////////////////////////////////////
// nvm_free
///////////////////////////////////////////////////////////////////

//#undef free
int nvm_free(void *ptr)
{
	if ( ptr == NULL ) return;

	var_info *tmp;

	size_t variable_size = return_variable_size_to_addr ( (long int) ptr);

    //printf("Free NVM varialbe\n");

	if (variable_size == 0) {
		return 0;
	}

	if (variable_size == -1) {
		perror("nam_free tries to free a memory not allocated before\n");
		exit(0);		
	}
	else{
		//printf("ptr, variable: %x, %lu\n", ptr, variable_size);
		if(munmap(ptr, variable_size) != 0){
	
		printf("ptr, variable: %x, %lu\n", ptr, variable_size);
			perror("error in munmap\n");
			exit(0);
		}

		// de-redister variable
		tmp = unregister_name_space ( (long int) ptr);

		unlink(tmp->fname);
		return 1;
	}
}
//#define free nvm_free

///////////////////////////////////////////////////////////////////
// Auxiliary functions
///////////////////////////////////////////////////////////////////
//FIXME
unsigned int f_idx = 0;

int get_file_index()
{
    if (var_hashvalue_flag) {
        //FIXME we need a hashfunction here.
    }
    else {  
        f_idx++;	
        
        if (f_idx > MAX_FILE_INDEX_LIMIT) {
            perror("Usuable file names are used up!");
            exit(0);		
        }
        
        return f_idx;
    }
}
