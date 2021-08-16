#include <functional>
#include <iostream> // std::cout

#include "json.hpp" // nlohmann::json
#include "plugin.hpp"

// 'using namespace' is bad, usually, but this source
// is tiny and not to be used by any other sources.

using namespace d;

/**
 * Plugin especializado CrearArbol
 */
class CrearArbol : public Plugin
{
public:
    void handler(const std::shared_ptr< restbed::Session > session);
};

/**
 * Factoría especializada para el plugin 
 */
class FactoriaCrearArbol : public PluginFactory
{
public:
    std::shared_ptr< restbed::Resource > get( std::shared_ptr< Control > c );
};

/**
 * Handler del web service Crear Arbol
 */
void CrearArbol::handler(const std::shared_ptr<restbed::Session> session)
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
                                       {"Content-Length", std::to_string(msg.length())}
                                   });
                           }
                           else {
                               auto req_string = std::string((char*)body.data(), body.size());
                               auto request = json::parse(req_string);

                               try {
                                   json response;
                                   auto id = this->getControl()->newTreeInterface(request);
                                   response["id"]=id;
                                   session->close( restbed::OK, response.dump(), {
                                           {"Content-Length", std::to_string(response.dump().length())}
                                       });
                               }
                               catch (std::exception& e){
                                   auto msg = std::string("Ocurrió un error al procesar la solicitud: ");
                                   msg.append(e.what());
                                   session->close(restbed::BAD_REQUEST, msg, {
                                           {"Content-Length", std::to_string(msg.length())}
                                       });
                               }
                               catch (...) {
                                   auto msg = std::string("Ocurrió un error al procesar la solicitud.");
                                   session->close(restbed::BAD_REQUEST, msg, {
                                           {"Content-Length", std::to_string(msg.length())}
                                       });                                    
                               }
                           }
                       });
}

/**
 * Getter principal del Plugin
 */
std::shared_ptr< restbed::Resource > FactoriaCrearArbol::get( std::shared_ptr< Control > c )
{
    auto r=std::make_shared< CrearArbol > ();

    r->setControl( c );
    r->set_path( "/crear-arbol" );

    auto f = std::bind(&CrearArbol::handler, r, std::placeholders::_1);
    r->set_method_handler( "POST",  f);

    return r;
}

/**
 * Objeto Global para Acceder a la biblioteca
 */
FactoriaCrearArbol pluginFactory;

