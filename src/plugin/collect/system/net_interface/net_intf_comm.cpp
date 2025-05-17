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

#include <unordered_map>
#include <unordered_set>
#include <dirent.h>
#include <fstream>
#include "oeaware/utils.h"
#include "net_intf_comm.h"

std::unordered_set<std::string> GetNetInterface()
{
    std::unordered_set<std::string> directories;
    std::string netDir = "/sys/class/net";
    DIR *dir = opendir(netDir.c_str());
    if (dir != nullptr) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string interface = std::string(entry->d_name);
            /* There may be many non network card files in the folder, and it is believed that
            the folder with a operstate is the network interface */
            if (interface != "." && interface != ".." && oeaware::FileExist(netDir + "/" + interface + "/operstate")) {
                directories.insert(interface);
            }
        }
        closedir(dir);
    }
    return directories;
}

bool ReadEthtoolDrvinfo(const std::string &name, struct ethtool_drvinfo &drvinfo)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq ifr;
    memset_s(&ifr, sizeof(ifr), 0, sizeof(ifr));
    strncpy_s(ifr.ifr_name, sizeof(ifr.ifr_name), name.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    drvinfo.cmd = ETHTOOL_GDRVINFO;
    ifr.ifr_data = reinterpret_cast<char *>(&drvinfo.cmd);
    int err = ioctl(fd, SIOCETHTOOL, &ifr);
    if (err < 0) {
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

int GetOperState(const std::string &name)
{
    int type = IF_OPER_UNKNOWN;
    std::string path = "/sys/class/net/" + name + "/operstate";
    std::ifstream file(path);
    if (!file.is_open()) {
        return IF_OPER_UNKNOWN;
    }

    std::string state;
    if (std::getline(file, state)) {
        state.erase(state.find_last_not_of(" \n\r\t") + 1);
    } else {
        return IF_OPER_UNKNOWN;
    }

    file.close();
    type = oeaware::GetNetOperateTypeByStr(state);
    return type;
}

void GetNetIntfBaseInfo(const std::string &name, NetIntfBaseInfo &info)
{
    info.name = name;
    info.operstate = GetOperState(name);
    info.ifindex = if_nametoindex(name.c_str());
}
