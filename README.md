# oqpi
*oqpi* pronounced _occupy_ is a header only concurrency library designed to be as easy to use as possible.

oqpi helps you reach that 100% core occupancy you're aiming for.

## Example
```cpp
#define OQPI_DEFAULT
#include "oqpi.hpp"

// Define our toolkit
using oqpi_tk = oqpi::default_helpers;

int main()
{
    // This will start a scheduler with a default workers configuration
    oqpi_tk::start_default_scheduler();

    // oqpi has 3 concepts for tasks:

    // The first one is the unit task:
    const auto taskHandle = oqpi_tk::schedule_task
    (
        "UnitTask"
        , [] { std::cout << "Hello! I'm a unit task!" << std::endl; }
    );
    taskHandle.wait();

    // The second one is the sequence:
    const auto sequenceHandle = oqpi_tk::sequence_tasks<oqpi::task_type::waitable>
    (
        "Sequence"
        , oqpi_tk::make_task_item("Seq1", [] { std::cout << "Hello! ";                     })
        , oqpi_tk::make_task_item("Seq2", [] { std::cout << "I am ";                       })
        , oqpi_tk::make_task_item("Seq3", [] { std::cout << "a sequence!" << std::endl;    })
    );
    sequenceHandle.wait();

    // The third and last one is the fork (or the parallel group)
    const auto forkHandle = oqpi_tk::fork_tasks<oqpi::task_type::waitable>
    (
        "Fork"
        , oqpi_tk::make_task_item("Fork1", [] { std::cout << "Hello! ";                 })
        , oqpi_tk::make_task_item("Fork2", [] { std::cout << "I am ";                   })
        , oqpi_tk::make_task_item("Fork3", [] { std::cout << "a fork!" << std::endl;    })
    );
    forkHandle.wait();

    // Stops the workers and join the threads
    oqpi_tk::stop_scheduler();
}
```

## Documentation
_Coming soon..._
