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
#include "domain_socket.h"
#include "utils.h"
#include "message_protocol.h"
#include "safe_queue.h"
#include "default_path.h"

namespace oeaware {
class OeClient::Impl {
public:
    int Init();
    int Subscribe(const Topic &topic, Callback callback);
    int Unsubscribe(const Topic &topic);
    int Publish(const Topic &topic, const DataList &dataList);
    void Close();
private:
    void HandleRecv();
    int HandleRequest(const Opt &opt, const std::vector<std::string> &payload);
private:
    std::shared_ptr<DomainSocket> domainSocket;
    std::shared_ptr<SocketStream> socketStream;
    std::unordered_map<std::string, std::vector<Callback>> topicHandle;
    std::shared_ptr<SafeQueue<Result>> resultQueue;
    bool finished = false;
};

void OeClient::Impl::HandleRecv()
{
    while (!finished) {
        MessageProtocol protocol;
        if (!RecvMessage(*socketStream, protocol)) {
            break;
        }
        Message message = protocol.GetMessage();
        Result result;
        switch (message.opt) {
            case Opt::SUBSCRIBE:
            case Opt::UNSUBSCRIBE:
            case Opt::PUBLISH: {
                Decode(result, message.payload[0]);
                resultQueue->Push(result);
                break;
            }
            case Opt::DATA: {
                DataList messageList;
                Decode(messageList, message.payload[0]);
                auto topic = messageList.topic;
                for (auto handle : topicHandle[Concat({topic.instanceName, topic.topicName, topic.params}, "::")]) {
                    handle(messageList);
                }
                break;
            }
            default:
                break;
        }
    }
}
int OeClient::Impl::Init()
{
    domainSocket = std::make_shared<DomainSocket>(DEFAULT_SDK_CONN_PATH);
    domainSocket->SetRemotePath(DEFAULT_SERVER_LISTEN_PATH);
    resultQueue = std::make_shared<SafeQueue<Result>>();
    int sock = domainSocket->Socket();
    if (sock < 0) {
        return -1;
    }
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

int OeClient::Impl::HandleRequest(const Opt &opt, const std::vector<std::string> &payload)
{
    MessageProtocol protocol(MessageHeader(MessageType::REQUEST), Message(opt, payload));
    SendMessage(*socketStream, protocol);
    Result result;
    if (!resultQueue->WaitTimeAndPop(result)) {
        return -1;
    }
    return result.code;
}

int OeClient::Impl::Subscribe(const Topic &topic, Callback callback)
{
    if (HandleRequest(Opt::SUBSCRIBE, {Encode(topic)}) < 0) {
        return -1;
    }
    auto key = Concat({topic.instanceName, topic.topicName, topic.params}, "::");
    topicHandle[key].emplace_back(callback);
    return 0;
}

int OeClient::Impl::Unsubscribe(const Topic &topic)
{
    if (HandleRequest(Opt::UNSUBSCRIBE, {Encode(topic)}) < 0) {
        return -1;
    }
    auto key = Concat({topic.instanceName, topic.topicName, topic.params}, "::");
    topicHandle.erase(key);
    return 0;
}

int OeClient::Impl::Publish(const Topic &topic, const DataList &dataList)
{
    return HandleRequest(Opt::PUBLISH, {Encode(topic), Encode(dataList)});
}

void OeClient::Impl::Close()
{
    domainSocket->Close();
    finished = true;
}

OeClient::OeClient() : impl(new Impl()) { }

OeClient::~OeClient()
{
    delete impl;
}
int OeClient::Init()
{
    return impl->Init();
}

int OeClient::Subscribe(const Topic &topic, Callback callback)
{
    return impl->Subscribe(topic, callback);
}

int OeClient::Unsubscribe(const Topic &topic)
{
    return impl->Unsubscribe(topic);
}

int OeClient::Publish(const Topic &topic, const DataList &dataList)
{
    return impl->Publish(topic, dataList);
}

void OeClient::Close()
{
    impl->Close();
}
}
