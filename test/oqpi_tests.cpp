#include <iostream>

#include "oqpi.hpp"

using thread = oqpi::thread_interface;

int main()
{
    thread th{ "MyThread", [](const char *c, int i, float f)
    {
        std::cout << c << std::endl;
        std::cout << i << std::endl;
        std::cout << f << std::endl;

    }, "haha", 43, 8.f };

    th.join();

    //std::thread t;

	return EXIT_SUCCESS;
}