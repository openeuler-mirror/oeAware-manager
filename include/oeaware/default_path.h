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
#ifndef OEAWARE_DEFAULT_PATH_H
#define OEAWARE_DEFAULT_PATH_H
#include <string>

namespace oeaware {
const std::string DEFAULT_PLUGIN_PATH = "/usr/lib64/oeAware-plugin";
const std::string DEFAULT_RUN_PATH = "/var/run/oeAware";
const std::string DEFAULT_LOG_PATH = "/var/log/oeAware";
const std::string DEFAULT_SERVER_LISTEN_PATH = "/var/run/oeAware/oeAware-server";
const std::string DEFAULT_SDK_CONN_PATH = "/var/run/oeAware/oeAware-sdk";
const std::string DEFAULT_PRELOAD_PATH = "/etc/oeAware/preload.yaml";
const std::string DEFAULT_ANALYSYS_PATH = "/etc/oeAware/analysis_config.yaml";
const std::string DEFAULT_PLUGIN_CONFIG_PATH = "/etc/oeAware/plugin";
const std::string DEFAULT_CONFIG_PATH = "/etc/oeAware/config.yaml";
const std::string DEFAULT_REALTIMETUNE_CONFIG_PATH = "/etc/oeAware/plugin/realtime_tune.yaml";
}

#endif