#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#define MAXFILESIZE 9999999

static char file[MAXFILESIZE];
static int fileSize = 0;
static int blockSize = 0;
static char searchString[80];
static int strLength = 0;

struct tentry {
    int matchCount;
    char *filePtr;
};

void *stringSearch(void *tdata) {
    char *nptr;
    struct tentry *tptr = (struct tentry *) tdata;
    nptr = strstr(tptr->filePtr, searchString);
    while( (nptr != NULL) && (nptr < (tptr->filePtr + blockSize + strLength/2))) {
        tptr->matchCount++;
        nptr = nptr + strLength;
        nptr = strstr(nptr, searchString);
    };
}

int main(int argc, char * argv[]) {
    int j = 1;
    for (j = 1; j <= 200; j ++) {
        int i = 0;
        int numOfThreads = j;
        pthread_t thread_no[numOfThreads];
        struct tentry thread_info[numOfThreads];
        int matchCount = 0;
        struct timeval start, end;
        long runningTime;

        if(argc != 2){
            printf("Usage: %s \"search_string\" < filename\n", argv[0]);
            return 1;
        }

        strLength = strlen(argv[1]);
        strncpy(searchString, argv[1], strLength);

        while( (file[fileSize] = getc(stdin)) != EOF){
            fileSize++;
        }

        if (fileSize < numOfThreads) {
            return 0;
        }
        gettimeofday(&start, NULL);
        blockSize = (int) (fileSize/numOfThreads);
        for(i = 0; i < numOfThreads; i++){
            thread_info[i].filePtr = &file[i*blockSize];
            thread_info[i].matchCount = 0;
            pthread_create(&thread_no[i], NULL, stringSearch, (void *)&thread_info[i]);
        }
        for(i = 0; i < numOfThreads; i++){
            pthread_join(thread_no[i], NULL);
            matchCount = matchCount + thread_info[i].matchCount;
        }
        gettimeofday(&end, NULL);
        runningTime = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;
        printf("blocksize: %d\tnumofThread: %d\tmatchCount: %d\trunningTime: %ld\n", blockSize, numOfThreads, matchCount, runningTime);
        //printf("%ld\n", runningTime);
    }
    return 0;
}
