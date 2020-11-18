#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <signal.h>

#include "common.h"

bool quit = false;

void handle_data(const struct data* din, struct data* dout) {
    memset(dout->str, 0, 64);
    dout->len = 0;
    switch (din->op) {
        case OP_ECHO:
            memcpy(dout->str, din->str, din->len);
            dout->len = din->len;
            break;
        case OP_UCASE:
            for (size_t i = 0; i < din->len; i++) {
                dout->str[i] = toupper(din->str[i]);
            }
            dout->len = din->len;
            break;
        case OP_LCASE:
            for (size_t i = 0; i < din->len; i++) {
                dout->str[i] = tolower(din->str[i]);
            }
            dout->len = din->len;
            break;
        case OP_QUIT:
            quit = true;
            break;
        default:
            snprintf(dout->str, 64, "ERR: UNKNOWN OP");
            dout->len = 15;
            break;
    }
}

void sigdie(int sig) {
    (void)sig;
    shm_unlink(MEM_NAME);
    exit(0);
}

int main(void) {
    signal(SIGINT, sigdie);

    int fd;

    // open the shared mem
    fd = shm_open(MEM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("shm_open");
        exit(1);
    }

    // we have the memory; now we can truncate it to size
    if (ftruncate(fd, sizeof(struct shmdata)) < 0) {
        perror("ftruncate");
        exit(1);
    }

    // map the memory to our structure
    struct shmdata* sd = mmap(NULL, sizeof(*sd), PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, 0);
    if (sd == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // initialize semaphores
    if (sem_init(&sd->sem1, 1, 0) < 0) {
        perror("sem_init: sem1");
        exit(1);
    }
    if (sem_init(&sd->sem2, 1, 0) < 0) {
        perror("sem_init: sem2");
        exit(1);
    }

    while (!quit) {
        if (sem_wait(&sd->sem1) < 0) {
            perror("sem_wait");
            exit(1);
        }

        handle_data(&sd->din, &sd->dout);

        if (sem_post(&sd->sem2) < 0) {
            perror("sem_post");
            exit(1);
        }
    }

    if (shm_unlink(MEM_NAME) < 0) {
        perror("shm_unlink");
        exit(1);
    }

    return 0;
}
