#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include "doctest.h"
#include "../json.hpp"
#include "../restful.hpp"

TEST_CASE ("Operaciones en BBDD mediante Persist")
{
    setenv( "RESTFUL_DB", "test/test.db", 1 );
    REQUIRE_NOTHROW ( Persist p );
    Persist p;

    SUBCASE ("El servicio de persistencia no verifica formato para insertar")
    {
        int id1=0, id2=0;

        MESSAGE ("Este caso también prueba, implícitamente, la consulta SELECT ID");
        REQUIRE_NOTHROW (id1 = p.insert( "Testing" ));

        SUBCASE ("El servicio de persistencia se encarga del UNIQUE CONSTRAINT")
        {
            REQUIRE_NOTHROW ( id2 = p.insert( "Testing" ) );
            CHECK_EQ ( id1, id2 );
        }

        SUBCASE ("El servicio de persistencia 'select' (SELECT ID) devuelve el valor correcto")
        {
            std::string s;
            REQUIRE_NOTHROW ( s = p.select( std::to_string(id1) ) );
            CHECK_EQ ( s, "Testing" );
        }
    }
}

TEST_CASE ("Insertar un árbol sin nodo raiz")
{
    setenv( "RESTFUL_DB", "test/test.db", 1 );
    setenv( "RESTFUL_PORT_NO", "37337", 1 );
    const auto c = std::make_shared< Control >();

    nlohmann::json o = {
        "left",{
            {"node",1}
        }
    };

    CHECK_THROWS( c->newTreeInterface( o ) );
}

TEST_CASE ("Consulta de ancestro común con ID erróneo")
{
    setenv( "RESTFUL_DB", "test/test.db", 1 );
    setenv( "RESTFUL_PORT_NO", "37337", 1 );
    const auto c = std::make_shared< Control >();

    nlohmann::json e = {
        {"id",     1000},
        {"node_a", 1},
        {"node_b", 3}
    };

    REQUIRE_THROWS( c->lowestCommonAncestorInterface( e ) );
}

TEST_CASE ("Uso normal, creación de árboles y consulta")
{
    setenv( "RESTFUL_DB", "test/test.db", 1 );
    setenv( "RESTFUL_PORT_NO", "37337", 1 );
    const auto c = std::make_shared< Control >();

    nlohmann::json o = {
        {"node",1},
        {"left",{
                {"node",2},
                {"left",{
                        {"node",3}
                    }
                }
            }
        }
    };

    nlohmann::json o2 = {
        {"node",1},
        {"left",{
                {"node",2},
                {"right",{
                        {"node",3}
                    }
                }
            }
        }
    };

    nlohmann::json big = {
        {"node",1},
        {"left",{
                {"node",2},
                {"left",{
                        {"node",3}
                    }
                }
            }
        },
        {"right",{
                {"node",4},
                {"left",{
                        {"node",5}
                    }
                },
                {"right",{
                        {"node",6}
                    }
                }
            }
        }
    };

    // SQLite asigna 1 como primer valor, ver un 0 en un test significa
    // un error al escribir el test
    int id1=0, id2=0, id3=0, id_big=0;
    std::shared_ptr<nlohmann::json> result;

    SUBCASE ("Insertar dos veces el mismo árbol no debe fallar fuera de SQLite (UNIQUE constraint)")
    {
        REQUIRE_NOTHROW( id1 = c->newTreeInterface( o ) );
        REQUIRE_NOTHROW( id2 = c->newTreeInterface( o ) );
    }

    // Es necesario reasignar por salir del scope del subcaso
    // El requisito de NOTHROW ya se cumplió
    id1 = c->newTreeInterface( o );
    id2 = c->newTreeInterface( o );

    SUBCASE ("El ID depende únicamente del JSON")
    {
        REQUIRE_NOTHROW( id3 = c->newTreeInterface( o2 ) );
        CHECK_EQ( id1, id2 );
        CHECK_NE( id1, id3 );
    }

    SUBCASE("Consulta normal de ancestro común")
    {
        nlohmann::json n = {
            {"id",     id1},
            {"node_a", 1},
            {"node_b", 3}
        };

        REQUIRE_NOTHROW( result = c->lowestCommonAncestorInterface( n ) );
        CHECK_EQ (result->dump(), "1");

        SUBCASE ("Un nodo es ancestro de sí mismo")
        {
            n = {
                {"id",     id1},
                {"node_a", 3},
                {"node_b", 3}
            };

            REQUIRE_NOTHROW( result = c->lowestCommonAncestorInterface( n ) );
            CHECK_EQ (result->dump(), "3");
        }
    }

    REQUIRE_NOTHROW( id_big = c->newTreeInterface( big ) );

    /////// BIG //////
    //         3    //
    //       /      //
    //      2       //
    //    /         //
    //  1       5   //
    //    \   /     //
    //      4       //
    //        \     //
    //          6   //
    //////////////////

    SUBCASE ("Tests sobre un árbol de 6 elementos")
    {
        nlohmann::json q;
        SUBCASE ("Ancestro de 6 y 3 es 1")
        {
            q = {
                {"id",     id_big},
                {"node_a", 6},
                {"node_b", 3}
            };
            REQUIRE_NOTHROW( result = c->lowestCommonAncestorInterface( q ) );
            CHECK_EQ( result->dump(), "1" );
        }
        SUBCASE ("Ancestro de 1 y 1 es 1 (nodo raíz)")
        {
            q = {
                {"id",     id_big},
                {"node_a", 1},
                {"node_b", 1}
            };
            REQUIRE_NOTHROW( result = c->lowestCommonAncestorInterface( q ) );
            CHECK_EQ( result->dump(), "1" );
        }
        SUBCASE ("Ancestro de 6 y 6 es 6 (hoja)")
        {
            q = {
                {"id",     id_big},
                {"node_a", 6},
                {"node_b", 6}
            };
            REQUIRE_NOTHROW( result = c->lowestCommonAncestorInterface( q ) );
            CHECK_EQ( result->dump(), "6" );
        }
        SUBCASE ("Ancestro de 6 y 2 es 1 (distintos niveles)")
        {
            q = {
                {"id",     id_big},
                {"node_a", 6},
                {"node_b", 2}
            };
            REQUIRE_NOTHROW( result = c->lowestCommonAncestorInterface( q ) );
            CHECK_EQ( result->dump(), "1" );
        }
        SUBCASE ("Ancestro de 2 y 6 es 1 (el orden no afecta el resultado)")
        {
            q = {
                {"id",     id_big},
                {"node_a", 2},
                {"node_b", 6}
            };
            REQUIRE_NOTHROW( result = c->lowestCommonAncestorInterface( q ) );
            CHECK_EQ( result->dump(), "1" );
        }
    }
}

TEST_CASE ("Insertar árboles de strings es posible")
{
    setenv( "RESTFUL_DB", "test/test.db", 1 );
    setenv( "RESTFUL_PORT_NO", "37337", 1 );
    const auto c = std::make_shared< Control >();

    nlohmann::json o = {
        {"node","un nodo"},
        {"left",{
                {"node","otro nodo"}
            }
        }
    };

    int id;
    std::shared_ptr<nlohmann::json> result;

    REQUIRE_NOTHROW( id = c->newTreeInterface( o ) );
    id = c->newTreeInterface( o );

        nlohmann::json q = {
        {"id",     id},
        {"node_a", "un nodo"},
        {"node_b", "otro nodo"}
    };

    REQUIRE_NOTHROW( result = c->lowestCommonAncestorInterface( q ) );
    CHECK_EQ( result->get<std::string>(), "un nodo" );
}

TEST_CASE ("Insertar árboles de objetos es posible")
{
    setenv( "RESTFUL_DB", "test/test.db", 1 );
    setenv( "RESTFUL_PORT_NO", "37337", 1 );
    const auto c = std::make_shared< Control >();

    nlohmann::json o = {
        {"node",{{"name","John"},{"surname","Doe"}}},
        {"left",{
                {"node",{{"name","Fulano"},{"surname","de Tal"}}}
            }
        }
    };

    int id;
    std::shared_ptr<nlohmann::json> result;

    REQUIRE_NOTHROW( id = c->newTreeInterface( o ) );
    id = c->newTreeInterface( o );

        nlohmann::json q = {
        {"id",     id},
        {"node_a", {{"name","John"},{"surname","Doe"}}},
        {"node_b", {{"name","Fulano"},{"surname","de Tal"}}}
    };

    REQUIRE_NOTHROW( result = c->lowestCommonAncestorInterface( q ) );
    CHECK_EQ( result->dump(), R"({"name":"John","surname":"Doe"})" );
}

