# Compiler & flags
CXX      = g++
CXXFLAGS = -Wall -std=c++17
LDFLAGS  = -pthread
INCLUDES = -Iinclude

# Directories
SRCDIR   = src
TESTDIR  = tests
OBJDIR   = obj
BINDIR   = bin

# Files we want to build
MAIN1    = $(SRCDIR)/data_readout.cpp
MAIN2    = $(SRCDIR)/receive_packet.cpp
MAIN3 	 = $(SRCDIR)/compact_data.cpp

# Object files (one per main)
OBJS1    = $(OBJDIR)/data_readout.o
OBJS2    = $(OBJDIR)/receive_packet.o
OBJS3 	 = $(OBJDIR)/compact_data.o

# Final executables
TARGET1  = $(BINDIR)/data_readout
TARGET2  = $(BINDIR)/receive_packet
TARGET3  = $(BINDIR)/compact_data

.PHONY: all clean

# By default, build both executables
all: $(TARGET1) $(TARGET2) $(TARGET3)

# 1. Build data_readout
$(TARGET1): $(OBJS1)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^

# 2. Build receive_packet
$(TARGET2): $(OBJS2)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^

# 3. Build compact_data
$(TARGET3): $(OBJS3)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^

# Compile rules for each .cpp
$(OBJS1): $(MAIN1)
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJS2): $(MAIN2)
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJS3): $(MAIN3)
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean up
clean:
	rm -rf $(OBJDIR) $(BINDIR)