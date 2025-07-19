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
#include "pmu_common.h"
#include <string>
#include <dirent.h>
#include "oeaware/utils.h"
#include <fstream>

static const std::string DEVICES_PATH = "/sys/bus/event_source/devices/";

bool IsRiscvPmuSupported() {
    std::string cpuTypePath = DEVICES_PATH + "cpu/type";
    if (oeaware::FileExist(cpuTypePath)) {
        std::ifstream typeFile(cpuTypePath);
        std::string typeStr;
        if (std::getline(typeFile, typeStr) && typeStr == "4") {
            return true;
        }
    }

    return false;
}

bool IsSupportPmu()
{
#ifdef __riscv
    return IsRiscvPmuSupported();
#endif
    DIR *dir = opendir(DEVICES_PATH.data());
    if (dir == nullptr) {
        return false;
    }
    struct dirent *dent = nullptr;
    while ((dent = readdir(dir))) {
        std::string armPmuPath = DEVICES_PATH + dent->d_name + "/cpus";
        if (oeaware::FileExist(armPmuPath)) {
            closedir(dir);
            return true;
        }
    }
    closedir(dir);
    return false;
}
