#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#include "util.h"

#define _to_string( x ) #x
#define to_string(x) _to_string(x)

char *get_data_dir() {
    char *env = getenv("XANDRI_DIR");
    if (env == 0) {
        return to_string(__DIR__) "/data";
    } else {
        return env;
    }
}

void get_blob_path(char *name, char *path) {
    path[0] = 0;
    char *data_dir = get_data_dir();
    strcat(path, data_dir);
    strcat(path, "/");
    strncat(path, name, PATH_MAX - strlen(data_dir) - 2);
}

void get_keys_path(char *name, char *path) {
    get_blob_path(name, path);
    strcat(path, "/keys");
}

void get_indices_path(char *name, char *path) {
    get_blob_path(name, path);
    strcat(path, "/indices");
}

// TODO Fix this monstrosity, sprintf is a thing lol
void get_subpath(char *name, char *path, char *p1, char *p2, char *p3) {
    get_blob_path(name, path);
    strcat(path, "/");
    strcat(path, p1);
    strcat(path, "/");
    strcat(path, p2);
    strcat(path, ".");
    strcat(path, p3);
}

int directory_exists(char *path) {
    DIR *dir = opendir(path);
    if (dir) {
        closedir(dir);
        return 1;
    }
    return 0;
}

int create_directory(char *path) {
    return mkdir(path, 0700);
}

char *get_named_argument(int argc, char *argv[], char c, char *def) {
    for (int i=0; i<argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == c) {
            if (i != argc - 1) return argv[i+1];
            else return def;
        }
    }
    return def;
}

char *get_positional_argument(int argc, char *argv[], int pos, char *def) {
    int cur = 0;
    for (int i=0; i<argc; i++) {
        if (argv[i][0] != '-') {
            if (cur == pos) {
                return argv[i];
            } else {
                cur++;
            }
        } else i++;
    }
    return def;
}

void write_to_file(FILE *f, void *ptr, size_t bytes) {
    if (!fwrite(ptr, bytes, 1, f)) {
        printf("Unable to log to file.\n");
    }
}

void write_to_file_as_type(FILE *f, val_accum_t va, int type, int width) {
    if (type == TYPE_FLOAT) {
        if (width == 4) {
            float val = (float)va;
            if (!fwrite(&val, width, 1, f)) {
                printf("Unable to log to file.\n");
            }
        } else if (width == 8) {
            double val = (double)va;
            if (!fwrite(&val, width, 1, f)) {
                printf("Unable to log to file.\n");
            }
        }
    } else if (type == TYPE_INT) {
        uint64_t val = 0;
        switch (width) {
            case 1: val = (uint8_t)va; break;
            case 2: val = (uint16_t)va; break;
            case 4: val = (uint32_t)va; break;
            case 8: val = (uint64_t)va; break;
        }
        if (!fwrite(&val, width, 1, f)) {
            printf("Unable to log to file.\n");
        }
    }
}
