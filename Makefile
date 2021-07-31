# COMPILADOR
CC:=g++

# TARGETS VIRTUALES
.PHONY: all json.hpp

all:restful
restful: restful.cpp json.hpp
	g++ -o $@ $<

json.hpp:
	[ -e json.hpp ] || wget https://github.com/nlohmann/json/releases/download/v3.9.1/json.hpp
