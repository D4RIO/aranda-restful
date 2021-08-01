#include <iostream>
#include <stack> // std::stack<>
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



std::shared_ptr<json> Control::lowestCommonAncestorInterface(const json obj)
{
    return modeloArbol->lowestCommonAncestor(obj);
}



void Modelo::createNewTree(const json obj)
{
    arbol = obj;
}



std::shared_ptr<json> Modelo::lowestCommonAncestor(const json objBusqueda)
{
    std::shared_ptr<json> LCA = std::make_shared<json>();
    std::stack<json> camino, nodo_a, nodo_b;
    std::stack< std::pair<json,json> > working;


    if (objBusqueda.find("id") == objBusqueda.end())
        throw std::string ("ID del árbol requerido (falta campo id)");
    if (objBusqueda.find("node_a") == objBusqueda.end()
        || objBusqueda.find("node_b") == objBusqueda.end())
        throw std::string ("Nodos de búsqueda requeridos (falta campo node_a o node_b)");
    if (arbol.is_null())
        throw std::string("No se encontró ningún árbol (campo id erróneo)");


    // Se comienza por el nodo raíz, sin último padre
    working.push({0, this->arbol});

    
    while (!working.empty())
    {
        auto i = working.top().first;
        auto o = working.top().second;
        working.pop();

        if (!camino.empty())
            while (i != camino.top()) {
                camino.pop();
            }

        if (o["node"].is_null())
            throw std::string (R"(Árbol mal formado, todos los nodos deben tener un campo "node")");
        
        camino.push(o["node"]);
        if (o["node"]==objBusqueda["node_a"])
            nodo_a = camino;
        if (o["node"]==objBusqueda["node_b"])
            nodo_b = camino;
        if (o.find("left") != o.end()) {
            working.push({o["node"],o["left"]});
        }
        if (o.find("right") != o.end()) {
            working.push({o["node"],o["right"]});
        }
    };

    if (!nodo_a.empty() and !nodo_b.empty())
    {
        // nodo_a y nodo_b contienen las rutas a cada nodo
        // todos los elementos de dirección compartidos son ancestros en común
        while (nodo_a.size()>nodo_b.size())
            nodo_a.pop();
        while (nodo_a.size()<nodo_b.size())
            nodo_b.pop();
        while (nodo_a.top()!=nodo_b.top()) {
            nodo_a.pop();
            nodo_b.pop();
        }
        LCA.reset(new json(nodo_a.top()));
        return LCA;
    }

    throw std::string("Error encontrando el ancestro. Verifique que el objeto no contenga más de un árbol.");
}



/* Control principal de los WS
 */
int Endpoint::runWS(std::shared_ptr<Control> c)
{
    /* TEMPORAL comportamiento simulado */
    try {
        json obj = json::parse(R"({"id":"some-tree","left":{"node":"ar"},"node":"dario","right":{"left":{"node":".ar"},"node":".com"}})");
        json req = json::parse(R"({"id":"some-tree","node_a":".ar","node_b":"dario"})");
        c->newTreeInterface(obj);
        std::cout << c->lowestCommonAncestorInterface(req)->dump(2) << std::endl;
    }
    catch (std::string e) {
        std::cerr << e << std::endl;
    }
    catch (...) {
        std::cerr << "Excepción inesperada en Endpoint.runWS()" << std::endl;
    }
    return 0;
}

