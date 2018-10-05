#include "nvm.h"

var_info var_info_table[MAX_OPEN_FILE_CNT];

void create_name_space()
{
	int i;

	for ( i = 0; i < MAX_OPEN_FILE_CNT; i++ ){
		var_info_table[i].addr = -1;
		var_info_table[i].fd = -1;
		bzero(var_info_table[i].fname, 80 * sizeof(char));
		var_info_table[i].size = -1;
		var_info_table[i].flag = 0;
	}
}

void destory_name_space()
{
	int i;

	for ( i = 0; i < MAX_OPEN_FILE_CNT; i++ ) {
		if (var_info_table[i].fd != -1) {
			close(var_info_table[i].fd);
        	unlink (var_info_table[i].fname );
		}
	}
}

int find_empty_slot()
{
	int i; 
	for ( i = 0; i < MAX_OPEN_FILE_CNT; i++ ) {
		if ( var_info_table[i].flag == 0 ) return i;
	}
	return -1;
}

void update_name_space( char *file_name, size_t file_size, long int addr)
{
	int i;
	int find_flag = 0;

	for (i = 0; i < MAX_OPEN_FILE_CNT; i++)
	{
		if ( ! strcmp (file_name, var_info_table[i].fname) ) break;
	}
	
	if (i == MAX_OPEN_FILE_CNT)
		printf("Warning in the update_name_space\n");	
	
	var_info_table[i].size = file_size;
	var_info_table[i].addr = addr;
}



void return_file_name_to_addr ( long int addr, char *fname )
{
	int i;
	int find_flag = 0;

	for (i = 0; i < MAX_OPEN_FILE_CNT; i++)
	{
		if ( var_info_table[i].addr == addr ) {
			find_flag = 1;
			break;
		}
	}
	
	if ( ( find_flag == 0 ) && ( i == MAX_OPEN_FILE_CNT ) ) {
		printf("return_file_name_to_addr(): file not found for addr: %ld, filename: %s\n", addr, fname);
		exit(0);
	}
	
	sprintf(fname, "%s", var_info_table[i].fname);
}

size_t return_variable_size_to_addr ( long int addr)
{
	int i;
	int find_flag = 0;

	for (i = 0; i < MAX_OPEN_FILE_CNT; i++)
	{
		if ( var_info_table[i].addr == addr ) {
			find_flag = 1;
			break;
		}
	}
	
	if ( ( find_flag == 0 ) && ( i == MAX_OPEN_FILE_CNT ) ) {
		printf("return_variable_size_to_addr(): file not found for addr: %p\n", (void*) addr);
		return 0;	
	}

	return (size_t) var_info_table[i].size;
}

void register_name_space( long int addr, int fd, char *fname, size_t size )		
{
	int i = find_empty_slot();

	if ( i == -1 ){
		printf("Name space is full!\n");
		exit(0);
	}
	
	var_info_table[i].addr = addr;
	var_info_table[i].fd = fd;
	sprintf(var_info_table[i].fname, "%s", fname);
	var_info_table[i].size = size;
	var_info_table[i].flag = 1;
}

var_info *unregister_name_space( long int addr )
{
	int i ;
	for ( i = 0; i < MAX_OPEN_FILE_CNT; i++ ){
		if (var_info_table[i].addr == addr )
			break;
	}
	if (i == MAX_OPEN_FILE_CNT) 
		printf("Warning in unregister_name_space\n");
	var_info_table[i].flag = 0;

	return &var_info_table[i];
}
