#ifndef DEFAULT_UART_H_18058512841B7290
#define DEFAULT_UART_H_18058512841B7290

void uart_init(void);
void uart_transmit(const void* data, unsigned length);
void uart_receive(void* data, unsigned length);

#endif
