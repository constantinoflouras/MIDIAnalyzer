// midi_parse.h
#ifndef MIDI_PARSE_H
#define MIDI_PARSE_H

/*
Function: midi_parse_eventSize
Parameters:
    unsigned char * byte_seq
        The starting sequence of which to search for the length
    int * timestamp
        where to return the timestamp of this MIDI event
Description:
    Calculates the variable size for the next MIDI event/component, starting
    from location byte_seq, for up to four bytes as per MIDI spec.
Returns:
    An int representing how many bytes were read. It will be 0
    if there was an error.
*/
int midi_parse_varSize(unsigned char * byte_seq, int * size);

/*
Function: midi_parse_eventType
Parameters:
    unsigned char * byte_seq
        The starting sequence of which to search for the length
    int * timestamp
        where to return the timestamp of this MIDI event
Description:
    Calculates the timestamp for the next MIDI event, starting from
    location byte_seq, for up to four bytes as per MIDI spec.
Returns:
    An int representing how many bytes were read. It will be 0
    if there was an error.
*/
int midi_parse_eventType(unsigned char * byte_seq, unsigned char * buffer);

unsigned char * midi_printBytes(unsigned char * byte_seq, int bytes);

int midi_parse_getEvent(unsigned char * buffer, int buffer_size, unsigned char * data);





















#endif
