#include <iostream>
#include <iomanip>
#include <thread>
#include <mutex>

void swapper(long int);

int arr[2];
std::mutex mtx;

int main(int argc, char* argv[]) {
    long int loop;

    // TODO: get value of loop var (from command line arg)
    loop = atoi(argv[1]);

    arr[0] = 0;
    arr[1] = 1;
    std::thread t1(swapper, loop);
    for (int k = 0; k < loop; k++) {
        mtx.lock(); // lock the mutex

        // TODO: swap the contents of arr[0] and arr[1]
        int temp_val = arr[0];
        arr[0] = arr[1];
        arr[1] = temp_val;

        mtx.unlock(); // unlock the mutex
    }
    t1.join();
    std::cout << "Values: " << std::setw(5) << arr[0] 
        << std::setw(5) << arr[1] << std::endl;
}

void swapper(long int num) {
    for (int k = 0; k < num; k++) {
        mtx.lock(); // lock the mutex

        // TODO: swap the contents of arr[0] and arr[1]
        int temp_val = arr[0];
        arr[0] = arr[1];
        arr[1] = temp_val;

        mtx.unlock(); // unlock the mutex
    }
}
