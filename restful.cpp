#include <iostream>
#include <memory>    // make_shared<>() ... etc
#include <stack>     // std::stack<>
#include "restful.hpp"
#include "plugin.hpp"



/** ***************************************************************************
 * Constructor. Instancia el Endpoint y el Modelo como miembros.
 ** ***************************************************************************/
Control::Control()
{
    webServices = std::make_shared<Endpoint>();
    modeloArbol = std::make_shared<Modelo>();
}

/** ***************************************************************************
 * Destructor. Llama a los destructores miembros.
 ** ***************************************************************************/
Control::~Control()
{
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
 * Destructor. Llama a los destructores miembro.
 ** ***************************************************************************/
Modelo::~Modelo()
{
    persistService->~Persist();
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
        throw std::logic_error( "Todos los árboles deben tener al menos un nodo!" );

    // Los errores en INSERT no se informan detalladamente al cliente, pero se loguean
    try {

        return persistService->insert(o.dump());

    }
    catch (std::exception& e) {
        std::cerr << "Error en INSERT: " << e.what() << std::endl;
        throw std::runtime_error ( "Error interno. No se puede crear el árbol." );
    }
    catch (...) {
        std::cerr << "Error inesperado en INSERT" << std::endl;
        throw std::runtime_error ( "Error interno. No se puede crear el árbol." );
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
        return o.find(nodo)!=o.end();
    };

    if (! contieneNodo (objBusqueda, "id"))
        throw std::logic_error ( "ID del árbol requerido (falta campo id)" );

    if (! contieneNodo (objBusqueda, "node_a") ||
        ! contieneNodo (objBusqueda, "node_b") )
        throw std::logic_error ( "Nodos de búsqueda requeridos (falta campo node_a o node_b)" );

    try {
        std::string arbol_string = this->persistService->select(objBusqueda["id"].dump());
        arbol = json::parse(arbol_string);
    }
    catch (std::exception& e) {
        std::cerr << "No se encontró el árbol ID: ["<< objBusqueda["id"] << "]" << std::endl;
        std::cerr << "Descripción: " << e.what() << std::endl;
        throw std::logic_error ( "No se encontró ningún árbol (campo id erróneo)" );
    }
    catch (...) {
        std::cerr << "No se encontró el árbol ID: ["<< objBusqueda["id"] << "]" << std::endl;
        throw std::logic_error ( "No se encontró ningún árbol (campo id erróneo)" );
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
            throw std::logic_error ( R"(Árbol mal formado, todos los nodos deben tener un campo "node")" );

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

    throw std::logic_error ( "Error encontrando el ancestro. Verifique que el objeto no contenga más de un árbol." );
}

/** ***************************************************************************
 * Contructor
 ** ***************************************************************************/
Endpoint::Endpoint()
{
    this->service = std::make_shared< restbed::Service >();
}

/** ***************************************************************************
 * Destructor
 ** ***************************************************************************/
Endpoint::~Endpoint()
{
    service->stop();
}

/** ***************************************************************************
 * Lanzador principal de los Web Services. Estos se ejecutan en hilos distintos
 * dependiendo de la configuración de RestBed.
 * @param control Controlador principal, para uso de sus interfaces.
 * @return Estado final cuando los Web Services se finalizan.
 ** ***************************************************************************/
int Endpoint::runWS(std::shared_ptr<Control> control)
{
    auto res1 = d::plugin("./libcrear-arbol.so", control);
    auto res2 = d::plugin("./libancestro-comun.so", control);

    char const *max_threads = getenv( "RESTFUL_MAX_THREADS" );
    if ( ! max_threads )
        max_threads = "4";

    char const *port_no = getenv ( "RESTFUL_PORT" );
    if ( ! port_no )
        port_no = "80";

    auto settings = std::make_shared< restbed::Settings >();

    try {
        settings->set_worker_limit( std::stoi( max_threads ) );
        settings->set_port( std::stoi( port_no ) );
        settings->set_default_header( "Connection", "close" );
    }
    catch (...) {
        std::cerr << "Error fatal estableciendo la configuración del servidor. "
            "Es posible que esto se deba a establecer mal las variables de entorno." << std::endl;
        return 1;
    }

    try {
        service->publish( res1 );
        service->publish( res2 );
        service->start( settings );
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
{
    char const *db_name = getenv("RESTFUL_DB");
    if ( ! db_name )
        db_name = "restful.db";

    // Conexión a la BBDD
    auto exit = sqlite3_open( db_name, &(this->db) );

    // Esta excepción debe llegar a MAIN, no capturar antes.
    if (exit)
        throw std::runtime_error (std::string("Error abriendo la base de datos: ").append(sqlite3_errmsg(db)));

    auto sql =                                    \
        "CREATE TABLE IF NOT EXISTS ARBOLES ( "   \
        "  ID INTEGER PRIMARY KEY,"                  \
        "  JSON TEXT UNIQUE NOT NULL"                    \
        ");";

    exit = sqlite3_exec (db, sql, NULL, NULL, NULL);

    // Esta excepción debe llegar a MAIN, no capturar antes.
    if (exit)
        throw std::runtime_error ( std::string("Error creando la tabla: ").append(sqlite3_errmsg(db)) );

    sql =                               \
        "INSERT INTO ARBOLES (JSON) "   \
        "VALUES (?);";

    exit = sqlite3_prepare_v2 ( this->db, sql, strlen (sql), &(this->insert_stmt), NULL );

    // Esta excepción debe llegar a MAIN, no capturar antes.
    if (exit)
        throw std::runtime_error ( std::string("Error compilando la consulta INSERT: ").append(sqlite3_errmsg(db)) );

    sql =                                       \
        "SELECT JSON FROM ARBOLES WHERE ID = ?;";

    exit = sqlite3_prepare_v2 ( this->db, sql, strlen (sql), &(this->select_json_stmt), NULL );

    // Esta excepción debe llegar a MAIN, no capturar antes.
    if (exit)
        throw std::runtime_error ( std::string("Error compilando la consulta SELECT JSON: ").append(sqlite3_errmsg(db)) );

    sql =                                       \
        "SELECT ID FROM ARBOLES WHERE JSON = ?;";

    exit = sqlite3_prepare_v2 ( this->db, sql, strlen (sql), &(this->select_id_stmt), NULL );

    // Esta excepción debe llegar a MAIN, no capturar antes.
    if (exit)
        throw std::runtime_error ( std::string("Error compilando la consulta SELECT ID: ").append(sqlite3_errmsg(db)) );
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
        throw std::runtime_error ( std::string("Error inesperado preparándose para la consulta INSERT (reset) [")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db)) );

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
        throw std::runtime_error (
            std::string("Error alimentando a la consulta INSERT (bind): ")
            .append(sqlite3_errmsg(db)) );

    exit = sqlite3_step ( this->insert_stmt );

    // Esta excepción debe llegar al WS.
    // El WS no debe informar el error al cliente. Solo un BAD REQUEST o un SERVER ERROR. Puede loguear.
    if (exit != SQLITE_DONE && exit != SQLITE_CONSTRAINT)
        throw std::runtime_error(
            std::string("Error ejecutando la consulta INSERT [")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db)) );

    /*==================================== SELECT ======================================*/


    exit = sqlite3_reset ( this->select_id_stmt );

    // Esta excepción debe llegar al WS.
    // El WS no debe informar el error al cliente. Solo un BAD REQUEST o un SERVER ERROR. Puede loguear.
    if (exit)
        throw std::runtime_error(
            std::string("Error inesperado preparándose para la consulta SELECT ID (reset): ")
            .append(sqlite3_errmsg(db)) );

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
        throw std::runtime_error (
            std::string("Error alimentando a la consulta SELECT ID (bind): ")
            .append(sqlite3_errmsg(db)) );

    exit = sqlite3_step ( this->select_id_stmt );

    // Esta excepción debe llegar al WS.
    // El WS no debe informar el error al cliente. Solo un BAD REQUEST o un SERVER ERROR. Puede loguear.
    if (exit != SQLITE_ROW)
        throw std::runtime_error (
            std::string("Error ejecutando la consulta SELECT_ID (sin resultados)[")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db)) );

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
        throw std::runtime_error (
            std::string("Error inesperado preparándose para la consulta SELECT JSON (reset) [")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db)) );

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
        throw std::runtime_error (
            std::string("Error alimentando a la consulta SELECT JSON (bind) [")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db)) );

    exit = sqlite3_step ( this->select_json_stmt );

    // Esta excepción debe llegar al WS.
    // El WS no debe informar el error al cliente. Solo un BAD REQUEST o un SERVER ERROR. Puede loguear.
    if (exit != SQLITE_ROW) {
        throw std::runtime_error (
            std::string("Error ejecutando la consulta SELECT JSON (sin resultados)[")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db)) );
    }

    auto result = std::string((char*)sqlite3_column_text ( this->select_json_stmt, 0 ));

    return result;
}

/** ***************************************************************************
 * Destructor. Finaliza los statements y cierra la conexión a BBDD.
 ** ***************************************************************************/
Persist::~Persist()
{
    const std::lock_guard<std::mutex> lock( this->stmt_mutex );
    std::cout << "Finalizando conexión a Base de Datos" << std::endl;

    
    if (auto exit = sqlite3_finalize ( this->insert_stmt ); exit)
        std::cerr << std::string("Error finalizando la consulta INSERT: [")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db))
                  << std::endl;

    this->insert_stmt = NULL;

    

    if (auto exit = sqlite3_finalize ( this->select_id_stmt ); exit)
        std::cerr << std::string("Error finalizando la consulta SELECT ID: [")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db))
                  << std::endl;

    this->select_id_stmt = NULL;


    if (auto exit = sqlite3_finalize ( this->select_json_stmt ); exit)
        std::cerr << std::string("Error finalizando la consulta SELECT JSON: [")
            .append(std::to_string(exit))
            .append("]: ")
            .append(sqlite3_errmsg(db))
                  << std::endl;

    this->select_json_stmt = NULL;

    sqlite3_close( this->db );

    this->db = NULL;
}
