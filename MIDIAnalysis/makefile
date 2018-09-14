#OBJS specifies which files to compile as part of the project
OBJS = midi_parse.c midianalysis.c 

#CC specifies which compiler we're using
CC = gcc

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
COMPILER_FLAGS = -Wall -std=c99

#DEBUG_STATEMENTS specifies any debug statements we want to add
DEBUG_STATEMENTS = -DDEBUG

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lm

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = midianalysis

#This is the target that compiles our executable
all : $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(DEBUG_STATEMENTS) $(LINKER_FLAGS) -o $(OBJ_NAME)
