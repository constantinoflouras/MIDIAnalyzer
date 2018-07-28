// Constantino Flouras
// July 26th, 2018
// MIDI Analyzer

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

// Function prototypes
FILE * initialize_file(int * argc, char * argv[]);
int test_file_if_midi(FILE * file);
void * findBlocks(FILE * file);

// Global variables
FILE * midi_file_input;                                                         // Declare the midi_file_input variable

int main(int argc, char * argv[])
{
    midi_file_input = initialize_file(&argc, argv);                             // The following method will initialize the file.
    //midi_file_input = fopen("just.midi", "rb");                                 // fopen() the midi file we specify (as of now it's hardcoded).

    if (midi_file_input == NULL)                                                // If for whatever reason, this file is null, then the file couldn't be opened.
    {                                                                           // We'll go ahead and output said error to the user, and then we'll quit the program.
        printf("The file could not be opened. Sorry.\n");
        return ENOENT;                                                          // This is the return code for "file not found".
    }

    int isMidi = test_file_if_midi(midi_file_input);
    printf("Is a MIDI file? %s\n", isMidi ? "Yes" : "No");


    // At this point, this file is a MIDI file. Now, we can do some interesting analysis...
    findBlocks(midi_file_input);
}

FILE * initialize_file(int * argc, char * argv[])
{
    #define FILENAME_MAX_SIZE 256
    char filename[FILENAME_MAX_SIZE];                                           // We're going to create the filename variable
    int filename_wiper = 0;                                                     // Then, we'll go ahead and wipe it so that it is zeroed out
    for (;filename_wiper < sizeof(filename); filename_wiper++)
        filename[filename_wiper] = '\0';

    if (*argc > 1)                                                              // If an argument was specified, we'll go ahead and input it automagically into the
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

        if (*argc <= 1)                                                         // If the argc counter is 1 or less, then no arguments were specified.
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

    FILE * file_input = fopen(filename, "rb");                                  // Finally.... once we know the filename, we can open it in read/binary mode.

    return file_input;                                                          // And then we'll go ahead and return the file that we made.
}

int test_file_if_midi(FILE * file)
{
    int returnStatus = 0;                                                       // 0 for FALSE, 1 for TRUE.
    long origFilePosition = ftell(file);                                        // Save the original file position.

    char mthd_indicator[4];                                                     // Now, we should read the first four bytes of the file and see if it matches
    int bytes_read = fread(&mthd_indicator[0], 1, 4, file);                     // with the following: MThd.

    returnStatus = (strncmp("MThd", mthd_indicator, 4) == 0) ? 1 : 0;           // We'll go ahead and compare the first four bytes of the file to what should be
    fseek(file, origFilePosition, SEEK_SET);
    return returnStatus;
}

void * findBlocks(FILE * file)
{
    // For my programs, it is standard practice to restore the original file position, unless it is explicit what the function is doing.
    long file_originalPosition = ftell(file);

    // Before we begin, we need to grab the beginning and end of the file. This will tell us when we've reached the end of
    // the file later on.

    fseek(file, 0, SEEK_END);                                                   // Seek to the end of the file, so that we can grab the size.
    long file_size = ftell(file);                                               // Store the size of the file (in bytes) into this variable.
    fseek(file, 0, SEEK_SET);                                                   // Go to the beginning of the file, in preparation for block seeking.

    /*
        Now, what we're going to do is search through the entire block, attempting to find information about the MIDI file.
        Each block of a MIDI file always starts out with 8 bytes:
            4 bytes -- Identifies what this block is going to tell us
            4 bytes -- Size of the block
    */
    while (ftell(file) < file_size)
    {
        static int block_num = 0;
        unsigned char buffer[8];                                                // We want a new buffer for each and every run.

        int buffer_pos = 0;                                                     // Wipe the buffer-- remember that C does not guarantee that
        for (; buffer_pos < 8; buffer_pos++)                                    // this memory space will be all zeroes. This is important!
            buffer[buffer_pos]='\0';

        int bytes_read = fread(&buffer[0], 1, 8, file);                         // Now, we'll go ahead and read in the first eight bytes of this file.
        if (bytes_read < 8)
            continue;   // See if we error out from reaching the end of the file.

        // get block size
        int block_size = 0;
        int offset = 4; // starting point for where the size is
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

            block_size += representation * pow(16, 8 - 1 - nibble_pos);

            //printf("For byte %x, found a: %d\n", nibble, representation);
        }


        // Now, we'll compare the first four chars, and if that's a match,
        // we'll also go ahead and grab the size.


        printf("Block #%d:\n\tType: %.4s\n\tSize: %02x %02x %02x %02x\n\tSize (decimal): %d\n\n",
            block_num, &buffer[0],
            buffer[4], buffer[5], buffer[6], buffer[7],block_size);

        break;
    }

}
