# This is a general use makefile for robotics cape projects written in C.
# Just change the target name to match your main source code filename.
TARGET = SailLogger

# use g++-6
CXX = /usr/bin/g++-6
CC = /usr/bin/gcc-6


OPTS := -gdwarf-4 -g3 -Os -Wall
CFLAGS := ${OPTS} -c -std=gnu11
CXXFLAGS := $(OPTS) -Wno-reorder -g -std=gnu++14 -pthread
CPPFLAGS := -D _USE_MATH_DEFINES
CPPFLAGS += -MMD -MT

LDFLAGS += -L /usr/local/lib
LDLIBS += -lm -lrt -lpthread -lgps
LDLIBS += -lroboticscape -lcurl -l boost_system

SOURCES		:= $(wildcard *.cpp)
INCLUDES	:= $(wildcard *.h)
OBJECTS		:= $(SOURCES:$%.cpp=$%.o)

prefix		:= /usr/local
RM			:= rm -f
INSTALL		:= install -m 4755
INSTALLDIR	:= install -d -m 755 

LINK		:= ln -s -f
LINKDIR		:= /etc/roboticscape
LINKNAME	:= link_to_startup_program


# linking Objects
$(TARGET): $(OBJECTS)
	@echo "Linking" $@
	$(CXX) $(CXXFLAGS) -o $@ $(LDFLAGS) $^ $(LDLIBS)

# compiling command
$(OBJECTS): %.o : %.cpp $(INCLUDES)
	@echo "Compiling" $@
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $@ -c -o $@ $<

all:
	$(TARGET)

debug:
	$(MAKE) $(MAKEFILE) DEBUGFLAG="-g -D DEBUG"
	@echo " "
	@echo "$(TARGET) Make Debug Complete"
	@echo " "

install:
	$(MAKE) --no-print-directory
	$(INSTALLDIR) $(DESTDIR)$(prefix)/bin
	$(INSTALL) $(TARGET) $(DESTDIR)$(prefix)/bin
	@echo "$(TARGET) Install Complete"

clean:
	$(RM) $(OBJECTS)
	$(RM) $(TARGET)
	@echo "$(TARGET) Clean Complete"

uninstall:
	$(RM) $(DESTDIR)$(prefix)/bin/$(TARGET)
	@echo "$(TARGET) Uninstall Complete"

runonboot:
	$(MAKE) install --no-print-directory
	$(LINK) $(DESTDIR)$(prefix)/bin/$(TARGET) $(LINKDIR)/$(LINKNAME)
	@echo "$(TARGET) Set to Run on Boot"

