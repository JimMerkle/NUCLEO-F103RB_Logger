// Module: log.c
//
// logging library (Let's keep it simple, not the most efficient, but easy to maintain)
// Assumptions:
// * Target device supports DMA feeding the USART transmitter
// * The target DMA buffer is configured to be "circular" in nature
//
// Operation:
// 1) Intermediate static buffer is used with vsnprintf() to compose a log message that
//      will be copied into the circular DMA queue buffer
// 2) Circular queue is shared between client(s) writing into queue and DMA process pulling messages
//      from the queue.
// 3) If the DMA / USART TX hardware is stopped, the client logging function will initiate the
//      background DMA process by calling a function that prepares and starts the DMA process.
// 4) When the USART DMA process has completed, the DMA ISR will check for additional messages to send.
//     If no messages are available, the DMA interrupt will be disabled, else another USART DMA Transmit will be started.
//
// Details:
// * If time stamps / colors are added later, they will be added to the client vsnprintf() function, storing additional information into
//     each log item.
// * For message termination, a line-feed character, '\n', will be written over the terminating null character in the buffer
// * Using queue Head and Tail positions, amount of data to DMA is always known
//
// ANSI escape codes:
// gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "log.h"
#include <unistd.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <main.h> // HAL definitions


// Shared globals used by this logging library
char _usart2_tx_dma_buffer[LOG_DMA_BUFFER_SIZE];
static char _log_compose_buffer[LOG_ITEM_MAX_SIZE];
static volatile uint16_t _queue_tail; // New messages are added to tail
static volatile uint16_t _queue_head; // DMA is used to pull messages from the head
static volatile uint16_t _last_dma_count; // count used with the previous DMA request

// Initialize the logger
int log_init(void)
{
	// Initialize the queue
	_queue_tail = 0;
	_queue_head = 0;
	_last_dma_count = 0;
	return HAL_OK;
}

// Circular Queue Space Available
// If head == tail, empty, queue size available
// If head > tail, available = size - (head-tail)
// If tail > head, available = tail - head
// This gives us an exact number
int queue_space_available(int head, int tail)
{
	if(head > tail)
		return LOG_DMA_BUFFER_SIZE - (head - tail) -1;
	else
		return (tail - head) -1;
}

//=============================================================================
// DMA register notes
//=============================================================================
// DMA_CCRx   : Channel Configuration Register - 15 bit fields configuring the DMA channel
// DMA_CNDTRx : Number of bytes to be transferred (0 up to 65535). This register can only be written when the
//   channel is disabled. Once the channel is enabled, this register is read-only, indicating the
//   remaining bytes to be transmitted. This register decrements after each DMA transfer.
// DMA_CMARx  : Base memory address to read from / write to
// Not sure if the CNDTRx register works a little differently with a circular buffer.



///*##-4- Wait for the end of the transfer ###################################*/
///*  Before starting a new communication transfer, you need to check the current
//    state of the peripheral; if it’s busy you need to wait for the end of current
//    transfer before starting a new one.
//    For simplicity reasons, this example is just waiting till the end of the
//    transfer, but application may perform other tasks while transfer operation
//    is ongoing. */
//while (HAL_UART_GetState(&UartHandle) != HAL_UART_STATE_READY)
//{
//}
//
///*##-5- Send the received Buffer ###########################################*/
//if (HAL_UART_Transmit_DMA(&UartHandle, (uint8_t *)aRxBuffer, RXBUFFERSIZE) != HAL_OK)
//{
//  /* Transfer error in transmission process */
//  Error_Handler();
//}

//=============================================================================
// If USART transmit DMA is stopped, restart it
// The DMA buffer needs to be configured for "Linear", not "Circular".
// As such, the DMA process needs to be broken into two parts when "wrapping"
// the end of the queue.
uint16_t restart_dma(void) {
//=============================================================================
	if(HAL_UART_GetState(&huart2) != HAL_UART_STATE_READY) return 0;

	uint16_t dma_bytes = (_queue_head <= _queue_tail)?    /* non-wrapped queue ? */
			(_queue_tail - _queue_head) :
			(LOG_DMA_BUFFER_SIZE - (_queue_head - _queue_tail));

	//if(!dma_bytes) return 0; // If queue empty, don't restart DMA

	uint16_t qty_to_send = (dma_bytes > (LOG_DMA_BUFFER_SIZE - _queue_head))?
			(LOG_DMA_BUFFER_SIZE - _queue_head):
			dma_bytes;

	_last_dma_count = qty_to_send;

	// This function call checks for busy and returns error if busy
	HAL_UART_Transmit_DMA(&huart2,(uint8_t *)&_usart2_tx_dma_buffer[_queue_head],_last_dma_count);

	return _last_dma_count;  // number of bytes just requested for DMA
}

//=============================================================================
// This function is the ONLY method for writing to the UART TX DMA buffer
// vsnprintf() writes to intermediate static buffer, then copied into circular DMA buffer
// This is the "lowest level" message API.  As timestamps, log level, and color become
//   implemented, this API will update to support these features.
// Prevent task switch while code proceeds though this function - a mutex comes to mind...
int logmsg(const char *format, ...) {
//=============================================================================
	// For version 2.1.0, add time stamps to messages
	// Allow "(265407628) " as an example prior to the message.
	// Size of the timestamp changes as the target gets larger and larger values for HAL_GetTick()

	//============================
	// Grab mutex (required for concurrent clients writing into buffer(s))
	//============================
	va_list arg_ptr;
	va_start(arg_ptr, format);

	// To limit the number of digits printed for timestamp, a "manual method" is needed to limit size of the timestamp.
	//   A printf() format string alone won't get us there.
	uint16_t ts_len = snprintf(_log_compose_buffer, LOG_MAX_TEXT, "(%lu) ",HAL_GetTick());
	// This will truncate the data written to the string as expected (with NULL termination)
	uint16_t log_length = vsnprintf(&_log_compose_buffer[ts_len], LOG_MAX_TEXT-ts_len, format, arg_ptr);
	// Convert length returned by vsnprintf() into actual length
	if(log_length >= LOG_MAX_TEXT) log_length = LOG_MAX_TEXT-1;
	// Add linefeed, \n, to the end
	log_length+=1;
	// Use log_length for total buffer data length
	log_length += ts_len;

	uint16_t log_space_available = (_queue_head <= _queue_tail)?    /* non-wrapped queue ? */
			(LOG_DMA_BUFFER_SIZE - (_queue_tail - _queue_head) -1) :
			(_queue_head - _queue_tail -1);
	if(log_length > log_space_available) {
		return -1; // not enough space for message
	}

	// Replace null termination on end of message with line feed '\n'
	_log_compose_buffer[log_length-1] = '\n';

	// We have enough space, copy intermediate buffer into circular DMA buffer
	// Instead of the slower byte by byte process, break the process into two memcpy() function calls (if required)
	if(_queue_tail + log_length >= LOG_DMA_BUFFER_SIZE) {
		// two memcpy() calls needed -- wrapping end of buffer
		uint16_t len_1 = LOG_DMA_BUFFER_SIZE - _queue_tail;
		memcpy(&_usart2_tx_dma_buffer[_queue_tail],&_log_compose_buffer,len_1); // copy first part
		memcpy(&_usart2_tx_dma_buffer,&_log_compose_buffer[len_1],log_length-len_1); // copy second part
		_queue_tail = log_length-len_1; // update queue tail
	} else {
		memcpy(&_usart2_tx_dma_buffer[_queue_tail],&_log_compose_buffer,log_length); // copy the whole thing -- no wrap
		_queue_tail += log_length; // update queue tail
	}

	// Log message is now in DMA queue, if not started, start the DMA transfer
	restart_dma();

	return log_length;  // return full log item length, not just text length
}


//=============================================================================
// In response to completing the last DMA transfer, restart the process if more data is available.
// Only after the DMA has completed can we move the "Head" index,
//   otherwise the "Head" index will get corrupted
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
//=============================================================================
	// DMA should be stopped, and the CMAR register should be pointing at the last item printed
	// The byte before this is the length.
	// Record the length and then Null the length byte in the buffer,
	//   indicating the string has been printed
	//uint8_t * ptext = (uint8_t *)huart->hdmatx->Instance->CMAR;
	//int dma_index = ptext - usart2_tx_dma_buffer;
	//uint8_t length = *(ptext -1); // record length of "log item"
	//*(ptext -1) = 0; // null this "log item" in the buffer
	// Look for next "log item"
	//ptext += length; // point to length field
	//length = *ptext; // next "log item" length
	//int dma_index = ptext - usart2_tx_dma_buffer; // Determine index of this "log item"
	// If length appears valid, and is still inside DMA buffer area, use it
	// to load next message.
//
//	if(length > 0 && length <= LOG_MAX_TEXT && dma_index <= LOG_LAST_DMA_INDEX) {
//		ptext++; // advance past length byte
//		// Start UART TX DMA with next "log item"
//		HAL_UART_Transmit_DMA(&huart2,(uint8_t *)ptext,length);
//	} else {
//		// Wrap to front of DMA buffer - check first position, does it appear valid / ready?
//		length = usart2_tx_dma_buffer[0];
//		if(length > 0 && length <= LOG_MAX_TEXT) {
//			// Start UART TX DMA with next "log item" - front of buffer
//			HAL_UART_Transmit_DMA(&huart2,(uint8_t *)&usart2_tx_dma_buffer[1],length);
//		}
//	}

	// If queue is empty
	if(_queue_head == _queue_tail) {
		// queue is empty
		return; // return w/o any further activity
	}

	// Advance the head
	_queue_head += _last_dma_count;
	if(_queue_head >= LOG_DMA_BUFFER_SIZE) {
		_queue_head -= LOG_DMA_BUFFER_SIZE;
	}

	_last_dma_count = 0;

	// If queue has more data/messages, setup the next USART TX DMA operation
	restart_dma();

	// Toggle the LED to signal UART transmit sent log item
	HAL_GPIO_TogglePin(LD2_GPIO_Port,LD2_Pin);
}

