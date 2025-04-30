#include <oeaware/oe_client.h>
#include <oeaware/data/analysis_data.h>
#include <cstdio>
#include <thread>
#include <fstream>
#include <iostream>
#include <vector>
#include <securec.h>
#include <unistd.h>

int analysisTime = 5;
int sleepTime = 8;
int Handler(const DataList *dataList)
{
    auto res = static_cast<AnalysisResultItem*>(dataList->data[0]);
    printf("%s %s %s\n", res->dataItem[0].metric, res->dataItem[0].value, res->dataItem[0].extra);
    printf("%s\n", res->conclusion);
    return 0;
}

void FileIoOperation()
{
    auto st =  std::chrono::steady_clock::now();
    int milliseconds = 1000;
    bool ok = true;
    while (ok) {
        std::ofstream ofs("temp.txt", std::ios::app);
        if (ofs.is_open()) {
            ofs << "Hello, World!" << std::endl;
            ofs.close();
        }
        auto now = std::chrono::steady_clock::now();
        auto t = std::chrono::duration_cast<std::chrono::milliseconds>(now - st);
        if (t.count() >= analysisTime * milliseconds) {
            ok = false;
            return;
        }
    }
}

int main()
{
    std::cout << "Starting file I/O operation to increase kernel CPU usage..." << std::endl;

    std::vector<std::thread> threads;
    for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
        threads.emplace_back(FileIoOperation);
    }

    for (auto& t : threads) {
        t.detach();
    }
    int pid = getpid();
    std::string s = "t:5,pid:" + std::to_string(pid);
    char *param = new char[s.size() + 1];
    int ret = strcpy_s(param, s.size() + 1, s.data());
    if (ret != EOK) {
        return -1;
    }
    CTopic topic{OE_XCALL_ANALYSIS, "xcall", param};
    OeInit();
    OeSubscribe(&topic, Handler);
    sleep(sleepTime);
    OeClose();
}
