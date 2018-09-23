/*
    Name: Constantino Flouras
    Date: September 22nd, 2018
    File: midi_errors.h
    Description:
        Contains the enumerated type that describes all potential errors.
*/
#ifndef midi_errors_h
#define midi_errors_h

enum midi_errors
{
    SUCCESS,
    ERROR_FILE_COULDNT_BE_OPENED,
    ERROR_NOT_A_MIDI_FILE
};

#endif
