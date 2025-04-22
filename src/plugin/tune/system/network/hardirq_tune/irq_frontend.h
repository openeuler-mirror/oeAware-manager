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

#ifndef HARDIRQ_FRONTEND_H
#define HARDIRQ_FRONTEND_H

#include <string>
#include <unordered_map>
#include <vector>

struct EthQueInfo {
    std::string dev;
    int operstate;
    std::string busInfo;
    std::vector<std::string> regex; /* create by dev/operstate/busInfo ... */
    std::unordered_map<int, int> que2Irq;  /* use regex to match irqWithDesc */
    void AddRegex(const std::vector<std::string> &regexSrc);
    void UpdateQue2Irq(std::unordered_map<int, std::string> &irqWithDesc);
};

/* this class is used to resolve the config and get the que2irq with rule */
class IrqFrontEnd {
public:
    IrqFrontEnd() {}
    ~IrqFrontEnd() {}
    bool InitConf(const std::string &path);
    std::vector<std::string> GetQueRegex() { return queRegex; }
    void GetEthQueData(std::unordered_map<std::string, EthQueInfo> &ethQueData, const std::vector<std::string> &queRegex);
    void GetIrqWithDesc(std::unordered_map<int, std::string> &irqWithDesc);
private:
    std::string confPath;
    std::vector<std::string> queRegex;
    bool ReadQue2IrqConf();
};

#endif