// Module: log.h
//
// logging library
#include <main.h>

// Define ANSI colors, to be used within printf() text
// The foreground colors 30 - 38, are the "normal" darker colors
// The foreground colors 90 - 98, are the "bright" lighter colors
// These #defines rely on string concatenation to function as desired
#define COLOR_YELLOW_ON_BLACK "\033[93m\033[40m"
#define COLOR_YELLOW_ON_BLUE  "\033[93m\033[44m"
#define COLOR_YELLOW_ON_GREEN "\033[93m\033[42m"
#define COLOR_YELLOW_ON_RED   "\033[93m\033[41m"
#define COLOR_YELLOW_ON_VIOLET "\033[93m\033[45m"
#define COLOR_WHITE
#define COLOR_RED    "\033[92m"   /* Bright Green text */
#define COLOR_GREEN  "\033[92m"   /* Bright Green text */
#define COLOR_VIOLET "\033[95m"   /* Bright Violet text */
#define COLOR_YELLOW "\033[93m"   /* Bright Yellow text */
#define COLOR_RESET  "\033[0m"    /* Reset text color to previous color */

#define LOG_ITEM_MAX_SIZE  128              // Max storage allowed in DMA buffer for a log item (includes null termination)
//#define LOG_ITEM_MAX_SIZE  4   	            // Max storage allowed in DMA buffer for a log item (includes null termination)
#define LOG_MAX_TEXT (LOG_ITEM_MAX_SIZE)	// maximum number of characters (snprintf() null terminates the string)
#define LOG_DMA_BUFFER_SIZE  4096
//#define LOG_DMA_BUFFER_SIZE  16

typedef enum {
    DBG_LOG_NONE,       /* No log output */
	DBG_LOG_ERROR,      /* Critical errors, software module can not recover on its own */
	DBG_LOG_WARN,       /* Error conditions from which recovery measures have been taken */
	DBG_LOG_INFO,       /* Information messages which describe normal flow of events */
	DBG_LOG_DEBUG,      /* Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
	DBG_LOG_VERBOSE     /* Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} dbg_log_level_t;

int logmsg(const char *format, ...);
extern const char bigstring[]; // log.c

extern UART_HandleTypeDef huart2; // main.c - UART being used for logger


char *esp_log_system_timestamp(void);
//Function which returns system timestamp to be used in log output.
//This function is used in expansion of ESP_LOGx macros to print the system time as “HH:MM:SS.sss”.
//The system time is initialized to 0 on startup, this can be set to the correct time with an SNTP sync,
//or manually with standard POSIX time functions.
//Currently this will not get used in logging from binary blobs (i.e WiFi & Bluetooth libraries), these will still print the RTOS tick time.
//Return timestamp, in “HH:MM:SS.sss”


// Provide a method to return a 32 bit millisecond time since the device started (ignore 32-bit roll-over)
// A free running 32-bit millisecond counter
// If the RTOS has a counter, this may be used
#define GET_LOG_MS xTaskGetTickCount(); // This FreeRTOS function returns RTOS ticks, not milliseconds, unless TICK_RATE_HZ == 1000 (default)


#if 0
void dbg_log(dbg_log_level_t level,
                   const char *tag,
                   const char *format, ...);
#else
// Start simple
void dbg_log(const char *format, ...);
#endif
