#ifndef _RESTFUL_HPP_
#define _RESTFUL_HPP_

#include <memory>    // shared_ptr
#include <mutex>     // mutex
#include <restbed>   // REST API
#include <sqlite3.h> // SQLite3
#include "json.hpp"  // soporte para JSON (nlohmann)
using json=nlohmann::json;


/* forward */
class Control;


/**
 * Funcionalidad similar a la de un Service en MVCS.
 * Encapsula la lógica de persistencia (BBDD) y la hace
 * transparente para el modelo. En caso de tener varias
 * opciones de BBDD, se puede refactorizar esta clase para proveer
 * insert() y select() de forma transparente al backend.
 */
class Persist {
private:
  sqlite3      *db;               //< Acceso a BBDD
  sqlite3_stmt *insert_stmt;      //< Consulta precompilada para insertar un JSON
  sqlite3_stmt *select_id_stmt;   //< Consulta precompilada para obtener ID desde un JSON
  sqlite3_stmt *select_json_stmt; //< Consulta precompilada para obtener JSON desde un ID
  std::mutex    stmt_mutex;       //< El mutex protege las consultas precompiladas
public:
  Persist(); // Constructor, crea el archivo de BBDD si no existe
  ~Persist();
  int insert (const std::string);
  std::string select (const std::string);
};


/**
 * Funcionalidad similar a MVC View.
 * Encapsula el manejo de cualquier opcion de interfaz.
 */
class Endpoint {
    std::shared_ptr< restbed::Service > service; //< Control de los web services
public:
  Endpoint ();
  ~Endpoint();
  int runWS(const std::shared_ptr<Control> control); //< Ejecución de los web services
};


/**
 * Funcionalidad similar a la de parte del MVC Model.
 * Encapsula la lógica del árbol y el uso del servicio de persistencia.
 */
class Modelo {
private:
  std::shared_ptr<Persist> persistService;  //< Acceso al servicio de persistencia en BBDD
public:
  Modelo();
  ~Modelo();
  int createNewTree(const json);
  std::shared_ptr<json> lowestCommonAncestor(const json);
};


/**
 * Funcionalidad similar a la del MVC Controller.
 * Provee interfaces funcionales, agnóstico de cómo se llevan a cabo.
 * Controla los flujos de información entre los Endpoints y el Modelo.
 */
class Control
  :public std::enable_shared_from_this<Control> {
private:
  std::shared_ptr<Endpoint> webServices; //< Acceso a la vista (Endpoint)
  std::shared_ptr<Modelo>   modeloArbol; //< Acceso al modelo
public:
  Control();
  ~Control();
  int run(void);
  int newTreeInterface(const json);
  std::shared_ptr<json> lowestCommonAncestorInterface(const json);
};



#endif
