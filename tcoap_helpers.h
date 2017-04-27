/**
 * tcoap_helpers.h
 *
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 * Copyright 2017 Serge Maslyakov. All rights reserved.
 *
 */


#ifndef __TCOAP_HELPERS_H
#define __TCOAP_HELPERS_H


#include <stdint.h>
#include "tcoap.h"


/**
 * @brief Find an option by its number
 *
 * @param options - list of options
 * @param opt_num - number of option
 *
 * @return pointer to the found option or NULL if option is absent
 */
const tcoap_option_data * tcoap_find_option_by_number(const tcoap_option_data * options, const uint16_t opt_num);


/**
 * @brief Extract an integer value from the buffer with network byte order
 *
 * @param src - pointer on the src buffer
 * @param dst - pointer on the dst buffer
 * @param len - length of data, from 1 till 4
 *
 */
void tcoap_extract_int_value(const uint8_t * const src, uint8_t * const dst, const uint32_t len);



#endif /* __TCOAP_HELPERS_H */
