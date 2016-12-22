#include <iostream>

#include "oqpi.hpp"

using thread = oqpi::thread_interface;

template<typename... _Types>
struct type_list {};

using my_tl = type_list<double, int, char, float>;

template<typename _Type>
struct holder
{
    int id = 0;
};

template<typename _TL>
struct entity;

template<typename... _Types>
struct entity<type_list<_Types...>>
    : public holder<_Types>...
{
    template<typename _Type>
    int getId()
    {
        return holder<_Type>::id;
    }

    template<typename _Type>
    void use()
    {
        holder<_Type>::id++;
    }
};

int main()
{
    thread th{ "MyThread", [](const char *c, int i, float f)
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

    entity<my_tl> t;
    t.use<int>();
    std::cout << t.getId<int>() << std::endl;

    th.join();

    //std::thread t;

	return EXIT_SUCCESS;
}