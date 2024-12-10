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
#include <string>
#include <mutex>
#include <condition_variable>
#include "client.h"
#include "analysis_cli.h"

std::condition_variable g_cv;
std::mutex g_mutex;
bool g_finish = false;

int main(int argc, char *argv[])
{
    AnalysisCli &anaCli = AnalysisCli::GetInstance();
    if (anaCli.IsAnalysisMode(argc, argv)) {
        return anaCli.Proc(argc, argv);
    }
    oeaware::Client client;
    if (!client.Init(argc, argv)) {
        exit(EXIT_FAILURE);
    }
    client.RunCmd();
    return 0;
}
