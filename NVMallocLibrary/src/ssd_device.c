#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nvm.h"

//#undef malloc
//#undef free
void read_ssd_info() {

	int i;
    
    char *line = NULL;
	char *str;
    size_t len = 0;
	int ssd = 0;
	struct ssd_info *ptr;

	FILE *fp;

    struct stat st;
    if (!stat(CONFIG_FILE, &st) == 0){
        perror ("The SSD config file does not exist.\n");
	}
    
    ssd_head = NULL;
    ssd_tail = NULL;
    
	fp = fopen(CONFIG_FILE, "r");
    if(fp == NULL) perror ("Open ERROR with the SSD config file.\n");
    
	while (getline(&line, &len, fp) != -1){
		if(strstr(line, "title")){
section:		if(strstr(line, "title SSD")){
				ptr = malloc(sizeof(struct ssd_info));
				if(ptr == NULL) perror ("Out of space.\n");

				if(ssd_head == NULL) {
					ssd_head = ptr;
					ssd_tail = ptr;
					ptr->prev = NULL;
					ptr->next = NULL;
				} else {
					
					ptr->prev = ssd_tail;
					ptr->next = NULL;
					ssd_tail->next = ptr;
					ssd_tail = ptr;
				}

				while (getline(&line, &len, fp) != -1){
					if(strstr(line, "title")){
						goto section;
					}

					if(strstr(line, "Name")){
						str = strstr(line, ":");
						str++;
						while(isspace(*str)) str++;
						strcpy(ptr->name, str);
					} else if(strstr(line, "Device")){
						str = strstr(line, ":");
                        str++;
                        while(isspace(*str)) str++;
                        strcpy(ptr->device, str);
                    } else if(strstr(line, "Mounted on")){
						str = strstr(line, ":");
                        str++;
                        while(isspace(*str)) str++;
                        strcpy(ptr->mountdir, str);
                    } else if(strstr(line, "File System")){
						str = strstr(line, ":");
                        str++;
                        while(isspace(*str)) str++;
                        strcpy(ptr->fs, str);
                    }
                    
                    for(i = 0; i < 80; i++){
                        if(ptr->name[i] == '\n') ptr->name[i] = '\0';
                        if(ptr->device[i] == '\n') ptr->device[i] = '\0';
                        if(ptr->mountdir[i] == '\n') ptr->mountdir[i] = '\0';
                        if(ptr->fs[i] == '\n') ptr->fs[i] = '\0';
                    }

				}
			}
		}
	}

        free(line);
        fclose(fp);	

}

void release_ssd_info() {

    while(ssd_head != NULL){
		if(ssd_head->next == NULL){
			free(ssd_head);
			ssd_head = NULL;
			break;
		} else {
            ssd_head = ssd_head->next;
			free(ssd_head->prev);
		}
    }
}

//#define malloc nvm_malloc
//#define free nvm_free

