/*
    Name: Constantino Flouras
    Date: September 22nd, 2018
    Project: MIDIAnalysis
    File: main.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "midi_reader.h"
#include "midi_parse.h"


/*
    Function prototypes
*/
int initialize_file(int argc, char * argv[], FILE ** midi_file);

int main(int argc, char * argv[])
{
    // Let's go ahead and open the MIDI file.
    int fd_midi_dev = open("/dev/midi3", O_WRONLY, 0);
    if (fd_midi_dev < 0)
    {
        // Couldn't open the MIDI device.
        printf("[WARNING] Couldn't open the MIDI device!");
        exit(1);
    }
    /*
    printf("[NOTICE] MIDI device is open!\n");
    char bytes[] = {0x90, 0x35, 0x39};
    write(fd_midi_dev, bytes, 4);
    */



    // Create a MIDI file.
    FILE * midi_file;

    // Attempt to initialize the MIDI file, whether or not that's from the
    // command line argument, or a name specified at runtime.
    if (initialize_file(argc, argv, &midi_file))
        return -1;
    printf("[main()] File has been initialized, %p\n", &midi_file);

    // Create an array of MIDIBlocks, as defined by the MIDI reader.
    struct MIDIBlock * midi_block_array;
    int midi_block_array_size = 0;
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    if (grab_midi_blocks(midi_file, &midi_block_array, &midi_block_array_size))
        return -1;
    printf("[main()] Grabbed the MIDI blocks.\n");

    // Each block has its own instructions that need to run simultaneously.
    // To emulate how this works, we'll simply need to switch between the
    // blocks simultaneously.
    struct MIDIBlockStatus
    {
        unsigned char * data;
        int isPlaying;      // Boolean representation of whether playing is complete
        int deltaTicks;     // Number of ticks to count down until next note played.
        int currentPos;     // Current position in the bytes (aka, byte offset)
    };

    struct MIDIBlockStatus mBlockStatus[midi_block_array_size];

    int midi_block_status_cnt = 0;
    for (; midi_block_status_cnt < midi_block_array_size; midi_block_status_cnt++)
    {
        // mBlockStatus[midi_block_status_cnt].block = midi_block_array[midi_block_status_cnt];
        mBlockStatus[midi_block_status_cnt].isPlaying = 1;
        mBlockStatus[midi_block_status_cnt].data =
            (midi_block_array[midi_block_status_cnt]).data;
        mBlockStatus[midi_block_status_cnt].deltaTicks = 0;
        mBlockStatus[midi_block_status_cnt].currentPos = 0;

        // Update the next delta time of the MIDI block
        int deltaDelta = 0;
        int deltaDeltaBytes = midi_parse_varSize(&(mBlockStatus[midi_block_status_cnt].data[mBlockStatus[midi_block_status_cnt].currentPos]), &deltaDelta);

        if (deltaDeltaBytes)
        {
            mBlockStatus[midi_block_status_cnt].currentPos += deltaDeltaBytes;
            mBlockStatus[midi_block_status_cnt].deltaTicks += deltaDelta;
        }

        int cntr = 0;
        printf("MIDI BLOCK %d:\n\n", midi_block_status_cnt);
        for (; cntr < midi_block_array[midi_block_status_cnt].size; cntr++)
        {
            printf("%02x %s", mBlockStatus[midi_block_status_cnt].data[cntr], ((cntr % 10) == 9) ? "\n" : "");
        }
    }

    while(/*1*/ mBlockStatus[2].isPlaying == 1 )
    {
        // Start the timer
        clock_t startTime = clock();

        midi_block_status_cnt = 1;
        for (; midi_block_status_cnt < midi_block_array_size; midi_block_status_cnt++)
        {
            if (mBlockStatus[midi_block_status_cnt].isPlaying)
            {
                while(mBlockStatus[midi_block_status_cnt].deltaTicks <= 0)
                {
                    #define EVENT_BUFFER_SIZE 32
                    // Create a buffer for the MIDI command to go into.
                    unsigned char buffer[EVENT_BUFFER_SIZE];
                    int bufferWipe = 0;
                    for (; bufferWipe < EVENT_BUFFER_SIZE; bufferWipe++)
                        buffer[bufferWipe] = 0x00;
                    int bytes_read = midi_parse_getEvent(&buffer[0], EVENT_BUFFER_SIZE, &(mBlockStatus[midi_block_status_cnt].data[mBlockStatus[midi_block_status_cnt].currentPos]));
                    mBlockStatus[midi_block_status_cnt].currentPos += bytes_read;
                    if (mBlockStatus[midi_block_status_cnt].currentPos >= midi_block_array[midi_block_status_cnt].size)
                    {
                        mBlockStatus[midi_block_status_cnt].isPlaying = 0;
                        break;  // Reached the end of the block
                    }
                    printf("MIDI EVENT: ");
                    int cntr=0;
                    for (; cntr < bytes_read; cntr++)
                    {
                        printf("%02x ", buffer[cntr]);
                    }
                    printf("\n");

                    write(fd_midi_dev, &buffer[0], bytes_read);

                    // Update the next delta time of the MIDI block
                    int deltaDelta = 0;
                    int deltaDeltaBytes = midi_parse_varSize(&(mBlockStatus[midi_block_status_cnt].data[mBlockStatus[midi_block_status_cnt].currentPos]), &deltaDelta);

                    if (deltaDeltaBytes)
                    {
                        mBlockStatus[midi_block_status_cnt].currentPos += deltaDeltaBytes;
                        mBlockStatus[midi_block_status_cnt].deltaTicks += deltaDelta;
                    }
                    else
                    {
                        printf("ERROR--Read in zero bytes for variable length!");
                        break;
                    }
                }
                if (mBlockStatus[midi_block_status_cnt].deltaTicks % 100 == 0)
                {
                    printf("Counting down ticks... currently at: %d...\n", mBlockStatus[midi_block_status_cnt].deltaTicks);
                }

                // Make sure this only runs ONCE per block. We only want one tick to run per loop!
                mBlockStatus[midi_block_status_cnt].deltaTicks--;
            }
        }

        while ( (double) (clock() - startTime) < 500);
    }












    // Free all allocated blocks from memory.
    freeBlocks(&midi_block_array, midi_block_array_size);
    printf("[main()] Free'd the MIDI blocks from memory.\n");
    fclose(midi_file);
    return 0;

}


















/*
    Function: FILE * initialize_file(int * argc, char * argv[])
    Description:
        Takes in the command line arguments that were provided to the main method,
        and either parses the MIDI file that was given by the user through arguments,
        or asks the user to input the filename of a MIDI file. Returns a FILE *.
*/
int initialize_file(int argc, char * argv[], FILE ** midi_file)
{
    #define FILENAME_MAX_SIZE 256
    char filename[FILENAME_MAX_SIZE];                                           // We're going to create the filename variable
    int filename_wiper = 0;                                                     // Then, we'll go ahead and wipe it so that it is zeroed out
    for (;filename_wiper < sizeof(filename); filename_wiper++)
        filename[filename_wiper] = '\0';

    if (argc > 1)                                                              // If an argument was specified, we'll go ahead and input it automagically into the
    {                                                                           // variable that we have created. (Copied from the first argument,)
        strncpy(filename, argv[1], sizeof(filename));

        #ifdef DEBUG_FILE_INPUT
            printf("[DEBUG: initialize_file()] %s%s\n",
                "A filename has been inputted on the command line:",
                filename);
        #endif
    }


    while (strlen(filename) == 0 || filename[0] == 10)                          // If the user didn't enter a filename, or simply pressed the enter key...
    {
        if (filename[0] == 10)                                                  // Friendly little reminder on how to force-close a program
            printf("(Psssssst.......... press [CTRL] + [C] to exit.)\n");

        for (filename_wiper = 0; filename_wiper < 256; filename_wiper++)        // Wipe the filename so that we don't have to worry about anything corrupt.
            filename[filename_wiper] = '\0';                                    // Wipe all characters with the NULL character, also known as '\0' or 0

        if (argc <= 1)                                                         // If the argc counter is 1 or less, then no arguments were specified.
        {
            printf("Please enter the MIDI filename, including extension: ");    // Prompt the user to enter a filename.
            fgets(filename, sizeof(filename), stdin);                           // If fgets() returns 0 or NULL, there was an error. Retry-- we have to input something.

            int newline_char_del = 0;                                           // This code removes the new line character.
            while (newline_char_del < sizeof(filename))                         // It simply iterates through until it finds it, or until it reaches the end of the buffer.
            {
                if (filename[newline_char_del] == '\n')                         // If we happen to find a new line character, delete it, and then break out of the loop.
                {
                    filename[newline_char_del] = '\0';
                    #ifdef DEBUG_FILE_INPUT
                        printf("[DEBUG: initialize_file()] %s%d\n",
                            "A new line character was removed at location: ",
                            newline_char_del);
                    #endif
                    break;
                }
                newline_char_del++;
            }
        }
    }
    #ifdef DEBUG_FILE_INPUT
    printf("[DEBUG: initialize_file()] %s%s\n",
        "A filename has been inputted by the user: ",
        filename);
    #endif

    (*midi_file) = fopen(filename, "rb");                                  // Finally.... once we know the filename, we can open it in read/binary mode.

    if (!(*midi_file))
    {
        return -1;
    }
    return 0;                                                          // And then we'll go ahead and return the file that we made.
}
