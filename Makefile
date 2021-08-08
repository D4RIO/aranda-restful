# COMPILADOR
CC:=g++

# FLAGS DE ENLAZADO
LINK_FLAGS:=-l restbed -l sqlite3

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
    -std=c++11 \
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
	doc/

# TARGETS VIRTUALES
.PHONY: all doc clean test

all:restful
restful: restful.o main.o
	$(CC) $(CCFLAGS) -o $@ $^ $(LINK_FLAGS)
main.o: main.cpp restful.hpp
	$(CC) $(CCFLAGS) -c $<
restful.o: restful.cpp json.hpp restful.hpp
	$(CC) $(CCFLAGS) -c $<
json.hpp:
	[ -e $@ ] || wget --quiet --show-progress https://github.com/nlohmann/json/releases/download/v3.9.1/json.hpp
doc:
	doxygen doxygen.config
clean:
	-rm -rf $(CLEAN_TARGETS)
test: test/test.cpp test/doctest.h json.hpp restful.o
	$(CC) $(CCFLAGS) -o test/$@ $^ $(LINK_FLAGS)
	-rm test/test.db
	test/$@ -s
	@echo "La base de datos test/test.db se borra con 'make clean' o antes de comenzar con 'make test'."
	@echo "Puede examinarla con 'sqlite3 test/test.db'."
test/doctest.h:
	[ -e $@ ] || wget -O $@ --quiet --show-progress https://raw.githubusercontent.com/onqtam/doctest/master/doctest/doctest.h
