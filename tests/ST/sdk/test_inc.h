#ifndef __TEST_INC_H__
#define __TEST_INC_H__

bool TestEnvStaticInfo(int time);
bool TestEnvRealTimeInfo(int time);
bool TestCpuUtilInfo(int time);

// net interface test
bool TestNetIntfBaseInfo(int time);
bool TestNetIntfDirverInfo(int time);
#endif