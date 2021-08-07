# COMPILADOR
CC:=g++

# FLAGS DE ENLAZADO
LINK_FLAGS:=-l restbed -l sqlite3
CCFLAGS:=-Wall

# TARGETS VIRTUALES
.PHONY: all json.hpp

all:restful
restful: restful.o main.o
	$(CC) $(CCFLAGS) -o $@ $^ $(LINK_FLAGS)
main.o: main.cpp restful.hpp
	$(CC) $(CCFLAGS) -c $<
restful.o: restful.cpp json.hpp restful.hpp
	$(CC) $(CCFLAGS) -c $<

json.hpp:
	[ -e json.hpp ] || wget --quiet --show-progress https://github.com/nlohmann/json/releases/download/v3.9.1/json.hpp

clean:
	-rm restful json.hpp *.o *~
