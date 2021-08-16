#include <iostream>
#include "restful.hpp"



int main (const int, const char**)
{
    try {
        auto restfulControl = std::make_shared<Control>();
        return restfulControl->run();
    }
    catch (std::exception& e) {
        std::cerr << "Error en el controlador principal: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Error en el controlador principal. Abortado." << std::endl;
    }
}

