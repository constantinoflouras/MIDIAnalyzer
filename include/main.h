#ifndef MAIN_H
#define MAIN_H

#define MAX_FILENAME_LENGTH 256

struct main_params
{
    FILE * midi_file;
    int device_file;
    char midi_filename[MAX_FILENAME_LENGTH];
    char dev_filename[MAX_FILENAME_LENGTH];
};

#endif
