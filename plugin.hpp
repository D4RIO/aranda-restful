#ifndef _PLUGIN_HPP_
#define _PLUGIN_HPP_

#include "restful.hpp"

namespace d
{
    /**
     * Clase para implementar plugins que sirvan como recursos de restbed.
     * La principal diferencia es el acceso al control, necesario en el
     * handler de los web services.
     */
    class Plugin : public restbed::Resource
    {
    private:
        std::shared_ptr< Control > control;
    public:
        virtual void handler(const std::shared_ptr< restbed::Session> session)=0;
        void setControl (std::shared_ptr< Control > c)
            {
                this->control = c;
            }
        auto getControl(void)
            {
                return this->control;
            }
    };

    /**
     * Es necesaria una factoría de Plugins porque estos son cargados mediante
     * un objeto estático y global.
     */
    class PluginFactory
    {
    public:
        virtual std::shared_ptr< restbed::Resource > get( std::shared_ptr< Control > c )=0;
    };

    // funciones de conveniencia para uso de la factoría
    extern std::shared_ptr< restbed::Resource > plugin ( const std::string& s,
                                                         std::shared_ptr< Control > c);
    extern std::shared_ptr< restbed::Resource > plugin ( const char *s,
                                                         std::shared_ptr< Control > c);

}

#endif

