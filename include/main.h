#ifndef MAIN_H
#define MAIN_H

#define MAX_FILENAME_LENGTH 256

struct main_params
{
    FILE * midi_file;
    int device_file;
    unsigned char midi_filename[MAX_FILENAME_LENGTH];
    unsigned char dev_filename[MAX_FILENAME_LENGTH];
};

#endif
