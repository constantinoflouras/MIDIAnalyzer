/*! @file
	Responsible for all implementations regarding parsing a MIDI file and its
	components. This includes building and allocating the individual blocks
	themselves, the size of the individual blocks, etc.
*/
#include "midi_parse.h"
#include <stdio.h>
#include <string.h>

int midi_parse_varSize(unsigned char * byte_seq, int * size)
{
    // If for whatever reason, the given integer isn't actually 0,
    // go ahead and reset that now.
    (*size) = 0;

    // Additionally, create a byte_cnt variable that will count
    // the number of reads that were necessary to grab the
    // entire size. This is the value that will be returned.
    int byte_cnt = 0;

    // To detect the end of the MIDI event size, the most significant
    // bit (counting from 0, bit 7) must be 0. If it is not 0, then
    // there are still more bytes to read.
    while ( (((byte_seq[byte_cnt]) & (1<<7)) != 0) && (byte_cnt < 3))
    {
        (*size) = ((*size) << 7) + (byte_seq[byte_cnt] & 0x7F);
        byte_cnt++;
    }

    if ((((byte_seq[byte_cnt]) & (1<<7)) == 0))
    {
        // Once we see that 0 bit, then we can shift what was previously
        // there and add the remainder.
        (*size) = ((*size) << 7) + (byte_seq[byte_cnt]);

        // Keep in mind, there was one more byte used at the end, so
        // increment byte_cnt before returning.
        return (++byte_cnt);
    }
    else
    {
        // We exceeded the number of bytes that was allowed for the
        // size. Return 0 to indicate an error.
        (*size) = 0;
        return 0;
    }
}

/*
    Function: int midi_parse_getEvent(unsigned char buffer *, int buffer_size, unsigned char * data);
    Description:
        Takes in a pointer to an unsigned buffer, an int unsigned buffer size,
        and an unsigned char * to the actual data.
        It returns the number of bytes that was necessary to read this particular
        event-- usually in the range of three, maybe four bytes.

*/
int midi_parse_getEvent(unsigned char * buffer, int buffer_size, unsigned char * byte_seq)
{
    int byte_cntr = 0;

    //int channelNum = byte_seq[byte_cnt] & 0x0F;
    switch(byte_seq[byte_cntr] >> 4)
    {
        case 0x8:
        case 0x9:
        case 0xA:
        case 0xB:
        case 0xE:
            // These events take up EXACTLY three bytes.
            // That's exactly what we'll copy to the buffer.
            byte_cntr += 3;
            memcpy(buffer, byte_seq, byte_cntr);
            break;
        case 0xC:
        case 0xD:
            byte_cntr += 2;
            memcpy(buffer, byte_seq, byte_cntr);
            break;
        case 0xF:
            // Possibly a SYSEX event, or a META event
            // We'll disambiguate here.
            switch(byte_seq[byte_cntr])
            {
                case 0xF0:
                case 0xF7:
                    // This is a SYSEX event, meaning that there is a variable
                    // length of bytes that need to be read.
                    byte_cntr += 1;
                    int sizeOfSYSEX = 0;
                    int byte_cntr_SYSEXsize = midi_parse_varSize(&byte_seq[byte_cntr], &sizeOfSYSEX);
                    if (sizeOfSYSEX)
                    {
                        byte_cntr += byte_cntr_SYSEXsize + sizeOfSYSEX;
                        if (byte_seq[byte_cntr] == 0xF0)
                        {
                            // F0 Sysex Event -- Buffer Prep
                            memcpy(buffer, byte_seq, byte_cntr);
                        }
                        else if (byte_seq[byte_cntr] == 0xF7)
                        {
                            // F7 Sysex Event -- Buffer Prep
                            memcpy(buffer+1, byte_seq, byte_cntr-1);
                        }
                        else
                        {
                            byte_cntr = 0;
                        }
                    }
                    break;
                case 0xFF:
                    // This is a META event.
                    // Will handle this later!
                    byte_cntr += 1;
                    switch(byte_seq[byte_cntr])
                    {
                        case 0x2F:
                            // This signifies the end of the track
                            // Force a return of 0.
                            byte_cntr = 0;
                            break;
                        case 0x00:
                        case 0x01:
                        case 0x02:
                        case 0x03:
                        case 0x04:
                        case 0x05:
                        case 0x06:
                        case 0x07:
                        case 0x20:
                        case 0x51:
                        case 0x54:
                        case 0x58:
                        case 0x59:
                        case 0x7F:
                        default:
                            byte_cntr += 1;
                            int sizeOfEVNT = 0;
                            int byte_cntr_EVNTsize = midi_parse_varSize(&byte_seq[byte_cntr], &sizeOfEVNT);
                            if (sizeOfEVNT)
                            {
                                byte_cntr += byte_cntr_EVNTsize + sizeOfEVNT;
                                memcpy(buffer, byte_seq, byte_cntr);
                            }
                            else
                            {
                                byte_cntr = 0;
                            }
                            break;
                    }
            }
            break;

        default:
            break;
    }
    return byte_cntr;
}


int midi_parse_eventType(unsigned char * byte_seq, unsigned char * buffer)
{
    // local byte_cnt from our scope
    int byte_cnt = 0;

    // Check and see if it's a "normal" event first:
    int channelNum = byte_seq[byte_cnt] & 0x0F;
    switch(byte_seq[byte_cnt] >> 4)
    {
        case 0x9:
            printf("[EVENT] Note %02X on Channel %d with velocity %02X",
            byte_seq[byte_cnt+1], channelNum, byte_seq[byte_cnt+2]);
            memcpy(buffer,&byte_seq[byte_cnt], 3);
            byte_cnt += 3;
            break;
        case 0xA:
            printf("[EVENT] Note %02X with pressure %02X on Channel %d",
            byte_seq[byte_cnt+1], byte_seq[byte_cnt+2], channelNum);
            byte_cnt += 3;
            break;
        case 0xB:
            printf("[EVENT] Controller %02X on Channel %d is now %02x",
            byte_seq[byte_cnt+1], channelNum, byte_seq[byte_cnt+2]);
            byte_cnt += 3;
            break;
        case 0xC:
            byte_cnt++;
            printf("[EVENT] Program change on Channel %d: %02X", channelNum,
            byte_seq[byte_cnt]);
            byte_cnt++;
            break;
        case 0xD:
            byte_cnt++;
            printf("[EVENT] Pressure on Channel %d: %02X", channelNum,
            byte_seq[byte_cnt]);
            byte_cnt++;
            break;
        default:
            break;

    }

    switch(byte_seq[byte_cnt])
    {
        case 0xFF:
            // META EVENT
            printf("[META EVENT] ");
            switch(byte_seq[++byte_cnt])
            {
                // If we happen to have any variables that need to be pulled, this
                // universal variable should do the trick.
                int byte_size;
                case 0x00:
                    // Sequence Number
                    // Variable-length
                case 0x01:
                    // Text Event
                case 0x02:
                    // Copyright Notice
                case 0x03:
                    printf("Sequence Track Name--Variable Length");
                    byte_cnt += midi_parse_varSize(&byte_seq[++byte_cnt], &byte_size);
                    byte_cnt += byte_size;
                    break;
                    // Sequence/Track Name
                case 0x04:
                    // Instrument Name
                case 0x05:
                    // Lyric
                case 0x06:
                    // Marker
                case 0x07:
                    // Cue Point
                case 0x09:
                    // Text?
                    // Variable length
                    printf("TEXT-- but I don't know how to handle it. Skipping for now...");
                    byte_cnt += midi_parse_varSize(&byte_seq[++byte_cnt], &byte_size);
                    byte_cnt += byte_size;
                    break;
                case 0x20:
                    switch(byte_seq[++byte_cnt])
                    {
                        case 0x01:
                            // MIDI channel prefix
                            printf("MIDI Channel Prefix: %1X\n", byte_seq[(++byte_cnt)]);
                            break;
                        default:
                            printf("UNKNOWN!");
                            break;
                    }
                    break;
                case 0x54:
                    switch(byte_seq[++byte_cnt])
                    {
                        case 0x05:
                            // SMTPE offset
                            printf("SMPTE Offset: %02X %02X %02X %02X %02X \n",
                                byte_seq[byte_cnt+1], byte_seq[byte_cnt+2],
                                byte_seq[byte_cnt+3], byte_seq[byte_cnt+4],
                                byte_seq[byte_cnt+5]);
                            byte_cnt += 6;
                            //TODO: Handle this later!
                            break;
                    }
                    break;
                case 0x58:
                    switch(byte_seq[++byte_cnt])
                    {
                        case 0x04:
                            printf("Time Signature: %02X %02X %02X %02X \n",
                            byte_seq[byte_cnt+1], byte_seq[byte_cnt+2],
                            byte_seq[byte_cnt+3], byte_seq[byte_cnt+4]);
                            byte_cnt += 5;
                    }
                    break;
                case 0x59:
                    switch(byte_seq[++byte_cnt])
                    {
                        case 0x02:
                            printf("Key Signature: %02X %02X \n",
                            byte_seq[byte_cnt+1], byte_seq[byte_cnt+2]);
                            byte_cnt += 3;
                            break;
                    }
                    break;
                case 0x51:
                    switch(byte_seq[++byte_cnt])
                    {
                        case 0x03:
                            printf("Tempo: %02X %02X %02X\n",
                            byte_seq[byte_cnt+1], byte_seq[byte_cnt+2],
                            byte_seq[byte_cnt+3]);
                            byte_cnt += 4;
                            break;
                    }
                    break;
                case 0x2F:
                    switch(byte_seq[++byte_cnt])
                    {
                        case 0x00:
                            printf("End of Track");
                            byte_cnt += 1;
                            break;
                    }
                    break;
                default:
                    printf("UNKNOWN?");
                    break;
            }
            break;
        default:
            //printf("UNKNOWN?");
            break;
    }
    printf("\n");
    return byte_cnt;

}
unsigned char * midi_printBytes(unsigned char * byte_seq, int bytes)
{
    int bytesToSave = 64;
    unsigned char retBytes[bytesToSave];
    unsigned char * retBytesPtr = retBytes;

    int ctr = 0;
    for (; ctr < bytes; ctr++)
    {
        int bytesWritten = snprintf((char *)retBytesPtr, bytesToSave, "%02x ", byte_seq[ctr]);
        retBytesPtr += bytesWritten;
        bytesToSave -= bytesWritten;
    }

    return retBytes;

}
