# COMPILADOR
CC:=g++

# FLAGS DE ENLAZADO
LINK_FLAGS:=-lrestbed -lsqlite3 -ldl

# FLAGS DEL COMPILADOR
# Básicamente se usa C++11 (pedantic), con todas las advertencias de compilación normales y extra
# (hay muchas otras que no se activaron). También se advierte por variables inicializadas pero sin uso
# (para mantener el código limpio), igual que con resultados sin usar. Se activa stack-protector-all,
# porque stack-protector deja algunas funciones dentro de json.hpp sin protección. Se optimiza todo
# lo posible para rendimiento. Tener en cuenta que para debugging se debe quitar -O3 y reemplazar por -Og.
CCFLAGS:=-Wall \
    -Wextra \
    -Wunused-but-set-variable \
    -Wunused-result \
    -std=c++17\
    -Wpedantic \
    -fstack-protector-all \
    -Wstack-protector \
    -O3

# CLEAN
CLEAN_TARGETS:=\
	restful \
	json.hpp \
	*.o *~ \
	test/*~ \
	test/doctest.h \
	test/test \
	test/test.db \
	doc/ \
	lib*.so

# TARGETS VIRTUALES
.PHONY: all doc clean test

all:restful libcrear-arbol.so libancestro-comun.so
restful: restful.o main.o plugin.o
	$(CC) $(CCFLAGS) -o $@ $^ $(LINK_FLAGS)
libcrear-arbol.so: crear-arbol.o plugin.o restful.o
	$(CC) $(CCFLAGS) -rdynamic -o $@ $^ -shared $(LINK_FLAGS)
libancestro-comun.so: ancestro-comun.o plugin.o restful.o
	$(CC) $(CCFLAGS) -rdynamic -o $@ $^ -shared $(LINK_FLAGS)
main.o: main.cpp restful.hpp
	$(CC) $(CCFLAGS) -c $<
plugin.o: plugin.cpp plugin.hpp restful.hpp
	$(CC) $(CCFLAGS) -c $< -fPIC
restful.o: restful.cpp json.hpp restful.hpp
	$(CC) $(CCFLAGS) -c $< -fPIC
crear-arbol.o: crear-arbol.cpp restful.hpp
	$(CC) $(CCFLAGS) -c $< -fPIC
ancestro-comun.o: ancestro-comun.cpp restful.hpp
	$(CC) $(CCFLAGS) -c $< -fPIC
json.hpp:
	[ -e $@ ] || wget --quiet --show-progress https://github.com/nlohmann/json/releases/download/v3.9.1/json.hpp
doc:
	doxygen doxygen.config
clean:
	-rm -rf $(CLEAN_TARGETS)
test/test: test/test.cpp test/doctest.h json.hpp restful.o plugin.o
	$(CC) $(CCFLAGS) -o $@ $^ $(LINK_FLAGS)
test: test/test
	-rm test/test.db
	$< -s
	@echo "La base de datos test/test.db se borra con 'make clean' o antes de comenzar con 'make test'."
	@echo "Puede examinarla con 'sqlite3 test/test.db'."
valgrind-test: test/test
	-rm test/test.db
	valgrind --leak-check=full -s $< -s
	@echo "La base de datos test/test.db se borra con 'make clean' o antes de comenzar con 'make test'."
	@echo "Puede examinarla con 'sqlite3 test/test.db'."
test/doctest.h:
	[ -e $@ ] || wget -O $@ --quiet --show-progress https://raw.githubusercontent.com/onqtam/doctest/master/doctest/doctest.h
