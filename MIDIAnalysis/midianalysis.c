// Constantino Flouras
// July 26th, 2018
// MIDI Analyzer

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

struct MIDIBlock                                                                // The following is a new struct that will define a MIDI block.
{
    char header[4];                                                             // Four byte header that identifies the type that this block is (for example, 'MTrk' or 'MThd')
    int size;                                                                   // How large the memory allocated block is.
    char * data;                                                                // Pointer to the raw data (which is of a variable size)
};

// Function prototypes
FILE * initialize_file(int * argc, char * argv[]);
int test_file_if_midi(FILE * file);
void * findBlocks(FILE * file);
int grabBlocks(FILE * file, struct MIDIBlock ** midiBlocks, int * size);

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



    struct MIDIBlock * pointer;

    // At this point, this file is a MIDI file. Now, we can do some interesting analysis...
    grabBlocks(midi_file_input, &pointer, NULL);





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
            continue;                                                           // See if we error out from reaching the end of the file.

        int block_size = 0;                                                     // We'll go ahead and figure out the block size.
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
        }


        // Now, we'll compare the first four chars, and if that's a match,
        // we'll also go ahead and grab the size.


        printf("Block #%d:\n\tType: %.4s\n\tSize: %02x %02x %02x %02x\n\tSize (decimal): %d\n\n",
            block_num, &buffer[0],
            buffer[4], buffer[5], buffer[6], buffer[7],block_size);

        fseek(file, block_size, SEEK_CUR);
        block_num++;
    }

    // We need to return a pointer!
    return 0;
}

int grabBlocks(FILE * file, struct MIDIBlock ** midiBlocks, int * size)
{
    // For my programs, it is standard practice to restore the original file position, unless it is explicit what the function is doing.
    long file_originalPosition = ftell(file);

    // Before we begin, we need to grab the beginning and end of the file. This will tell us when we've reached the end of
    // the file later on.

    fseek(file, 0, SEEK_END);                                                   // Seek to the end of the file, so that we can grab the size.
    long file_size = ftell(file);                                               // Store the size of the file (in bytes) into this variable.
    fseek(file, 0, SEEK_SET);                                                   // Go to the beginning of the file, in preparation for block seeking.

    struct Node                                                                 // So, the idea is that we're going to create an 'internal' linked list of MIDIBlock structs
    {                                                                           // as we go, and then convert it to an array of MIDIBlocks after we seek through the entire file.
        struct Node * nextNode;                                                 // Why do this? We're trying to eliminate the number of disk seeks-- I want to do this entirely in memory.
        struct MIDIBlock midiBlock;
    };

    struct Node * initialNode = malloc(sizeof(struct Node));                    // The initial node is where we're going to start reading from when we process the data.
    struct Node * currentNode = initialNode;                                    // The current node is where we're going to write to next. Right now, it's the initial node.

    int block_num = 0;
    while (ftell(file) < file_size)
    {
        unsigned char buffer[8];                                                // We want a new buffer for each and every run.

        int buffer_pos = 0;                                                     // Wipe the buffer-- remember that C does not guarantee that
        for (; buffer_pos < 8; buffer_pos++)                                    // this memory space will be all zeroes. This is important!
            buffer[buffer_pos]='\0';

        int bytes_read = fread(&buffer[0], 1, 8, file);                         // Now, we'll go ahead and read in the first eight bytes of this file.
        if (bytes_read < 8)
        {
            printf("[WARNING: grabBlocks()] %s%d%s\n",                          // If for whatever reason we didn't read all eight bytes, there's a chance
                "Attempted to read 8 bytes from file, read ",                   // that we're misaligned with the contents of the file. That means that we're
                bytes_read,                                                     // effectively reading garbage, and that's not particularly good.
                " bytes. There is a possiblity for data misalignment!");
            continue;                                                           // See if we error out from reaching the end of the file.
        }

        int block_size = 0;                                                     // We'll go ahead and figure out the block size.
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
        }


        // Now, we'll compare the first four chars, and if that's a match,
        // we'll also go ahead and grab the size.


        printf("Block #%d:\n\tType: %.4s\n\tSize: %02x %02x %02x %02x\n\tSize (decimal): %d\n\n",
            block_num, &buffer[0],
            buffer[4], buffer[5], buffer[6], buffer[7],block_size);

        /*
            // The following is a new STRUCT that will define a MIDI block.
            struct MIDIBlock
            {
                char header[4];                                                             // Four byte header that identifies the type that this block is (for example, 'MTrk' or 'MThd')
                int size;                                                                   // How large the memory allocated block is.
                void * data;                                                                // Pointer to the raw data (which is of a variable size)
            };
        */

        // Instead of fseek(ing), we're actually going to fread directly into the block of memory that will contain this node.
        (*currentNode).nextNode = malloc(sizeof(struct Node));
        strncpy(((*currentNode).midiBlock).header, buffer, 4);                 // Set the char[] header
        (*currentNode).midiBlock.size = block_size;                          // Set the block size
        (*currentNode).midiBlock.data = malloc(block_size);                  // Allocate space for the data to go.

        fread(((*currentNode).midiBlock).data, block_size, 1, file);

        block_num++;
        currentNode = (*currentNode).nextNode;
        (*currentNode).nextNode = NULL;                                         // We haven't found another block to put into the linked list, so this should be NULL.

    }


        // The purpose of this code was to test whether not I was actually storing the contents of the
        // MIDI file within memory. This appears to have been successful, so I think we're in good shape.

        currentNode = initialNode;

        int block_counter_two = 0;
        while ((*currentNode).nextNode != NULL)
        {
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
            block_counter_two++;
            currentNode = (*currentNode).nextNode;
        }


    // Now, we need to convert the linked list of MIDIBlocks into an array. We'll convert that here.
    // Reset the currentNode to the original initialNode
    currentNode = initialNode;

    (*midiBlocks) = malloc(sizeof(struct MIDIBlock) * block_num);
    int block_counter_arr = 0;
    while (currentNode != NULL)
    {
        (*midiBlocks)[block_counter_arr] = (*currentNode).midiBlock;

        currentNode = (*currentNode).nextNode;
        free(initialNode);
        initialNode = currentNode;
        block_counter_arr++;
    }

    return 0;
}
