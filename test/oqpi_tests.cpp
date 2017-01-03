#include <iostream>
#include <queue>
#include <mutex>

#pragma warning (disable : 4127)
#pragma warning (disable : 4459)

#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#include "websocketpp/config/asio_no_tls.hpp"
#include "websocketpp/server.hpp"
 
 #include "oqpi.hpp"

#include "cqueue.hpp"
#include "timer_contexts.hpp"

using thread = oqpi::thread_interface;
using semaphore = oqpi::semaphore_interface;


template<typename T>
using cqueue = qqueue<T, std::mutex>;
volatile bool a = true;
using namespace std::chrono_literals;

using dispatcher_type = oqpi::dispatcher<cqueue>;

using tc = oqpi::task_context_container<timer_task_context>;
using gc = oqpi::group_context_container<timer_group_context>;

using oqpi_tk = oqpi::td<dispatcher_type, gc, tc>;


using server = websocketpp::server<websocketpp::config::asio>;

void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
    std::cout << msg->get_payload() << std::endl;
}

static std::mutex coutMutex;
void SleepFor(int duration)
{
    {
        std::lock_guard<std::mutex> l(coutMutex);
        std::cout << "Sleeping for " << duration << "ms" << std::endl;
    }
    oqpi::this_thread::sleep_for(std::chrono::milliseconds(duration));
}

int main()
{

//     const auto attr = oqpi::thread_attributes{ "MyThread", (std::numeric_limits<uint32_t>::min)(), oqpi::core_affinity::all_cores, oqpi::thread_priority::highest };
//     thread th{ attr, []()
//     {
//         server print_server;
// 
//         print_server.set_message_handler(&on_message);
// 
//         print_server.init_asio();
//         print_server.listen(9002);
//         print_server.start_accept();
// 
//         print_server.run();
// 
//     }};    
    
    //const auto t = thread::hardware_concurrency();
    //th.setCoreAffinity(3);
    //const auto a = th.getCoreAffinityMask();
    //oqpi_checkf(a == oqpi::core3, "Core Affinity = 0x%x, expected 0x%x", int(a), int(oqpi::core3));



    const int STACK_SIZE = 0;

    const oqpi::worker_config workersConfig[] =
    {
    { oqpi::thread_attributes("WT(L) - ", STACK_SIZE, oqpi::core_affinity::all_cores, oqpi::thread_priority::normal) , oqpi::worker_priority::wprio_normal_or_low,  1},
    { oqpi::thread_attributes("WT(H) - ", STACK_SIZE, oqpi::core_affinity::all_cores, oqpi::thread_priority::highest), oqpi::worker_priority::wprio_normal_or_high, 3},
    };
    

    oqpi_tk::dispatcher_.registerWorkers<thread, semaphore>(workersConfig);
    oqpi_tk::dispatcher_.start();


    auto t1 = oqpi::make_task<tc>("MyWaitableTask", oqpi::task_priority::high, SleepFor, 500);
    auto t2 = oqpi::make_task_item<tc>("MyFireAndForgetTask", oqpi::task_priority::high, SleepFor, 200);

    oqpi_tk::dispatch_task(oqpi::task_handle(t1));
    oqpi_tk::dispatch_task(oqpi::task_handle(t2));
    
    auto tg = oqpi_tk::make_parallel_group<oqpi::task_type::waitable>("MyFork", oqpi::task_priority::normal, 5);
    int32_t t = 0;
    tg->addTask(oqpi_tk::make_task_item("MyFT1", oqpi::task_priority::normal, SleepFor, t += 10));
    tg->addTask(oqpi_tk::make_task_item("MyFT2", oqpi::task_priority::normal, SleepFor, t += 10));
    tg->addTask(oqpi_tk::make_task_item("MyFT3", oqpi::task_priority::normal, SleepFor, t += 10));
    tg->addTask(oqpi_tk::make_task_item("MyFT4", oqpi::task_priority::normal, SleepFor, t += 10));

    oqpi_tk::dispatch_task(oqpi::task_handle(tg));

    oqpi::this_thread::sleep_for(200ms);

//     a = false;
//     th.join();

    tg->wait();

    oqpi::this_thread::sleep_for(400ms);
    oqpi_tk::dispatcher_.stop();

    return 0;
}