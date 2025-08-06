# STM32 DMA debug logger #

With attention to Small, Efficient, and minimally intrusive, implement a logger that:
1) Uses DMA to load data into the UART, freeing CPU cycles for main task
2) Avoids walking memory, such at strlen(), freeing CPU cycles for main task
3) Uses "normal" circular queue for easier development & debug
4) To keep things "simple", a composition buffer is used to compose a message with snprintf()
     This buffer is copied into the circular queue if space permits.
     (Don't like this copy, but makes things MUCH simpler.)
5) Although "circular" DMA buffer is supported by STM32 parts, this functionality doesn't support
     a limited message length.  As such, a "linear" DMA buffer is used.  If a DMA request
     "wraps" the end of the buffer, the DMA request will be broken into two parts to manage
     "the wrap".

### Client Interface ###
The API, logmsg(const char *format, ...) accepts a prinf() formatted string.
The client adds a terminating line-feed at the end of each message.
  (The terminating null character is replaced with a line feed '\n')
Features not implemented:
 * timestamps
 * log level
 * color

### Current Status ###

* This initial version, 2.0.0, manages most things well.

### Who do I talk to? ###

* Jim Merkle
