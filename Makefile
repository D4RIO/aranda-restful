# COMPILADOR
CC:=g++

# TARGETS VIRTUALES
.PHONY: all

all:restful
restful: restful.cpp
	g++ -o $@ $<
