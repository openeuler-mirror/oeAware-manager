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
#include "error_code.h"

const std::unordered_map<ErrorCode, std::string> ErrorText::error_codes = {
    {ErrorCode::ENABLE_INSTANCE_NOT_LOAD, "instance is not loaded"},
    {ErrorCode::ENABLE_INSTANCE_UNAVAILABLE, "instance is unavailable"},
    {ErrorCode::ENABLE_INSTANCE_ALREADY_ENABLED, "instance is already enabled"},
    {ErrorCode::DISABLE_INSTANCE_NOT_LOAD, "instance is not loaded"},
    {ErrorCode::DISABLE_INSTANCE_UNAVAILABLE, "instance is unavailable"},
    {ErrorCode::DISABLE_INSTANCE_ALREADY_DISABLED, "instance is already disabled"},
    {ErrorCode::REMOVE_PLUGIN_NOT_EXIST, "plugin does not exist"},
    {ErrorCode::REMOVE_INSTANCE_IS_RUNNING, "instance is running"},
    {ErrorCode::REMOVE_INSTANCE_HAVE_DEP, "instance with pre-dependency"},
    {ErrorCode::LOAD_PLUGIN_FILE_NOT_EXIST, "plugin file does not exist"},
    {ErrorCode::LOAD_PLUGIN_FILE_IS_NOT_SO, "file is not a plugin file"},
    {ErrorCode::LOAD_PLUGIN_FILE_PERMISSION_DEFINED, "plugin file permission is not the specified permission"},
    {ErrorCode::LOAD_PLUGIN_EXIST, "plugin already loaded"},
    {ErrorCode::LOAD_PLUGIN_DLOPEN_FAILED, "plugin dlopen failed"},
    {ErrorCode::LOAD_PLUGIN_DLSYM_FAILED, "plugin dlsym failed"},
    {ErrorCode::QUERY_PLUGIN_NOT_EXIST, "plugin does not exist"},
    {ErrorCode::QUERY_DEP_NOT_EXIST, "instance does not exist"},
    {ErrorCode::DOWNLOAD_NOT_FOUND, "unable to find a match"},
};
std::string ErrorText::get_error_text(ErrorCode code) {
    auto it = ErrorText::error_codes.find(code);
    if (it != ErrorText::error_codes.end()) {
        return it->second;
    } else {
        return "unknown error.";
    }
}