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
#ifndef COMMON_SAFE_QUEUE_H
#define COMMON_SAFE_QUEUE_H
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>

namespace oeaware {
template<typename T>
class SafeQueue {
public:
    explicit SafeQueue(std::chrono::seconds timeout = std::chrono::seconds(2)):timeout(timeout) { }
    void Push(T value)
    {
        std::unique_lock<std::mutex> lock(mutex);
        queue.push_back(value);
        cond.notify_one();
    }
    bool TryPop(T &value)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (queue.empty()) {
            return false;
        }
        value = std::move(queue.front());
        queue.pop_front();
        return true;
    }
    void WaitAndPop(T &value)
    {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [this] { return !queue.empty(); });
        value = queue.front();
        queue.pop_front();
    }
    bool WaitTimeAndPop(T &value)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (cond.wait_for(lock, timeout, [this] {
            return !queue.empty();
        })) {
            value = std::move(queue.front());
            queue.pop_front();
            return true;
        }
        return false;
    }
    bool Empty()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.empty();
    }
private:
    mutable std::mutex mutex;
    std::deque<T> queue;
    std::condition_variable cond;
    std::chrono::seconds timeout; // Queue timeout interval, in milliseconds.
};
}

#endif // !COMMON_QUEUE_H
