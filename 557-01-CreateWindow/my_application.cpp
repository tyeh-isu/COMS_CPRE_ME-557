#include "my_application.h"

void MyApplication::run() 
{
    while (!m_myWindow.shouldClose()) 
    {
        glfwPollEvents();
    }
}

