DIR=$(shell pwd)
CC=g++
CFLAGS=-Wall -std=c++11
LDFLAGS=-fPIC -shared -lpapi
INCLUDE=-I$(DIR)/include
LIB_SOURCE=$(DIR)/src/PAPIProf.cpp
LIB=$(DIR)/lib/libpapiprof.so
EXAMPLES=$(wildcard $(DIR)/examples/*.cpp)
EXECUTABLES=$(patsubst %.cpp, %, $(EXAMPLES))

all: $(LIB) $(EXECUTABLES)
    
%: %.cpp
	$(CC) $(CFLAGS) $(INCLUDE) -L$(DIR)/lib -Wl,-rpath=$(DIR)/lib -o $@ $^ -lpapiprof

$(LIB): $(LIB_SOURCE)
	$(CC) $(CFLAGS) $(INCLUDE) $(LDFLAGS) $< -o $@

clean:
	rm $(LIB) $(EXECUTABLES)

.PHONY: install
install: $(LIB)
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/include
	cp $(LIB) $(PREFIX)/lib/
	cp $(DIR)/include/*.h $(PREFIX)/include/