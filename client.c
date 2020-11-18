#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "common.h"

#define LEN_MIN(a,b) ((a)<(b)?(a):(b))

void handle_command(struct shmdata* sd, uint8_t op, char* str, size_t len) {
    sd->din.op = op;
    if (op == OP_QUIT) {
        sd->din.len = 0;
    } else {
        sd->din.len = LEN_MIN(len, 64);
        memcpy(sd->din.str, str, LEN_MIN(len, 64));
    }

    // tell the daemon we have data for it
    if (sem_post(&sd->sem1) < 0) {
        perror("sem_post");
        exit(1);
    }

    // wait for it to tell us data is ready
    if (sem_wait(&sd->sem2) < 0) {
        perror("sem_wait");
        exit(1);
    }

    if (sd->dout.len) {
        printf("daemon responded: ");
        fwrite(sd->dout.str, 1, sd->dout.len, stdout);
        printf("\n");
    }
}

int main(int argc, char** argv) {
    if (argc == 1) {
        fprintf(stderr, "usage:\n"
                "    %s -q\n"
                "    %s [-ule] message\n\n"
                "      q: quit the daemon (other args will be ignored)\n"
                "      u: uppercase the message\n"
                "      l: lowercase the message\n"
                "      e: echo the message\n",
                argv[0], argv[0]);
        return 1;
    }

    int fd = shm_open(MEM_NAME, O_RDWR, 0);
    if (fd < 0) {
        perror("shm_open");
        exit(1);
    }

    struct shmdata* sd = mmap(NULL, sizeof(*sd), PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, 0);
    if (sd == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    bool quit = false;

    if (strchr(argv[1], 'q') != NULL) {
        printf("quitting daemon...\n");
        quit = true;
    }

    if (!quit) {
        if (argc < 3) {
            fprintf(stderr, "not enough arguments\n");
            exit(1);
        }

        for (char* c = argv[1]; *c; c++) {
            switch (*c) {
                case 'l':
                    handle_command(sd, OP_LCASE, argv[2], strlen(argv[2]));
                    break;
                case 'e':
                    handle_command(sd, OP_ECHO, argv[2], strlen(argv[2]));
                    break;
                case 'u':
                    handle_command(sd, OP_UCASE, argv[2], strlen(argv[2]));
                    break;
                case '-':
                    break;
                default:
                    fprintf(stderr, "unknown flag: %c\n", *c);
                    break;
            }
        }
    } else {
        handle_command(sd, OP_QUIT, NULL, 0);
    }

    return 0;
}
