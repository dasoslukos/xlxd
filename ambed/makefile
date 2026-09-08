CXX ?= g++

D2XX_ROOT ?= /usr/local
D2XX_LIBDIR ?= $(D2XX_ROOT)/lib

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
SBINDIR = $(PREFIX)/sbin
SYSCONFDIR = /etc
SYSTEMD_UNITDIR = /etc/systemd/system
DESTDIR =

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
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(SBINDIR)
	install -d $(DESTDIR)$(SYSCONFDIR)/default
	install -d $(DESTDIR)$(SYSTEMD_UNITDIR)

	install -m 0755 $(EXECUTABLE) \
		$(DESTDIR)$(BINDIR)/ambed

	install -m 0755 ../scripts/ambed-usb-unbind \
		$(DESTDIR)$(SBINDIR)/ambed-usb-unbind

	install -m 0644 ../scripts/ambed.service \
		$(DESTDIR)$(SYSTEMD_UNITDIR)/ambed.service

	@if [ ! -e "$(DESTDIR)$(SYSCONFDIR)/default/ambed" ]; then \
		install -m 0644 ../config/ambed \
			$(DESTDIR)$(SYSCONFDIR)/default/ambed; \
	else \
		echo "Preserving existing $(SYSCONFDIR)/default/ambed"; \
	fi
