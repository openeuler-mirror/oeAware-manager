#include <gtest/gtest.h>
#include "data_register.h"
#include "utils.h"
#include "securec.h"

struct TestData {
    int a;
    std::string b;
};

int TestDataSerialize(const void *data, oeaware::OutStream &out)
{
    auto *testData = static_cast<const TestData*>(data);
    out << testData->a << testData->b;
    return 0;
}

int TestDataDeserialize(void **data, oeaware::InStream &in)
{
    *data = new TestData();
    TestData *testData = static_cast<TestData*>(*data);
    in >> testData->a >> testData->b;
    return 0;
}

TEST(DataListSerialize, DataListSerialize)
{
    DataList dataList;
    oeaware::SetDataListTopic(&dataList, "test", "data", "");
    oeaware::Register::GetInstance().RegisterData("test", oeaware::RegisterEntry(TestDataSerialize, TestDataDeserialize));
    dataList.len = 1;
    dataList.data = new void* [1];
    TestData data{1, "hello"};
    dataList.data[0] = &data;
    oeaware::OutStream out;
    oeaware::DataListSerialize(&dataList, out);

    oeaware::InStream in(out.Str());
    DataList newDataList;
    oeaware::DataListDeserialize(&newDataList, in);

    EXPECT_EQ(0, strcmp(dataList.topic.instanceName, newDataList.topic.instanceName));
    EXPECT_EQ(0, strcmp(dataList.topic.topicName, newDataList.topic.topicName));
    EXPECT_EQ(0, strcmp(dataList.topic.params, newDataList.topic.params));
    EXPECT_EQ(dataList.len, newDataList.len);
    auto newTestData = static_cast<TestData*>(newDataList.data[0]);
    EXPECT_EQ(data.a, newTestData->a);
    EXPECT_EQ(data.b, newTestData->b);
}

TEST(TopicSerialize, TopicSerialize)
{
    CTopic topic;
    std::string instanceName("hello");
    std::string topicName("new");
    std::string params("world");
    topic.instanceName = new char[instanceName.size() + 1];
    strcpy_s(topic.instanceName, instanceName.size() + 1, instanceName.data());
    topic.topicName = new char[topicName.size() + 1];
    strcpy_s(topic.topicName, topicName.size() + 1, topicName.data());
    topic.params = new char[params.size() + 1];
    strcpy_s(topic.params, params.size() + 1, params.data());
    oeaware::OutStream out;
    oeaware::TopicSerialize(&topic, out);
    oeaware::InStream in(out.Str());
    CTopic newTopic;
    oeaware::TopicDeserialize(&newTopic, in);
    
    EXPECT_EQ(0, strcmp(topic.instanceName, newTopic.instanceName));
    EXPECT_EQ(0, strcmp(topic.topicName, newTopic.topicName));
    EXPECT_EQ(0, strcmp(topic.params, newTopic.params));
}