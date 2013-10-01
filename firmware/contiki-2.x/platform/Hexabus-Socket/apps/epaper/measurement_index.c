#include "measurement_index.h"
#include "epd27_conf.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

uint8_t index_get_entry( struct index_entry_t* entry, uint16_t index) {
  struct at45_page_t* page = malloc(sizeof(uint8_t [AT45_PAGE_SIZE]));
  memset(page, 0x00, AT45_PAGE_SIZE);
  //char conversion_buffer[50];

  /* Calculate offset of the index-th entry */
  uint16_t index_in_page = index % INDEX_ENTRIES_PER_PAGE;
  uint16_t index_page = index/INDEX_ENTRIES_PER_PAGE;
// 
//      uart_puts_P("\r\n index: ");
//      itoa(index, conversion_buffer, 10);
//      uart_puts(conversion_buffer);
//      uart_puts_P("\r\n index_in_page: ");
//      itoa(index_in_page, conversion_buffer, 10);
//      uart_puts(conversion_buffer);
//      uart_puts_P("\r\n page: ");
//      itoa(index_page, conversion_buffer, 10);
//      uart_puts(conversion_buffer);
//
  if (! at45_read_page(page, index_page)) {
    free(page);
    return AT45_FAILURE;
  } else {
    uint16_t offset = (index_in_page * sizeof(struct index_entry_t));
    uint8_t* addr =(uint8_t*)(page) + offset;

    memcpy(entry, addr, sizeof(struct index_entry_t));
    entry->temp = entry->temp;
    entry->hum = entry->hum;
    entry->page_idx = entry->page_idx;
    if (entry->temp != 0xde && entry->hum != 0xad &&
        entry->page_idx != 0xbeef) 
    {
      free(page);
      return AT45_TABLE_SUCCESS;
    } else {
      free(page);
      return AT45_END_OF_TABLE;
    }
  }
}

uint8_t index_find_entry(struct index_entry_t* entry, uint8_t temp, uint8_t hum)
{
  struct at45_page_t* page = malloc(sizeof(uint8_t [AT45_PAGE_SIZE]));
	uint8_t result;

	for (uint16_t index_page = 0; ; index_page++) {
		memset(page, 0x00, sizeof(*page));
		if (!at45_read_page(page, index_page)) {
			result = AT45_FAILURE;
			goto exit;
		}
		for (uint16_t index_in_page = 0; index_in_page < INDEX_ENTRIES_PER_PAGE; index_in_page++) {
			memcpy(entry, page->data + index_in_page * sizeof(*entry), sizeof(*entry));
			if (entry->temp == 0xde && entry->hum == 0xad && entry->page_idx == 0xbeef) {
				result = AT45_END_OF_TABLE;
				goto exit;
			} else if (entry->temp == temp && entry->hum == hum) {
				result = AT45_TABLE_SUCCESS;
				goto exit;
			}
		}
	}
	result = AT45_END_OF_TABLE;

exit:
	free(page);
	return result;
}

uint8_t index_get_first_entry(struct index_entry_t* entry) {
  uint8_t errcode = AT45_TABLE_SUCCESS;
  if (((errcode = index_get_entry(entry, 0)) 
        != AT45_END_OF_TABLE)) {
    if (errcode == AT45_FAILURE) {
      return errcode;
    } else {
      return AT45_TABLE_SUCCESS;
    }
  }
  return errcode;
}

uint8_t index_get_last_entry(struct index_entry_t* entry) {
  uint16_t current_entry_idx = 0;
  uint8_t errcode = AT45_TABLE_SUCCESS;
  while (((errcode = index_get_entry(entry, current_entry_idx)) 
        != AT45_END_OF_TABLE)) {
    if (errcode == AT45_FAILURE) {
      return errcode;
    } 
    current_entry_idx++;
  }
  errcode = index_get_entry(entry, current_entry_idx - 1);
  return AT45_TABLE_SUCCESS;
}

uint16_t index_get_special_screen(uint8_t id)
{
	static uint16_t first_special_screen = 0;

	if (!first_special_screen) {
		struct at45_page_t* page = malloc(sizeof(uint8_t [AT45_PAGE_SIZE]));
		uint16_t last_page = 0;

		for (uint16_t index_page = 0; ; index_page++) {
			if (!at45_read_page(page, index_page)) {
				goto exit;
			}
			for (uint16_t index_in_page = 0; index_in_page < INDEX_ENTRIES_PER_PAGE; index_in_page++) {
				struct index_entry_t* entry;
				entry = (struct index_entry_t*) (page->data + index_in_page * sizeof(*entry));
				if (entry->temp == 0xde && entry->hum == 0xad && entry->page_idx == 0xbeef) {
					first_special_screen = last_page + PAGES_PER_SCREEN;
					goto exit;
				} else {
					last_page = entry->page_idx;
				}
			}
		}

exit:
		free(page);
	}

	if (first_special_screen == 0)
		return 0;

	return first_special_screen + id * PAGES_PER_SCREEN;
}
