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
#ifndef ENV_H
#define ENV_H

#include <vector>
#include <sched.h>
#include <cstdint>
#include <sched.h>


class Env {
private:
    Env() = default;
    ~Env() = default;
public:
    static Env &GetInstance()
    {
        static Env instance;
        return instance;
    }
    Env(const Env &) = delete;
    Env &operator = (const Env &) = delete;
    // common para
    int numaNum;
    int cpuNum;
    unsigned long pageMask = 0;
    std::vector<int> cpu2Node;
    std::vector<std::vector<int>> distance;
    int maxDistance;
    int diffDistance;
    bool Init();
    void InitDistance();
};

#endif