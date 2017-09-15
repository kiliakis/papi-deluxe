CC=g++
CFLAGS=-Wall -std=c++11
LDFLAGS=-fPIC -shared -lpapi
INCLUDE=-I/afs/cern.ch/work/k/kiliakis/install/include
SOURCES=PAPIProf.cpp
SHARED_LIBRARY=PAPIProf.so
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
	rm *.so *.o