# Executable name
EXE = app

# C compiler
CC = gcc
# C++ compiler
CXX = g++
# Linker
LD = g++

# C flags
CFLAGS = 
# C++ flags
CXXFLAGS = 
# C/C++ flags
CPPFLAGS = -Wall -g

# SDL3 static linking
SDL3_CFLAGS = -I$(PWD)/src/SDL3/include
SDL3_LIBS = $(PWD)/src/SDL3/lib/libSDL3.a -lm -ldl -lpthread
CPPFLAGS += $(SDL3_CFLAGS)
LDLIBS += $(SDL3_LIBS) -lX11 -lXext -lXcursor -lXrandr -lXinerama -lXi


# Dependency generation flags
DEPFLAGS = -MMD -MP
# Linker flags
LDFLAGS = 

# Build directories
BIN = bin
OBJ = obj
SRC = src

# Find all source files recursively
SOURCES := $(shell find $(SRC) -type f \( -name "*.c" -o -name "*.cc" -o -name "*.cpp" -o -name "*.cxx" \))

# Generate object file paths in obj/ with matching directory structure
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(filter %.c, $(SOURCES))) \
           $(patsubst $(SRC)/%.cc, $(OBJ)/%.o, $(filter %.cc, $(SOURCES))) \
           $(patsubst $(SRC)/%.cpp, $(OBJ)/%.o, $(filter %.cpp, $(SOURCES))) \
           $(patsubst $(SRC)/%.cxx, $(OBJ)/%.o, $(filter %.cxx, $(SOURCES)))

# Include compiler-generated dependency rules
DEPENDS := $(OBJECTS:.o=.d)

# Compile C source
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@
# Compile C++ source
COMPILE.cxx = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) -c -o $@
# Link objects
LINK.o = $(LD) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(BIN)/$(EXE)

.DEFAULT_GOAL = all

.PHONY: all
all: $(BIN)/$(EXE)

$(BIN)/$(EXE): $(SRC) $(OBJ) $(BIN) $(OBJECTS)
	$(LINK.o)

$(SRC):
	mkdir -p $(SRC)

$(OBJ):
	mkdir -p $(OBJ)

$(BIN):
	mkdir -p $(BIN)

# Ensure obj directory structure mirrors src
$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(COMPILE.c) $<

$(OBJ)/%.o: $(SRC)/%.cc
	@mkdir -p $(dir $@)
	$(COMPILE.cxx) $<

$(OBJ)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(COMPILE.cxx) $<

$(OBJ)/%.o: $(SRC)/%.cxx
	@mkdir -p $(dir $@)
	$(COMPILE.cxx) $<

# Force rebuild
.PHONY: remake
remake: clean $(BIN)/$(EXE)

# Execute the program
.PHONY: run
run: $(BIN)/$(EXE)
	./$(BIN)/$(EXE)

# Remove previous build and objects
.PHONY: clean
clean:
	$(RM) $(OBJECTS)
	$(RM) $(DEPENDS)
	$(RM) $(BIN)/$(EXE)

# Remove everything except source
.PHONY: reset
reset:
	$(RM) -r $(OBJ)
	$(RM) -r $(BIN)

-include $(DEPENDS)
