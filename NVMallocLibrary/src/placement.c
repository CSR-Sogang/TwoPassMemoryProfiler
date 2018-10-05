#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nvm.h"


void read_placement_info() {
    FILE *fp;
    char *line = NULL;
    char *str;
    size_t len = 0;
    size_t index, layer;
    size_t hashvalue;

    fp = fopen(PLACEMENT_FILE, "r");
    if (fp == NULL) {
        printf("No placement file found! All varialbes are placed on the first SSD...\n");
        return;
    }

    while (getline(&line, &len, fp) != -1){
        if (strstr("Hashvalue", line)) {
            str = strstr(":", line);
            str++;
            sscanf(str, "%zu", &hashvalue);
            var_layers[index].hashvalues = hashvalue;
        } else {
            sscanf(line, "%zu %zu", &index, &layer);
            var_layers[index].layer = layer;
        }
    }
        
    free(line);
    fclose(fp);	
    
}
