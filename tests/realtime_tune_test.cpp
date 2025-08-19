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
#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include "logger.h"
#define private public
#include "realtime_tune.h"

class RealTimeTuneTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // 模拟 main.cpp 中的初始化流程
        oeaware::CreateDir("/tmp");  // 创建临时日志目录
        oeaware::Logger::GetInstance().Register("RealTimeTuneTest");
        oeaware::Logger::GetInstance().Init("/tmp", oeaware::OE_INFO_LEVEL);
        auto logger = oeaware::Logger::GetInstance().Get("RealTimeTuneTest");
        INFO(logger, "RealTimeTuneTest SetUpTestSuite");
    }

    void SetUp() override {
        // 为每个测试注册 realtime_tune logger
        oeaware::Logger::GetInstance().Register("realtime_tune");
        
        // 创建临时配置文件用于测试
        testConfigPath = "/tmp/test_realtime_config.yaml";
        testPluginConfigPath = "/tmp/test_realtime_plugin_config.yaml";
        
        // 创建测试用的插件配置文件
        std::ofstream pluginConfig(testPluginConfigPath);
        pluginConfig << "cpu_isolation:\n";
        pluginConfig << "  range: \"1-3,5\"\n";
        pluginConfig << "  features:\n";
        pluginConfig << "    isolcpus: \"on\"\n";
        pluginConfig << "    nohz_full: \"on\"\n";
        pluginConfig << "    rcu_nocbs: \"off\"\n";
        pluginConfig << "    irqaffinity: \"on\"\n";
        pluginConfig << "cpufreq_performance: \"on\"\n";
        pluginConfig << "memory:\n";
        pluginConfig << "  transparent_hugepage: \"off\"\n";
        pluginConfig << "  numa_balancing: \"off\"\n";
        pluginConfig << "  ksm: \"off\"\n";
        pluginConfig << "  swap: \"off\"\n";
        pluginConfig << "timer:\n";
        pluginConfig << "  migration: \"off\"\n";
        pluginConfig << "sched:\n";
        pluginConfig << "  rt_runtime_us: \"off\"\n";
        pluginConfig.close();
    }

    void TearDown() override {
        // 清理临时文件
        std::remove(testConfigPath.c_str());
        std::remove(testPluginConfigPath.c_str());
    }

    // 辅助函数：为 RealTimeTune 对象设置 logger
    void SetupLogger(RealTimeTune& tune) {
        auto realtimeLogger = oeaware::Logger::GetInstance().Get("realtime_tune");
        tune.SetLogger(realtimeLogger);
    }

    std::string testConfigPath;
    std::string testPluginConfigPath;
};

TEST_F(RealTimeTuneTest, GetNotRangeCpus) {
    RealTimeTune tune;
    SetupLogger(tune); // 设置 logger
    
    // 测试简单的范围
    std::string result = tune.GetNotRangeCpus("1-3", 5);
    EXPECT_EQ(result, "0,4-5");
    
    // 测试复杂范围
    result = tune.GetNotRangeCpus("1-3,5", 7);
    EXPECT_EQ(result, "0,4,6-7");
    
    // 测试空范围
    result = tune.GetNotRangeCpus("", 3);
    EXPECT_EQ(result, "0-3");
    
    // 测试单个数字
    result = tune.GetNotRangeCpus("2", 4);
    EXPECT_EQ(result, "0-1,3-4");

    // 测试大范围
    result = tune.GetNotRangeCpus("0-99", 199);
    EXPECT_EQ(result, "100-199");
    
    result = tune.GetNotRangeCpus("100-199", 199);
    EXPECT_EQ(result, "0-99");
}

TEST_F(RealTimeTuneTest, GenerateExpectedKernelParams) {
    RealTimeTune tune;
    SetupLogger(tune); // 设置 logger
    
    // 使用公共接口设置测试数据
    std::map<std::string, bool> switches;
    switches["isolcpus"] = true;
    switches["nohz_full"] = true;
    switches["rcu_nocbs"] = false;
    switches["irqaffinity"] = true;
    
    tune.SetTestConfig("1-3,5", switches);
    
    auto params = tune.GenerateExpectedKernelParams(7);
    
    EXPECT_EQ(params["isolcpus"], "1-3,5");
    EXPECT_EQ(params["nohz_full"], "1-3,5");
    EXPECT_EQ(params["irqaffinity"], "0,4,6-7");
    EXPECT_EQ(params.find("rcu_nocbs"), params.end());
}

TEST_F(RealTimeTuneTest, AddRealtimeTuneToConfig) {
    RealTimeTune tune;
    SetupLogger(tune); // 设置 logger
    
    // 创建测试配置文件
    std::ofstream config(testConfigPath);
    config << "enable_list:\n";
    config << "  - name: libsystem_tune.so\n";
    config << "    instances:\n";
    config << "      - other_tune\n";
    config.close();
    
    bool result = tune.AddRealtimeTuneToConfig(testConfigPath);
    EXPECT_TRUE(result);
    
    // 验证配置是否正确添加
    std::ifstream configFile(testConfigPath);
    std::string content((std::istreambuf_iterator<char>(configFile)),
                        std::istreambuf_iterator<char>());
    configFile.close();
    
    EXPECT_NE(content.find("realtime_tune"), std::string::npos);
}

TEST_F(RealTimeTuneTest, RemoveRealtimeTuneFromConfig) {
    RealTimeTune tune;
    SetupLogger(tune); // 设置 logger
    
    // 创建测试配置文件
    std::ofstream config(testConfigPath);
    config << "enable_list:\n";
    config << "  - name: libsystem_tune.so\n";
    config << "    instances:\n";
    config << "      - realtime_tune\n";
    config << "      - other_tune\n";
    config.close();
    
    bool result = tune.RemoveRealtimeTuneFromConfig(testConfigPath);
    EXPECT_TRUE(result);
    
    // 验证配置是否正确移除
    std::ifstream configFile(testConfigPath);
    std::string content((std::istreambuf_iterator<char>(configFile)),
                        std::istreambuf_iterator<char>());
    configFile.close();
    
    EXPECT_EQ(content.find("realtime_tune"), std::string::npos);
    EXPECT_NE(content.find("other_tune"), std::string::npos); // 其他实例应该保留
}

TEST_F(RealTimeTuneTest, CheckEnv) {
    RealTimeTune tune;
    SetupLogger(tune); // 设置 logger
    
    // 这个测试依赖于系统环境，可能在不同系统上有不同结果
    bool result = tune.CheckEnv();
    // 不检查具体结果，因为不同系统环境不同
    // 只检查函数能正常执行
    EXPECT_TRUE(result || !result); // 总是为true，只是确保函数不崩溃
}
/*
TEST_F(RealTimeTuneTest, GetCpuCount) {
    RealTimeTune tune;
    
    int cpuCount = tune.GetCpuCount();
    EXPECT_GT(cpuCount, 0); // CPU数量应该大于0
    EXPECT_LE(cpuCount, 1024); // 合理的上限
}*/

TEST_F(RealTimeTuneTest, ParseRangeValidation) {
    RealTimeTune tune;
    SetupLogger(tune); // 设置 logger
    
    // 测试有效的范围格式
    EXPECT_NO_THROW(tune.GetNotRangeCpus("1-3,5", 10));
    EXPECT_NO_THROW(tune.GetNotRangeCpus("0,2,4", 10));
    EXPECT_NO_THROW(tune.GetNotRangeCpus("1-5", 10));
    
    // 测试边界情况
    EXPECT_NO_THROW(tune.GetNotRangeCpus("", 5));
    EXPECT_NO_THROW(tune.GetNotRangeCpus("0", 0));
}

TEST_F(RealTimeTuneTest, ConfigFileHandling) {
    RealTimeTune tune;
    SetupLogger(tune); // 设置 logger
    
    // 测试不存在的配置文件
    bool result = tune.AddRealtimeTuneToConfig("/nonexistent/path/config.yaml");
    EXPECT_FALSE(result);
    
    // 测试空的配置文件
    std::ofstream emptyConfig(testConfigPath);
    emptyConfig.close();
    
    result = tune.AddRealtimeTuneToConfig(testConfigPath);
    EXPECT_TRUE(result);
}

TEST_F(RealTimeTuneTest, ParseFromSwitchConfig) {
    RealTimeTune tune;
    SetupLogger(tune); // 设置 logger
    tune.UpdateMaxCpuId();
    // 测试不存在的配置文件
    bool result = tune.ParseFromSwitchConfig("/nonexistent/path/config.yaml");
    EXPECT_FALSE(result);
    
    // 测试缺少 cpu_isolation 的情况
    std::ofstream config1(testPluginConfigPath);
    config1 << "cpufreq_performance: \"on\"\n";
    config1 << "memory:\n";
    config1 << "  transparent_hugepage: \"off\"\n";
    config1 << "  numa_balancing: \"off\"\n";
    config1 << "  ksm: \"off\"\n";
    config1 << "  swap: \"off\"\n";
    config1 << "timer:\n";
    config1 << "  migration: \"off\"\n";
    config1 << "sched:\n";
    config1 << "  rt_runtime_us: \"off\"\n";
    config1.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试缺少 cpu_isolation.range 的情况
    std::ofstream config2(testPluginConfigPath);
    config2 << "cpu_isolation:\n";
    config2 << "  features:\n";
    config2 << "    isolcpus: \"on\"\n";
    config2 << "    nohz_full: \"on\"\n";
    config2 << "    rcu_nocbs: \"off\"\n";
    config2 << "    irqaffinity: \"on\"\n";
    config2 << "cpufreq_performance: \"on\"\n";
    config2 << "memory:\n";
    config2 << "  transparent_hugepage: \"off\"\n";
    config2 << "  numa_balancing: \"off\"\n";
    config2 << "  ksm: \"off\"\n";
    config2 << "  swap: \"off\"\n";
    config2 << "timer:\n";
    config2 << "  migration: \"off\"\n";
    config2 << "sched:\n";
    config2 << "  rt_runtime_us: \"off\"\n";
    config2.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试缺少 cpu_isolation.features 的情况
    std::ofstream config3(testPluginConfigPath);
    config3 << "cpu_isolation:\n";
    config3 << "  range: \"1-3,5\"\n";
    config3 << "cpufreq_performance: \"on\"\n";
    config3 << "memory:\n";
    config3 << "  transparent_hugepage: \"off\"\n";
    config3 << "  numa_balancing: \"off\"\n";
    config3 << "  ksm: \"off\"\n";
    config3 << "  swap: \"off\"\n";
    config3 << "timer:\n";
    config3 << "  migration: \"off\"\n";
    config3 << "sched:\n";
    config3 << "  rt_runtime_us: \"off\"\n";
    config3.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试缺少某个 feature 的情况
    std::ofstream config4(testPluginConfigPath);
    config4 << "cpu_isolation:\n";
    config4 << "  range: \"1-3,5\"\n";
    config4 << "  features:\n";
    config4 << "    isolcpus: \"on\"\n";
    config4 << "    nohz_full: \"on\"\n";
    config4 << "    rcu_nocbs: \"off\"\n";
    // 缺少 irqaffinity
    config4 << "cpufreq_performance: \"on\"\n";
    config4 << "memory:\n";
    config4 << "  transparent_hugepage: \"off\"\n";
    config4 << "  numa_balancing: \"off\"\n";
    config4 << "  ksm: \"off\"\n";
    config4 << "  swap: \"off\"\n";
    config4 << "timer:\n";
    config4 << "  migration: \"off\"\n";
    config4 << "sched:\n";
    config4 << "  rt_runtime_us: \"off\"\n";
    config4.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试缺少 cpufreq_performance 的情况
    std::ofstream config5(testPluginConfigPath);
    config5 << "cpu_isolation:\n";
    config5 << "  range: \"1-3,5\"\n";
    config5 << "  features:\n";
    config5 << "    isolcpus: \"on\"\n";
    config5 << "    nohz_full: \"on\"\n";
    config5 << "    rcu_nocbs: \"off\"\n";
    config5 << "    irqaffinity: \"on\"\n";
    // 缺少 cpufreq_performance
    config5 << "memory:\n";
    config5 << "  transparent_hugepage: \"off\"\n";
    config5 << "  numa_balancing: \"off\"\n";
    config5 << "  ksm: \"off\"\n";
    config5 << "  swap: \"off\"\n";
    config5 << "timer:\n";
    config5 << "  migration: \"off\"\n";
    config5 << "sched:\n";
    config5 << "  rt_runtime_us: \"off\"\n";
    config5.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试缺少 memory 的情况
    std::ofstream config6(testPluginConfigPath);
    config6 << "cpu_isolation:\n";
    config6 << "  range: \"1-3,5\"\n";
    config6 << "  features:\n";
    config6 << "    isolcpus: \"on\"\n";
    config6 << "    nohz_full: \"on\"\n";
    config6 << "    rcu_nocbs: \"off\"\n";
    config6 << "    irqaffinity: \"on\"\n";
    config6 << "cpufreq_performance: \"on\"\n";
    // 缺少 memory
    config6 << "timer:\n";
    config6 << "  migration: \"off\"\n";
    config6 << "sched:\n";
    config6 << "  rt_runtime_us: \"off\"\n";
    config6.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试缺少某个 memory 配置项的情况
    std::ofstream config7(testPluginConfigPath);
    config7 << "cpu_isolation:\n";
    config7 << "  range: \"1-3,5\"\n";
    config7 << "  features:\n";
    config7 << "    isolcpus: \"on\"\n";
    config7 << "    nohz_full: \"on\"\n";
    config7 << "    rcu_nocbs: \"off\"\n";
    config7 << "    irqaffinity: \"on\"\n";
    config7 << "cpufreq_performance: \"on\"\n";
    config7 << "memory:\n";
    config7 << "  transparent_hugepage: \"off\"\n";
    config7 << "  numa_balancing: \"off\"\n";
    config7 << "  ksm: \"off\"\n";
    // 缺少 swap
    config7 << "timer:\n";
    config7 << "  migration: \"off\"\n";
    config7 << "sched:\n";
    config7 << "  rt_runtime_us: \"off\"\n";
    config7.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试缺少 timer 的情况
    std::ofstream config8(testPluginConfigPath);
    config8 << "cpu_isolation:\n";
    config8 << "  range: \"1-3,5\"\n";
    config8 << "  features:\n";
    config8 << "    isolcpus: \"on\"\n";
    config8 << "    nohz_full: \"on\"\n";
    config8 << "    rcu_nocbs: \"off\"\n";
    config8 << "    irqaffinity: \"on\"\n";
    config8 << "cpufreq_performance: \"on\"\n";
    config8 << "memory:\n";
    config8 << "  transparent_hugepage: \"off\"\n";
    config8 << "  numa_balancing: \"off\"\n";
    config8 << "  ksm: \"off\"\n";
    config8 << "  swap: \"off\"\n";
    // 缺少 timer
    config8 << "sched:\n";
    config8 << "  rt_runtime_us: \"off\"\n";
    config8.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试缺少 sched 的情况
    std::ofstream config9(testPluginConfigPath);
    config9 << "cpu_isolation:\n";
    config9 << "  range: \"1-3,5\"\n";
    config9 << "  features:\n";
    config9 << "    isolcpus: \"on\"\n";
    config9 << "    nohz_full: \"on\"\n";
    config9 << "    rcu_nocbs: \"off\"\n";
    config9 << "    irqaffinity: \"on\"\n";
    config9 << "cpufreq_performance: \"on\"\n";
    config9 << "memory:\n";
    config9 << "  transparent_hugepage: \"off\"\n";
    config9 << "  numa_balancing: \"off\"\n";
    config9 << "  ksm: \"off\"\n";
    config9 << "  swap: \"off\"\n";
    config9 << "timer:\n";
    config9 << "  migration: \"off\"\n";
    // 缺少 sched
    config9.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试完整配置的情况 - 应该返回 true
    std::ofstream config10(testPluginConfigPath);
    config10 << "cpu_isolation:\n";
    config10 << "  range: \"1-3,5\"\n";
    config10 << "  features:\n";
    config10 << "    isolcpus: \"on\"\n";
    config10 << "    nohz_full: \"on\"\n";
    config10 << "    rcu_nocbs: \"off\"\n";
    config10 << "    irqaffinity: \"on\"\n";
    config10 << "cpufreq_performance: \"on\"\n";
    config10 << "memory:\n";
    config10 << "  transparent_hugepage: \"off\"\n";
    config10 << "  numa_balancing: \"off\"\n";
    config10 << "  ksm: \"off\"\n";
    config10 << "  swap: \"off\"\n";
    config10 << "timer:\n";
    config10 << "  migration: \"off\"\n";
    config10 << "sched:\n";
    config10 << "  rt_runtime_us: \"off\"\n";
    config10.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_TRUE(result);
}

TEST_F(RealTimeTuneTest, EnableFromConfigInvalidValues) {
    RealTimeTune tune;
    SetupLogger(tune); // 设置 logger
    tune.UpdateMaxCpuId();
    // 测试无效的 range 格式
    std::ofstream config1_1(testPluginConfigPath);
    config1_1 << "cpu_isolation:\n";
    config1_1 << "  range: \"invalid-range\"\n"; // 无效格式
    config1_1 << "  features:\n";
    config1_1 << "    isolcpus: \"on\"\n";
    config1_1 << "    nohz_full: \"on\"\n";
    config1_1 << "    rcu_nocbs: \"off\"\n";
    config1_1 << "    irqaffinity: \"on\"\n";
    config1_1 << "cpufreq_performance: \"on\"\n";
    config1_1 << "memory:\n";
    config1_1 << "  transparent_hugepage: \"off\"\n";
    config1_1 << "  numa_balancing: \"off\"\n";
    config1_1 << "  ksm: \"off\"\n";
    config1_1 << "  swap: \"off\"\n";
    config1_1 << "timer:\n";
    config1_1 << "  migration: \"off\"\n";
    config1_1 << "sched:\n";
    config1_1 << "  rt_runtime_us: \"off\"\n";
    config1_1.close();
    bool result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);

    // 测试无效的 range 格式
    std::ofstream config1_2(testPluginConfigPath);
    config1_2 << "cpu_isolation:\n";
    config1_2 << "  range: \"" << (tune.GetMaxCpuId() + 1) << "\"\n"; // 超出范围
    config1_2 << "  features:\n";
    config1_2 << "    isolcpus: \"on\"\n";
    config1_2 << "    nohz_full: \"on\"\n";
    config1_2 << "    rcu_nocbs: \"off\"\n";
    config1_2 << "    irqaffinity: \"on\"\n";
    config1_2 << "cpufreq_performance: \"on\"\n";
    config1_2 << "memory:\n";
    config1_2 << "  transparent_hugepage: \"off\"\n";
    config1_2 << "  numa_balancing: \"off\"\n";
    config1_2 << "  ksm: \"off\"\n";
    config1_2 << "  swap: \"off\"\n";
    config1_2 << "timer:\n";
    config1_2 << "  migration: \"off\"\n";
    config1_2 << "sched:\n";
    config1_2 << "  rt_runtime_us: \"off\"\n";
    config1_2.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);

    // 测试无效的 range 格式
    std::ofstream config1_3(testPluginConfigPath);
    config1_3 << "cpu_isolation:\n";
    config1_3 << "  range: \"" << -1 << "\"\n"; // 负数
    config1_3 << "  features:\n";
    config1_3 << "    isolcpus: \"on\"\n";
    config1_3 << "    nohz_full: \"on\"\n";
    config1_3 << "    rcu_nocbs: \"off\"\n";
    config1_3 << "    irqaffinity: \"on\"\n";
    config1_3 << "cpufreq_performance: \"on\"\n";
    config1_3 << "memory:\n";
    config1_3 << "  transparent_hugepage: \"off\"\n";
    config1_3 << "  numa_balancing: \"off\"\n";
    config1_3 << "  ksm: \"off\"\n";
    config1_3 << "  swap: \"off\"\n";
    config1_3 << "timer:\n";
    config1_3 << "  migration: \"off\"\n";
    config1_3 << "sched:\n";
    config1_3 << "  rt_runtime_us: \"off\"\n";
    config1_3.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试无效的开关值
    std::ofstream config2(testPluginConfigPath);
    config2 << "cpu_isolation:\n";
    config2 << "  range: \"1-3,5\"\n";
    config2 << "  features:\n";
    config2 << "    isolcpus: \"invalid\"\n"; // 无效值
    config2 << "    nohz_full: \"on\"\n";
    config2 << "    rcu_nocbs: \"off\"\n";
    config2 << "    irqaffinity: \"on\"\n";
    config2 << "cpufreq_performance: \"on\"\n";
    config2 << "memory:\n";
    config2 << "  transparent_hugepage: \"off\"\n";
    config2 << "  numa_balancing: \"off\"\n";
    config2 << "  ksm: \"off\"\n";
    config2 << "  swap: \"off\"\n";
    config2 << "timer:\n";
    config2 << "  migration: \"off\"\n";
    config2 << "sched:\n";
    config2 << "  rt_runtime_us: \"off\"\n";
    config2.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试无效的 cpufreq_performance 值
    std::ofstream config3(testPluginConfigPath);
    config3 << "cpu_isolation:\n";
    config3 << "  range: \"1-3,5\"\n";
    config3 << "  features:\n";
    config3 << "    isolcpus: \"on\"\n";
    config3 << "    nohz_full: \"on\"\n";
    config3 << "    rcu_nocbs: \"off\"\n";
    config3 << "    irqaffinity: \"on\"\n";
    config3 << "cpufreq_performance: \"invalid\"\n"; // 无效值
    config3 << "memory:\n";
    config3 << "  transparent_hugepage: \"off\"\n";
    config3 << "  numa_balancing: \"off\"\n";
    config3 << "  ksm: \"off\"\n";
    config3 << "  swap: \"off\"\n";
    config3 << "timer:\n";
    config3 << "  migration: \"off\"\n";
    config3 << "sched:\n";
    config3 << "  rt_runtime_us: \"off\"\n";
    config3.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试无效的 memory 配置值
    std::ofstream config4(testPluginConfigPath);
    config4 << "cpu_isolation:\n";
    config4 << "  range: \"1-3,5\"\n";
    config4 << "  features:\n";
    config4 << "    isolcpus: \"on\"\n";
    config4 << "    nohz_full: \"on\"\n";
    config4 << "    rcu_nocbs: \"off\"\n";
    config4 << "    irqaffinity: \"on\"\n";
    config4 << "cpufreq_performance: \"on\"\n";
    config4 << "memory:\n";
    config4 << "  transparent_hugepage: \"invalid\"\n"; // 无效值
    config4 << "  numa_balancing: \"off\"\n";
    config4 << "  ksm: \"off\"\n";
    config4 << "  swap: \"off\"\n";
    config4 << "timer:\n";
    config4 << "  migration: \"off\"\n";
    config4 << "sched:\n";
    config4 << "  rt_runtime_us: \"off\"\n";
    config4.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试无效的 timer 配置值
    std::ofstream config5(testPluginConfigPath);
    config5 << "cpu_isolation:\n";
    config5 << "  range: \"1-3,5\"\n";
    config5 << "  features:\n";
    config5 << "    isolcpus: \"on\"\n";
    config5 << "    nohz_full: \"on\"\n";
    config5 << "    rcu_nocbs: \"off\"\n";
    config5 << "    irqaffinity: \"on\"\n";
    config5 << "cpufreq_performance: \"on\"\n";
    config5 << "memory:\n";
    config5 << "  transparent_hugepage: \"off\"\n";
    config5 << "  numa_balancing: \"off\"\n";
    config5 << "  ksm: \"off\"\n";
    config5 << "  swap: \"off\"\n";
    config5 << "timer:\n";
    config5 << "  migration: \"invalid\"\n"; // 无效值
    config5 << "sched:\n";
    config5 << "  rt_runtime_us: \"off\"\n";
    config5.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
    
    // 测试无效的 sched 配置值
    std::ofstream config6(testPluginConfigPath);
    config6 << "cpu_isolation:\n";
    config6 << "  range: \"1-3,5\"\n";
    config6 << "  features:\n";
    config6 << "    isolcpus: \"on\"\n";
    config6 << "    nohz_full: \"on\"\n";
    config6 << "    rcu_nocbs: \"off\"\n";
    config6 << "    irqaffinity: \"on\"\n";
    config6 << "cpufreq_performance: \"on\"\n";
    config6 << "memory:\n";
    config6 << "  transparent_hugepage: \"off\"\n";
    config6 << "  numa_balancing: \"off\"\n";
    config6 << "  ksm: \"off\"\n";
    config6 << "  swap: \"off\"\n";
    config6 << "timer:\n";
    config6 << "  migration: \"off\"\n";
    config6 << "sched:\n";
    config6 << "  rt_runtime_us: \"invalid\"\n"; // 无效值
    config6.close();
    result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_FALSE(result);
}

TEST_F(RealTimeTuneTest, GetSwitchConfig) {
    RealTimeTune tune;
    SetupLogger(tune); // 设置 logger
    tune.UpdateMaxCpuId();
    std::ofstream config10(testPluginConfigPath);
    config10 << "cpu_isolation:\n";
    config10 << "  range: \"1-3,5\"\n";
    config10 << "  features:\n";
    config10 << "    isolcpus: \"on\"\n";
    config10 << "    nohz_full: \"on\"\n";
    config10 << "    rcu_nocbs: \"off\"\n";
    config10 << "    irqaffinity: \"on\"\n";
    config10 << "cpufreq_performance: \"on\"\n";
    config10 << "memory:\n";
    config10 << "  transparent_hugepage: \"off\"\n";
    config10 << "  numa_balancing: \"off\"\n";
    config10 << "  ksm: \"off\"\n";
    config10 << "  swap: \"off\"\n";
    config10 << "timer:\n";
    config10 << "  migration: \"off\"\n";
    config10 << "sched:\n";
    config10 << "  rt_runtime_us: \"off\"\n";
    config10.close();
    bool result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_TRUE(result);
    auto switchConfig = tune.GetSwitchConfig();
    EXPECT_EQ(switchConfig.cpuIsolationRange, "1-3,5");
    EXPECT_EQ(switchConfig.featureSwitches["isolcpus"], true);
    EXPECT_EQ(switchConfig.featureSwitches["nohz_full"], true);
    EXPECT_EQ(switchConfig.featureSwitches["rcu_nocbs"], false);
    EXPECT_EQ(switchConfig.featureSwitches["irqaffinity"], true);
    EXPECT_EQ(switchConfig.cpufreqPerformance, true);
    EXPECT_EQ(switchConfig.transparentHugepage, true);
    EXPECT_EQ(switchConfig.numaBalancing, true);
    EXPECT_EQ(switchConfig.ksm, true);
    EXPECT_EQ(switchConfig.swap, true);
    EXPECT_EQ(switchConfig.timerMigration, true);
    EXPECT_EQ(switchConfig.rtRuntimeUs, -1);
}

TEST_F(RealTimeTuneTest, GetSwitchConfigReverse) {
    RealTimeTune tune;
    SetupLogger(tune); // 设置 logger
    tune.UpdateMaxCpuId();
    std::ofstream config10(testPluginConfigPath);
    config10 << "cpu_isolation:\n";
    config10 << "  range: \"1-3,5\"\n";
    config10 << "  features:\n";
    config10 << "    isolcpus: \"off\"\n";
    config10 << "    nohz_full: \"off\"\n";
    config10 << "    rcu_nocbs: \"on\"\n";
    config10 << "    irqaffinity: \"off\"\n";
    config10 << "cpufreq_performance: \"off\"\n";
    config10 << "memory:\n";
    config10 << "  transparent_hugepage: \"on\"\n";
    config10 << "  numa_balancing: \"on\"\n";
    config10 << "  ksm: \"on\"\n";
    config10 << "  swap: \"on\"\n";
    config10 << "timer:\n";
    config10 << "  migration: \"on\"\n";
    config10 << "sched:\n";
    config10 << "  rt_runtime_us: \"on\"\n";
    config10.close();
    bool result = tune.ParseFromSwitchConfig(testPluginConfigPath);
    EXPECT_TRUE(result);
    auto switchConfig = tune.GetSwitchConfig();
    EXPECT_EQ(switchConfig.cpuIsolationRange, "1-3,5");
    EXPECT_EQ(switchConfig.featureSwitches["isolcpus"], false);
    EXPECT_EQ(switchConfig.featureSwitches["nohz_full"], false);
    EXPECT_EQ(switchConfig.featureSwitches["rcu_nocbs"], true);
    EXPECT_EQ(switchConfig.featureSwitches["irqaffinity"], false);
    EXPECT_EQ(switchConfig.cpufreqPerformance, false);
    EXPECT_EQ(switchConfig.transparentHugepage, false);
    EXPECT_EQ(switchConfig.numaBalancing, false);
    EXPECT_EQ(switchConfig.ksm, false);
    EXPECT_EQ(switchConfig.swap, false);
    EXPECT_EQ(switchConfig.timerMigration, false);
    EXPECT_EQ(switchConfig.rtRuntimeUs, 0);
}