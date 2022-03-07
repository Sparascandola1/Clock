#include "clock_includes.h"
#include "LCD_Lib.h"
#include "lcd_graphic.h"
#include "font.h"

int open_physical(int);
void *map_physical(int, unsigned int, unsigned int);
void close_physical(int);
int unmap_physical(void *, unsigned int);
int bcd2SevenSeg(int);
int timeParser(int, int, int);
void setAlarm();
void activeAlarm();
volatile int *HEX_ptr1; // virtual address pointer to seven seg HEX0_3
volatile int *HEX_ptr2; // virtual address pointer to seven seg HEX4_5
volatile int *SW_ptr;   // virtual address for the Switch port
volatile int *KEY_ptr;
int alarmTime;

#define HW_REGS_BASE (ALT_STM_OFST)
#define HW_REGS_SPAN (0x04000000)
#define HW_REGS_MASK (HW_REGS_SPAN - 1)
// Shared Circular Buffer
// struct CIRCULAR_BUFFER
// {
//     int count; // Number of items in the buffer
//     int lower; // Next slot to read in the buffer
//     int upper; // Next slot to write in the buffer
//     int buffer[3];
// };

// struct CIRCULAR_BUFFER *buffer = NULL;
/*
 * author: Salvatore Parascandola
 * version: 1.0
 * desc: Display system time on the 7-segment display
 */
int main(void)
{

    int fd = -1;      // used to open /dev/mem for access to physical addresses
    void *LW_virtual; // used to map physical addresses for the light-weight bridge
    void *virtual_base2;

    LCD_CANVAS LcdCanvas;

    // Create virtual memory access to the FPGA light-weight bridge
    if ((fd = open_physical(fd)) == -1)
        return (-1);
    if ((LW_virtual = map_physical(fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN)) == NULL)
        return (-1);


    // Set virtual address pointer to I/O port for HEX0_3 & HEX4_5
    HEX_ptr1 = (unsigned int *)(LW_virtual + HEX3_HEX0_BASE);
    HEX_ptr2 = (unsigned int *)(LW_virtual + HEX5_HEX4_BASE);
    SW_ptr = (unsigned int *)(LW_virtual + SW_BASE);
    KEY_ptr = (unsigned int *)(LW_virtual + KEY_BASE);

    virtual_base2 = mmap(NULL, HW_REGS_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, HW_REGS_BASE);

    if (virtual_base2 == MAP_FAILED)
    {
        printf("ERROR: mmap() failed...\n");
        close(fd);
        return (1);
    }

    LcdCanvas.Width = LCD_WIDTH;
    LcdCanvas.Height = LCD_HEIGHT;
    LcdCanvas.BitPerPixel = 1;
    LcdCanvas.FrameSize = LcdCanvas.Width * LcdCanvas.Height / 8;
    LcdCanvas.pFrame = (void *)malloc(LcdCanvas.FrameSize);

    if (LcdCanvas.pFrame == NULL)
    {
        printf("failed to allocate lcd frame buffer\r\n");
    }
    else
    {

        LCDHW_Init(virtual_base2);
        LCDHW_BackLight(true); // turn on LCD backlight

        LCD_Init();

        // clear screen
        DRAW_Clear(&LcdCanvas, LCD_WHITE);

        // demo grphic api
        DRAW_Rect(&LcdCanvas, 0, 0, LcdCanvas.Width - 1, LcdCanvas.Height - 1, LCD_BLACK); // retangle

        // demo font
        DRAW_PrintString(&LcdCanvas, 20, 2, "Use switches", LCD_BLACK, &font_16x16);
        DRAW_PrintString(&LcdCanvas, 20, 2 + 10, "to operate", LCD_BLACK, &font_16x16);
        DRAW_PrintString(&LcdCanvas, 20, 2 + 20, "the clock", LCD_BLACK, &font_16x16);
        DRAW_PrintString(&LcdCanvas, 20, 2 + 30, "and buttons", LCD_BLACK, &font_16x16);
        DRAW_PrintString(&LcdCanvas, 20, 2 + 40, "for the alarm", LCD_BLACK, &font_16x16);
        DRAW_Refresh(&LcdCanvas);

        free(LcdCanvas.pFrame);
    }

    // Create shared memory for the Circular Buffer to be shared between the Parent and Child  Processes
    // buffer = (struct CIRCULAR_BUFFER *)mmap(0, sizeof(buffer), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    // buffer->count = 0;
    // buffer->lower = 0;
    // buffer->upper = 0;

    /*
     * Main program logic:
     * find the system time, parse hour; min; and sec into one string and parse the string into an int
     * Then update the 7 segment display
     */
    while (1)
    {

        /*<-----  ALARM  ----->*/

        if (*KEY_ptr == 8)
        {

            // // clear screen
            // DRAW_Clear(&LcdCanvas, LCD_WHITE);

            // // demo grphic api
            // DRAW_Rect(&LcdCanvas, 0, 0, LcdCanvas.Width - 1, LcdCanvas.Height - 1, LCD_BLACK); // retangle

            // // demo font
            // DRAW_PrintString(&LcdCanvas, 40, 5, "Hello", LCD_BLACK, &font_16x16);
            // DRAW_PrintString(&LcdCanvas, 40, 5 + 10, "from", LCD_BLACK, &font_16x16);
            // DRAW_PrintString(&LcdCanvas, 40, 5 + 20, "the other", LCD_BLACK, &font_16x16);
            // DRAW_PrintString(&LcdCanvas, 40, 5 + 30, "side", LCD_BLACK, &font_16x16);
            // DRAW_Refresh(&LcdCanvas);

            setAlarm();
        }

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

        // parse into string and then into integer
        int result = timeParser(hour, min, sec);

        if (result == alarmTime)
        {
            activeAlarm();
        }

        if (*SW_ptr + 1 == 2)
        {

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
        else if (*SW_ptr + 1 == 4)
        {
            // parse into string and then into integer
            int result = timeParser(hour + 1, min, sec);

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
        else if (*SW_ptr + 1 == 8)
        {
            // parse into string and then into integer
            int result = timeParser(hour + 2, min, sec);

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
        else if (*SW_ptr + 1 == 16)
        {
            // parse into string and then into integer
            int result = timeParser(hour + 3, min, sec);

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
        else
        {
            *HEX_ptr2 = 0;
            *HEX_ptr1 = 0;
        }
    }

    // release and close the physical memory
    unmap_physical(LW_virtual, LW_BRIDGE_SPAN);
    close_physical(fd);
    return 0;
}

void setAlarm()
{

    int choice = 1;
    int result = 0;
    int min = 0;
    int hour = 0;

    int hex4_5 = (bcd2SevenSeg(result / 100000) << 8) |
                 (bcd2SevenSeg((result / 10000) % 10));

    int hex0_3 = (bcd2SevenSeg((result / 1000) % 10) << 24) |
                 (bcd2SevenSeg((result / 100) % 10) << 16) |
                 (bcd2SevenSeg((result / 10) % 10) << 8) |
                 bcd2SevenSeg(result % 10);

    *HEX_ptr2 = hex4_5;
    *HEX_ptr1 = hex0_3;

    /*---- MINUTE LOOP ----*/
    while (choice == 1)
    {

        switch (*SW_ptr + 1)
        {
        case 2:
            min = 5;
            break;

        case 3:
            min = 10;
            break;

        case 4:
            min = 15;
            break;

        case 5:
            min = 20;
            break;

        case 6:
            min = 25;
            break;

        case 7:
            min = 30;
            break;

        case 8:
            min = 35;
            break;

        case 9:
            min = 40;
            break;

        case 10:
            min = 45;
            break;

        case 11:
            min = 50;
            break;

        case 12:
            min = 55;
            break;

        default:
            result = 0;
            break;
        }

        result = timeParser(0, min, 0);

        int hex4_5 = (bcd2SevenSeg(result / 100000) << 8) |
                     (bcd2SevenSeg((result / 10000) % 10));

        int hex0_3 = (bcd2SevenSeg((result / 1000) % 10) << 24) |
                     (bcd2SevenSeg((result / 100) % 10) << 16) |
                     (bcd2SevenSeg((result / 10) % 10) << 8) |
                     bcd2SevenSeg(result % 10);

        *HEX_ptr2 = hex4_5;
        *HEX_ptr1 = hex0_3;

        if (*KEY_ptr == 2)
        {
            choice = 2;
        };
    }

    /*---- HOUR LOOP ----*/
    while (choice == 2)
    {

        switch (*SW_ptr + 1)
        {
        case 2:
            hour = 1;
            break;

        case 3:
            hour = 2;
            break;

        case 4:
            hour = 3;
            break;

        case 5:
            hour = 4;
            break;

        case 6:
            hour = 5;
            break;

        case 7:
            hour = 6;
            break;

        case 8:
            hour = 7;
            break;

        case 9:
            hour = 8;
            break;

        case 10:
            hour = 9;
            break;

        case 11:
            hour = 10;
            break;

        case 12:
            hour = 11;
            break;

        case 13:
            hour = 12;
            break;

        case 14:
            hour = 13;
            break;

        case 15:
            hour = 14;
            break;

        case 16:
            hour = 15;
            break;

        case 17:
            hour = 16;
            break;

        case 18:
            hour = 17;
            break;

        case 19:
            hour = 18;
            break;

        case 20:
            hour = 19;
            break;

        case 21:
            hour = 20;
            break;

        case 22:
            hour = 21;
            break;

        case 23:
            hour = 22;
            break;

        case 24:
            hour = 23;
            break;

        case 25:
            hour = 24;
            break;

        default:
            result = 0;
            break;
        }

        result = timeParser(hour, min, 0);

        int hex4_5 = (bcd2SevenSeg(result / 100000) << 8) |
                     (bcd2SevenSeg((result / 10000) % 10));

        int hex0_3 = (bcd2SevenSeg((result / 1000) % 10) << 24) |
                     (bcd2SevenSeg((result / 100) % 10) << 16) |
                     (bcd2SevenSeg((result / 10) % 10) << 8) |
                     bcd2SevenSeg(result % 10);

        *HEX_ptr2 = hex4_5;
        *HEX_ptr1 = hex0_3;

        if (*KEY_ptr == 4)
        {
            alarmTime = timeParser(hour, min, 0);
            break;
        };
    }
}

void activeAlarm()
{

    // int hex4_5 = 0;
    // int hex0_3 = 0;

    int fake = timeParser(88, 88, 88);

    while (1)
    {

        // *HEX_ptr1 = 0;
        // *HEX_ptr2 = 0;

        // hex4_5 = (bcd2SevenSeg(alarmTime / 100000) << 8) |
        //          (bcd2SevenSeg((alarmTime / 10000) % 10));

        // hex0_3 = (bcd2SevenSeg((alarmTime / 1000) % 10) << 24) |
        //          (bcd2SevenSeg((alarmTime / 100) % 10) << 16) |
        //          (bcd2SevenSeg((alarmTime / 10) % 10) << 8) |
        //          bcd2SevenSeg(alarmTime % 10);

        // *HEX_ptr2 = hex4_5;
        // *HEX_ptr1 = hex0_3;

        // sleep(2);

        *HEX_ptr1 = 0;
        *HEX_ptr2 = 0;

        int hex4_5 = (bcd2SevenSeg(fake / 100000) << 8) |
                 (bcd2SevenSeg((fake / 10000) % 10));

        int hex0_3 = (bcd2SevenSeg((fake / 1000) % 10) << 24) |
                 (bcd2SevenSeg((fake / 100) % 10) << 16) |
                 (bcd2SevenSeg((fake / 10) % 10) << 8) |
                 bcd2SevenSeg(fake % 10);

        *HEX_ptr2 = hex4_5;
        *HEX_ptr1 = hex0_3;

        if (*KEY_ptr == 2)
        {
            break;
        }
    }
}

/*
 * desc: parse the parameters into one int so it can be displayed on the seven seg
 * params: int, int, int
 * returns: int (the full time as an integer in 24 notation)
 */
int timeParser(int hour, int min, int sec)
{
    char str[] = "000000";

    if (min < 10)
    {
        if (sec < 10)
        {
            sprintf(str, "%d%d%d%d%d", hour, 0, min, 0, sec);
            return strtol(str, NULL, 10);
        }
        sprintf(str, "%d%d%d%d", hour, 0, min, sec);
        return strtol(str, NULL, 10);
    }
    else if (sec < 10)
    {
        sprintf(str, "%d%d%d%d", hour, min, 0, sec);
        return strtol(str, NULL, 10);
    }
    else
    {
        sprintf(str, "%d%d%d", hour, min, sec);
        return strtol(str, NULL, 10);
    }
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