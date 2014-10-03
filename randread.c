/*
    Crude program to torture tesk a disk with multi-threaded random reads.

    Build: gcc -o randread randread.c -lpthread

    License: public domain
*/

#define _GNU_SOURCE

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ALIGN 4096
#define BLOCK 65536  // 64KB

const char* fileName;
off_t endOffset;

void * doRandomReads(void *arg);

int main(int argc, char** argv) {
    int devFd;
    int numThreads;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <device file> <threads>\n", argv[0]);
        return 1;
    }
    fileName = argv[1];
    numThreads = atoi(argv[2]);

    devFd = open(fileName, O_RDONLY);
    if (devFd == -1) {
        perror("Failed to open file");
        return 1;
    }

    endOffset = lseek(devFd, 0, SEEK_END);    
    printf("File size is %ld\n", endOffset);

    close(devFd);

    for (long i = 0; i < numThreads; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, doRandomReads, (void *)i)) {
            perror("Error creating pthread");
            return 1;
        }
    }

    while (1) {
        sleep(1);
    }
}

void * doRandomReads(void *arg) {
    long threadId;
    int devFd;
    off_t offset;
    struct timespec currTime;
    char *buf;
    size_t readLen;

    threadId = (long)arg;
    printf("Started thread %ld\n", threadId);

    if (posix_memalign((void **)&buf, ALIGN, BLOCK)) {
        fprintf(stderr, "Thread %ld - Failed to allocate memory\n", threadId);
        exit(1);
    }

    devFd = open(fileName, O_RDONLY | O_DIRECT);
    if (devFd == -1) {
        fprintf(stderr, "Thread %ld - Failed to open file: %s\n", threadId,
            strerror(errno));
        perror("Failed to open file");
        exit(1);
    }

    clock_gettime(CLOCK_MONOTONIC, &currTime);
    srandom(currTime.tv_nsec);

    uint64_t count = 0;
    while(1) {
        offset = random() % (endOffset - sizeof(buf));
        // Align to 4096
        offset = (offset + ALIGN - 1) & ~(ALIGN - 1);
        offset = lseek(devFd, offset, SEEK_SET);

        readLen = read(devFd, buf, BLOCK);
        if (readLen == -1) {
            fprintf(stderr, "Thread %ld - Read at offset %ld failed: %s\n",
                threadId, offset, strerror(errno));
            //exit(1);
        } else {
            count++;
            if (count % 10 == 0) {
                printf("Thread %ld\t%ld reads.\n", threadId, count);
            }
        }
    }
}

