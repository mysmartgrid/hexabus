#ifndef MEASUREMENT_INDEX_H
#define MEASUREMENT_INDEX_H 1

/***
 * this file provides an abstraction for the measurement index
 * the index maps (temp, hum)-pairs to the start page of the screen
 * to be displayed on the e-paper display.
 */

#include <stdint.h>
#include "at45.h"

#define AT45_END_OF_TABLE 3
#define AT45_TABLE_SUCCESS 1
#define AT45_FAILURE 0

struct index_entry_t {
  uint8_t temp;
  uint8_t hum;
  uint16_t page_idx;
};

static const uint16_t INDEX_ENTRIES_PER_PAGE = AT45_PAGE_SIZE/sizeof(struct index_entry_t);

uint8_t index_get_entry(struct index_entry_t* entry, uint16_t index);
uint8_t index_find_entry(struct index_entry_t* entry, uint8_t temp, uint8_t hum);
uint8_t index_get_first_entry(struct index_entry_t* entry);
uint8_t index_get_last_entry(struct index_entry_t* entry);

uint16_t index_get_special_screen(uint8_t id);

#endif /* MEASUREMENT_INDEX_H */

