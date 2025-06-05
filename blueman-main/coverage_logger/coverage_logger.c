#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <json-c/json.h>
#include <string.h>
#include <unistd.h>

#define MAX_ENTRIES 2000000

static json_object *coverage_array = NULL;
static char output_file_path[256] = "coverage_output.json";
static int entry_count = 0;
const char* json_str = NULL;

void init_coverage_logger(const char *filename) {
    coverage_array = json_object_new_array();
    if (!coverage_array) {
        fprintf(stderr, "Failed to initialize JSON array.\n");
        exit(EXIT_FAILURE);
    }

    entry_count = 0;

    if (filename && strlen(filename) < sizeof(output_file_path)) {
        strncpy(output_file_path, filename, sizeof(output_file_path) - 1);
        output_file_path[sizeof(output_file_path) - 1] = '\0';
    } else {
        strcpy(output_file_path, "coverage_output.json");
    }
}

void add_coverage_entry(uint64_t time, int coverage) {
    if (!coverage_array) {
        fprintf(stderr, "Coverage logger not initialized.\n");
        return;
    }

    if (entry_count >= MAX_ENTRIES) {
        fprintf(stderr, "Maximum number of entries reached (%d). Ignoring further data.\n", MAX_ENTRIES);
        return;
    }

    json_object *entry = json_object_new_object();
    json_object_object_add(entry, "time", json_object_new_uint64(time));
    json_object_object_add(entry, "coverage", json_object_new_int(coverage));

    json_object_array_add(coverage_array, entry);
    json_str = json_object_to_json_string_ext(coverage_array, JSON_C_TO_STRING_PRETTY);
    entry_count++;
}

void save_coverage_to_file() {
    if (!coverage_array) return;

    FILE *fp = fopen(output_file_path, "w");
    if (!fp) {
        perror("Failed to open file for writing");
        return;
    }

    fprintf(fp, "%s\n", json_str);
    fclose(fp);

    printf("Coverage saved to %s\n", output_file_path);
}


