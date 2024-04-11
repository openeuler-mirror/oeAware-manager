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
#include "message_manager.h"
#include <thread>
#include "default_path.h"

int TcpSocket::domain_listen(const char *name) {
    int len;
    struct sockaddr_un un;
    unlink(name);
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    len = offsetof(struct sockaddr_un, sun_path)  + strlen(name);

    if (bind(sock, (struct sockaddr*)&un, len) < 0) {
        ERROR("[MessageManager] bind error!");
        return -1;
    }
    if (listen(sock, 20) < 0) {
        ERROR("[MessageManager] listen error!");
        return -1;
    }
    INFO("[MessageManager] listen : " << name);
    return 0;
}

bool TcpSocket::init() {
    create_dir(DEFAULT_RUN_PATH);
    std::string path = DEFAULT_RUN_PATH + "/oeAware-sock";
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        ERROR("[MessageManager] socket error!");
        return false;
    }
    if (domain_listen(path.c_str()) < 0) {
        close(sock);
        return false;
    }
    epfd = epoll_create(1);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);
    return true;

}
static void send_msg(Msg &msg, SafeQueue<Message> *handler_msg) {
    Message message;
    message.set_opt(msg.opt());
    message.set_type(MessageType::EXTERNAL);
    for (int i = 0; i < msg.payload_size(); ++i) {
        message.add_payload(msg.payload(i));
    }
    handler_msg->push(message);
}
static void recv_msg(Msg &msg, SafeQueue<Message> *res_msg) {
    Message res;
    res_msg->wait_and_pop(res);
    msg.set_opt(res.get_opt());
    for (int i = 0; i < res.get_payload_len(); ++i) {
        msg.add_payload(res.get_payload(i));
    }
}

void TcpSocket::serve_accept(SafeQueue<Message> *handler_msg, SafeQueue<Message> *res_msg){
    struct epoll_event evs[MAX_EVENT_SIZE];
    int sz = sizeof(evs) / sizeof(struct epoll_event);
    char buf[MAX_RECV_BUFF_SIZE];
    while (true) {
        int num = epoll_wait(epfd, evs, sz, -1);
        for (int i = 0; i < num; ++i) {
            int cur_fd = evs[i].data.fd;
            if (cur_fd == sock) {
                conn = accept(cur_fd, NULL, NULL);
                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = conn;
                epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &ev);
                DEBUG("[MessageManager] client connected!\n");
            } else {
                SocketStream stream(cur_fd);
                MessageProtocol msg_protocol;
                Msg client_msg;
                Msg internal_msg;
                if (!handle_request(stream, msg_protocol)) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, cur_fd, NULL);
                    close(cur_fd);
                    DEBUG("[MessageManager] one client disconnected!\n");
                    continue;
                }
                decode(client_msg, msg_protocol.body);
                send_msg(client_msg, handler_msg);
                recv_msg(internal_msg, res_msg);
                if (!send_response(stream, internal_msg)) {
                    WARN("[MessageManager] send msg to client failed!");
                }
            }
        }
    }
}

void handler(MessageManager *mgr) {
    TcpSocket* tcp_socket = mgr->tcp_socket;
    if (!tcp_socket->init()) {
        return;
    }
    tcp_socket->serve_accept(mgr->handler_msg, mgr->res_msg);
}

void MessageManager::run() {
    std::thread t(handler, this);
    t.detach();
}