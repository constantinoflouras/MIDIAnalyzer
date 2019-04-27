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
int isPlaying(struct MIDIFile midiFile);
int updateDeltaTimePos(struct MIDIBlock * midiBlock);

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
	struct MIDIBlockNode * list = alloc_midi_file(params.midi_file);

	/*	Convert the linked list into an array	*/
	struct MIDIFile midiFile = convert_ll_to_MIDIFile(list);

	/*	At this point, we no longer need to keep the MIDI file open. We can close it now!	*/
	fclose(params.midi_file);

	for (int cntr = 0; cntr < midiFile.num_blocks; cntr++)
	{
		printf("ARR_BLOCK #%d:\n"
			"\tHeader: %.4s\n"
			"\tSize: %d\n"
			"\n",
			cntr,
			midiFile.blockArr[cntr].header,
			midiFile.blockArr[cntr].n_data_size);
	}

	for (int cntr = 0; cntr < midiFile.num_blocks; cntr++)
	{
		if (!strncmp("MTrk", midiFile.blockArr[cntr].header, 4))
		{
			DEBUG("Block #%d is a MTrk that can be played.\n", cntr);
			midiFile.blockArr[cntr].bActive = 1;
			midiFile.blockArr[cntr].nCurrentPos = 0;
			updateDeltaTimePos( &(midiFile.blockArr[cntr]));
		}
		else
		{
			DEBUG("Block #%d of type %.4s cannot be played.\n", cntr, midiFile.blockArr[cntr].header);
			midiFile.blockArr[cntr].bActive = 0;
		}

	}

	#define EVENT_BUFFER_SIZE 32
	unsigned char dev_buffer[EVENT_BUFFER_SIZE];

	while(isPlaying(midiFile))
	{
		/*	Initialize the timer.	*/
		clock_t startTime = clock();

		int block_iter = 0;
		for (block_iter = 0; block_iter < 2/*midiFile.num_blocks*/; block_iter++)
		{
			struct MIDIBlock * currentBlock = &(midiFile.blockArr[block_iter]);

			printf("BYTES for BLOCK #%d: ", block_iter);
			for (int i = 0; i < 15; i++)
			{
				printf("%02x ", currentBlock->data[i]);
			}
			printf("\n");
			if (currentBlock->bActive)
			{
				DEBUG("This is an active track.\n");
				/*	This is a playable track	*/
				/*	Continuously play notes until we hit something that asks
					for a delay	*/
				while (currentBlock->bActive && currentBlock->nDeltaTicks <= 0)
				{
					DEBUG("Clearing buffer.\n");
					/* Clear the buffer */
					memset(dev_buffer, 0, sizeof(dev_buffer));

					/*	Parse the next event, and then determine if there's anything else left to parse.	*/
					DEBUG("Getting event.\n");
					int bytes_read = midi_parse_getEvent(dev_buffer, EVENT_BUFFER_SIZE, &(currentBlock->data[currentBlock->nCurrentPos]) );
					DEBUG("Read %d bytes for event.\n", bytes_read);

					currentBlock->nCurrentPos += bytes_read;
					currentBlock->bActive = (currentBlock->nCurrentPos < currentBlock->n_data_size);
					//if (!currentBlock->bActive) break;


					/*	Write the next event to the device, if it exists	*/
					if (params.device_file > 0)
					{
						DEBUG("Playing note on device %d, %d bytes...\n", params.device_file, bytes_read);

						write(params.device_file, &(dev_buffer[0]), bytes_read);
					}

					if (updateDeltaTimePos(currentBlock) == 0)
					{
						WARN("Block #%d appears to be corrupt, or something weird happened-- will no longer play from this point on.\n", block_iter);
						currentBlock->bActive = 0;
						break;
					}


				}
				currentBlock->nDeltaTicks--;
			}
		}

		while ( (double) (clock() - startTime) < 100000);
	}








    return 0;

}







/*!
   \brief Given a MIDI file, informs caller whether it is still playing or not.

   @param midiFile a struct MIDIFile to check if it is playing
   @return an integer representing how many blocks are still playing
*/
int isPlaying(struct MIDIFile midiFile)
{
    int isPlaying = 0;
    int incr = 0;
    for (; incr < midiFile.num_blocks; incr++)
    {
        isPlaying += midiFile.blockArr[incr].bActive;
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
/*! \brief Updates the time delta for the next note within a MIDIBlock
	@param midiBlock pointer to a MIDIBlock structure
	@return number of bytes that were read
*/

int updateDeltaTimePos(struct MIDIBlock * midiBlock)
{
	/*	See if we've reached the end of the file. If so, just return zero	*/
    if (midiBlock->nCurrentPos >= midiBlock->n_data_size)
    {
		/*	End of the block has been reached-- we can't do any more updates.	*/
        return 0;
    }

    // Grab the initial delta-time of the next block.
    int deltaDeltaTime = 0;

	int deltaDeltaBytes = midi_parse_varSize(
		&(midiBlock->data[midiBlock->nCurrentPos]),
		&deltaDeltaTime);

	DEBUG("Read %d bytes to calculate delta ticks time of %d.\n", deltaDeltaBytes, deltaDeltaTime);
    // If bytes were actually read from the file, report back the time delay
	//	to the block.
    if (deltaDeltaBytes)
    {
        midiBlock->nDeltaTicks += deltaDeltaTime;
		midiBlock->nCurrentPos += deltaDeltaBytes;
    }
    else
    {
        // Something weird happened, and we should return 0.
		DEBUG("WEIRDNESS HAPPENED");
        return 0;
    }

    // Return the number of bytes that was read.
    return deltaDeltaBytes;
}
