/*! @file */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "midi_parse.h"
#include "midi_reader.h"
#include "debug.h"

// Global variables
FILE * midi_file_input;                                                         // Declare the midi_file_input variable

/*
    Function: int request_MIDI_blocks(FILE * midi_file_input, struct MIDIBlock [])
    Description:
        Returns an array of MIDIBlocks to the caller, as well as the number of blocks read in successfully.
*/
int request_MIDI_blocks(FILE * midi_file_input, struct MIDIBlock * midi_block_array, int * MIDIBlockArrSize)
{
    return 0;
}

void process_bytes(unsigned char * byte_seq, int num_bytes)
{
    // Alright, now let's go ahead and open a MIDI device...

    int fd = open("/dev/midi2", O_WRONLY, 0);
    if (fd < 0)
    {
        // Couldn't open the MIDI device.
        printf("[WARNING] Couldn't open the MIDI device!");
        //exit(1);
    }

    int byte_cnt = 0;
    int last_delta_time = 0;
    while (byte_cnt < num_bytes)
    {
        // The general format for each event in MIDI is the following:
        // <delta_time><event_bytes>

        // Find the <delta_time>
        int delta_time;
        int delta_time_bytes_read = midi_parse_varSize(&byte_seq[byte_cnt], &delta_time);
        printf("[%d],%d: ", byte_cnt, delta_time);
        if (delta_time_bytes_read)
            byte_cnt += delta_time_bytes_read;

        //printf("MIDI Event:\n\tdelta-time:%d\n", delta_time);
        unsigned char buffer[3]  = {'\0','\0','\0'};
        // Now, find the event itself.
        int event_type_bytes_read = midi_parse_eventType(&byte_seq[byte_cnt], buffer);
        if (event_type_bytes_read)
        {
            byte_cnt += event_type_bytes_read;
        }
        int firstOne = 0;
        printf("\t %3s", buffer);
        int delay = 0;
        for (; delay < last_delta_time; delay++)
        {
            int tst = 0;
            if (firstOne)
                for (; tst < 500000; tst++);
            firstOne = 1;
        }
        write(fd, buffer, 3);
        last_delta_time = delta_time;
    }
}

int test_file_if_midi(FILE * file)
{
    int returnStatus = 0;                                                       // 0 for FALSE, 1 for TRUE.
    long origFilePosition = ftell(file);                                        // Save the original file position.

    char mthd_indicator[4];                                                     // Now, we should read the first four bytes of the file and see if it matches
    int bytes_read = fread(&mthd_indicator[0], 1, 4, file);                     // with the following: MThd.
    if (bytes_read);

    returnStatus = (strncmp("MThd", mthd_indicator, 4) == 0) ? 1 : 0;           // We'll go ahead and compare the first four bytes of the file to what should be
    fseek(file, origFilePosition, SEEK_SET);
    return returnStatus;
}

/*!
    Given an 8-byte char array representing a MIDI block header, calculate the size
	of the given block.

    @param header A character array of size 8. Points to the MIDI block header itself.
        It is always assumed that this array is eight bytes long. Only the last four
        bytes of the array will be read.
    @return An integer representing the number of bytes that the block is, past
        the header.
*/
int parse_midi_block_header(unsigned char * header)
{
	DEBUG("Analyze the header: %02x%02x %02x%02x %02x%02x %02x%02x\n",
			header[0], header[1], header[2], header[3],
			header[4], header[5], header[6], header[7]);
    /*  Counter for the block size  */
    int block_size = 0;

    /*  Iterator for each nibble within the size.   */
    for (int nibble_pos = 0; nibble_pos < 8; nibble_pos++)
    {
        /*	00 00 00 00 FF FF FF
        	Remember... each  ^ is a nibble, or half of a byte.	*/
        unsigned char nibble = (nibble_pos % 2 == 0) ?
            (header[4 + (nibble_pos/2)] >> 4) :		/*	Upper nibble	*/
            (header[4 + (nibble_pos/2)] & 0x0F);	/*	Lower nibble	*/

		/*	Since each position in hex represents the power of 16... convert it to decimal,
			based on it's representation and position within the number string.	*/
		block_size += (nibble << (4 * (7-nibble_pos) ) );
    }

	/*	Return the final, calculated block size.	*/
	DEBUG("Calculated size of the header is: %d\n", block_size);
    return block_size;
}














/*!
    \brief Given a MIDI file, loads all MIDI blocks into memory.

    @param file A pointer to a MIDI file.
    @param midiBlocks A pointer to an array of struct MIDIBlock.
        This will contain the output of the function, in the form of an array.
    @param size A pointer to an integer, representing the size of the outputted
        midiBlocks array.
    @return A success (0) or failure (positive) integer.
*/
int grab_midi_blocks(FILE * file, struct MIDIBlock * midiBlocks[], int * size)
{
    /*  Grab the original/current file position.    */
    long file_originalPosition = ftell(file);

    /*  Determine the size of the file by seeking to the end, and then go to the beginning. */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    /*  The idea is that we're going to create an 'internal' linked list of
        MIDIBlock structs as we go. Then, convert it to an array of MIDIBlocks
        after we seek through the entirety of the file.

        Since these are effectively pointers (notably within the struct MIDIBlock),
        this is a fairy inexpensive operation to do, and it allows us to do some
        more manipulation as we go along.   */

    /*  Node struct for our simple linked-list implementation.  */
    struct Node
    {
        struct Node * nextNode;
        struct MIDIBlock midiBlock;
    };

    // The initial node is where we're going to start reading from when we process the data.
    // The current node is where we're going to write to next. Right now, it's the initial node.
    struct Node * initialNode = malloc(sizeof(struct Node));

    /*  Error-checking to ensure that the allocation was successful.    */
    if (initialNode == NULL)
    {
        ERROR("Couldn't allocate the initial node!\n");
        return -1;
    }
    struct Node * currentNode = initialNode;

    int block_num = 0;
    while (ftell(file) < file_size)
    {
        // We want a new buffer for each and every run.
        unsigned char buffer[8];

        // Wipe the buffer-- remember that C does not guarantee that
        // this memory space will be all zeroes. This is important!
        memset(buffer, 0, sizeof(buffer));

        // Now, we'll go ahead and read in the first eight bytes of this file.
        int bytes_read;
        if ((bytes_read = fread(&buffer[0], 1, 8, file)) < 8)
        {
            /*  If for whatever reason we didn't read all eight bytes, there's a
                chance that we're misaligned with the contents of the file.
                Although we're not reading garbage, it's simply not useful to us
                because the blocks aren't lined up with where we expect blocks. */

            WARN("Attempted to read 8 bytes from file, read %d bytes."
                " Data may be misaligned!\n", bytes_read);
            continue;
        }

        /*  Attempt to set the block size.
            If this fails, there's a good chance this is an invalid block.  */
        currentNode->midiBlock.size = parse_midi_block_header(buffer);

        // Set the char[] header
        strncpy( (char *) (currentNode->midiBlock).header, (char *) buffer, 4);


        // Allocate space for the data to go.
        currentNode->midiBlock.data = malloc(currentNode->midiBlock.size);

        fread((currentNode->midiBlock).data, currentNode->midiBlock.size, 1, file);
        block_num++;

        // Prepare for the next node to be written.
        currentNode->nextNode = malloc(sizeof(struct Node));
        currentNode = currentNode->nextNode;

        // More of a precaution, but for when we jump into the next node, it's
        // nextNode pointer should be null, since we haven't gotten that far.
        (*currentNode).nextNode = NULL;

    }

	//	TODO: Remove debugging code.
	return 0;
    // The purpose of this code was to test whether not I was actually storing the contents of the
    // MIDI file within memory. This appears to have been successful, so I think we're in good shape.

    currentNode = initialNode;

    int block_counter_two = 0;
    while ((*currentNode).nextNode != NULL)
    {
        #ifdef DEBUG_MIDI_READER_PRINT_BLOCK_DATA
        printf("BLOCK %d:\n", block_counter_two);
        printf("\tTYPE: %.4s\n", ((*currentNode).midiBlock).header);
        printf("\tSIZE: %d\n", ((*currentNode).midiBlock).size);
        printf("\tDATA: \n\t");
        int data_counter = 0;
        for (; data_counter < ((*currentNode).midiBlock).size; data_counter++)
        {
            printf("%02X %s", (unsigned char) (((*currentNode).midiBlock).data)[data_counter],
                    data_counter % 16 == 15 ? "\n\t" : "");
        }

        printf("\n");
        #endif
        block_counter_two++;
        currentNode = (*currentNode).nextNode;
    }


    // Now, we need to convert the linked list of MIDIBlocks into an array.
    // Reset the currentNode to the original initialNode
    currentNode = initialNode;

    *midiBlocks = (struct MIDIBlock *) malloc((sizeof(struct MIDIBlock)) * block_num);
    int block_counter_arr = 0;
    while ((*currentNode).nextNode != NULL)
    {
        (*midiBlocks)[block_counter_arr] = (*currentNode).midiBlock;

        currentNode = (*currentNode).nextNode;
        free(initialNode);
        initialNode = currentNode;
        block_counter_arr++;
    }
    *size = block_counter_arr;

    // There's an additional free() here: this is because before, we allocated
    // the memory for the next block, in preparation that there would be more
    // to store. Since this is the final (empty) node, it can simply be free'd.
    free(initialNode);

    fseek(file, file_originalPosition, SEEK_SET);                           // Return to the file's original position
    return 0;
}

int freeBlocks(struct MIDIBlock ** midiBlocks, int number_of_blocks)
{
    int counter = 0;
    for (; counter < number_of_blocks; counter++)
    {
        free(((*midiBlocks)[counter]).data);
    }
    free(*midiBlocks);

    return 0;
}
