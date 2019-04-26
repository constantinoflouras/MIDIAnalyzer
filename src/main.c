/*! @file
	Main program executable entry point.
*/

/*	Library header includes	*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

/*	Program header includes	*/
#include "main.h"
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
int MIDIBlockStatus_isPlaying(struct MIDIBlockStatus * midi_block_status, int count);
int MIDIBlockStatus_updateDeltaTime(struct MIDIBlockStatus * midi_block_status);

/*!
    Handles all arguments passed into the program. Success means that the
    program received arguments in the proper syntax-- but it does not verify
    the integrity of the arguments passed.

    @param argc int representing number of arguments
    @param argv pointer to the argument array
    @param params pointer to a struct storing the program arguments
    @return An int representing success (0) or invalid (positive)
 */
int parse_args(int argc, char * argv[], struct main_params * params)
{
    /*  Internal counter for us to keep track of
        which argument we're processing.    */
    int cntr = 1;

    /*  Internal flag that simply tells us whether or not
        we passed in correct argument syntax-- this doesn't
        necessarily mean that we were able to read a file or
        whatnot, it just means that the user inputted arguments
        in the proper syntax.   */
    /*  Remember: zero is success, 1 is failure     */
    int ret = 1;

    /*  Default initialization values   */
    params->midi_file = NULL;
    params->device_file = -1;
    memset(params->midi_filename, 0, MAX_FILENAME_LENGTH);
    memset(params->dev_filename, 0, MAX_FILENAME_LENGTH);

    /*  Every single argument that is passed will be
        read and considered-- but if we run out out of
        arguments, then we'll have an issue.    */
    while (cntr < argc)
    {
        if (!strncmp("--mididev=", argv[cntr], 10))
        {
            /*  This is the MIDI device that we'd like to output to.   */
            DEBUG("Opening the following device: %s\n", &(argv[cntr][10]));
            params->device_file = open(&(argv[cntr][10]), O_WRONLY, 0);

            /*  Store the filename of the device    */
            strncpy( &(params->dev_filename[0]), &(argv[cntr][10]), MAX_FILENAME_LENGTH);
            DEBUG("The resulting FD number was: %d\n", params->device_file);
        }
        else if (cntr == (argc-1))
        {
            /*  In an ideal situation, this would be the file itself.   */
            DEBUG("Opening the following MIDI file: %s\n", argv[cntr]);
            params->midi_file = fopen(argv[cntr], "rb");

            /*  Store the filename of the MIDI file.    */
            strncpy( &(params->midi_filename[0]), &(argv[cntr][0]), MAX_FILENAME_LENGTH);
            DEBUG("Opening the file was %s.\n", (params->midi_file == NULL) ? "unsuccessful--an error occurred" : "successful");
        }
        cntr++;
    }

    /*  Function return value   */
    return ret;
}


/*!
   \brief Main entry point for the application.

   @param argc int representing number of arguments
   @param argv pointer to the argument array
*/
int main(int argc, char * argv[])
{
    /*  File/descriptor for MIDI file and device output */
    struct main_params params;

    /*  The following int, represented as a boolean, indicates whether or not
        we have enough arguments to successfully run the program.   */
    if (!parse_args(argc, argv, &params))
    {
        /*	Processing the arguments failed. Something weird happened.	*/
        printf("Invalid arguments. Expected the following:\n"
                "./%s [--mididev=*dev/midi*] *file*.midi", argv[0]);
    }

    /*	Arguments have been saved into the params structure.
		Verify the integrity of the arguments.	*/
  	if (params.midi_file == NULL)
  	{
  		ERROR("Couldn't open the following MIDI file: %s\n", params.midi_filename);
  		return -1;
  	}

	/*	MIDI file has been successfully loaded, ready to analyze.	*/
    DEBUG("File %s is ready to be analyzed.\n", params.midi_filename);

	/*	Given a MIDI file, begin to load it into memory.	*/
	struct MIDIBlockNode * list;
	list = load_MIDI_file_into_mem(params.midi_file);

	exit(0);

	//	TODO: Add a cleanup function for the params structure, to close out all
	//	of the files that I didn't close earlier.
	fclose(params.midi_file);
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
