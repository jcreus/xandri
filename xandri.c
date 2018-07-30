#include <stdio.h>
#include <string.h>

#include "server.h"
#include "blob.h"
#include "util.h"

int print_help(int argc, char *argv[]) {
    if (argc == 0) {
        goto standard;
    } else if (strcmp(argv[0], "blob") == 0) {
        printf("help for blob\n");
    } else {
        goto standard;
    }
    return 0;

standard:
    printf("Usage: x [command] [command arguments]\n");
    return 0;
}

int main(int argc, char *argv[]) {
    char *data_dir = get_data_dir();
    if (!directory_exists(data_dir)) {
        create_directory(data_dir);
    }

    if (argc == 1) {
        return print_help(0, NULL);
    } else if (strcmp(argv[1], "help") == 0) {
        return print_help(argc-2, argv+2);
    } else if (strcmp(argv[1], "blob") == 0) {
        return blob_cmd(argc-2, argv+2);
    } else if (strcmp(argv[1], "serve") == 0) {
        return serve_cmd(argc-2, argv+2);
    } else {
        return print_help(0, NULL);
    }
}
