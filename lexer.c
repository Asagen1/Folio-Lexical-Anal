#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "lexical_analyzer.h"

void read_file(const char *string){
    uint8_t size = 200;
    FILE *fptr = fopen(string, "r");
    char *input = (char*)malloc(size+1 * sizeof(char));
    
    if(fptr == NULL){
        perror("There is no file...");
        free(input);
        return;
    }

    if(input == NULL){
        perror("There is no memory...");
        return;
    }

    size_t bytes_read = fread(input, sizeof(char), size, fptr);
    input[bytes_read] = '\0';

    printf("File content: \n%s", input);

    fclose(fptr);
    free(input);
}

