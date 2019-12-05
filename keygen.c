#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char* keygen(int keylen){
    // Seed rng
    srand(time(0));
    char* key = malloc(sizeof(char[keylen+1]));
    // Generate random letters
    int k;
    for(k = 0; k < keylen; k++){
        int lett = (rand() % 27)+64;
        if(lett==64){
            lett = 32;
        }
        key[k]=lett;
    }
    // Append newline
    key[k]='\n';
    return key;
}

int main(int argc, char* argv[]){
    // If only one argument
    if(argc==2){
        // Convert keylength to integer
        int i = atoi(argv[1]);
        // Output random key
        if(i>0){
            char* key = keygen(i);
            // Key to stdout
            printf("%s",key);
            free(key);
            return 0;
        }
        // ERROR #2
        // Bad keylength
        else{
            fprintf(stderr, "Bad keylength: %s\n", argv[1]);
            return 2;   
        }
    }
    // ERROR #1
    // Else syntax error
    else{
        fprintf(stderr, "Syntax Error!\nUsage: %s keylength\n", argv[0]);
        return 1;
    }
}