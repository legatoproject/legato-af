
#include <legato.h>
#include <thread>
#include <string>
#include <list>
#include <iostream>


#ifndef MUST_BE_DEFINED
#error MUST_BE_DEFINED was not defined.
#endif


COMPONENT_INIT
{
    LE_INFO("Hello world, from thread 1.");

    std::thread newThread([]()
        {
            le_thread_InitLegatoThreadData("thread 2");

            // This will crash if the Legato thread-specific data has not been initialized.
            le_thread_GetCurrent();

            LE_INFO("Hello world, from %s.", le_thread_GetMyName());

            le_thread_CleanupLegatoThreadData();
        });

    LE_INFO("Thead 2 stared, and waiting for it to complete.");
    newThread.join();

    LE_INFO("Thead 2 ended, all done with init.");

    std::list<std::string> stuff;

    // C++ 11
    for (auto const &s : stuff)
    {
        std::cout << "stuff: " << s << std::endl;
    }

    exit(EXIT_SUCCESS);
}
