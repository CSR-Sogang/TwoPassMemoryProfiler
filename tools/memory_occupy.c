/* Authoer: Xu Ji 
   Need sudo.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>

const uint64_t GB_size = 1024 * 1024 * 1024;

int main(int argc, const char *argv[])
{
    if (argc < 2) {
        perror("Usage: GBs to occupy.\n");
    }

    uint64_t GBs = atoi(argv[1]);

    printf("Gbs %d\n", GBs);
    
    char *memory = (char*) malloc(GBs * GB_size);

    if (memory == NULL) {
        perror("Memory alloc error\n");
        return 0;
    }
    
    size_t page_size = getpagesize();
    
    for (size_t i = 0; i < GBs * GB_size; i += page_size) {
        memory[i] = 0;
    }

    //printf("here\n");

    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        perror("mlock error\n");
        return 0;
    }

    while (1) {
        sleep(1);
    }
    
    return 0;
}


