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

#include "seep_tune.h"
#include "interface.h"
#include <fstream>
#include <sstream>
#include <unistd.h>

using namespace oeaware;

Seep::Seep()
{
    name = "seep_tune";
    description = "";
    version = "1.0.0";
    period = -1;
    priority = 2;
    type = 1;
}

oeaware::Result Seep::OpenTopic(const oeaware::Topic &topic)
{
    (void)topic;
    return oeaware::Result(OK);
}

void Seep::CloseTopic(const oeaware::Topic &topic)
{
    (void)topic;
}

void Seep::UpdateData(const DataList &dataList)
{
    (void)dataList;
}

static bool IsFileExists(const std::string &path)
{
    return access(path.c_str(), F_OK) == 0;
}

static bool WriteFile(const std::string &filePath, const std::string &value)
{
    std::ofstream file(filePath);
    if (!file.is_open())
        return false;
    file << value;
    file.close();
    return true;
}

bool Seep::WriteSeepFile(const unsigned int &cpu, const std::string &action)
{
    std::string value = "0";
    std::ostringstream seepPath;
    seepPath << "/sys/devices/system/cpu/cpu" << cpu << "/cpufreq/auto_select";

    if (action == "enable")
        value = "1";

    if (IsFileExists(seepPath.str()))
    {
        std::cout << "Writing " << value << " to: " << seepPath.str() << std::endl;

        if (WriteFile(seepPath.str(), value))
        {
            std::cout << "Successfully " << action << " seep for cpu" << cpu << std::endl;
            return true;
        }
        else
        {
            std::cerr << "Failed to " << action << " seep for cpu" << cpu << std::endl;
            return false;
        }
    }
    else
    {
        std::cerr << "cpu" << cpu << " seep directory not available" << std::endl;
        return false;
    }
}

oeaware::Result Seep::Enable(const std::string &param)
{
    (void)param;
    int result;

    // 后续要考虑seep已经使能的情况下插件该如何管理，例如在初始化阶段可以获取各cpu的状态
    if (!isInit)
    {
        int cpuNum = sysconf(_SC_NPROCESSORS_ONLN);
        if (cpuNum == -1)
        {
            std::cerr << "Can not get the cpu num" << std::endl;
            return oeaware::Result(FAILED);
        }

        for (unsigned int i = 0; i < cpuNum; ++i)
        {
            result = WriteSeepFile(i, "enable");
            if (!result)
            {
                std::cout << "Try to restore the enabled cpu" << std::endl;
                for (unsigned int j = 0; j < i; ++j)
                {
                    WriteSeepFile(j, "disable");
                }
                return oeaware::Result(FAILED);
            }
        }

        isInit = true;
    }

    return oeaware::Result(OK);
}

void Seep::Disable()
{
    for (unsigned int i = 0; i < cpuNum; ++i)
    {
        WriteSeepFile(i, "disable");
    }
    isInit = false;
}

void Seep::Run()
{
}

void Seep::ReadConfig()
{
    // 后续需要支持几个参数配置
}
