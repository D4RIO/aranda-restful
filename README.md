# Aranda Restful #
## Prueba técnica ##

Este repositorio corresponde a una prueba técnica para Aranda Software.

## Prerrequisitos - Dependencias ##

Esta interfaz usa [Restbed de Corvusoft](https://github.com/Corvusoft/restbed "Restbed is a comprehensive and consistent programming model for building applications that require seamless and secure communication over HTTP, with the ability to model a range of business processes, designed to target mobile, tablet, desktop and embedded production environments.") y fue programada y probada en [Ubuntu 21.04](https://ubuntu.com/download/desktop "Ubuntu is an ancient African word meaning ‘humanity to others’. It is often described as reminding us that ‘I am what I am because of who we all are’. We bring the spirit of Ubuntu to the world of computers and software. The Ubuntu distribution represents the best of what the world’s software community has shared with the world.").

Para usar restbed en Ubuntu:

``` bash
apt install librestbed0 librestbed-dev
```

Esta interfaz usa [SQLite3](https://www.sqlite.org/index.html "SQLite is a C-language library that implements a small, fast, self-contained, high-reliability, full-featured, SQL database engine. SQLite source code is in the public-domain and is free to everyone to use for any purpose."), en Ubuntu:

``` bash
apt install libsqlite3-0 libsqlite3-dev
```

Para compilar se utilizan [`GNU make`](https://www.gnu.org/software/make/ "GNU Make is a tool which controls the generation of executables and other non-source files of a program from the program's source files.") y `g++`, parte de [`GCC`](https://gcc.gnu.org/ "The GNU Compiler Collection includes front ends for C, C++, Objective-C, Fortran, Ada, Go, and D, as well as libraries for these languages (libstdc++,...). GCC was originally written as the compiler for the GNU operating system. The GNU system was developed to be 100% free software, free in the sense that it respects the user's freedom."), para instalar:

``` bash
apt install build-essential
```

## Compilación ##

Con todas las dependencias anteriores satisfechas, se puede compilar tan simplemente como:

``` bash
make
```

Para compilar la documentación [DOXYGEN](https://www.doxygen.nl/index.html "Doxygen is the de facto standard tool for generating documentation from annotated C++ sources, but it also supports other popular programming languages such as C, Objective-C, C#, PHP, Java, Python, IDL (Corba, Microsoft, and UNO/OpenOffice flavors), Fortran, VHDL and to some extent D."):

``` bash
make doc
```

Puede ver la documentación una vez compilada abriendo [doc/html/index.html](doc/html/index.html "Documentación Doxygen").

## Uso y Pruebas ##

Una vez compilado, el servidor puede iniciarse directamente mediante su ejecutable:

``` bash
./restful
```

Sin embargo, ejecutar los web services en el puerto 80 puede solicitar privilegios. Los siguientes son consejos de seguridad:

 1. Asgúrese que el ejecutable NO tiene set-user-ID ni set-group-ID:

 ``` bash
 chmod u-s,g-s restful
 ```
 2. En lo posible, tenga un usuario separado, sin privilegios ni permisos. Este usuario debe poder escribir en el directorio en que se ubica el binario. Supondremos que ese usuario es `foobar`. Cambie el propietario y grupo de restful a este usuario:

 ``` bash
 chown foobar restful
 chgrp foobar restful
 ```
 3. Use setcap para darle a restful acceso a los puertos bajos (setcap requiere privilegios):

 ``` bash
 setcap CAP_NET_BIND_SERVICE=+eip restful
 ```

Los webservices se inician en el servidor, que puede ser la misma máquina que el cliente o no. Para usarse, se envian solicitudes del tipo `application/json`, via POST para el webservice `crear-arbol`, y vía GET para el webservice `ancestro-comun`. En el primero, cada árbol se forma con objetos nodo, cada uno con el siguiente formato:

``` json
{
    "node": <datos>,
    "left": {<objeto>},
    "right": {<objeto>}
}
```

En todos los nodos debe existir el campo `node`, y los datos pueden ser cualesquiera que desee siempre que cumplan con el modelo de JSON. Los campos `left` y `right` son opcionales. En conformidad con el [estándar JSON](https://datatracker.ietf.org/doc/html/rfc8259.html#section-1 "RFC 8259: The JavaScript Object Notation (JSON) Data Interchange Format"), el orden de los campos no importa.

Para probar los servicios manualmente, se puede usar [curl](https://curl.se/docs/manpage.html "CURL: command line tool and library for transferring data with URLs"), por ejemplo:

``` bash
# CREAR ARBOL
# Devuelve un JSON con un ID para consultar
curl --header "Content-Type: application/json" \
     --request POST \
     -w'\n'\
     --data '{"node":1,"left":{"node":2}}' \
     http://localhost/crear-arbol


# CONSULTAR ANCESTRO COMÚN
# Usar el id devuelto por el servicio anterior
curl --header 'Content-Type: application/json' \
     -s -G -w'\n' \
     --data-urlencode 'q={"id":1,"node_a":1,"node_b":2}' \
     http://localhost/ancestro-comun
```

