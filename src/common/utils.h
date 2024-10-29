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
#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H
#include <string>
#include <vector>

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
}

#endif // !COMMON_UTILS_H