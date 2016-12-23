 #include <iostream>
 
 #include "oqpi.hpp"
 
using thread = oqpi::thread_interface;

int main()
{
    const auto attr = oqpi::thread_attributes{ "MyThread", (std::numeric_limits<uint32_t>::min)(), oqpi::core_affinity::all_cores, oqpi::thread_priority::highest };
    thread th{ attr, [](const char *c, int i, float f)
    {
        std::cout << c << std::endl;
        std::cout << i << std::endl;
        std::cout << f << std::endl;

        volatile bool a = true;
        while (a)
        {
            static volatile int b = 0;
            b++;
        }

    }, "haha", 43, 8.f };    
    
    const auto t = thread::hardware_concurrency();
    th.setCoreAffinity(3);
    const auto a = th.getCoreAffinityMask();
    oqpi_checkf(a == oqpi::core3, "Core Affinity = 0x%x, expected 0x%x", int(a), int(oqpi::core3));

    th.join();

    return 0;
}