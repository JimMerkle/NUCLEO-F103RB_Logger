// Copyright Jim Merkle, 3/26/2020
// Module: hexdump.c
#include <stdio.h>

// Here's what I want for a hexdump() routine:
//00000000  02 03 1f 00 0d 00 00 00  00 00 00 00 00 00 00 00  |................|
//00000010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
void hexdump(const void* address, unsigned count) {
    unsigned remaining = count; // bytes remaining to be displayed
    unsigned char * data = (unsigned char*)address;
    unsigned displayaddr = 0; // starting address to display (this may be a parameter for other versions of this function)
    unsigned i;
    while (remaining) {
        unsigned thisline = remaining < 16?remaining : 16; // number of bytes to process for this line of output
        printf("%08X  ",displayaddr); // display address
        // Display a row of data
        for(i=0;i<thisline;i++) {
            printf("%02X ",data[i]);
            // We like that little gap between each set of 8 displayed bytes
            if(i==7) printf(" ");
        }
        // if we didn't display a complete line, write spaces to the buffer
        if(thisline < 16) {
            for(i=thisline;i<16;i++)
                printf("   ");
            if(thisline < 8)
                printf(" "); // add another space for that little gap
        }
        // Add some space before ASCII displays
        printf("  |");
        // Display ASCII
        for(i=0;i<thisline;i++) {
            if(data[i] >= ' ' && data[i] <= '~')
                printf("%c",(char)data[i]); // printable ascii
            else
                printf(".");  // non-printable
        } // for-loop
        
        // Print end marker and line feed
        printf("|\n");
        
        // Line by line updates
        data += thisline;
        remaining -= thisline;
        displayaddr += thisline;
    }
    // Add an additional line feed if necessary
    //printf("\n");
} // hexdump()
