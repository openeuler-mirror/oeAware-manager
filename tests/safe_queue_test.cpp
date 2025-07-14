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
#include <future>
#include <chrono>
#include "oeaware/safe_queue.h"

TEST(SafeQueue, TryPop)
{
    oeaware::SafeQueue<int> q;
    int x = 0;
    EXPECT_EQ(q.TryPop(x), false);
    q.Push(1);
    EXPECT_EQ(q.TryPop(x), true);
    EXPECT_EQ(x, 1);
}

TEST(SafeQueue, WaitTimeAndPop)
{
    oeaware::SafeQueue<int> q;
    int x = 0;
    EXPECT_EQ(q.WaitTimeAndPop(x), false);
    q.Push(1);
    EXPECT_EQ(q.WaitTimeAndPop(x), true);
    EXPECT_EQ(x, 1);
}

TEST(SafeQueue, WaitAndPop)
{
    oeaware::SafeQueue<int> q;
    int x = 0;
    auto future = std::async([&q, &x]() {
        q.WaitAndPop(x);
    });
    auto status = future.wait_for(std::chrono::seconds(2));
    EXPECT_EQ(status, std::future_status::timeout);
    q.Push(1);
    future.get();
    EXPECT_EQ(x, 1);
}