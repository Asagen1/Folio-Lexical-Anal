#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "lexical_analyzer.h"

#define MAX_STATES 256

static char *read_file(const char *file);

void lexer(const char *string){
    char *input = read_file(string);

    if(input == NULL){
        return;
    }

    free(input);
}

static char *read_file(const char *file){ 
    FILE *fptr = fopen(file, "r");
    if (!fptr){
        perror(file); 
        return NULL; 
    }

    size_t cap = 256, len = 0;
    char *input = malloc(cap);
    if (!input){
        fclose(fptr); 
        return NULL; 
    }

    int c;
    while ((c = fgetc(fptr)) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            char *tmp = realloc(input, cap);
            if (!tmp) { free(input); fclose(fptr); return NULL; }
            input = tmp;
        }
        input[len++] = (char)c;
    }
    input[len] = '\0';

    fclose(fptr);
    return input;
}
