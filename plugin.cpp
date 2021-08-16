#include "plugin.hpp"
#include <dlfcn.h>  // dlopen, dlsym, dlclose
#include <iostream> // std::cout

/** ***************************************************************************
 * Esta función brinda compatibilidad con strings de C++.
 * @param s Ruta de la biblioteca a abrir
 * @param c Control de la aplicación
 * @return Objeto de un tipo que hereda de restbed::Resource.
 ** ***************************************************************************/
std::shared_ptr< restbed::Resource > d::plugin(const std::string& s, std::shared_ptr< Control > c)
{
    return plugin ( s.c_str(), c );
}

/** ***************************************************************************
 * Abre la biblioteca, carga la factoría, y obtiene un objeto Plugin.
 * @param s Ruta de la biblioteca a abrir
 * @param c Control de la aplicación
 * @return Objeto de un tipo que hereda de restbed::Resource.
 ** ***************************************************************************/
std::shared_ptr< restbed::Resource > d::plugin(const char *s, std::shared_ptr< Control > c)
{
    auto lib = dlopen ( s, RTLD_LAZY );

    if (!lib)
        throw std::runtime_error( dlerror() );

    auto factory = (d::PluginFactory*) dlsym ( lib, "pluginFactory" );

    if (!factory)
        throw std::runtime_error( dlerror() );

    auto plugin = factory->get( c );

    return plugin;
}

