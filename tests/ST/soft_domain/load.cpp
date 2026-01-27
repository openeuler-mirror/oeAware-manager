#include <thread>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <chrono>

void busyLoop()
{
    int counter = 0;
    while (counter >= 0) {
        // 执行一些计算处理
        counter++;
        for (int i = 0; i < 1000000; i++) {
            counter += i;
            counter %= 10000;
        }
        
        // 每个周期休眠100ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <n> <docker_name>" << std::endl;
        std::cerr << "  n: number of threads to create" << std::endl;
        std::cerr << "  docker_name: docker container name (for observation)" << std::endl;
        return 1;
    }
    
    int n = std::atoi(argv[1]);
    if (n <= 0) {
        std::cerr << "Error: n must be a positive integer" << std::endl;
        return 1;
    }
    
    // docker_name参数用于方便观察进程属于哪个docker，不需要处理
    const char *docker_name = argv[2];
    
    std::vector<std::thread> threads;
    for (int i = 0; i < n; i++) {
        threads.emplace_back(busyLoop);
    }
    
    // 等待所有线程（实际上会一直运行）
    for (auto &t : threads) {
        t.join();
    }
    
    return 0;
}
