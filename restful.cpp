#include <iostream>
#include "restful.hpp"



Control::Control()
{
    webServices = std::make_shared<Endpoint>();
    modeloArbol = std::make_shared<Modelo>();
}



/* Procedimiento principal del control
 */
int Control::run(void)
{

    return webServices->runWS (shared_from_this());

}



/* Interfaz de creación de árboles
 */
void Control::newTreeInterface(const json obj)
{
    modeloArbol->createNewTree(obj);
}



void Modelo::createNewTree(const json obj)
{
    arbol = obj;
}



/* Control principal de los WS
 */
int Endpoint::runWS(std::shared_ptr<Control> c)
{
    return 0; // TODO: Implement
}



int main (const int, const char**)
{
    auto restfulControl = std::make_shared<Control>();
    return restfulControl->run();
}
