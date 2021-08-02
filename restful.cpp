#include <iostream>
#include <memory>  // make_shared<>() ... etc
#include <restbed> // REST API
#include <stack>   // std::stack<>
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
    if (obj.find("node")==obj.end())
        throw std::string("Todos los árboles deben tener al menos un nodo!");
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
int Endpoint::runWS(std::shared_ptr<Control> control)
{
    auto res1 = std::make_shared<restbed::Resource>();
    auto res2 = std::make_shared<restbed::Resource>();

    res1->set_path( "/crear-arbol" );
    res1->set_method_handler( "POST",
            [&](const std::shared_ptr<restbed::Session> session)
            { /* Web Service 1 : POST (Crear tree) */

                const auto request = session->get_request();

                // Se obtiene la longitud del contenido de la solicitud en content_length
                int content_length;
                request->get_header("Content-Length", content_length, 0);

                // Procesa el contenido del POST
                session->fetch(content_length,
                        [&](const std::shared_ptr<restbed::Session> session, const restbed::Bytes &body)
                        {
                            // RestBed devuelve un contenedor (vector<uint8_t>)
                            // Para impedir que el webservice permita crear árboles excesivamente
                            // grandes, se limita el tamaño máximo del pedido a 1 MiB, que debería
                            // ser suficiente para representar árboles binarios.
                            if (body.size()>(1024*1024)) {
                                auto msg = std::string("Se admiten hasta 1 MiB de datos");
                                session->close(restbed::BAD_REQUEST, msg.c_str(), {
                                        {"Content-Length", std::to_string(msg.length())},
                                        {"Connection", "close"}
                                    });
                            }
                            else {
                                auto req_string = std::string((char*)body.data(), body.size());
                                auto request = json::parse(req_string);

                                try {
                                    auto msg = std::string("Árbol creado correctamente");
                                    control->newTreeInterface(request);
                                    session->close( restbed::OK, msg, {
                                            {"Content-Length", std::to_string(msg.length())},
                                            {"Connection", "close"}
                                        });
                                }
                                catch (std::string e){
                                    auto msg = std::string("Ocurrió un error al procesar la solicitud: ");
                                    msg.append(e);
                                    session->close(restbed::BAD_REQUEST, msg, {
                                            {"Content-Length", std::to_string(msg.length())},
                                            {"Connection", "close"}
                                        });
                                }
                                catch (...) {
                                    auto msg = std::string("Ocurrió un error al procesar la solicitud.");
                                    session->close(restbed::BAD_REQUEST, msg, {
                                            {"Content-Length", std::to_string(msg.length())},
                                            {"Connection", "close"}
                                        });                                    
                                }
                            }
                        });
            });

  
    res2->set_path( "/ancestro-comun" );
    res2->set_method_handler( "GET",
            [&](const std::shared_ptr<restbed::Session> session)
            { /* Web Service 2 : GET */
                              
                const auto request = session->get_request( );

                auto content_length = 0;
                request->get_header( "Content-Length", content_length);
                std::string qValue = request->get_query_parameter("q", "");

                if (qValue == "") {
                    auto msg = std::string("Campo de solicitud vacío (q)");
                    session->close(restbed::BAD_REQUEST, msg, {
                            {"Content-Length", std::to_string(msg.length())},
                            {"Connection", "close"}
                        });
                }
                else {

                    try {
                        std::shared_ptr<json> LCA = control->lowestCommonAncestorInterface(json::parse(qValue));
                        std::string LCAConvertido;
                        if (LCA->is_string()) {
                            LCAConvertido = LCA->get<std::string>();
                        }
                        else {
                            LCAConvertido = LCA->dump();
                        }
                        session->close (restbed::OK, LCAConvertido, {
                                {"Content-Length", std::to_string(LCAConvertido.length())},
                                {"Connection", "close"}
                            });
                    }
                    catch (std::string e){
                        auto msg = std::string("Ocurrió un error al procesar la solicitud: ");
                        msg.append(e);
                        session->close(restbed::BAD_REQUEST, msg, {
                                {"Content-Length", std::to_string(msg.length())},
                                {"Connection", "close"}
                            });
                    }
                    catch (...) {
                        auto msg = std::string("Ocurrió un error al procesar la solicitud.");
                        session->close(restbed::BAD_REQUEST, msg, {
                                {"Content-Length", std::to_string(msg.length())},
                                {"Connection", "close"}
                            });                                    
                    }

                }
            });

    try {
        restbed::Service service;
        service.publish( res1 );
        service.publish( res2 );
        service.start();
    }
    catch (...) {
        std::cerr << "Error fatal: No se pudieron exponer los webservices! "
                     "Asegurese de tener permiso y que el puerto no esté en uso!" << std::endl;
        return 1;
    }

    return EXIT_SUCCESS;
}

