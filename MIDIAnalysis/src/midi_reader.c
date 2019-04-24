/*
    Name: Constantino Flouras
    Date: July 26th, 2018
*/
// Constantino Flouras
// July 26th, 2018
// MIDI Reader

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


        /*
        // <delta-time><event>
        // Find the delta-time for this event.
        unsigned int delta_length = 0;
        while ( (byte_seq[byte_cnt] & (1<<7)) != 0)
        {
            printf("analyze: %d\n", byte_seq[byte_cnt]);
            printf("binary: %d\n", byte_seq[byte_cnt] & (1<<7));
            delta_length = delta_length + ((byte_seq[byte_cnt]) << 8);
            printf("delta_length(byte %d): %d", byte_cnt, delta_length);
            byte_cnt++;
        }

        printf("delta_length: %d", delta_length);
        printf("sizeof(int")
        break;
        */












        /*
        switch(byte_seq[byte_cnt])
        {
            case 0x00:
                // No operation, skip byte
                break;
            case 0xFF:
                // Meta-events
                byte_cnt++;
                switch(byte_seq[byte_cnt]<<8 | byte_seq[byte_cnt+1])
                {
                    case 0x5405:
                        //valid?
                        printf("Found the SMPTE case!");
                        byte_cnt = byte_cnt + 5;
                        break;
                    default:
                        break;
                };
                break;  // end meta-event;
            case 0xFF:
            default:
                break;
                //Unimplemented sequence
        };
        */
    }


    /*
    int i = 0;
    for (; i < number_of_bytes; i++)
    {
        printf("%02x ", (unsigned char) byteString[i]);
    }

    i = 0;
    for (; i < number_of_bytes; i++)
    {
        if (byteString[i] == 0xFF)
            printf("%d-%02x\n", i, (unsigned char) byteString[i]);
    }
    */
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

int grab_midi_blocks(FILE * file, struct MIDIBlock ** midiBlocks, int * size)
{
    // For my programs, it is standard practice to restore the original file
    // position, unless it is explicit what the function is doing.
    long file_originalPosition = ftell(file);

    // Before we begin, we need to grab the beginning and end of the file.
    // This will tell us when we've reached the end of the file later on.
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // So, the idea is that we're going to create an 'internal' linked list of
    // MIDIBlock structs as we go, and then convert it to an array of MIDIBlocks
    // after we seek through the entire file.
    struct Node
    {
        struct Node * nextNode;
        struct MIDIBlock midiBlock;
    };

    // The initial node is where we're going to start reading from when we process the data.
    // The current node is where we're going to write to next. Right now, it's the initial node.
    struct Node * initialNode = malloc(sizeof(struct Node));
    struct Node * currentNode = initialNode;

    int block_num = 0;
    while (ftell(file) < file_size)
    {
        // We want a new buffer for each and every run.
        unsigned char buffer[8];

        // Wipe the buffer-- remember that C does not guarantee that
        // this memory space will be all zeroes. This is important!
        int buffer_pos = 0;
        for (; buffer_pos < 8; buffer_pos++)
            buffer[buffer_pos]='\0';

        // Now, we'll go ahead and read in the first eight bytes of this file.
        int bytes_read = fread(&buffer[0], 1, 8, file);
        if (bytes_read < 8)
        {
            // If for whatever reason we didn't read all eight bytes, there's a
            // chance that we're misaligned with the contents of the file.
            // Although we're not reading garbage, it's simply not useful to us
            // because the blocks aren't lined up with where we expect blocks.
            printf("[WARNING: grabBlocks()] %s%d%s\n",
                "Attempted to read 8 bytes from file, read ",
                bytes_read,
                " bytes. There is a possiblity for data misalignment!");
            continue;
        }

        // Read in the size of the block
        int block_size = 0;
        int offset = 4;
        int nibble_pos = 0;
        for (; nibble_pos < 8; nibble_pos++)
        {
            unsigned char nibble = buffer[offset+(nibble_pos/2)];
            nibble = (nibble_pos % 2 == 0) ? nibble >> 4 : nibble & 0x0F;
            int representation = -1;
            switch(nibble)
            {
                case 'A': representation = 10; break;
                case 'B': representation = 11; break;
                case 'C': representation = 12; break;
                case 'D': representation = 13; break;
                case 'E': representation = 14; break;
                case 'F': representation = 15; break;
                default: representation = nibble; break;
            }

            // This algorithm converts the hexadecimal numbers to binary!
            block_size += representation * pow(16, 8 - 1 - nibble_pos);
        }

        // Instead of fseek(ing), we're actually going to fread directly into
        // the block of memory that will contain this node.


        // Set the char[] header
        strncpy( (char *) ((*currentNode).midiBlock).header, (char *) buffer, 4);

        // Set the block size
        (*currentNode).midiBlock.size = block_size;

        // Allocate space for the data to go.
        (*currentNode).midiBlock.data = malloc(block_size);

        fread(((*currentNode).midiBlock).data, block_size, 1, file);
        block_num++;

        // Prepare for the next node to be written.
        (*currentNode).nextNode = malloc(sizeof(struct Node));
        currentNode = (*currentNode).nextNode;

        // More of a precaution, but for when we jump into the next node, it's
        // nextNode pointer should be null, since we haven't gotten that far.
        (*currentNode).nextNode = NULL;

    }


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
