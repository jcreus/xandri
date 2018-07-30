#ifndef BLOB_H
#define BLOB_H

#include <stdint.h>

#define MAX_SUMMARIES 4
#define SUMMARY_MULT 256

// TODO: yikes, stop being O(n)
#define MAX_INDICES 16

#define TYPE_INT 0
#define TYPE_FLOAT 1

typedef uint64_t accum_t;
typedef double val_accum_t;

static const long SUMMARY_SIZES[MAX_SUMMARIES] = {32, 256, 2048, 16384};

typedef struct __attribute__((packed)) {
	int type;
	int width;
	int index_length;
	/* index name, not zero terminated */
} value_t;

typedef struct __attribute__((packed)) {
	long length;
	int width;
	int num_summaries;
    long num_points[MAX_SUMMARIES + 1];
} index_t;

typedef struct __attribute__((packed)) {
    index_t info;
    int fds[MAX_SUMMARIES+1];
    void *ptr[MAX_SUMMARIES+1]; 
} index_files_t;

typedef struct {
	int zoom;
	long low;
	long high;
} index_query_t;

int blob_get_num_summaries(long length);

int blob_print_help();
int blob_create(char *name);
int blob_index_from_file(char *name, char *key, char *path, int width, int use_summary);
int blob_cmd(int argc, char *argv[]);
void blob_query_index(index_files_t *idx, unsigned long low, unsigned long high, long points, index_query_t *result);
index_files_t *blob_open_index(char *name, char *key);

#endif
