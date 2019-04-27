
#ifndef MIDI_READER_H
#define MIDI_READER_H

/*	Include headers	*/
#include <stdint.h>

// Debug #define commands
#define DEBUG_MIDI_READER_PRINT_BLOCK_DATA

struct MIDIBlock                                                                // The following is a new struct that will define a MIDI block.
{
	/*	Information fields	*/
    unsigned char 	header[4];		/*!	Header, identifies the type that this block
									is in ASCII (for example, 'MTrk' or 'MThd')	*/
    int 			n_data_size;	/*!	Size of data array, in bytes.	*/
    unsigned char * data;           /*! Pointer to the raw data (which is of a variable size) */

	/*	Status indicators for the program	*/
    uint8_t bActive : 1;      // Boolean representation of whether playing is complete
    int 	nDeltaTicks;     // Number of ticks to count down until next note played.
    int 	nCurrentPos;     // Current position in the bytes (aka, byte offset)
};

/*  Node struct for our simple linked-list implementation.  */
struct MIDIBlockNode
{
	struct Node * nextNode;
	struct MIDIBlock midiBlock;
};

struct MIDIFile
{
	int num_blocks;
	struct MIDIBlock * blockArr;
};











/*
    Function prototypes
*/
struct MIDIBlockNode * alloc_midi_file(FILE * midi_file);
int test_file_if_midi(FILE * file);
int grab_midi_blocks(FILE * file, struct MIDIBlock ** midiBlocks, int * size);
int freeBlocks(struct MIDIBlock ** midiBlocks, int number_of_blocks);
void process_bytes(unsigned char * byteString, int number_of_bytes);
int parse_hex_size(unsigned char * header, int size);
struct MIDIFile convert_ll_to_MIDIFile(struct MIDIBlockNode * list);

#endif
