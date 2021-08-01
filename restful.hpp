#ifndef _RESTFUL_HPP_
#define _RESTFUL_HPP_

#include <memory> // shared_ptr
#include "json.hpp" // soporte para JSON (nlohmann)
using json=nlohmann::json;


/* forward */
class Control;



/* Funcionalidad similar a la de un Service en MVCS
 * La idea es encapsular la lógica de persistencia (BBDD) y
 * hacerla transparente para el modelo
 */
class Persist {
public:
  Persist() {}
};



/* Funcionalidad similar a MVC View
 * La idea es encapsular el manejo de cualquier opcion de interfaz.
 */
class Endpoint {
public:
  Endpoint () {}
  int runWS(std::shared_ptr<Control> control);      // Control de los WebServices
};



/* Funcionalidad similar a la de parte del MVC Model
 * La idea es encapsular la lógica del árbol y nada más
 */
class Modelo {
protected:
  json arbol; // Por conveniencia, el árbol es un objeto JSON
  std::shared_ptr<Persist> persistService;
public:
  Modelo() {std::make_shared<Persist>();}
  void createNewTree(const json);
  // el objeto de búsqueda debe contener: id, node_a, node_b
  std::shared_ptr<json> lowestCommonAncestor(const json);
};



/* Funcionalidad similar a la del MVC Controller
 */
class Control
  :public std::enable_shared_from_this<Control> {
private:
  std::shared_ptr<Endpoint> webServices;
  std::shared_ptr<Modelo> modeloArbol;
public:
  Control();
  int run(void); // la interfaz externa principal
  void newTreeInterface(const json);
  std::shared_ptr<json> lowestCommonAncestorInterface(const json);
};



#endif
