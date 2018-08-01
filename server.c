#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "server.h"
#include "util.h"

frame_t frames[MAX_FRAMES] = {0};
int num_frames = 0;

index_iter *server_init_indices(char *name) {
    char path[PATH_MAX];
    get_indices_path(name, path);
    DIR *dir = opendir(path);
    if (dir == 0) {
        printf("Unable to open indices directory.\n");
        return 0;
    }
    index_iter *out = 0;
    index_iter *prev;
    struct dirent *entry;
    while ((entry = readdir(dir)) != 0) {
        int len = strlen(entry->d_name);
        if (len >= 5 && strcmp(entry->d_name + len - 5, ".meta") == 0) {
            entry->d_name[len - 5] = 0;
            index_iter *iter = (index_iter*)malloc(sizeof(index_iter));
            if (out == 0) {
                out = iter; // first
                prev = iter;
            }
            prev->next = iter;
            iter->next = 0;
            iter->name = strdup(entry->d_name);
            prev = iter;

            index_files_t *idx = blob_open_index(name, entry->d_name);
            memcpy(&iter->data, idx, sizeof(index_files_t));
            free(idx);
        }
    }
    closedir(dir);
    return out;
}

char **server_init_keys(char *name, int *nkeys) {
    char path[PATH_MAX];
    get_keys_path(name, path);
    DIR *dir = opendir(path);
    if (dir == 0) {
        printf("Unable to open keys directory.\n");
        return 0;
    }
    struct dirent *entry;
    char **out = (char**)malloc(0);
    while ((entry = readdir(dir)) != 0) {
        int len = strlen(entry->d_name);
        if (len >= 5 && strcmp(entry->d_name + len - 5, ".meta") == 0) {
            (*nkeys)++;
            out = (char**)realloc(out, (*nkeys)*sizeof(void*));
            entry->d_name[len - 5] = 0;
            out[*nkeys-1] = strdup(entry->d_name);
        }
    }
    closedir(dir);
    return (char**)out;
}

int server_init_cache() {
    char *data_dir = get_data_dir();
    DIR *dir = opendir(data_dir);
    if (dir == 0) {
        printf("Unable to open data directory.\n");
        return 1;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != 0) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            printf("Found frame %s.\n", entry->d_name);
            index_iter *idx = server_init_indices(entry->d_name);
            if (idx == 0) {
                printf("Unable to initialize indices.\n");
                return 1;
            }
            frames[num_frames].index = idx;
            frames[num_frames].name = strdup(entry->d_name);
            frames[num_frames].keys = server_init_keys(entry->d_name, &frames[num_frames].nkeys);
            num_frames++;
        }
    }
    closedir(dir);
    return 0;
}

void bye(FILE *f, char *num, char *msg) {
    fprintf(f, "HTTP/1.0 %s\n", num);
    fprintf(f, "Content-type: text/html\n");
    fprintf(f, "\n");
    fprintf(f, "Error! %s\n", msg);
}

void process_frames(FILE *f) {
    fprintf(f, "Content-type: application/json\n");
    fprintf(f, "\r\n");
    fprintf(f, "[");
    for (int i=0; i<num_frames; i++) {
        fprintf(f, "\"%s\"", frames[i].name);
        if (i != num_frames - 1) {
            fprintf(f, ", ");
        }
    }
    fprintf(f, "]");
}

void process_keys(FILE *f, char *name) {
    int i;
    for (i=0; i<num_frames; i++) {
        if (strcmp(frames[i].name, name) == 0) {
            break;
        }
    }
    if (i == num_frames) return;
    frame_t frame = frames[i];

    fprintf(f, "Content-type: application/json\n");
    fprintf(f, "\r\n");
    fprintf(f, "{\"indices\": [");
    index_iter *iter = frame.index;
    while (iter != 0) {
        fprintf(f, "{\"name\": \"%s\", \"width\": %d}", iter->name, iter->data.info.width);
        if (iter->next != 0) {
            fprintf(f, ",");
        }
        iter = iter->next;
    }
    fprintf(f, "], \"keys\": [");

    for (int j=0; j<frame.nkeys; j++) {
        char meta_path[PATH_MAX];
        get_subpath(name, meta_path, "keys", frame.keys[j], "meta");
        FILE *meta_file = fopen(meta_path, "rb");
        value_t key;
        if (fread(&key, sizeof(value_t), 1, meta_file) != 1) {
            fclose(meta_file);
            continue;
        }
        char key_name[NAME_MAX];
        if (fread(&key_name, key.index_length, 1, meta_file) != 1) {
            fclose(meta_file);
            continue;
        }
        key_name[key.index_length] = 0;
        fclose(meta_file);
        fprintf(f, "[\"%s\",%d,%d,\"%s\"]", frame.keys[j], key.type, key.width, key_name);
        if (j != frame.nkeys - 1) {
            fprintf(f, ", ");
        }
    }
    fprintf(f, "]}");
}

void process_index(FILE *f, char *name, char *key, unsigned long low, unsigned long high, int points) {
    fprintf(f, "Content-type: application/octet-stream\n");
    index_files_t *idx = blob_open_index(name, key); // TODO iter
    index_query_t result;
    blob_query_index(idx, low, high, (long)points, &result);

    fprintf(f, "Xandri-Data: [%d, %lu, %lu]\n", result.zoom, result.low, result.high);
    fprintf(f, "\r\n");

    char *start = ((char*)idx->ptr[result.zoom]) + idx->info.width * result.low;
    fwrite(start, result.high - result.low + 1, idx->info.width, f);
}

void process_key(FILE *f, char *name, char *key, unsigned long low, unsigned long high, int zoom) {
    fprintf(f, "Content-type: application/octet-stream\n");
    char meta_path[PATH_MAX];
    get_subpath(name, meta_path, "keys", key, "meta");
    FILE *meta_file = fopen(meta_path, "rb");
    value_t info;
    if (fread(&info, sizeof(value_t), 1, meta_file) != 1) {
        fclose(meta_file);
        printf("could not open meta\n");
        return;
    }
    fclose(meta_file);

    char path_buf[PATH_MAX];
    char suff[] = "x.bin";
    suff[0] = '0'+zoom;
    get_subpath(name, path_buf, "keys", key, suff);
    int df = open(path_buf, O_RDONLY);
    if (df < 0) {
        printf("could not open data\n");
        return;
    }
    struct stat s;
    fstat(df, &s);

    fprintf(f, "\r\n");

    void *map = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, df, 0);

    char *start = ((char*)map) + info.width * low;
    fwrite(start, high - low + 1, info.width, f);
    munmap(map, s.st_size);
    close(df);
}



void process(int fd) {
    FILE *f = fdopen(fd, "r+");
    if (f == 0) {
        close(fd);
        printf("Could not open socket.\n");
        return;
    }
    char buf[BUFSIZE];
    char method[BUFSIZE];
    char uri[BUFSIZE];
    char version[BUFSIZE];
    if (fgets(buf, BUFSIZE, f) == 0) goto err;
    sscanf(buf, "%s %s %s\n", method, uri, version);
    if (strcasecmp(method, "get") != 0) {
        bye(f, "501", "Unsupported method");
        goto finalize;
    }
    if (fgets(buf, BUFSIZE, f) == 0) goto err;
    while(strcmp(buf, "\r\n")) {
        if (fgets(buf, BUFSIZE, f) == 0) goto err;
    }
    clock_t start = clock();
    fprintf(f, "HTTP/1.0 200 OK\n");
    fprintf(f, "Access-Control-Allow-Origin: *\n");
    fprintf(f, "Access-Control-Expose-Headers: Xandri-Data\n");
    if (strncmp(uri, "/frames", strlen("/frames")) == 0) {
        process_frames(f);
    } else if (strncmp(uri, "/keys", strlen("/keys")) == 0) {
        char *name = uri + strlen("/keys") + 1;
        process_keys(f, name);
    } else if (strncmp(uri, "/index", strlen("/index")) == 0) {
        unsigned long low, high;
        int points;
        char name[NAME_MAX];
        char key[NAME_MAX];
        int i = 0;
        // TODO buffer overflow on sscanf
        if ((i=sscanf(uri, "/index/%[^/]/%[^/]/%lu/%lu/%d", name, key, &low, &high, &points)) != 5) {
            fprintf(f, "\r\nlength %d | %s\n",i, uri);
            goto finalize;
        }
        process_index(f, name, key, low, high, points);
    } else if (strncmp(uri, "/key", strlen("/key")) == 0) {
        long low, high;
        int zoom;
        char name[NAME_MAX];
        char key[NAME_MAX];
        int i = 0;
        if ((i=sscanf(uri, "/key/%[^/]/%[^/]/%d/%ld/%ld", name, key, &zoom, &low, &high)) != 5) {
            fprintf(f, "\r\nlength %d | %s\n", i, uri);
            goto finalize;
        }
        process_key(f, name, key, low, high, zoom);
    }
    printf("Processing took %.2f ms\n", (clock()-start)/((double)CLOCKS_PER_SEC)*1000);
    goto finalize;
err:
    bye(f, "500", "Some error");
finalize:
    fflush(f);
    fclose(f);
    close(fd);
    printf("done!\n");
}

int serve_cmd(int argc, char *argv[]) {
    signal(SIGCHLD,SIG_IGN);
    if (server_init_cache()) {
        printf("Unable to initialize cache.\n");
        return 1;
    }

    char *port_str = get_named_argument(argc, argv, 'p', "2718");
    unsigned short port = (unsigned short)atoi(port_str);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        printf("Could not open socket.\n");
        return 1;
    }
    int val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&val, sizeof(int));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) < 0) {
        printf("Failed to bind socket.\n");
        return 1;
    }

    if (listen(server_fd, 128) < 0) {
        printf("Failed to set up socket for listening.\n");
        return 1;
    }

    struct sockaddr_in client_addr;
    unsigned int client_len;

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len); 
        if (client_fd < 0) {
            printf("Could not accept request.\n");
            continue;
        }
        pid_t pid = fork();
        if (pid == -1) {
            printf("Error forking.\n");
            close(client_fd);
        } else if (pid > 0) {
            close(client_fd);
            printf("[Parent] Forked off!\n");
        } else if (pid == 0) {
            printf("[Child] processing request... ");
            process(client_fd);
            break;
        }
    }

    return 0;
}
