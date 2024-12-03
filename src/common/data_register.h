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
#ifndef COMMON_DATA_REGISTER_H
#define COMMON_DATA_REGISTER_H
#include <unordered_map>
#include "oeaware/serialize.h"
#include "oeaware/data_list.h"

namespace oeaware {
using DeserializeFunc = int(*)(void**, InStream &in);
using SerializeFunc = int(*)(const void*, OutStream &out);
using DataFreeFunc = void(*)(void *);

struct RegisterEntry {
    RegisterEntry() { }
    RegisterEntry(const SerializeFunc &se, const DeserializeFunc &de) : se(se), de(de) { }
    RegisterEntry(const SerializeFunc &se, const DeserializeFunc &de, const DataFreeFunc &free) : se(se),
        de(de), free(free) { }
    SerializeFunc se;
    DeserializeFunc de;
    DataFreeFunc free;
};

class Register {
public:
    Register(const Register&) = delete;
    Register& operator=(const Register&) = delete;
    static Register& GetInstance()
    {
        static Register reg;
        return reg;
    }
    // Init all data structure
    void InitRegisterData();
    DeserializeFunc GetDataDeserialize(const std::string &name);
    SerializeFunc GetDataSerialize(const std::string &name);
    DataFreeFunc GetDataFreeFunc(const std::string &name);
    void RegisterData(const std::string &name, const RegisterEntry &func);
private:
    Register() { };

    std::unordered_map<std::string, RegisterEntry> registerEntry;
};
void DataListFree(DataList *dataList);
int DataListSerialize(const DataList *dataList, OutStream &out);
int DataListDeserialize(DataList *dataList, InStream &in);
int ResultDeserialize(void *data, InStream &in);
int TopicSerialize(const CTopic *topic, OutStream &out);
int TopicDeserialize(CTopic *topic, InStream &in);
void TopicFree(CTopic *topic);
}

#endif
