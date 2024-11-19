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
#ifndef PLUGIN_MGR_EVENT_ERROR_CODE_H
#define PLUGIN_MGR_EVENT_ERROR_CODE_H
#include <unordered_map>
#include <string>

namespace oeaware {
enum class ErrorCode {
    ENABLE_INSTANCE_NOT_LOAD,
    ENABLE_INSTANCE_UNAVAILABLE,
    ENABLE_INSTANCE_ALREADY_ENABLED,
    ENABLE_INSTANCE_ENV,
    DISABLE_INSTANCE_NOT_LOAD,
    DISABLE_INSTANCE_UNAVAILABLE,
    DISABLE_INSTANCE_ALREADY_DISABLED,
    REMOVE_PLUGIN_NOT_EXIST,
    REMOVE_INSTANCE_IS_RUNNING,
    REMOVE_INSTANCE_HAVE_DEP,
    LOAD_PLUGIN_FILE_NOT_EXIST,
    LOAD_PLUGIN_FILE_PERMISSION_DEFINED,
    LOAD_PLUGIN_FILE_IS_NOT_SO,
    LOAD_PLUGIN_EXIST,
    LOAD_PLUGIN_DLOPEN_FAILED,
    LOAD_PLUGIN_DLSYM_FAILED,
    QUERY_PLUGIN_NOT_EXIST,
    QUERY_DEP_NOT_EXIST,
    DOWNLOAD_NOT_FOUND,
    OK,
};

class ErrorText {
public:
    static std::string GetErrorText(ErrorCode code);
    static const std::unordered_map<ErrorCode, std::string> errorCodes;
};
}

#endif
