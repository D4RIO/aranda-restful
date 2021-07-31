#ifndef _RESTFUL_HPP_
#define _RESTFUL_HPP_

#include <memory> // shared_ptr
#include "json.hpp" // soporte para JSON (nlohmann)
using json=nlohmann::json;

/* Funcionalidad similar a la de un Service en MVCS
 * La idea es encapsular la lógica de persistencia (BBDD) y
 * hacerla transparente para el modelo
 */
class Persist {
public:
  Persist() {}
};



/* Funcionalidad similar a la del MVC Controller
 */
class Control
  :public std::enable_shared_from_this<Control> {
public:
  Control() {}
  int run(void); // la interfaz externa principal
  void newTreeInterface(const json);
};



/* Funcionalidad similar a MVC View
 * La idea es encapsular el manejo de cualquier opcion de interfaz.
 */
class Endpoint {
protected:
  Endpoint() {}         // Se impide la construcción sin un control
  std::shared_ptr<Control> control;
public:
  Endpoint (std::shared_ptr<Control> c) {control = c;}
  auto Controlador() {return control;}
  int runWS(void);      // Control de los WebServices
};



/* Funcionalidad similar a la de parte del MVC Model
 * La idea es encapsular la lógica del árbol y nada más
 */
class Modelo {
protected:
  std::shared_ptr<Persist> persistService;
public:
  Modelo() {std::make_shared<Persist>();}
  void createNewTree(const json);
};




#endif
