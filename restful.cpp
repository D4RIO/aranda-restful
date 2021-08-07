#include <iostream>
#include <memory>    // make_shared<>() ... etc
#include <restbed>   // REST API
#include <stack>     // std::stack<>
#include "restful.hpp"



/** ***************************************************************************
 * Constructor. Instancia el Endpoint y el Modelo como miembros.
 ** ***************************************************************************/
Control::Control()
{
    webServices = std::make_shared<Endpoint>();
    modeloArbol = std::make_shared<Modelo>();
}

/** ***************************************************************************
 * Procedimiento principal del control. Ejecuta los Web Services.
 * @return Estado de error de salida.
 ** ***************************************************************************/
int Control::run(void)
{
    return webServices->runWS (shared_from_this());
}

/** ***************************************************************************
 * Interfaz de creación de árboles del controlador.
 * @see Modelo::createNewTree(const json)
 * @param obj Objeto nlohmann::json con el árbol a guardar
 * @return ID del árbol creado (o ya existente)
 ** ***************************************************************************/
int Control::newTreeInterface(const json obj)
{
    return modeloArbol->createNewTree(obj);
}

/** ***************************************************************************
 * Interfaz de búsqueda de ancestro común del controlador.
 * @see Modelo::lowestCommonAncestor(const json)
 * @param obj Objeto nlohmann::json con la búsqueda (id, node_a, node_b)
 * @return JSON conteniendo el ancestro común
 ** ***************************************************************************/
std::shared_ptr<json> Control::lowestCommonAncestorInterface(const json obj)
{
    return modeloArbol->lowestCommonAncestor(obj);
}

/** ***************************************************************************
 * Constructor. Instancia el servicio de persistencia en BD.
 ** ***************************************************************************/
Modelo::Modelo()
{
    persistService = std::make_shared<Persist>();
}

/** ***************************************************************************
 * Creación de árbol a partir de JSON. Persiste el JSON en BD para que sea
 * accesible mediante consultas. Usa el servicio insert.
 * @see Persist::insert(std::string)
 * @param Objeto nlohmann::json con el árbol a guardar
 * @return ID del árbol creado (o ya existente)
 ** ***************************************************************************/
int Modelo::createNewTree(const json o)
{
    if (o.find("node")==o.end())
        throw std::string("Todos los árboles deben tener al menos un nodo!");

    // Los errores en INSERT no se informan detalladamente al cliente, pero se loguean
    try {

        return persistService->insert(o.dump());

    }
    catch (std::string e) {
        std::cerr << "Error en INSERT: " << e << std::endl;
        throw std::string ("Error interno. No se puede crear el árbol.");
    }
    catch (...) {
        std::cerr << "Error inesperado en INSERT" << std::endl;
        throw std::string ("Error interno. No se puede crear el árbol.");
    }
}

/** ***************************************************************************
 * Búsqueda de ancestro común más cercano. Se debe proporcionar una búsqueda del
 * formato {"id":<id>,"node_a":<node>, "node_b":<node>} donde el ID corresponde
 * al de un árbol creado mediante el servicio de creación de árboles.
 * @param objBusqueda Objeto nlohmann::json con la búsqueda (id, node_a, node_b)
 * @return JSON conteniendo el ancestro común
 ** ***************************************************************************/
std::shared_ptr<json> Modelo::lowestCommonAncestor(const json objBusqueda)
{
    std::shared_ptr<json> LCA = std::make_shared<json>();
    std::stack<json> camino, nodo_a, nodo_b;
    std::stack< std::pair<json,json> > working;
    json arbol;

    auto contieneNodo = [] (json o, std::string nodo) {
        // closure para simplificar la verificación de existencia
        // de un nodo en un objeto JSON
        return o.find(nodo)!=o.end();
    };

    if (! contieneNodo (objBusqueda, "id"))
        throw std::string ("ID del árbol requerido (falta campo id)");

    if (! contieneNodo (objBusqueda, "node_a") ||
        ! contieneNodo (objBusqueda, "node_b") )
        throw std::string ("Nodos de búsqueda requeridos (falta campo node_a o node_b)");

    try {
        std::string arbol_string = this->persistService->select(objBusqueda["id"].dump());
        arbol = json::parse(arbol_string);
    }
    catch (std::string e) {
        std::cerr << "No se encontró el árbol ID: ["<< objBusqueda["id"] << "]" << std::endl;
        std::cerr << "Descripción: " << e << std::endl;
        throw std::string("No se encontró ningún árbol (campo id erróneo)");
    }
    catch (...) {
        std::cerr << "No se encontró el árbol ID: ["<< objBusqueda["id"] << "]" << std::endl;
        throw std::string("No se encontró ningún árbol (campo id erróneo)");
    }

    // Se comienza por el nodo raíz, sin último padre
    working.push({0, arbol});

    
    while (! working.empty())
    {
        auto i = working.top().first;
        auto o = working.top().second;
        working.pop();

        if (! camino.empty())
            while (i != camino.top()) {
                camino.pop();
            }

        if (! contieneNodo(o, "node"))
            throw std::string (R"(Árbol mal formado, todos los nodos deben tener un campo "node")");

        camino.push(o["node"]);

        // guardado de la pila como dirección de los nodos
        if (o["node"]==objBusqueda["node_a"])  nodo_a = camino;
        if (o["node"]==objBusqueda["node_b"])  nodo_b = camino;

        // continúa el DFS
        if (contieneNodo (o, "left")) {
            working.push({o["node"],o["left"]});
        }
        if (contieneNodo (o, "right")) {
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

/** ***************************************************************************
 * Lanzador principal de los Web Services. Estos se ejecutan en hilos distintos
 * dependiendo de la configuración de RestBed.
 * @param control Controlador principal, para uso de sus interfaces.
 * @return Estado final cuando los Web Services se finalizan.
 ** ***************************************************************************/
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
                                    json response;
                                    auto id = control->newTreeInterface(request);
                                    response["id"]=id;
                                    session->close( restbed::OK, response.dump(), {
                                            {"Content-Length", std::to_string(response.dump().length())},
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

    auto settings = std::make_shared< restbed::Settings >();
    settings->set_worker_limit( 4 );

    try {
        restbed::Service service;
        service.publish( res1 );
        service.publish( res2 );
        service.start( settings );
    }
    catch (...) {
        std::cerr << "Error fatal: No se pudieron exponer los webservices! "
                     "Asegurese de tener permiso y que el puerto no esté en uso!" << std::endl;
        return 1;
    }

    return EXIT_SUCCESS;
}

/** ***************************************************************************
 * Constructor. Crea el archivo de Base de Datos si no existe, la tabla, y
 * compila las consultas a BBDD que serán usadas en la aplicación.
 ** ***************************************************************************/
Persist::Persist()
{   // Conexión a la BBDD
    auto exit = sqlite3_open( "example.db", &(this->db) );

    // Esta excepción debe llegar a MAIN, no capturar antes.
    if (exit)
        throw std::string("Error abriendo la base de datos: ").append(sqlite3_errmsg(db));

    auto sql =                                    \
        "CREATE TABLE IF NOT EXISTS ARBOLES ( "   \
        "  ID INTEGER PRIMARY KEY,"                  \
        "  JSON TEXT UNIQUE NOT NULL"                    \
        ");";

    exit = sqlite3_exec (db, sql, NULL, NULL, NULL);

    // Esta excepción debe llegar a MAIN, no capturar antes.
    if (exit)
        throw std::string("Error creando la tabla: ").append(sqlite3_errmsg(db));

    sql =                               \
        "INSERT INTO ARBOLES (JSON) "   \
        "VALUES (?);";

    exit = sqlite3_prepare_v2 ( this->db, sql, strlen (sql), &(this->insert_stmt), NULL );

    // Esta excepción debe llegar a MAIN, no capturar antes.
    if (exit)
        throw std::string("Error compilando la consulta INSERT: ").append(sqlite3_errmsg(db));

    sql =                                       \
        "SELECT JSON FROM ARBOLES WHERE ID = ?;";

    exit = sqlite3_prepare_v2 ( this->db, sql, strlen (sql), &(this->select_json_stmt), NULL );

    // Esta excepción debe llegar a MAIN, no capturar antes.
    if (exit)
        throw std::string("Error compilando la consulta SELECT JSON: ").append(sqlite3_errmsg(db));

    sql =                                       \
        "SELECT ID FROM ARBOLES WHERE JSON = ?;";

    exit = sqlite3_prepare_v2 ( this->db, sql, strlen (sql), &(this->select_id_stmt), NULL );

    // Esta excepción debe llegar a MAIN, no capturar antes.
    if (exit)
        throw std::string("Error compilando la consulta SELECT ID: ").append(sqlite3_errmsg(db));
}

/** ***************************************************************************
 * Servicio de inserción en BBDD con mutex para los hilos de RestBed.
 * @param json_to_save std::string con JSON del árbol a guardar en BBDD
 * @return ID del árbol guardado
 ** ***************************************************************************/
int Persist::insert( const std::string json_to_save )
{
    const std::lock_guard<std::mutex> lock( this->stmt_mutex );
    /*=================================== INSERT =======================================*/

    auto exit = sqlite3_reset ( this->insert_stmt );

    // Esta excepción debe llegar al WS.
    // El WS no debe informar el error al cliente. Solo un BAD REQUEST o un SERVER ERROR. Puede loguear.
    if (exit && exit != SQLITE_CONSTRAINT)
        throw std::string("Error inesperado preparándose para la consulta INSERT (reset) [")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db));

    // INSERT INTO ARBOLES (JSON)
    // VALUES (?);
    exit = sqlite3_bind_text (
        this->insert_stmt,      // Statement compilado
        1,                      // Enlazar al 1er valor de la consulta
        json_to_save.c_str(),   // Qué valor enlazar
        json_to_save.length(),  // Longitud del valor enlazado
        NULL
        );

    // Esta excepción debe llegar al WS.
    // El WS no debe informar el error al cliente. Solo un BAD REQUEST o un SERVER ERROR. Puede loguear.
    if (exit)
        throw std::string("Error alimentando a la consulta INSERT (bind): ").append(sqlite3_errmsg(db));

    exit = sqlite3_step ( this->insert_stmt );

    // Esta excepción debe llegar al WS.
    // El WS no debe informar el error al cliente. Solo un BAD REQUEST o un SERVER ERROR. Puede loguear.
    if (exit != SQLITE_DONE && exit != SQLITE_CONSTRAINT)
        throw std::string("Error ejecutando la consulta INSERT [")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db));

    /*==================================== SELECT ======================================*/


    exit = sqlite3_reset ( this->select_id_stmt );

    // Esta excepción debe llegar al WS.
    // El WS no debe informar el error al cliente. Solo un BAD REQUEST o un SERVER ERROR. Puede loguear.
    if (exit)
        throw std::string("Error inesperado preparándose para la consulta SELECT ID (reset): ").append(sqlite3_errmsg(db));

    // SELECT ID FROM ARBOLES
    // WHERE JSON=?;
    exit = sqlite3_bind_text (
        this->select_id_stmt,   // Statement compilado
        1,                      // Enlazar al 1er valor de la consulta
        json_to_save.c_str(),   // Qué valor enlazar
        json_to_save.length(),  // Longitud del valor enlazado
        NULL
        );

    // Esta excepción debe llegar al WS.
    // El WS no debe informar el error al cliente. Solo un BAD REQUEST o un SERVER ERROR. Puede loguear.
    if (exit)
        throw std::string("Error alimentando a la consulta SELECT ID (bind): ").append(sqlite3_errmsg(db));

    exit = sqlite3_step ( this->select_id_stmt );

    // Esta excepción debe llegar al WS.
    // El WS no debe informar el error al cliente. Solo un BAD REQUEST o un SERVER ERROR. Puede loguear.
    if (exit != SQLITE_ROW)
        throw std::string("Error ejecutando la consulta SELECT_ID (sin resultados)[")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db));

    auto id = sqlite3_column_int ( this->select_id_stmt, 0 );

    return id;
}

/** ***************************************************************************
 * Servicio de obtención del árbol a partir de su ID.
 * @param id std::string con ID del árbol a buscar
 * @return std::string con JSON del árbol
 ** ***************************************************************************/
std::string Persist::select(const std::string id)
{
    const std::lock_guard<std::mutex> lock( this->stmt_mutex );

    auto exit = sqlite3_reset ( this->select_json_stmt );

    // Esta excepción debe llegar al WS.
    // El WS no debe informar el error al cliente. Solo un BAD REQUEST o un SERVER ERROR. Puede loguear.
    if (exit)
        throw std::string("Error inesperado preparándose para la consulta SELECT JSON (reset) [")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db));

    // SELECT JSON FROM ARBOLES
    // WHERE ID=?;
    exit = sqlite3_bind_text (
        this->select_json_stmt, // Statement compilado
        1,                      // Enlazar al 1er valor de la consulta
        id.c_str(),             // Qué valor enlazar
        id.length(),            // Longitud del valor enlazado
        NULL
        );

    // Esta excepción debe llegar al WS.
    // El WS no debe informar el error al cliente. Solo un BAD REQUEST o un SERVER ERROR. Puede loguear.
    if (exit)
        throw std::string("Error alimentando a la consulta SELECT JSON (bind) [")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db));

    exit = sqlite3_step ( this->select_json_stmt );

    // Esta excepción debe llegar al WS.
    // El WS no debe informar el error al cliente. Solo un BAD REQUEST o un SERVER ERROR. Puede loguear.
    if (exit != SQLITE_ROW) {
        throw std::string("Error ejecutando la consulta SELECT JSON (sin resultados)[")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db));
    }

    auto result = std::string((char*)sqlite3_column_text ( this->select_json_stmt, 0 ));

    return result;
}

/** ***************************************************************************
 * Destructor. Finaliza los statements y cierra la conexión a BBDD.
 ** ***************************************************************************/
Persist::~Persist()
{
    auto exit = sqlite3_finalize ( this->insert_stmt );

    if (exit)
        std::cerr << std::string("Error finalizando la consulta INSERT: [")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db))
                  << std::endl;

    exit = sqlite3_finalize ( this->select_id_stmt );

    if (exit)
        std::cerr << std::string("Error finalizando la consulta SELECT ID: [")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db))
                  << std::endl;

    sqlite3_close( this->db );
}
