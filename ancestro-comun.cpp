#include "plugin.hpp"
#include <iostream> // std::cout

// 'using namespace' is bad, usually, but this source
// is tiny and not to be used by any other sources.

using namespace d;

/**
 * Plugin especializado CrearArbol
 */
class AncestroComun : public Plugin
{
public:
    void handler(const std::shared_ptr< restbed::Session > session);
};

/**
 * Factoría especializada para el plugin 
 */
class FactoriaAncestroComun : public PluginFactory
{
public:
    std::shared_ptr< restbed::Resource > get( std::shared_ptr< Control > c );
};

/**
 * Handler del web service Crear Arbol
 */
void AncestroComun::handler(const std::shared_ptr<restbed::Session> session)
{
    /* Web Service 2 : GET */
    const auto request = session->get_request( );

    auto content_length = 0;
    request->get_header( "Content-Length", content_length);
    std::string qValue = request->get_query_parameter("q", "");

    if (qValue == "") {
        auto msg = std::string("Campo de solicitud vacío (q)");
        session->close(restbed::BAD_REQUEST, msg, {
                {"Content-Length", std::to_string(msg.length())}
            });
    }
    else {

        try {
            std::shared_ptr<json> LCA = this->getControl()->lowestCommonAncestorInterface(json::parse(qValue));
            json response;
            if (LCA->is_string()) {
                response["node"] = LCA->get<std::string>();
            }
            else if (LCA->is_number()) {
                response["node"] = LCA->get<int>();
            }
            else {
                response["node"] = LCA->dump();
            }
            session->close (restbed::OK, response.dump(), {
                    {"Content-Length", std::to_string(response.dump().length())}
                });
        }
        catch (std::string e){
            auto msg = std::string("Ocurrió un error al procesar la solicitud: ");
            msg.append(e);
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
}

/**
 * Getter principal del Plugin
 */
std::shared_ptr< restbed::Resource > FactoriaAncestroComun::get( std::shared_ptr< Control > c )
{
    auto r=std::make_shared< AncestroComun > ();

    r->setControl( c );
    r->set_path( "/ancestro-comun" );

    auto f = std::bind(&AncestroComun::handler, r, std::placeholders::_1);
    r->set_method_handler( "GET",  f);

    return r;
}

/**
 * Objeto Global para Acceder a la biblioteca
 */
FactoriaAncestroComun pluginFactory;

