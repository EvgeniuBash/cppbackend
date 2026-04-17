#include <iostream>

int main(int argc, char* argv[]) {
    std::this_thread::sleep_for(std::chrono::seconds(1000));
    return 0;
}