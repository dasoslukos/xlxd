CXX ?= g++

D2XX_ROOT ?= /usr/local
D2XX_LIBDIR ?= $(D2XX_ROOT)/lib

CPPFLAGS ?=
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic
LDFLAGS ?=
LDLIBS ?= -lftd2xx -pthread

SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
EXECUTABLE := ambed

.PHONY: all clean install

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) -L$(D2XX_LIBDIR) $(OBJECTS) $(LDLIBS) -o $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -pthread -c $< -o $@

clean:
	$(RM) $(EXECUTABLE) $(OBJECTS)

install: $(EXECUTABLE)
	mkdir -p /ambed
	cp $(EXECUTABLE) /ambed/
	cp ./run /ambed/
