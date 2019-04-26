EXE = midianalysis

SRC_DIR = src
OBJ_DIR = obj

#	Doxygen
export DOXYGEN_QUIET = YES

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CPPFLAGS += -Iinclude
CFLAGS += -g -Wall -std=gnu99
LDFLAGS += -Llib
LDLIBS += -lm

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ)
	doxygen doxygenConfig
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ)
