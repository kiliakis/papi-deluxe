DIR=$(shell pwd)
# PAPI_LIB=$(WORK)/install/lib
CC=g++
CFLAGS=-Wall -std=c++11 -Wl,-rpath=$(DIR)/lib
LDFLAGS=-fPIC -shared -L/afs/cern.ch/work/k/kiliakis/install/lib 
#-L$(PAPI_LIB)
INCLUDE=-I$(DIR)/include
LIB_SOURCE=$(DIR)/src/PAPIProf.cpp $(DIR)/src/PAPIProfMultiThread.cpp
LIB=$(DIR)/lib/libpapiprof.so
EXAMPLES=$(wildcard $(DIR)/examples/*.cpp)
EXECUTABLES=$(patsubst $(DIR)/examples/%.cpp, $(DIR)/bin/%.exe, $(EXAMPLES))

all: $(LIB) $(EXECUTABLES)
# $(info $$LD_LIBRARY_PATH is [${LD_LIBRARY_PATH}])
$(DIR)/bin/%.exe: $(DIR)/examples/%.cpp
	# $(CC) $(CFLAGS) $(INCLUDE) -L$(DIR)/lib -o $@ $^ -lpapiprof
	$(CC) $(CFLAGS) $(INCLUDE) -L$(DIR)/lib -o $@ $^ -lpapiprof

$(LIB): $(LIB_SOURCE)
	$(CC) $(CFLAGS) $(INCLUDE) $(LDFLAGS) $^ -o $@ -lpapi

clean:
	rm $(LIB) bin/*.exe

.PHONY: install
install: $(LIB)
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/include
	cp $(LIB) $(PREFIX)/lib/
	cp $(DIR)/include/*.h $(PREFIX)/include/
