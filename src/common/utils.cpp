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
#include "oeaware/utils.h"
#include <algorithm>
#include <fstream>
#include <regex>
#include <securec.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <grp.h>
#include <iostream>
#include <cctype>
#include <array>

namespace oeaware {
const static int ST_MODE_MASK = 0777;
const static int HTTP_OK = 200;
const static int EXEC_COMMAND_BUFLEN = 512;

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

bool CheckFileUsers(const std::string &path, const std::vector<uid_t> &users)
{
    struct stat st;
    if (lstat(path.c_str(), &st) < 0) {
        return false;
    }
    return std::any_of(users.begin(), users.end(), [&](uid_t uid) {
        return uid == st.st_uid;
    });
}

bool CheckFileGroups(const std::string &path, const std::vector<gid_t> &groups)
{
    struct stat st;
    if (lstat(path.c_str(), &st) < 0) {
        return false;
    }
    return std::any_of(groups.begin(), groups.end(), [&](gid_t gid) {
        return gid == st.st_gid;
    });
}

int GetGidByGroupName(const std::string &groupName)
{
    struct group *grp = getgrnam(groupName.c_str());
    if (grp == nullptr) {
        return -1;
    }
    return grp->gr_gid;
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

std::string Concat(const std::vector<std::string>& strings, const std::string &split)
{
    std::string ret;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i) {
            ret += split;
        }
        ret += strings[i];
    }
    return ret;
}

std::vector<std::string> SplitString(const std::string &str, const std::string &split)
{
    std::vector<std::string> tokens;
    std::regex re(split);
    std::sregex_token_iterator iter(str.begin(), str.end(), re, -1);
    std::sregex_token_iterator end;
    while (iter != end) {
        tokens.emplace_back(*iter++);
    }
    return tokens;
}
bool CreateDir(const std::string &path)
{
    size_t  pos = 0;
    do {
        pos = path.find_first_of("/", pos + 1);
        std::string subPath = path.substr(0, pos);
        struct stat buffer;
        if (stat(subPath.c_str(), &buffer) == 0) {
            continue;
        }
        if (mkdir(subPath.c_str(), S_IRWXU | S_IXGRP | S_IRGRP | S_IROTH | S_IXOTH) != 0) {
            return false;
        }
    } while (pos != std::string::npos);
    return true;
}

bool SetDataListTopic(DataList *dataList, const std::string &instanceName, const std::string &topicName,
    const std::string &params)
{
    dataList->topic.instanceName = new char[instanceName.size() + 1];
    if (dataList->topic.instanceName == nullptr) {
        return false;
    }
    strcpy_s(dataList->topic.instanceName, instanceName.size() + 1, instanceName.data());
    dataList->topic.topicName = new char[topicName.size() + 1];
    if (dataList->topic.topicName == nullptr) {
        delete[] dataList->topic.instanceName;
        dataList->topic.instanceName = nullptr;
        return false;
    }
    strcpy_s(dataList->topic.topicName, topicName.size() + 1, topicName.data());
    dataList->topic.params = new char[params.size() + 1];
    if (dataList->topic.params == nullptr) {
        delete[] dataList->topic.instanceName;
        delete[] dataList->topic.topicName;
        dataList->topic.instanceName = nullptr;
        dataList->topic.topicName = nullptr;
        return false;
    }
    strcpy_s(dataList->topic.params, params.size() + 1, params.data());
    return true;
}

uint64_t GetCpuCycles(int cpu)
{
    std::string freqPath = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cpufreq/scaling_cur_freq";
    std::ifstream freqFile(freqPath);

    if (!freqFile.is_open()) {
        return 0;
    }

    uint64_t freq;
    freqFile >> freq;
    freqFile.close();

    if (freqFile.fail()) {
        return 0;
    }

    return freq * 1000; // 1000: kHz to Hz
}

uint64_t GetCpuFreqByDmi()
{
    FILE *pipe = popen("dmidecode -t processor | grep 'Max Speed' | head -n 1 | awk '{print $3}'", "r");
    if (!pipe) {
        std::cout << "failed to run dmidecode" << std::endl;
        return 0;
    }

    char buffer[128];
    if (fgets(buffer, sizeof(buffer), pipe) == nullptr) {
        std::cout << "failed to get cpufreq by dmidecode" << std::endl;
        pclose(pipe);
        return 0;
    }
    pclose(pipe);

    std::string str(buffer);
    str.erase(str.find_last_not_of(" \t\n\r") + 1);

    for (char c : str) {
        if (!std::isdigit(c)) {
            std::cerr << "invalid CPU frequency format: " << str << std::endl;
            return 0;
        }
    }

    return std::stoull(buffer) * 1000000; // 1000000: MHz to Hz
}

// exec command and get output, not use blocking cmd(eg top)
bool ExecCommand(const std::string &command, std::string &result)
{
    result = "";
    std::array<char, EXEC_COMMAND_BUFLEN> buffer;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        return false;
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return true;
}

bool ServiceIsActive(const std::string &serviceName, bool &isActive)
{
    std::string command = "systemctl is-active " + serviceName;
    std::string ret;
    isActive = false;
    if (!ExecCommand(command, ret)) {
        return false;
    }
    ret.erase(std::remove(ret.begin(), ret.end(), '\n'), ret.end());
    isActive = ret == "active";
    return true;
}

bool ServiceControl(const std::string &serviceName, const std::string &action)
{
    // support cmd
    std::vector<std::string> actions = { "start", "stop", "restart" };
    if (std::find(actions.begin(), actions.end(), action) == actions.end()) {
        return false;
    }
    std::string cmd = "systemctl " + action + " " + serviceName;
    if (std::system(cmd.c_str()) != 0) {
        perror("system");
        return false;
    }
    return true;
}

std::unordered_map<std::string, std::string> GetKeyValueFromString(const std::string &params)
{
    std::unordered_map<std::string, std::string> result;
    std::stringstream ss(params);
    std::string pair;
    while (std::getline(ss, pair, ',')) {
        auto pos = pair.find(':');
        if (pos != std::string::npos) {
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
            result[key] = value;
        }
    }
    return result;
}
// 0,1,3-5 => {0,1,3,4,5}
std::vector<int> ParseRange(const std::string &rangeStr)
{
    std::vector<int> numbers;
    std::stringstream ss(rangeStr);
    std::string token;
    while (std::getline(ss, token, ',')) {
        size_t dashPos = token.find('-');
        if (dashPos != std::string::npos) {
            int start = std::stoi(token.substr(0, dashPos));
            int end = std::stoi(token.substr(dashPos + 1));
            for (int i = start; i <= end; ++i) {
                numbers.emplace_back(i);
            }
        } else {
            numbers.emplace_back(std::stoi(token));
        }
    }
    return numbers;
}

bool IrqSetSmpAffinity(int preferredCpu, int irqNum)
{
    std::string smpAffinityPath = "/proc/irq/" + std::to_string(irqNum) + "/smp_affinity_list";

    std::ofstream file(smpAffinityPath);
    if (!file.is_open()) {
        return false;
    }

    file << preferredCpu << '\n';
    file.close();
    return true;
}

}
