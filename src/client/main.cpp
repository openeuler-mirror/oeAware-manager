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
#include "oe_client.h"
#include "adapt_data.h"

std::condition_variable g_cv;
std::mutex g_mutex;
bool g_finish = false;

static int AnalysisCallback(const DataList *dataList)
{
	if (dataList && dataList->len && dataList->data) {
		AdaptData* data = static_cast<AdaptData*>(dataList->data[0]);
		for (int i = 0;  i < data->len; i++) {
			std::cout << data->data[i] << std::endl;
		}
	}
	std::lock_guard<std::mutex> guard(g_mutex);
	g_finish = true;
	g_cv.notify_one();
	
	return 0;
}

int main(int argc, char *argv[])
{
	if (argv) {
		std::string analysis = argv[1];
		if (analysis == "analysis") {
			CTopic topic = {"analysis_aware", "analysis_aware", ""};
			OeInit();
			OeSubscribe(&topic, AnalysisCallback);
			std::unique_lock<std::mutex> lock(g_mutex);
			g_cv.wait(lock, []{ return g_finish; });
			return 0;
		}
	}
	
    oeaware::Client client;
    if (!client.Init(argc, argv)) {
        exit(EXIT_FAILURE);
    }
    client.RunCmd();
    return 0;
}
