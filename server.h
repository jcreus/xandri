#ifndef SERVER_H
#define SERVER_H

#include "blob.h"

#define MAX_FRAMES 16
#define BUFSIZE 1024

typedef struct index_iter {
    index_files_t data;
    char *name;
    struct index_iter *next;
} index_iter;

typedef struct {
    index_iter *index;
    int nkeys;
    char **keys;
} frame_t;

int serve_cmd(int argc, char *argv[]);

#endif
