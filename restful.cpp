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
    /* TEMPORAL comportamiento simulado */
    json obj = json::parse(R"({"left":{"node":2},"node":1,"right":{"left":{"node":4},"node":3}})");
    c->newTreeInterface(obj);
    return 0;
}



int main (const int, const char**)
{
    auto restfulControl = std::make_shared<Control>();
    return restfulControl->run();
}
