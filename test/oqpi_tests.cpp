#include <iostream>

#include "oqpi.hpp"

using thread = oqpi::thread_interface;

int main()
{
    thread th{ "MyThread", []()
    {
        return;
    } };

    //std::thread t;

	return EXIT_SUCCESS;
}