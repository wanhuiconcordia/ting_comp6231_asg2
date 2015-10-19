#include <math.h>
#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include <unistd.h>


#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 5 * 1024 * 1024

typedef struct{
    char letter;
    int count;
}LetterCountPair;

char* readFile(char* fileName){
    char* buffer = NULL;
    FILE * pFile = fopen (fileName , "r" );


    if (pFile == NULL) {
        perror ("");
    }else{
        // obtain file size:
        fseek (pFile , 0 , SEEK_END);
        size_t lSize = ftell (pFile);
        rewind (pFile);
        // allocate memory to contain the whole file:
        buffer = (char*) calloc (lSize + 1, sizeof(char));
        if (buffer == NULL) {
            perror ("");
        }else{
            // copy the file into the buffer:
            size_t result = fread (buffer, 1, lSize, pFile);


            if (result != lSize) {
                free (buffer);
                buffer = NULL;
                perror ("");
            }
            buffer[lSize] = 0;
            /*else the whole file is now loaded in the memory buffer.*/
        }
        fclose (pFile);
    }
    return buffer;
}




void calcDispls(int unitCount, int unitSize, int n, int sendcounts[],
                int displs[]){
    int rem = unitCount % n;
    int defaultSize = unitCount / n;
    int sum = 0;
    for(int i = 0; i < n; i++){
        sendcounts[i] = defaultSize * unitSize;
        if(rem){
            sendcounts[i] += unitSize;
            rem--;
        }
        displs[i] = sum;
        sum += sendcounts[i];
    }
}

void printLetterCountPair(LetterCountPair *p){
    printf("%c\t%d\n", p->letter, p->count);
}

void printLetterCountPairArray(LetterCountPair (*pArr)[26]){
    for(int i = 0; i < 26; i++){
        printLetterCountPair(pArr[0] + i);
    }
}

void printNLetterCountPairArray(LetterCountPair (*pArr)[26], int n){
    for(int i = 0; i < n; i++){
        printLetterCountPairArray(pArr + i);
    }
}


int main(int argc, char **argv) {
    int size, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int sendcounts[size];
    int displs[size];


    LetterCountPair globalStep1LetterCountPair[26];
    LetterCountPair step1LetterCountPair[size][26];
    LetterCountPair step2LetterCountPair[size][26];
    memset(step1LetterCountPair, 0, size * 26 * sizeof(LetterCountPair));
    memset(step2LetterCountPair, 0, size * 26 * sizeof(LetterCountPair));
    memset(globalStep1LetterCountPair, 0, sizeof(LetterCountPair) * 26);

    for(int i = 0; i < 26; i++){
        globalStep1LetterCountPair[i].letter = 'A' + i;
        globalStep1LetterCountPair[i].count = (rank + 1) * 100;
    }

    if(rank == 0){
        printf("Processor %d is init with data:\n", rank);
        printLetterCountPairArray(&globalStep1LetterCountPair);
    }
    calcDispls(26, sizeof(LetterCountPair), size, sendcounts, displs);

    for(int i = 0; i < size; i++){
        MPI_Scatterv(globalStep1LetterCountPair, sendcounts, displs, MPI_CHAR, step1LetterCountPair[i], sizeof(LetterCountPair) * 26, MPI_CHAR, i, MPI_COMM_WORLD);
    }
    //    printf("Processor %d got data:\n", rank);
    //    printNLetterCountPairArray(step1LetterCountPair, size);

    for(int k = 1; k < size; k++){
        for(int i = 0; i < 26; i++){
            step1LetterCountPair[0][i].count += step1LetterCountPair[k][i].count;
        }
    }

    if(rank == 0){
        printf("Total step1 result:\n");
        printLetterCountPairArray(step1LetterCountPair);
    }

    MPI_Gather(step1LetterCountPair[0], sizeof(LetterCountPair) * 26, MPI_CHAR, step2LetterCountPair, sizeof(LetterCountPair) * 26, MPI_CHAR, 0, MPI_COMM_WORLD);

    if(rank == 0){
        printf("Final result:\n");
        printNLetterCountPairArray(step2LetterCountPair, size);
    }
    MPI_Finalize();
    return 0;
}
