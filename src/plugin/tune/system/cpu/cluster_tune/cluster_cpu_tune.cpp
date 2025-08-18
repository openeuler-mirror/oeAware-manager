/******************************************************************************
 * Copyright (c) 2025 Huawei Technologies Co., Ltd. All rights reserved.
 * oeAware is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 ******************************************************************************/

#include "cluster_cpu_tune.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unistd.h>

using namespace oeaware;
#define CLUSTER_PATH "/proc/sys/kernel/sched_cluster"
#define ENABLE_CLUSTER "1"

ClusterCpuTune::ClusterCpuTune()
{
    name = OE_CLUSTER_TUNE;
    description = "CPU cluster scheduler is designed for Kunpeng Platform CPU architecture";
    version = "1.0.0";
    period = 1000;  // 1000 period is meaningless, only Enable() is executed, not Run()
    priority = 2;
    type = oeaware::TUNE;
}

oeaware::Result ClusterCpuTune::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void ClusterCpuTune::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void ClusterCpuTune::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

static bool IsFileExists(const std::string &path)
{
    return access(path.c_str(), F_OK) == 0;
}

bool ClusterCpuTune::ReadFile(const std::string &filePath, std::string &value)
{
	std::string buf;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
	while (std::getline(file, buf)) {
		value = buf;
	}
    file.close();
    return true;
}

bool ClusterCpuTune::ReadClusterFile(std::string &value)
{
    if (!IsFileExists(CLUSTER_PATH)) {
        ERROR(logger, "cluster scheduler not available in current environment");
        return false;
    }

    DEBUG(logger, "Reading file from: " << CLUSTER_PATH);
    if (ReadFile(CLUSTER_PATH, value)) {
        DEBUG(logger, "Successfully read " << value << " from cluster scheduler");
        return true;
    } else {
        ERROR(logger, "Failed to read from file: " << CLUSTER_PATH);
        return false;
    }
}

bool ClusterCpuTune::WriteFile(const std::string &filePath, const std::string &value)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    file << value;
    if (!file.flush()) {
        ERROR(logger, "Failed to echo " << value << " to " << filePath);
        return false;
    }
    file.close();
    return true;
}

bool ClusterCpuTune::WriteClusterFile(const std::string &value)
{
    if (!IsFileExists(CLUSTER_PATH)) {
        ERROR(logger, "cluster scheduler not available in current environment");
        return false;
    }

    DEBUG(logger, "Writing " << value << " to: " << CLUSTER_PATH);
    if (WriteFile(CLUSTER_PATH, value)) {
        DEBUG(logger, "Successfully write " << value << " for cluster scheduler");
        return true;
    } else {
        ERROR(logger, "Failed to write " << value << " for cluster scheduler");
        return false;
    }
}

bool ClusterCpuTune::Init()
{
	std::ifstream file(CLUSTER_PATH);
	if (!file.is_open()) {
		return false;
	}
	file.close();
    return true;
}

oeaware::Result ClusterCpuTune::Enable(const std::string &param)
{
    (void)param;
    int result;

    result = Init();
    if (!result) {
        ERROR(logger, "Init failed");
        return oeaware::Result(FAILED, "Init failed");
    }

	result = ReadClusterFile(ClusterStatus);
	if (!result) {
		return oeaware::Result(FAILED, "enable cluster scheduler failed");
	}
	result = WriteClusterFile(ENABLE_CLUSTER);
	if (!result) {
		return oeaware::Result(FAILED, "enable cluster scheduler failed");
	}

    return oeaware::Result(OK);
}

void ClusterCpuTune::Disable()
{
	WriteClusterFile(ClusterStatus);
}

void ClusterCpuTune::Run()
{
}
