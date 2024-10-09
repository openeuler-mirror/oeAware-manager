/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <securec.h>
#include "pmu_uncore.h"

static int hha_num = 0;
static struct uncore_config *uncore_rx_outer = NULL;
static struct uncore_config *uncore_rx_sccl = NULL;
static struct uncore_config *uncore_rx_ops_num = NULL;

int get_uncore_hha_num()
{
    return hha_num;
}

struct uncore_config *get_rx_outer()
{
    return uncore_rx_outer;
}

struct uncore_config *get_rx_sccl()
{
    return uncore_rx_sccl;
}

struct uncore_config *get_rx_ops_num()
{
    return uncore_rx_ops_num;
}

static int read_single_uncore_event(const char *hha_name, struct uncore_config *uncore_event, const char *event_name)
{
    char hha_path[MAX_PATH_LEN] = {0};

    // Read cfg
    snprintf_truncated_s(hha_path, MAX_PATH_LEN, "%s/%s/", hha_name, event_name);
    
    strcpy(uncore_event->uncore_name, hha_path);

    return 0;
}

static void free_namelist(int n, struct dirent **namelist)
{
    while (n--) {
        free(namelist[n]);
    }
    free(namelist);
}

static int hha_read_uncore_config(int n, struct dirent **namelist)
{
    int index = 0;

    while (index < n) {
        if (read_single_uncore_event(namelist[index]->d_name, &uncore_rx_outer[index], "rx_outer") != 0) {
            return -1;
        }
        if (read_single_uncore_event(namelist[index]->d_name, &uncore_rx_sccl[index], "rx_sccl") != 0) {
            return -1;
        }
        if (read_single_uncore_event(namelist[index]->d_name, &uncore_rx_ops_num[index], "rx_ops_num") != 0) {
            return -1;
        }

        index++;
    }

    return 0;
}

static int hha_scandir_select(const struct dirent *ptr)
{
    int ret = 0;
    if (strstr(ptr->d_name, "hha") != NULL) {
        ret = 1;
    }

    return ret;
}

int hha_uncore_config_init(void)
{
    int ret;
    struct dirent **namelist;

    hha_num = scandir(DEVICE_PATH, &namelist, hha_scandir_select, alphasort);
    if (hha_num <= 0) {
        printf("scandir failed\n");
        return -1;
    }

    uncore_rx_outer = (struct uncore_config *)calloc(hha_num, sizeof(struct uncore_config));
    if (uncore_rx_outer == NULL) {
        free_namelist(hha_num, namelist);
        return -1;
    }
    uncore_rx_sccl = (struct uncore_config *)calloc(hha_num, sizeof(struct uncore_config));
    if (uncore_rx_sccl == NULL) { // free uncore_rx_sccl in function uncore_config_fini
        free_namelist(hha_num, namelist);
        return -1;
    }
    uncore_rx_ops_num = (struct uncore_config *)calloc(hha_num, sizeof(struct uncore_config));
    if (uncore_rx_ops_num == NULL) { // free uncore_rx_ops_num in function uncore_config_fini
        free_namelist(hha_num, namelist);
        return -1;
    }

    ret = hha_read_uncore_config(hha_num, namelist);
    free_namelist(hha_num, namelist);

    return ret;
}

void uncore_config_fini(void)
{
#define UNCORE_CONFIG_FREE(uncore_event) do { \
    if (uncore_event != NULL) { \
        free(uncore_event); \
        uncore_event = NULL; \
    } \
} while (0)

    UNCORE_CONFIG_FREE(uncore_rx_outer);
    UNCORE_CONFIG_FREE(uncore_rx_sccl);
    UNCORE_CONFIG_FREE(uncore_rx_ops_num);

    hha_num = 0;
}
