DIR=$(shell pwd)
CC=g++
CFLAGS=-Wall -std=c++11
LDFLAGS=-fPIC -shared -lpapi
INCLUDE=-I$(DIR)/include
SOURCES=$(DIR)/src/PAPIProf.cpp
SHARED_LIBRARY=$(DIR)/lib/PAPIProf.so
# OBJECTS=$(SOURCES:.cpp=.o)
# EXECUTABLE=hello

all: $(SHARED_LIBRARY)
    
# $(EXECUTABLE): $(OBJECTS) 
#     $(CC) $(LDFLAGS) $(OBJECTS) -o $@

# .cpp.o:
#     $(CC) $(CFLAGS) $< -o $@

$(SHARED_LIBRARY): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDE) $(LDFLAGS) $< -o $@

clean:
	rm $(DIR)/lib/*.so