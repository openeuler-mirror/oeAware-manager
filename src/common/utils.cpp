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
#include <fstream>
#include <sys/stat.h>

namespace oeaware {
const static int ST_MODE_MASK = 0777;
const static int HTTP_OK = 200;

static size_t WriteData(char *ptr, size_t size, size_t nmemb, FILE *file)
{
    return fwrite(ptr, size, nmemb, file);
}

// set curl options
static void CurlSetOpt(CURL *curl, const std::string &url, FILE *file)
{
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
}

static bool CurlHandle(CURL *curl, const std::string &url, const std::string &path)
{
    FILE *file = fopen(path.c_str(), "wb");
    if (file == nullptr) {
        return false;
    }
    CurlSetOpt(curl, url, file);
    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    if (fclose(file) == EOF) {
        return false;
    }
    if (res == CURLE_OK && httpCode == HTTP_OK) {
        return true;
    }
    return false;
}

// Downloads file from the specified url to the path.
bool Download(const std::string &url, const std::string &path)
{
    CURL *curl = nullptr;
    bool ret = true;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        if (!CurlHandle(curl, url, path)) {
            ret = false;
        }
    } else {
        ret = false;
    }
    curl_global_cleanup();
    curl_easy_cleanup(curl);
    return ret;
}

// Check the file permission. The file owner is root.
bool CheckPermission(const std::string &path, int mode)
{
    struct stat st;
    lstat(path.c_str(), &st);
    int curMode = (st.st_mode & ST_MODE_MASK);
    if (st.st_gid != 0 && st.st_uid != 0) {
        return false;
    }
    if (curMode != mode) {
        return false;
    }
    return true;
}

bool FileExist(const std::string &fileName)
{
    std::ifstream file(fileName);
    return file.good();
}

bool EndWith(const std::string &s, const std::string &ending)
{
    if (s.length() >= ending.length()) {
        return (s.compare(s.length() - ending.length(), ending.length(), ending) == 0);
    } else {
        return false;
    }
}

}
