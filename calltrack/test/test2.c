#include <stdio.h>
#include <stdlib.h>

#include "src/nvm.h"

#define SIZE 4096 * 128
int *a;
int *b;


int main() {
    int loc = 0;
    int i, tmp;
    nvm_init();
    srand( 0 );
    a = malloc( SIZE * sizeof(int) );
    b = malloc( SIZE * sizeof(int) );
    printf("after malloc\n");
    for ( i = 0; i < 1000; i++ ) {
        printf("i :%d\n", i);
        loc = rand()%SIZE;
        if ( i%2 ) {
            a [loc] = 1.0;
            printf("Write @ %d\n", loc * sizeof(int) / 4096);
        } else {
            tmp = a[loc];
            printf("Read @ %d\n", loc * sizeof(int) / 4096);
        }
    }
    free(a);
    free(b);
    nvm_exit();
    return 0;
}
