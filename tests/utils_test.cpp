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
#include <gtest/gtest.h>
#include "oeaware/utils.h"

TEST(SplitString, SplitString)
{
    std::string s = "a   b";
    auto values = oeaware::SplitString(s, " ");
    EXPECT_EQ(values[0], "a");
    EXPECT_EQ(values[1], "");
    EXPECT_EQ(values[2], "");
    EXPECT_EQ(values[3], "b");
}

TEST(FileExist, FileExist)
{
    EXPECT_TRUE(oeaware::FileExist("/home"));
    EXPECT_FALSE(oeaware::FileExist("/home/111"));
}

TEST(CheckPermission, CheckPermission)
{
    EXPECT_TRUE(oeaware::CheckPermission("/home", 0755));
    EXPECT_FALSE(oeaware::CheckPermission("/home", 0756));
}

TEST(EndWith, EndWith)
{
    EXPECT_TRUE(oeaware::EndWith("hello world", "world"));
    EXPECT_TRUE(oeaware::EndWith("hello world", "hello world"));
    EXPECT_FALSE(oeaware::EndWith("hello world", "worl"));
}

TEST(Concat, Concat)
{
    EXPECT_EQ("oeaware::hello", oeaware::Concat({"oeaware", "hello"}, "::"));
    EXPECT_EQ("oeaware:: ::hello", oeaware::Concat({"oeaware", " ", "hello"}, "::"));
    EXPECT_EQ("::oeaware::hello", oeaware::Concat({"", "oeaware", "hello"}, "::"));
}

TEST(GetGidByGroupName, GetGidByGroupName)
{
    EXPECT_EQ(0, oeaware::GetGidByGroupName("root"));
    EXPECT_EQ(1, oeaware::GetGidByGroupName("bin"));
}

TEST(SetDataListTopic, SetDataListTopic)
{
    DataList dataList;
    oeaware::SetDataListTopic(&dataList, "hello", "new", "world");
    EXPECT_EQ(0, strcmp(dataList.topic.instanceName, "hello"));
    EXPECT_EQ(0, strcmp(dataList.topic.topicName, "new"));
    EXPECT_EQ(0, strcmp(dataList.topic.params, "world"));
}

TEST(UtilsTest, ExecCommand)
{
    std::string output;
    bool ret = oeaware::ExecCommand("cat /proc/cpuinfo | grep processor | wc -l", output);
    output.erase(std::remove(output.begin(), output.end(), '\n'), output.end());
    int validCpuNum = sysconf(_SC_NPROCESSORS_ONLN);
    EXPECT_EQ(true, ret);
    EXPECT_EQ(std::to_string(validCpuNum), output);
}

TEST(UtilsTest, ServiceControlTest)
{
    bool isActive = true;
    bool ret = oeaware::ServiceControl("irqbalance", "stop");
    EXPECT_EQ(true, ret);
    ret = oeaware::ServiceIsActive("irqbalance", isActive);
    EXPECT_EQ(true, ret);
    EXPECT_EQ(false, isActive);
    ret = oeaware::ServiceControl("irqbalance", "start");
    EXPECT_EQ(true, ret);
    ret = oeaware::ServiceIsActive("irqbalance", isActive);
    EXPECT_EQ(true, ret);
    EXPECT_EQ(true, isActive);
}