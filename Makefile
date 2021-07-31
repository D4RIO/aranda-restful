# COMPILADOR
CC:=g++

# TARGETS VIRTUALES
.PHONY: all json.hpp

all:restful
restful: restful.o
	$(CC) -o $@ $^
restful.o: restful.cpp json.hpp
	$(CC) -c $<

json.hpp:
	[ -e json.hpp ] || wget --quiet --show-progress https://github.com/nlohmann/json/releases/download/v3.9.1/json.hpp

clean:
	-rm restful json.hpp *.o *~
