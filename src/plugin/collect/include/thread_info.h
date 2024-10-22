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
#ifndef THREAD_INFO_H
#define THREAD_INFO_H
#include <string>
#include "base_data.h"

struct ThreadInfo : oeaware::BaseData {
    ThreadInfo() = default;
    explicit ThreadInfo(const ThreadInfo* ptr): pid(ptr->pid), tid(ptr->tid), name(ptr->name){}
    ThreadInfo(int pid, int tid, std::string name) : pid(pid), tid(tid), name(name){}
    int pid = 0;
    int tid = 0;
    std::string name {};
    void Serialize(oeaware::OutStream &out) const
    {}
    void Deserialize(oeaware::InStream &in)
    {}
};

#endif // !THREAD_INFO_H