#include "src/calltrack.h"
#include <stdlib.h>
#include <stdio.h>

int main(){

	char *a, *b;
	
	int i;

	for(i = 1; i < 6; i++){
		a = malloc(1024*i);
		b = calloc(2, 1024*i);
		a = realloc(a, 3*1024*i);
		free(b);
		free(a);
	}

	return 0;
}

