#include <iostream>
#include "restful.hpp"


/* Procedimiento principal del control
 */
int Control::run(void)
{

    Endpoint webServices(shared_from_this());
    return webServices.runWS();

}



/* Interfaz de creación de árboles
 */
void Control::newTreeInterface(const json obj)
{
    // para implementar Control necesita acceder al Modelo
}



/* Control principal de los WS
 */
int Endpoint::runWS()
{
    return 0; // TODO: Implement
}



int main (const int, const char**)
{
    auto restfulControl = std::make_shared<Control>();
    return restfulControl->run();
}
