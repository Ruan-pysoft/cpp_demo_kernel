extern "C" {

#include <stdint.h>

void outb(uint16_t port, uint8_t byte);
void outw(uint16_t port, uint16_t word);
void outdw(uint16_t port, uint32_t double_word);

uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t indw(uint16_t port);

}
