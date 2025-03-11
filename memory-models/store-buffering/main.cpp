#include <iostream>
#include <thread>
#include <atomic>

void TurnOffCompilerReordering() {
    asm("" : : : "memory");
}

int main() {
    uint64_t num_it = 0;
    while (true) {
        std::atomic<int> x{0};
        std::atomic<int> y{0};

        int a = 0;
        int b = 0;

        std::thread t1([&]() {
            x.store(1, std::memory_order::release);
            TurnOffCompilerReordering();
            a = y.load(std::memory_order::acquire);
        });
        std::thread t2([&]() {
            y.store(1, std::memory_order::release);
            TurnOffCompilerReordering();
            b = x.load(std::memory_order::acquire);
        });

        t1.join();
        t2.join();

        if (a == 0 && b == 0) {
            std::cout << "fail " << a << " " << b << std::endl;
            std::cout << num_it << std::endl;
            break;
        }

        if (num_it % 1000 == 0) {
            std::cout << num_it << "\n";
        }

        ++num_it;
    }

    return 0;
}
