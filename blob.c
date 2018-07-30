#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "util.h"
#include "blob.h"

char *idxcache_keys[MAX_INDICES] = {0};
index_files_t *idxcache_values[MAX_INDICES] = {0};

int blob_print_help() {
    printf("Usage: x blob [create/create-index]\n");
    return 0;
}

int blob_create(char *name) {
    char path[PATH_MAX];

    get_blob_path(name, path);
    if (directory_exists(path)) {
        printf("Blob %s already exists!\n", name);
        return 1;
    }

    if (create_directory(path)) goto err;
    get_keys_path(name, path);
    if (create_directory(path)) goto err;
    get_indices_path(name, path);
    if (create_directory(path)) goto err;
    
    return 0;

err:
    printf("Error creating data directories.\n");
    return 1;
}

int blob_get_num_summaries(long length) {
    int num = 0;
    for (int i=0; i<MAX_SUMMARIES; i++) {
        if (length >= SUMMARY_SIZES[i] * SUMMARY_MULT) num++;
    } 
    return num;
}

int blob_index_from_file(char *name, char *key, char *path, int width, int use_summary) {
    printf("Creating index for %s.%s from %s, opts %d, %d.\n", name, key, path, width, use_summary);
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        printf("Could not open file %s.\n", path);
        return 1;
    }
    fseek(file, 0L, SEEK_END);
    long sz = ftell(file);
    long num = sz / width;
    rewind(file);

    index_t index;
    index.length = num;
    index.width = width;
    index.num_summaries = use_summary ? blob_get_num_summaries(num) : 0;
    index.num_points[0] = num;
    printf("using %d summaries\n", index.num_summaries);

    accum_t cur_sum[MAX_SUMMARIES] = {0};
    long cur_counts[MAX_SUMMARIES] = {0};
    FILE *files[MAX_SUMMARIES+1];
    char path_buf[PATH_MAX];
    get_subpath(name, path_buf, "indices", key, "0.bin");
    files[0] = fopen(path_buf, "wb");
    for (int i=0; i<MAX_SUMMARIES; i++) {
        char suff[] = "x.bin";
        suff[0] = '1'+i;
        get_subpath(name, path_buf, "indices", key, suff);
        files[i+1] = fopen(path_buf, "wb");
    }


    for (int i=0; i<num; i++) {
        uint64_t data = 0;
        if (fread(&data, width, 1, file) != 1) goto err;
        write_to_file(files[0], &data, width);
        for (int j=0; j<index.num_summaries; j++) {
            cur_counts[j]++;
            cur_sum[j] += data;
            if (cur_counts[j] == SUMMARY_SIZES[j]) {
                uint64_t val = cur_sum[j]/cur_counts[j];
                write_to_file(files[j+1], &val, width); // NOTE: little endianness assumed!
                index.num_points[j+1]++;
                cur_counts[j] = 0;
                cur_sum[j] = 0;
            }
        }
    }
    for (int j=0; j<index.num_summaries; j++) {
        if (cur_counts[j] != 0) {
            uint64_t val = cur_sum[j]/cur_counts[j];
            write_to_file(files[j+1], &val, width);
            index.num_points[j+1]++;
        }
    }

    char meta_path[PATH_MAX]; get_subpath(name, meta_path, "indices", key, "meta");
    FILE *meta_file = fopen(meta_path, "wb");
    write_to_file(meta_file, &index, sizeof(index));
    fclose(meta_file);

    return 0;

err:
    printf("Error reading file.\n");
    return 1;
}

int blob_create_index(int argc, char *argv[]) {
    char *width_str = get_named_argument(argc, argv, 'b', "4");
    int width = atoi(width_str);
    if (width != 1 && width != 2 && width != 4 && width != 8) {
        printf("Unsupported byte width %d.\n", width);
        return 1;
    }

    char *summary_str = get_named_argument(argc, argv, 's', "yes");
    int use_summary = strcmp(summary_str, "yes") == 0;

    char *name = get_positional_argument(argc, argv, 0, NULL);
    char *key = get_positional_argument(argc, argv, 1, NULL);
    char *path = get_positional_argument(argc, argv, 2, NULL);
    if (name == NULL || path == NULL) {
        return blob_print_help();
    }

    return blob_index_from_file(name, key, path, width, use_summary);
}

long blob_bs_find_low(void *data, long n, int width, uint64_t val) {
    int stop = 0;
    switch (width) {
        case 1: stop = ((uint8_t*)data)[0] >= val; break;
        case 2: stop = ((uint16_t*)data)[0] >= val; break;
        case 4: stop = ((uint32_t*)data)[0] >= val; break;
        case 8: stop = ((uint64_t*)data)[0] >= val; break;
    }
    if (stop) return 0;
    long low = 0;
    long high = n;
    long mid;
    long midval = 0;
    while (low != high) {
        mid = (low + high)/2;
        switch (width) {
            case 1: midval = ((uint8_t*)data)[mid]; break;
            case 2: midval = ((uint16_t*)data)[mid]; break;
            case 4: midval = ((uint32_t*)data)[mid]; break;
            case 8: midval = ((uint64_t*)data)[mid]; break;
        }
        if (midval <= val) {
            low = mid+1;
        } else {
            high = mid;
        }
    }
    return low == 0 ? low : low - 1;
}

long blob_bs_find_high(void *data, long n, int width, uint64_t val) {
    int stop = 0;
    switch (width) {
        case 1: stop = ((uint8_t*)data)[n-1] <= val; break;
        case 2: stop = ((uint16_t*)data)[n-1] <= val; break;
        case 4: stop = ((uint32_t*)data)[n-1] <= val; break;
        case 8: stop = ((uint64_t*)data)[n-1] <= val; break;
    }
    if (stop) return n-1;
    long low = 0;
    long high = n;
    long mid;
    long midval = 0;
    while (low != high) {
        mid = (low + high)/2;
        switch (width) {
            case 1: midval = ((uint8_t*)data)[mid]; break;
            case 2: midval = ((uint16_t*)data)[mid]; break;
            case 4: midval = ((uint32_t*)data)[mid]; break;
            case 8: midval = ((uint64_t*)data)[mid]; break;
        }
        if (midval >= val) {
            high = mid;
        } else {
            low = mid+1;
        }
    }
    return low;
}


void blob_query_index(index_files_t *idx, unsigned long low, unsigned long high, long points, index_query_t *result) {
    // TODO figure out inclusivity of ranges
    for (int i=MAX_SUMMARIES; i >= 0; i--) {
        //printf(" - level %d (%d, %d) -\n", i, (int)points, (int)idx->info.num_points[i]);
        if (idx->info.num_points[i] < points) continue;

        long bs_low = blob_bs_find_low(idx->ptr[i], idx->info.num_points[i], idx->info.width, low);
        if (idx->info.num_points[i] - bs_low < points) continue;

        long bs_high = blob_bs_find_high(idx->ptr[i], idx->info.num_points[i], idx->info.width, high);
        //printf("bs_low %d bs_high %d\n", (int)bs_low, (int)bs_high);

        result->zoom = i;
        result->low = bs_low;
        result->high = bs_high;

        if (bs_high - bs_low + 1 >= points) {
            return;
        }
    }
}

int blob_query_index_cmd(int argc, char *argv[]) {
    char *name = get_positional_argument(argc, argv, 0, NULL);
    char *key = get_positional_argument(argc, argv, 1, NULL);
    char *low = get_positional_argument(argc, argv, 2, NULL);
    char *high = get_positional_argument(argc, argv, 3, NULL);
    char *points = get_positional_argument(argc, argv, 4, "0");
    if (name == NULL || key == NULL || low == NULL || high == NULL) {
        return blob_print_help();
    }

    index_files_t *idx = blob_open_index(name, key);
    if (idx == 0) {
        printf("unable to load index\n");
        return 1;
    }
    index_query_t result;
    blob_query_index(idx, (unsigned long)atol(low), (unsigned long)atol(high), atol(points), &result);

    printf("index %d: (%ld, %ld)\n", result.zoom, result.low, result.high);
    return 0;
}

index_files_t *blob_open_index(char *name, char *key) {
    int max_index;
    for (max_index=0; max_index<MAX_INDICES; max_index++) {
        if (idxcache_keys[max_index] == 0) break;
        if (strcmp(idxcache_keys[max_index], name) == 0 &&
            strcmp(idxcache_keys[max_index] + strlen(name), key) == 0) {
            printf("found cache!\n");
            return idxcache_values[max_index];
        }
    }
    if (max_index == MAX_INDICES) max_index = 0;
    printf("Adding index to cache\n");
    index_files_t *ret = (index_files_t*)malloc(sizeof(index_files_t));
    idxcache_keys[max_index] = strdup(key);
    idxcache_values[max_index] = ret;
    for (int i=0; i<(MAX_SUMMARIES+1); i++) {
        ret->fds[i] = 0;
        ret->ptr[i] = 0;
    }
    char meta_path[PATH_MAX];
    get_subpath(name, meta_path, "indices", key, "meta");
    FILE *meta_file = fopen(meta_path, "rb");
    if (meta_file == 0) {
        printf("Error opening file for %s.%s\n", name, key);
        return 0;
    }
    if (fread(&ret->info, sizeof(index_t), 1, meta_file) != 1) {
        printf("Error reading information for %s.%s\n", name, key);
        return 0;
    }
    fclose(meta_file);

    char path_buf[PATH_MAX];
    for (int i=0; i<(ret->info.num_summaries+1); i++) {
        char suff[] = "x.bin";
        suff[0] = '0'+i;
        get_subpath(name, path_buf, "indices", key, suff);
        ret->fds[i] = open(path_buf, O_RDONLY);
        if (ret->fds[i] < 0) {
            printf("Unable to open subkey %d for %s.\n", i, key);
            return 0;
        }
        struct stat s;
        fstat(ret->fds[i], &s);
        ret->ptr[i] = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED | MAP_POPULATE, ret->fds[i], 0);
        if (ret->ptr[i] == MAP_FAILED) {
            printf("mmap failed :'( %s\n", strerror(errno));
            return 0;
        }
    }
    return ret;
}

int blob_key_from_file(char *name, char *key, char *path, char *type, int width, char *index_str) {
    printf("Creating key for %s.%s from %s.\n", name, key, path);

    index_t *index = &blob_open_index(name, index_str)->info;
    if (index == 0) {
        printf("Could not find index %s.\n", index_str);
    }
    printf("index shows %d summaries\n", index->num_summaries);

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        printf("Could not open file %s.\n", path);
        return 1;
    }
    fseek(file, 0L, SEEK_END);
    long sz = ftell(file);
    long num = sz / width;
    rewind(file);
    if (num != index->length) {
        printf("Array dimension does not match index!\n");
        return 1;
    }

    value_t value;
    if (strcmp(type, "float") == 0) value.type = TYPE_FLOAT;
    else if (strcmp(type, "int") == 0) value.type = TYPE_INT;
    else {
        printf("Unsupported data type %s.\n", type);
        return 1;
    } // TODO whine about float8 and float16
    value.width = width;
    value.index_length = strlen(index_str);

    char meta_path[PATH_MAX]; get_subpath(name, meta_path, "keys", key, "meta");
    FILE *meta_file = fopen(meta_path, "wb");
    write_to_file(meta_file, &value, sizeof(value));
    write_to_file(meta_file, index_str, strlen(index_str));
    fclose(meta_file);

    //

    val_accum_t cur_sum[MAX_SUMMARIES] = {0};
    int cur_counts[MAX_SUMMARIES] = {0};
    FILE *files[MAX_SUMMARIES+1];
    char path_buf[PATH_MAX];
    get_subpath(name, path_buf, "keys", key, "0.bin");
    files[0] = fopen(path_buf, "wb");
    for (int i=0; i<index->num_summaries; i++) {
        char suff[] = "x.bin";
        suff[0] = '1'+i;
        get_subpath(name, path_buf, "keys", key, suff);
        files[i+1] = fopen(path_buf, "wb");
    }

    for (int i=0; i<num; i++) {
        uint64_t data = 0;
        if (fread(&data, width, 1, file) != 1) goto err;
        write_to_file(files[0], &data, width);
        val_accum_t val = 0;
        switch (value.type) {
        case TYPE_FLOAT:
            switch (value.width) {
                case 4: val = *((float*)&data); break;
                case 8: val = *((double*)&data); break;
            }
            break;
        case TYPE_INT:
            switch (value.width) {
                case 1: val = *((uint8_t*)&data); break;
                case 2: val = *((uint16_t*)&data); break;
                case 4: val = *((uint32_t*)&data); break;
                case 8: val = *((uint64_t*)&data); break;
            }
        }
        for (int j=0; j<index->num_summaries; j++) {
            cur_counts[j]++;
            cur_sum[j] += val;
            if (cur_counts[j] == SUMMARY_SIZES[j]) {
                val_accum_t val = cur_sum[j]/cur_counts[j];
                write_to_file_as_type(files[j+1], val, value.type, value.width);
                cur_counts[j] = 0;
                cur_sum[j] = 0;
            }
        }
    }
    for (int j=0; j<index->num_summaries; j++) {
        if (cur_counts[j] != 0) {
            val_accum_t val = cur_sum[j]/cur_counts[j];
            write_to_file_as_type(files[j+1], val, value.type, value.width);
        }
    }

    return 0;

err:
    printf("Error reading file.\n");
    return 1;
}


int blob_create_key(int argc, char *argv[]) {
    char *width_str = get_named_argument(argc, argv, 'b', "4");
    int width = atoi(width_str);
    if (width != 1 && width != 2 && width != 4 && width != 8) {
        printf("Unsupported byte width %d.\n", width);
        return 1;
    }
    char *type_str = get_named_argument(argc, argv, 't', "float");
    char *name = get_positional_argument(argc, argv, 0, NULL);
    char *key = get_positional_argument(argc, argv, 1, NULL);
    char *index_str = get_positional_argument(argc, argv, 2, NULL);
    char *path = get_positional_argument(argc, argv, 3, NULL);
    if (name == NULL || key == NULL || path == NULL || index_str == NULL) {
        return blob_print_help();
    }

    return blob_key_from_file(name, key, path, type_str, width, index_str);
}

int blob_cmd(int argc, char *argv[]) {
	// TODO error checking
	if (strcmp(argv[0], "create") == 0) {
	    return blob_create(argv[1]);
	} else if (strcmp(argv[0], "create-index") == 0) {
        return blob_create_index(argc - 1, argv + 1);
    } else if (strcmp(argv[0], "query-index") == 0) {
        return blob_query_index_cmd(argc - 1, argv + 1);
    } else if (strcmp(argv[0], "create-key") == 0) {
        return blob_create_key(argc - 1, argv + 1);
    }
	return 0;
}
