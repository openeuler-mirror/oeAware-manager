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
#ifndef COMMON_MESSAGE_H
#define COMMON_MESSAGE_H

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <vector>
#include <string>

namespace server {
    enum class Opt {
        LOAD,
        ENABLED,
        DISABLED,
        REMOVE,
        QUERY,
        QUERY_ALL,
        QUERY_TOP,
        QUERY_ALL_TOP,
        LIST,
        RESPONSE,
        SHUTDOWN,
    };

    class Msg {
        private:
            friend class boost::serialization::access;
            template <typename Archive>
            void serialize(Archive &ar, const unsigned int version) {
                ar & _opt;
                ar & _payload;
            }
        public:
            int payload_size() {
                return this->_payload.size();
            }
            std::string payload(int id) {
                return this->_payload[id];
            }
            Opt opt() {
                return this->_opt;
            }
            void add_payload(std::string &context) {
                this->_payload.emplace_back(context);
            }
            void add_payload(std::string &&context) {
                this->_payload.emplace_back(context);
            }
            void set_opt(Opt opt) {
                this->_opt = opt;
            }
        private:
            Opt _opt;
            std::vector<std::string> _payload;
    };

}

#endif // !COMMON_MESSAGE_H