#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

#include "blob.h"

char *get_data_dir();
void get_blob_path(char *name, char *path);
void get_indices_path(char *name, char *path);
void get_keys_path(char *name, char *path);
void get_subpath(char *name, char *path, char *p1, char *p2, char *p3);

int directory_exists(char *path);

int create_directory(char *path);

char *get_named_argument(int argc, char *argv[], char c, char *def);
char *get_positional_argument(int argc, char *argv[], int pos, char *def);

void write_to_file(FILE *file, void *ptr, size_t bytes);
void write_to_file_as_type(FILE *file, val_accum_t val, int type, int width);

#endif
