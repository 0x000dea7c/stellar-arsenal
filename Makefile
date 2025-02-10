CC               := g++
CC_FLAGS_WARN    := -Wall -Wextra -pedantic -Wformat -Wformat-security -Wconversion -Wshadow
CC_FLAGS_DEBUG   := -O0 -ggdb3 -fstack-clash-protection -fcf-protection=full -march=native -DDEBUG -pg -std=c++17
CC_FLAGS_RELEASE := -O3 -g -ffast-math -funroll-loops -flto -march=native -std=c++17
SHARED_FLAGS     := -shared -fPIC -fvisibility=hidden
INCLUDE_DIRS     := $(shell find code/hyper -type d)
INCLUDE_FLAGS    := $(addprefix -I, $(INCLUDE_DIRS))

# TEMPORARY until I figure out how to use make correctly

# these are the sources for hot reloading
GAME_LIB_SOURCES := code/stellar_game_logic.cc \
code/hyper/renderer/hyper_renderer.cc \
code/hyper/core/hyper_math.cc

# all engine and game sources
SOURCES := code/stellar_game_logic.cc \
code/hyper/renderer/hyper_renderer.cc \
code/stellar_hot_reload.cc \
code/hyper/core/hyper_math.cc \
code/stellar_gnulinux.cc

OBJECTS  := $(SOURCES:code/%.c=obj/%.o)
TARGET   := stellar-arsenal
GAME_LIB := libgamelogic.so
LD_FLAGS := -lSDL3 -ldl -lm

$(shell mkdir -p obj)

all: release

release: CC_FLAGS := $(CC_FLAGS_WARN) $(CC_FLAGS_RELEASE) $(INCLUDE_FLAGS)
release: $(TARGET) $(GAME_LIB)

debug: CC_FLAGS := $(CC_FLAGS_WARN) $(CC_FLAGS_DEBUG) $(INCLUDE_FLAGS)
debug: $(TARGET) $(GAME_LIB)

$(TARGET): $(OBJECTS)
	$(CC) $(CC_FLAGS) $(OBJECTS) -o $@ $(LD_FLAGS)

$(GAME_LIB): $(GAME_LIB_SOURCES)
	$(CC) $(CC_FLAGS) $(SHARED_FLAGS) $^ -o $@

obj/%.o: code/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) -c $< -o $@ $(LD_FLAGS)

run:
	LD_LIBRARY_PATH=$${LD_LIBRARY_PATH}:/usr/local/lib:. LSAN_OPTIONS="suppressions=./lsan_suppressions.txt" ./stellar-arsenal

gdb:
	LD_LIBRARY_PATH=$${LD_LIBRARY_PATH}:/usr/local/lib:. LSAN_OPTIONS="suppressions=./lsan_suppressions.txt" gdb ./stellar-arsenal

clean:
	rm -f $(TARGET) $(GAME_LIB)  obj/*.o
	rmdir obj

.PHONY: all clean release debug run gdb
