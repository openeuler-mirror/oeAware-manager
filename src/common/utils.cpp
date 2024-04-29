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
#include "utils.h"
#include <curl/curl.h>

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *file) {
    return fwrite(ptr, size, nmemb, file);
}

// set curl options
static void curl_set_opt(CURL *curl, const std::string &url, FILE *file) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        
}
static bool curl_handle(CURL *curl, const std::string &url, const std::string &path) {
    FILE *file = fopen(path.c_str(), "wb");
    if (file == nullptr) {
        return false;
    }
    curl_set_opt(curl, url, file);
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    fclose(file);
    if (res == CURLE_OK && http_code >= 200 && http_code < 300) {
        return true;
    } 
    return false;
}

// Downloads file from the specified url to the path.
bool download(const std::string &url, const std::string &path) {
    CURL *curl = nullptr;
    bool ret = true;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        if (!curl_handle(curl, url, path)) ret = false;
    } else {
        ret = false;
    }
    curl_global_cleanup();
    curl_easy_cleanup(curl);
    return ret;
}