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
#ifndef OEAWARE_MANAGER_DOCKER_ADAPT_H
#define OEAWARE_MANAGER_DOCKER_ADAPT_H
#include <unordered_map>
#include <unordered_set>
#include <dirent.h>
#include <sys/stat.h>
#include "oeaware/interface.h"
#include "oeaware/data/docker_data.h"

class DockerAdapt : public oeaware::Interface {
public:
    DockerAdapt();
    ~DockerAdapt() override = default;
    oeaware::Result OpenTopic(const oeaware::Topic &topic) override;
    void CloseTopic(const oeaware::Topic &topic) override;
    void UpdateData(const DataList &dataList) override;
    oeaware::Result Enable(const std::string &param) override;
    void Disable() override;
    void Run() override;

private:
    void DockerUpdate(const std::unordered_set<std::string> &sub_dir);
    void DockerCollect();
    bool openStatus = false;
    std::unordered_map<std::string, Container*> containers;
};
#endif // OEAWARE_MANAGER_DOCKER_ADAPT_H