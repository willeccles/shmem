#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <semaphore.h>

#define MEM_NAME "/testmem"

// operations for the daemon to perform
#define OP_ECHO     0x0
#define OP_UCASE    0x1
#define OP_LCASE    0x2
#define OP_QUIT     0xFF

struct data {
    uint8_t op;
    size_t len;
    char str[64];
};

struct shmdata {
    sem_t sem1; // master -> daemon data is ready
    sem_t sem2; // daemon -> master data is ready
    struct data din;    // master -> daemon data
    struct data dout;   // daemon -> master data
};

#endif
