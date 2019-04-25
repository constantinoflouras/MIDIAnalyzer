/*! @file */

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
#include "debug.h"

// Structs
struct MIDIBlockStatus
{
    unsigned char * data;
    int dataSize;       // Size of the data.
    int isPlaying;      // Boolean representation of whether playing is complete
    int deltaTicks;     // Number of ticks to count down until next note played.
    int currentPos;     // Current position in the bytes (aka, byte offset)
};

/*  Function prototypes     */
int initialize_file(int argc, char * argv[], FILE ** midi_file);
int MIDIBlockStatus_isPlaying(struct MIDIBlockStatus * midi_block_status, int count);
int MIDIBlockStatus_updateDeltaTime(struct MIDIBlockStatus * midi_block_status);

/*!
    Handles all arguments passed into the program. Success means that the
    program would be ready to execute.

    @param argc int representing number of arguments
    @param argv pointer to the argument array
    @param file pointer to a file descriptor
    @param dev  pointer to a device descriptor
    @return An int representing success (0) or failure (positive)
 */
int process_args(int argc, char * argv[], FILE ** file, int * dev)
{
    /*  Internal counter for us to keep track of
        which argument we're processing.    */
    int cntr = 0;

    /*  Every single argument that is passed will be
        read and considered-- but if we run out out of
        arguments, then we'll have an issue.    */
    while (cntr < argc)
    {
        if (!strncmp("--mididev=", argv[cntr], 10))
        {
            /*  This is the MIDI device that we'd like to output to.   */
            DEBUG("Opening the following device: %s\n", &(argv[cntr][10]));
            *dev = open(&(argv[cntr][10]), O_WRONLY, 0);
            DEBUG("[DEBUG: %s] The resulting FD number was: %d\n", *dev);
        }
        else if (cntr == (argc-1))
        {
            /*  In an ideal situation, this would be the file itself.   */
            DEBUG("Opening the following MIDI file: %s\n", argv[cntr]);
            *file = fopen(argv[cntr], "rb");
            DEBUG("Opening the file was %s.\n", (*file == NULL) ? "unsuccessful--an error occurred" : "successful");
        }
        cntr++;
    }
    return 0;
}










/*!
   \brief Main entry point for the application.
*/
int main(int argc, char * argv[])
{

    // Let's go ahead and open the MIDI file.int * dev
    int fd_midi_dev;
    // Create a MIDI file.
    FILE * midi_file;
    process_args(argc, argv, &midi_file, &fd_midi_dev);


    if (fd_midi_dev < 0)
    {
        // Couldn't open the MIDI device.
        printf("[WARNING] Couldn't open the MIDI device!");
        exit(1);
    }

    exit(1);
    /*
    printf("[NOTICE] MIDI device is open!\n");
    char bytes[] = {0x90, 0x35, 0x39};
    write(fd_midi_dev, bytes, 4);
    */





    // Attempt to initialize the MIDI file, whether or not that's from the
    // command line argument, or a name specified at runtime.
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
    struct MIDIBlockStatus mBlockStatus[midi_block_array_size];

    // The following for loop will initialize the MIDIBlockStatus struct array
    // with the proper initial values. The main program loop works in conjunction
    // with the MIDIBlock array.
    int midi_block_status_cnt = 0;
    for (; midi_block_status_cnt < midi_block_array_size; midi_block_status_cnt++)
    {
        mBlockStatus[midi_block_status_cnt].data =
            (midi_block_array[midi_block_status_cnt]).data; // Pointer to the data
        mBlockStatus[midi_block_status_cnt].dataSize =
            (midi_block_array[midi_block_status_cnt]).size; // Size of the data chunk
        mBlockStatus[midi_block_status_cnt].isPlaying = 1;  // Marks whether we've reached the end of the block
        mBlockStatus[midi_block_status_cnt].deltaTicks = 0; // How many ticks are required until we can continue playing notes
        mBlockStatus[midi_block_status_cnt].currentPos = 0; // The current position (in bytes) for our memory location. Aka, an offset.

        int deltaBytesRead = MIDIBlockStatus_updateDeltaTime(&mBlockStatus[midi_block_status_cnt]);
        if (!deltaBytesRead)
        {
            // Something has gone wrong-- we shouldn't have read in zero bytes.
            // Maybe we hit the end of the file?
            // Disable this channel, and pass along a warning.
            mBlockStatus[midi_block_status_cnt].isPlaying = 0;
            printf("[WARNING] Block #%d appears to be corrupt. This channel will no longer play from this point on.\n", midi_block_status_cnt);
        }
        else
        {
            mBlockStatus[midi_block_status_cnt].currentPos += deltaBytesRead;
        }
        /*
        // Grab the initial delta-time of the next block.
        int deltaDelta = 0;
        int deltaDeltaBytes = midi_parse_varSize(&(mBlockStatus[midi_block_status_cnt].data[mBlockStatus[midi_block_status_cnt].currentPos]), &deltaDelta);

        if (deltaDeltaBytes)
        {
            mBlockStatus[midi_block_status_cnt].currentPos += deltaDeltaBytes;
            mBlockStatus[midi_block_status_cnt].deltaTicks += deltaDelta;
        }
        else
        {
            // For whatever reason, the size itself was zero bytes-- that means something
            // has gone horribly wrong. We should break, and disable this channel
            // in particular for the time being.
            mBlockStatus[midi_block_status_cnt].isPlaying = 0;
            printf("[WARNING] Block #%d appears to be corrupt. This channel will no longer play from this point on.");
        }
        */
        int cntr = 0;
        printf("MIDI BLOCK %d:\n\n", midi_block_status_cnt);
        for (; cntr < midi_block_array[midi_block_status_cnt].size; cntr++)
        {
            printf("%02x %s", mBlockStatus[midi_block_status_cnt].data[cntr], ((cntr % 10) == 9) ? "\n" : "");
        }
    }

    int totalNumberOfTicks = 0;
    while(MIDIBlockStatus_isPlaying(mBlockStatus, midi_block_status_cnt))
    {
        // Initialize the timer.
        // This is so the events are appropriately spaced out.
        clock_t startTime = clock();

        // tick header
        int tickHeaderShown = 0;
        midi_block_status_cnt = 1;
        for (; midi_block_status_cnt < midi_block_array_size; midi_block_status_cnt++)
        {
            if (mBlockStatus[midi_block_status_cnt].isPlaying)
            {
                while(mBlockStatus[midi_block_status_cnt].deltaTicks <= 0 && mBlockStatus[midi_block_status_cnt].isPlaying)
                {
                    if (!tickHeaderShown)
                    {
                        tickHeaderShown = 1;
                        printf("----TICK #%d----\n", totalNumberOfTicks);
                        printf("isPlaying: ");
                        int cntr_temp = 0;
                        for (; cntr_temp < midi_block_array_size; cntr_temp++)
                        {
                            if (mBlockStatus[cntr_temp].isPlaying)
                            {
                                printf("[%d]", cntr_temp);
                            }
                        }
                        printf("\n");
                    }
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

                    printf("[EVENT--Block #%d] Offset: %d; Data: ", midi_block_status_cnt, mBlockStatus[midi_block_status_cnt].currentPos);
                    int cntr=0;
                    for (; cntr < bytes_read; cntr++)
                    {
                        printf("%02x ", buffer[cntr]);
                    }
                    printf("\n");


                    write(fd_midi_dev, &buffer[0], bytes_read);

                    // Update the next delta time of the MIDI block
                    int deltaBytesRead = MIDIBlockStatus_updateDeltaTime(&mBlockStatus[midi_block_status_cnt]);
                    if (!deltaBytesRead)
                    {
                        // Something has gone wrong-- we shouldn't have read in zero bytes.
                        // Maybe we hit the end of the file?
                        // Disable this channel, and pass along a warning.
                        mBlockStatus[midi_block_status_cnt].isPlaying = 0;
                        printf("[WARNING] Block #%d appears to be corrupt. This channel will no longer play from this point on.\n", midi_block_status_cnt);
                    }
                    else
                    {
                        mBlockStatus[midi_block_status_cnt].currentPos += deltaBytesRead;
                    }
                }

                // Make sure this only runs ONCE per block. We only want one tick to run per loop!
                mBlockStatus[midi_block_status_cnt].deltaTicks--;
            }
        }

        totalNumberOfTicks++;
        // This little segment of code ensures that we don't play the MIDI
        // too quickly. Maybe one day, this could be adjustable at runtime?
        if (totalNumberOfTicks > 125000 && totalNumberOfTicks <= 131071)
        {
            while ( (double) (clock() - startTime) < 2500);
        }

        if (totalNumberOfTicks > 131071)
        {
            printf("----TICK #%d----\n", totalNumberOfTicks);
            printf("isPlaying: ");
            int cntr_temp = 0;
            for (; cntr_temp < midi_block_array_size; cntr_temp++)
            {
                if (mBlockStatus[cntr_temp].isPlaying)
                {
                    printf("[%d]", cntr_temp);
                }
            }
            printf("\n");
            getchar();
        }

    }












    // Free all allocated blocks from memory.
    freeBlocks(&midi_block_array, midi_block_array_size);
    printf("[main()] Free'd the MIDI blocks from memory.\n");
    fclose(midi_file);
    return 0;

}






/*
    Function: int MIDIBlockStatus_isPlaying(MIDIBlockStatus * midi_block_status, int count)
    Description:
        Will return the number of currently playing MIDI tracks.
*/
int MIDIBlockStatus_isPlaying(struct MIDIBlockStatus * midi_block_status, int count)
{
    int isPlaying = 0;
    int incr = 0;
    for (; incr < count; incr++)
    {
        isPlaying += midi_block_status[incr].isPlaying;
    }

    return isPlaying;
}

/*
    Function: int MIDIBlockStatus_updateDeltaTime(struct MIDIBlockStatus * midi_block_status)
    Description:
        Updates the delta-time, based on the currentPos of the block passed.
        Returns the number of bytes read for the time-- should throw an error if
        the file limit has been reached.
*/
int MIDIBlockStatus_updateDeltaTime(struct MIDIBlockStatus * midi_block_status)
{
    if ((*midi_block_status).currentPos >= (*midi_block_status).dataSize)
    {
        // Reached the end of the MIDIBlock. Do not attempt to read
        // in any more bytes-- errors and/or overflow into unwanted memory
        // will occur!
        return 0;
    }

    // Grab the initial delta-time of the next block.
    int deltaDelta = 0;
    int deltaDeltaBytes = midi_parse_varSize(
        &((*midi_block_status).data[(*midi_block_status).currentPos]), &deltaDelta);

    // If bytes were actually read from the file, report back to this block.
    if (deltaDeltaBytes)
    {
        (*midi_block_status).deltaTicks += deltaDelta;
    }
    else
    {
        // Something weird happened, and we should return 0.
        return 0;
    }

    // Return the number of bytes that was read.
    return deltaDeltaBytes;
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
