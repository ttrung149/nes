#------------------------------------------------------------------------------
#  NES simulator 
#  Copyright (c) 2020 - Trung Truong
#------------------------------------------------------------------------------
# Compiler Flags
CXX         = g++
CFLAGS      = -g -std=c++11 -pedantic -Wall -Werror -Wextra \
              -Wno-overlength-strings -Wfatal-errors -pedantic
LDFLAGS     = -lSDL2 -lSDL2_ttf
RM          = rm -rf

# Binary name
BIN         = nes

# Object file directory
OBJ_DIR     = obj

# Source file directory
SRC_DIR 	= $(shell pwd)/src
SRC_FILES   = $(wildcard src/*.cpp)
OBJ_FILES   = $(addprefix obj/,$(notdir $(SRC_FILES:.cpp=.o)))
DEPENDS     = $(wildcard obj/*d)

#------------------------------------------------------------------------------
# Make instructions
#------------------------------------------------------------------------------
all: CFLAGS += -O3 -DNDEBUG
all: $(OBJ_DIR) $(BIN)

debug-build: CFLAGS += -DDEBUG
debug-build: $(OBJ_DIR) $(BIN) 

$(OBJ_DIR):
	mkdir -p obj

obj/%.o: src/%.cpp
	$(CXX) $(CFLAGS) -c -MMD -MP -o $@ $<

$(BIN): $(OBJ_FILES)
	$(CXX) $(LDFLAGS) -o $@ $^

# Clean up commands
clean: 
	$(RM) $(OBJ_DIR) core* $(BIN) *.o 

-include $(DEPENDS)
