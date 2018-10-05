#ifndef _CALLTRACK_H
#define _CALLTRACK_H

#include <stdlib.h>
#include <stdio.h>

extern void *calltrack_malloc(size_t size);
extern void *calltrack_calloc(size_t nmemb, size_t size);
extern void *calltrack_realloc(void *ptr, size_t size);
extern void calltrack_free(void *ptr);

#define malloc calltrack_malloc
#define calloc calltrack_calloc
#define realloc calltrack_realloc
#define free calltrack_free

#endif
