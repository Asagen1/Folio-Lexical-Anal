#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(){
    uint8_t size = 200;
    FILE *fptr = fopen("folio_input.txt", "r");
    char *input = (char*)malloc(size+1 * sizeof(char));
    
    if(fptr == NULL){
        perror("There is no file...");
        free(input);
        return 1;
    }

    if(input == NULL){
        perror("There is no memory...");
        return 1;
    }

    size_t bytes_read = fread(input, sizeof(char), size, fptr);
    input[bytes_read] = '\0';

    printf("File content: \n%s", input);

    fclose(fptr);
    free(input);
    return 0;
}