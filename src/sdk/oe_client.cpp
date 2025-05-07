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
#include "oe_client.h"
#include <unordered_map>
#include <thread>
#include <unistd.h>
#include "domain_socket.h"
#include "oeaware/utils.h"
#include "message_protocol.h"
#include "oeaware/safe_queue.h"
#include "oeaware/default_path.h"
#include "data_register.h"

namespace oeaware {
class Impl {
public:
    Impl() noexcept : domainSocket(nullptr), socketStream(nullptr) { }
    int Init();
    int Subscribe(const CTopic &topic, Callback callback);
    int Unsubscribe(const CTopic &topic);
    int Publish(const DataList &dataList);
    void Close();
private:
    void HandleRecv();
    int HandleRequest(const Opt &opt, const std::vector<std::string> &payload);
private:
    std::shared_ptr<DomainSocket> domainSocket;
    std::shared_ptr<SocketStream> socketStream;
    std::mutex subMutex;
    std::mutex unsubMutex;
    std::mutex pubMutex;
    std::unordered_map<std::string, std::vector<Callback>> topicHandle;
    std::shared_ptr<SafeQueue<Result>> resultQueue;
    std::mutex quitMutex;
    bool isQuit;
    std::condition_variable cond;
    bool finished = false;
};

void Impl::HandleRecv()
{
    while (!finished) {
        MessageProtocol protocol;
        if (!RecvMessage(*socketStream, protocol)) {
            continue;
        }
        Message message = protocol.GetMessage();
        Result result;
        switch (message.opt) {
            case Opt::SUBSCRIBE:
            case Opt::UNSUBSCRIBE:
            case Opt::PUBLISH: {
                InStream in(message.payload[0]);
                ResultDeserialize(&result, in);
                resultQueue->Push(result);
                break;
            }
            case Opt::DATA: {
                DataList dataList;
                InStream in(message.payload[0]);
                if (DataListDeserialize(&dataList, in) < 0) {
                    continue;
                }
                auto topic = dataList.topic;
                auto key = Concat({topic.instanceName, topic.topicName, topic.params}, "::");
                for (auto handle : topicHandle[key]) {
                    handle(&dataList);
                }
                DataListFree(&dataList);
                break;
            }
            default:
                break;
        }
    }
    std::unique_lock<std::mutex> lock(quitMutex);
    isQuit = true;
    cond.notify_one();
}
int Impl::Init()
{
    pid_t pid = getpid();
    auto home = getenv("HOME");
    std::string homeDir;
    if (home == nullptr) {
        homeDir = "/var/run/oeAware";
    } else {
        homeDir = home;
        homeDir += "/.oeaware";
    }
    
    CreateDir(homeDir);
    isQuit = false;
    finished = false;
    domainSocket = std::make_shared<DomainSocket>(homeDir + "/oeaware-sdk-" + std::to_string(pid) + ".sock");
    domainSocket->SetRemotePath(DEFAULT_SERVER_LISTEN_PATH);
    resultQueue = std::make_shared<SafeQueue<Result>>();
    int sock = domainSocket->Socket();
    if (sock < 0) {
        return -1;
    }
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    socketStream = std::make_shared<SocketStream>(sock);
    if (domainSocket->Bind() < 0) {
        return -1;
    }
    if (domainSocket->Connect() < 0) {
        return -1;
    }
    std::thread t([this]() {
        this->HandleRecv();
    });
    t.detach();
    return 0;
}

int Impl::HandleRequest(const Opt &opt, const std::vector<std::string> &payload)
{
    MessageProtocol protocol(MessageHeader(MessageType::REQUEST), Message(opt, payload));
    if (socketStream == nullptr) {
        return -1;
    }
    SendMessage(*socketStream, protocol);
    Result result;
    if (!resultQueue->WaitTimeAndPop(result)) {
        return -1;
    }
    delete[] result.payload;
    return result.code;
}

int Impl::Subscribe(const CTopic &topic, Callback callback)
{
    OutStream out;
    TopicSerialize(&topic, out);
    auto key = Concat({topic.instanceName, topic.topicName, topic.params}, "::");
    std::lock_guard<std::mutex> lock(subMutex);
    topicHandle[key].emplace_back(callback);
    if (HandleRequest(Opt::SUBSCRIBE, {out.Str()}) < 0) {
        topicHandle[key].pop_back();
        return -1;
    }
    return 0;
}

int Impl::Unsubscribe(const CTopic &topic)
{
    OutStream out;
    TopicSerialize(&topic, out);
    std::lock_guard<std::mutex> lock(unsubMutex);
    if (HandleRequest(Opt::UNSUBSCRIBE, {out.Str()}) < 0) {
        return -1;
    }
    auto key = Concat({topic.instanceName, topic.topicName, topic.params}, "::");
    topicHandle.erase(key);
    return 0;
}

int Impl::Publish(const DataList &dataList)
{
    OutStream out;
    DataListSerialize(&dataList, out);
    std::lock_guard<std::mutex> lock(pubMutex);
    return HandleRequest(Opt::PUBLISH, {out.Str()});
}

void Impl::Close()
{
    finished = true;
    // wait handrev over.
    std::unique_lock<std::mutex> lock(quitMutex);
    cond.wait(lock, [this] {return isQuit;});
    domainSocket->Close();
    domainSocket.reset();
    resultQueue.reset();
    socketStream.reset();
    topicHandle.clear();
}
}

static oeaware::Impl impl;

int OeInit()
{
    oeaware::Register::GetInstance().InitRegisterData();
    return impl.Init();
}

int OeSubscribe(const CTopic *topic, Callback callback)
{
    return impl.Subscribe(*topic, callback);
}

int OeUnsubscribe(const CTopic *topic)
{
    return impl.Unsubscribe(*topic);
}

int OePublish(const DataList *dataList)
{
    return impl.Publish(*dataList);
}

void OeClose()
{
    impl.Close();
}
