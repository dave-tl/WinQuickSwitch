# =============================================================================
# Makefile - Cross-compile WinQuickSwitch on Linux using MinGW-w64
#
# Usage:
#   make              Build WinQuickSwitch.exe
#   make clean        Remove build artifacts
#
# Requirements:
#   apt install g++-mingw-w64-x86-64
# =============================================================================

CXX      = x86_64-w64-mingw32-g++
WINDRES  = x86_64-w64-mingw32-windres

CXXFLAGS = -O2 -Wall -Wextra -DUNICODE -D_UNICODE -DNDEBUG -Isrc -mwindows -municode
LDFLAGS  = -static -mwindows -municode
LIBS     = -lole32 -luuid -lshell32 -luser32 -ladvapi32 -lcomctl32

TARGET   = WinQuickSwitch.exe
SRCDIR   = src
SOURCES  = $(SRCDIR)/main.cpp $(SRCDIR)/virtual_desktop.cpp
RCFILE   = $(SRCDIR)/app.rc
RESOBJ   = $(SRCDIR)/app.res.o
OBJECTS  = $(SOURCES:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(RESOBJ): $(RCFILE)
	$(WINDRES) $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS) $(RESOBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)
	@echo ""
	@echo "Build successful: $(TARGET)"
	@file $(TARGET)

clean:
	rm -f $(OBJECTS) $(RESOBJ) $(TARGET)
