#include "my_application.h"

// std
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main()
{
    MyApplication app{};

    try 
    {
        app.run();
    }
    catch (const std::exception& e) 
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

