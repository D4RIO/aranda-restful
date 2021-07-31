#include "restful.hpp"



int main (const int, const char**)
{
    auto restfulControl = std::make_shared<Control>();
    return restfulControl->run();
}

