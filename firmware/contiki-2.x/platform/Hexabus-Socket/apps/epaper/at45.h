#ifndef AT45_H
#define AT45_H 1

#include <stdint.h>

#define AT45_CS_PORT PORTA
#define AT45_CS_DDR DDRA
#define AT45_CS PA7
#define AT45_MOSI_PORT PORTB
#define AT45_MOSI_DDR DDRB
#define AT45_MOSI PB5
#define AT45_MISO_PORT PORTB
#define AT45_MISO_DDR DDRB
#define AT45_MISO PB6
#define AT45_SCK_PORT PORTB
#define AT45_SCK_DDR DDRB
#define AT45_SCK PB7

#define AT45_PAGE_SIZE 528

struct at45_version_t {
  uint8_t data[4];
};

struct at45_page_t {
  uint8_t data[AT45_PAGE_SIZE];
};

void at45_init(void);
void at45_erase_all_pages(void);
uint8_t at45_status(void); 
uint8_t at45_is_ready(void);
uint8_t at45_wait_ready(void);
void at45_get_version(struct at45_version_t* version);
uint8_t at45_read_page(struct at45_page_t* page, uint16_t addr_page);
void at45_dump_page(struct at45_page_t* page);

/** low level routines below **/
uint8_t at45_read_page_to_buf_1(uint16_t addr_page);
uint8_t at45_write_from_buf_1(uint16_t addr_page);
void at45_read_from_buf_1(uint8_t * dst, uint16_t count, uint16_t addr);
void at45_write_to_buf_1(const uint8_t *src, uint16_t count, uint16_t addr);

#endif /* AT45_H */

