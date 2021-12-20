#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "./address_map_arm.h"

int open_physical(int);
void *map_physical(int, unsigned int, unsigned int);
void close_physical(int);
int unmap_physical(void *, unsigned int);
int bcd2SevenSeg(int);

/*
* author: Salvatore Parascandola
* version: 1.0
* desc: Display system time on the 7-segment display 
*/
int main(void)
{
    volatile int *HEX_ptr1; // virtual address pointer to seven seg HEX0_3
    volatile int *HEX_ptr2; // virtual address pointer to seven seg HEX4_5

    int fd = -1;      // used to open /dev/mem for access to physical addresses
    void *LW_virtual; // used to map physical addresses for the light-weight bridge

    // Create virtual memory access to the FPGA light-weight bridge
    if ((fd = open_physical(fd)) == -1)
        return (-1);
    if ((LW_virtual = map_physical(fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN)) == NULL)
        return (-1);

    // Set virtual address pointer to I/O port for HEX0_3 & HEX4_5
    HEX_ptr1 = (unsigned int *)(LW_virtual + HEX3_HEX0_BASE);
    HEX_ptr2 = (unsigned int *)(LW_virtual + HEX5_HEX4_BASE);

    /*
    * Main program logic: 
    * find the system time, parse hour; min; and sec into one string and parse the string into an int
    * Then update the 7 segment display
    */
    while (1)
    {

        /*<-----  TIME  ----->*/

        // time var for ho;d raw data
        time_t rawtime;

        // struct to make manipulating system information easier
        struct tm *timeinfo;

        // get the time from the system
        time(&rawtime);

        // fill the timeinfo structor with date and time
        timeinfo = localtime(&rawtime);

        // get out the data I want
        int hour = timeinfo->tm_hour;
        int min = timeinfo->tm_min;
        int sec = timeinfo->tm_sec;

        // printf("Hour %d\n", hour);
        // printf("min %d\n", min);
        // printf("sec %d\n", sec);

        // sleep(5);

        // parse into string and then into integer
        char str[100];
        sprintf(str, "%d%d%d", hour, min, sec);
        int result = strtol(str, NULL, 10);

        //printf("Time: %d\n", result);

        /*<-----  7-SEG  ----->*/

        // reset the hex pointers!
        *HEX_ptr1 = 0;
        *HEX_ptr2 = 0;

        // Use bit operators in order to assign each digit of the time to the correct segment display starting with Hex5 and moving left on the display.
        int hex4_5 = (bcd2SevenSeg(result / 100000) << 8) | 
                     (bcd2SevenSeg((result / 10000) % 10));

        int hex0_3 = (bcd2SevenSeg((result / 1000) % 10) << 24) |
                     (bcd2SevenSeg((result / 100) % 10) << 16) |
                     (bcd2SevenSeg((result / 10) % 10) << 8) |
                     bcd2SevenSeg(result % 10);

        *HEX_ptr2 = hex4_5;
        *HEX_ptr1 = hex0_3;
    }

    // release and close the physical memory
    unmap_physical(LW_virtual, LW_BRIDGE_SPAN);
    close_physical(fd);
    return 0;
}

/*
* desc: base hex values for decimals, will use these to assign values to the seven seg display
* params: int
* returns: int (Hex value for decimal)
*/
int bcd2SevenSeg(int dec)
{
    switch (dec)
    {
    case 0:
        return 0x3f;
    case 1:
        return 0x06;
    case 2:
        return 0x5b;
    case 3:
        return 0x4f;
    case 4:
        return 0x66;
    case 5:
        return 0x6d;
    case 6:
        return 0x7d;
    case 7:
        return 0x07;
    case 8:
        return 0x7f;
    case 9:
        return 0x67;
    default:
        return 0xff;
    }
}

/*
* desc: opens physical memory
* params: int
* returns : int
*/
int open_physical(int fd)
{
    if (fd == -1)
        if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1)
        {
            printf("ERROR: could not open \"/dev/mem\"...\n");
            return (-1);
        }
    return fd;
}

/*
* desc: closes physical memory
* params: int
* returns: void
*/ 
void close_physical(int fd)
{
    close(fd);
}

/*
 * desc: establish a virtual address mapping for the physical addresses starting at base
 * params: int, int, int
 * returns: virtual_base
 */
void *map_physical(int fd, unsigned int base, unsigned int span)
{
    void *virtual_base;

    // Get a mapping from physical addresses to virtual addresses
    virtual_base = mmap(NULL, span, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, base);
    if (virtual_base == MAP_FAILED)
    {
        printf("ERROR: mmap() failed...\n");
        close(fd);
        return (NULL);
    }
    return virtual_base;
}

/*
 * desc: close the previously-opened virtual address mapping
 * params: virtual_base, int
 * returns: int
 */
int unmap_physical(void *virtual_base, unsigned int span)
{
    if (munmap(virtual_base, span) != 0)
    {
        printf("ERROR: munmap() failed...\n");
        return (-1);
    }
    return 0;
}