#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "common.h"
#include <string.h>

#define INTERVAL      3
#define FORMAT        "%s %i"
#define TOKSEP        "=\n"

#define POWER_PATH      "/sys/class/power_supply"
#define BAT_PREFIX      "BAT"
#define BAT_INFO        "uevent"

#define KEY_PREFIX    "POWER_SUPPLY_"

#define NAME_KEY      KEY_PREFIX "NAME"
#define STATUS_KEY    KEY_PREFIX "STATUS"
#define CAPACITY_KEY  KEY_PREFIX "CAPACITY"

bool filter_on_prefix(const struct dirent *entry, const char *prefix, const char len)
{
    const char *s = entry->d_name;

    if(strncmp(s,prefix,len) == 0) {
        return true;
    }
    return false;
}

size_t get_batteries(const char *path, const char *prefix, const size_t len, char ***ls){
    size_t count = 0;
    DIR *dp = NULL;
    struct dirent *ep = NULL;

    dp = opendir(path);
    if (dp == NULL) {
        fprintf(stderr, "No such directory: '%s'.\n", path);
        return 0;
    }

    *ls = NULL;
    ep = readdir(dp);
    while(ep != 0) {
        count ++;
        ep = readdir(dp);
    }
    rewinddir(dp);

    *ls = calloc(count, sizeof(char *));
    count = 0;
    ep = readdir(dp);
    while(ep != NULL) {
        if (filter_on_prefix(ep, prefix, len)) {
            (*ls)[count] = calloc(MAXLEN, sizeof(char));
            snprintf((*ls)[count], MAXLEN, "%s/%s/%s", path, ep->d_name, BAT_INFO);
            count ++;
        }
        ep = readdir(dp);
    }

    closedir(dp);
    return count;
}

int put_infos(char **paths, const size_t count)
{
    size_t i;

    for (i=0; i < count; i++) {

        FILE *bf = fopen(paths[i], "r");
        if (bf == NULL) {
            fprintf(stderr, "Can't open '%s'.\n", paths[i]);
            return EXIT_FAILURE;
        }

        char line[MAXLEN] = {0};
        char status[MAXLEN] = {0};
        char name[MAXLEN] = {0};
        int capacity = -1;
        bool found_name = false, found_status = false, found_capacity = false;
        while (fgets(line, sizeof(line), bf) != NULL) {
            char *key = strtok(line, TOKSEP);
            if (key != NULL) {
                if (!found_status && strcmp(key, STATUS_KEY) == 0) {
                    strncpy(status, strtok(NULL, TOKSEP), sizeof(status));
                    found_status = true;
                } else if (!found_name && strcmp(key, NAME_KEY) == 0) {
                    strncpy(name, strtok(NULL, TOKSEP), sizeof(name));
                    found_name = true;
                } else if (capacity == -1 && strcmp(key, CAPACITY_KEY) == 0) {
                    capacity = atoi(strtok(NULL, TOKSEP));
                    found_capacity = true;
                }
            }
        }
        fclose(bf);

        if (!found_name || !found_capacity || !found_status) {
            fprintf(stderr, "The battery informations are missing.\n");
            return EXIT_FAILURE;
        } else {
            printf("%s %s %i,", name, status, capacity);
        }
    }
    printf("\n");
    fflush(stdout);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    char *path = POWER_PATH;
    char *prefix = BAT_PREFIX;
    int interval = INTERVAL;
    bool snoop = false;

    char opt;
    while ((opt = getopt(argc, argv, "hsf:i:p:n:")) != -1) {
        switch (opt) {
            case 'h':
                printf("battery [-h|-s|-i INTERVAL|-p PATH|-x PREFIX]\n");
                exit(EXIT_SUCCESS);
                break;
            case 's':
                snoop = true;
                break;
            case 'p':
                path = optarg;
                break;
            case 'x':
                prefix = optarg;
                break;
            case 'i':
                interval = atoi(optarg);
                break;
        }
    }

    char real_path[MAXLEN] = {0};
    char real_prefix[MAXLEN] = {0};
    snprintf(real_path, sizeof(real_path), path);
    snprintf(real_prefix, sizeof(real_prefix), prefix);

    size_t count;
    char **bat_paths;
    count = get_batteries(real_path, real_prefix, strlen(real_prefix), &bat_paths);

    if (count == 0) {
        fprintf(stderr, "No batteries found under '%s' with prefix '%s'.'\n", real_path, real_prefix);
        return EXIT_FAILURE;
    }

    int exit_code;




    if (snoop)
        while ((exit_code = put_infos(bat_paths, count)) != EXIT_FAILURE)
            sleep(interval);
    else
        exit_code = put_infos(bat_paths, count);

    return exit_code;
}
