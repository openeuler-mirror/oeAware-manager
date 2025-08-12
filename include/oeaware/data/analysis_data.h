/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/

#ifndef OEAWARE_ANALYSIS_DATA_H
#define OEAWARE_ANALYSIS_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ANALYSIS_REPORT_REALTIME,
	ANALYSIS_REPORT_SUMMARY
} AnalysisReportType;
typedef struct {
	int reportType; // AnalysisReportType
	char *data;
} AnalysisReport;

typedef enum {
	DATA_TYPE_CPU,
	DATA_TYPE_MEMORY,
	DATA_TYPE_IO,
	DATA_TYPE_NETWORK
#ifdef __riscv
	, DATA_TYPE_HWPROBE
#endif
} DataType;

typedef struct {
	int type; // DataType
	char *metric;
	char *value;
	char *extra;
} DataItem;

typedef struct {
	char *suggestion;
	char *opt;
	char *extra;
} SuggestionItem;

typedef struct {
	int dataItemLen;
	DataItem *dataItem;
	char *conclusion;
	SuggestionItem suggestionItem;
} AnalysisResultItem;
#ifdef __cplusplus
}
#endif

#endif
