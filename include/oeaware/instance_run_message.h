
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
#ifndef OEAWARE_INSTANCE_RUN_MESSAGE_H
#define OEAWARE_INSTANCE_RUN_MESSAGE_H
#include <mutex>
#include <condition_variable>
#include <oeaware/topic.h>

namespace oeaware {
enum class RunType {
    /* Message from PluginManager. */
    ENABLED,
    DISABLED,
    DISABLED_FORCE,
    SUBSCRIBE,
    UNSUBSCRIBE,
    UNSUBSCRIBE_SDK,
    PUBLISH_DATA,
    PUBLISH,
    SHUTDOWN,
 };

/* Message for communication between plugin manager and instance scheduling */
class InstanceRunMessage {
public:
    InstanceRunMessage() {}
    explicit InstanceRunMessage(RunType type) : type(type) { }
    InstanceRunMessage(RunType type, const std::vector<std::string> &payload) : payload(payload), type(type),
        finish(false) { }
    RunType GetType()
    {
        return type;
    }
    void Wait()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [this]() {
            return finish;
        });
    }
    void NotifyOne()
    {
        std::unique_lock<std::mutex> lock(mutex);
        finish = true;
        cond.notify_one();
    }

    std::vector<std::string> payload;
    Result result;
    DataList dataList;
private:
    RunType type;
    std::mutex mutex;
    std::condition_variable cond;
    bool finish;
};

enum class InstanceMessageType {
    SUBSCRIBE,
    UNSUBSCRIBE,
    PUBLISH_DATA,
};
}
#endif