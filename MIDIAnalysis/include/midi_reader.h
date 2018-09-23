
#ifndef MIDI_READER_H
#define MIDI_READER_H

// Debug #define commands
#define DEBUG_MIDI_READER_PRINT_BLOCK_DATA
struct MIDIBlock                                                                // The following is a new struct that will define a MIDI block.
{
    unsigned char header[4];                                                    // Four byte header that identifies the type that this block is (for example, 'MTrk' or 'MThd')
    int size;                                                                   // How large the memory allocated block is.
    unsigned char * data;                                                       // Pointer to the raw data (which is of a variable size)
};

/*
    Function prototypes
*/
int test_file_if_midi(FILE * file);
int grab_midi_blocks(FILE * file, struct MIDIBlock ** midiBlocks, int * size);
int freeBlocks(struct MIDIBlock ** midiBlocks, int number_of_blocks);
void process_bytes(unsigned char * byteString, int number_of_bytes);

#endif
