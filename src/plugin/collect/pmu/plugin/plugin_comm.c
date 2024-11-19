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
#include <securec.h>
#include "pmu.h"
#include "interface.h"

struct DataRingBuf *init_buf(int buf_len, const char *instance_name)
{
    struct DataRingBuf *data_ringbuf;

    data_ringbuf = (struct DataRingBuf *)malloc(sizeof(struct DataRingBuf));
    if (!data_ringbuf) {
        printf("malloc data_ringbuf failed\n");
        return NULL;
    }

    (void)memset_s(data_ringbuf, sizeof(struct DataRingBuf), 0, sizeof(struct DataRingBuf));
    
    data_ringbuf->instance_name = instance_name;
    data_ringbuf->index = -1;

    data_ringbuf->buf = (struct DataBuf *)malloc(sizeof(struct DataBuf) * buf_len);
    if (!data_ringbuf->buf) {
        printf("malloc data_ringbuf buf failed\n");
        free(data_ringbuf);
        data_ringbuf = NULL;
        return NULL;
    }

    (void)memset_s(data_ringbuf->buf, sizeof(struct DataBuf) * buf_len, 0, sizeof(struct DataBuf) * buf_len);
    data_ringbuf->buf_len = buf_len;

    return data_ringbuf;
}

void free_buf(struct DataRingBuf *data_ringbuf)
{
    if (!data_ringbuf) {
        return;
    }

    if (!data_ringbuf->buf) {
        goto out;
    }

    free(data_ringbuf->buf);
    data_ringbuf->buf = NULL;

out:
    free(data_ringbuf);
    data_ringbuf = NULL;
}

void fill_buf(struct DataRingBuf *data_ringbuf, struct PmuData *pmu_data, int len)
{
    struct DataBuf *buf;
    int index;

    index = (data_ringbuf->index + 1) % data_ringbuf->buf_len;
    data_ringbuf->index = index;
    data_ringbuf->count++;
    buf = &data_ringbuf->buf[index];

    if (buf->data != NULL) {
        PmuDataFree(buf->data);
        buf->data = NULL;
        buf->len = 0;
    }

    buf->len = len;
    buf->data = (void *)pmu_data;
}
