/******************************************************************************
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/
#ifndef OEAWARE_UTILS_H
#define OEAWARE_UTILS_H
#include <string>
#include <vector>
#include <unordered_map>
#include <oeaware/data_list.h>
#include <oeaware/data/analysis_data.h>
#include <sys/resource.h>

namespace oeaware {
bool Download(const std::string &url, const std::string &path);
// Check the file permissions.
bool CheckPermission(const std::string &path, int mode);
// If the file owner is in users, return true. Otherwise, return false.
bool CheckFileUsers(const std::string &path, const std::vector<uid_t> &users);
// If the file group is in groups, return true. Otherwise, return false.
bool CheckFileGroups(const std::string &path, const std::vector<gid_t> &groups);
int GetGidByGroupName(const std::string &groupName);
bool FileExist(const std::string &fileName);
bool EndWith(const std::string &s, const std::string &ending);
std::string Concat(const std::vector<std::string>& strings, const std::string &split);
// Separate "str" with the separator "split"
std::vector<std::string> SplitString(const std::string &str, const std::string &split);
bool CreateDir(const std::string &path);
bool SetDataListTopic(DataList *dataList, const std::string &instanceName, const std::string &topicName,
    const std::string &params);
uint64_t GetCpuCycles(int cpu);
uint64_t GetCpuFreqByDmi();
std::string GetCpuPartId();
bool ExecCommand(const std::string &command, std::string &result);
bool ServiceIsActive(const std::string &serviceName, bool &isActive);
bool ServiceControl(const std::string &serviceName, const std::string &action);
std::unordered_map<std::string, std::string> GetKeyValueFromString(const std::string &params);
std::vector<int> ParseRange(const std::string &range);
bool IrqSetSmpAffinity(int preferredCpu, int irqNum);
/// @brief Check whether a string is an integer or a floating point number.
bool IsNum(const std::string &s);
std::string GetNetOperateStr(int state);
int GetNetOperateTypeByStr(const std::string &state);
bool GetSysFsNrOpen(long &nrOpen);
bool SetFileDescriptorLimit(long limit);
bool SetMemLockRlimt(rlim_t limit);
std::string ReplaceString(const std::string &input, const std::string &target, const std::string &replacement);
bool ReadSchedFeatures(std::string &schedPath, std::vector<std::string> &features);
void AnalysisResultItemFree(AnalysisResultItem *analysisResultItem);
}

#endif