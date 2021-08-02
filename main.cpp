#include <iostream>
#include "restful.hpp"



int main (const int, const char**)
{
    try {
        auto restfulControl = std::make_shared<Control>();
        return restfulControl->run();
    }
    catch (...) {
        std::cerr << "Error en el Controlador principal. Abortado." << std::endl;
    }
}

