/**
 * tcoap_helpers.c
 *
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 * Copyright 2017 Serge Maslyakov. All rights reserved.
 *
 */


#include "tcoap_helpers.h"



/**
 * @brief See description in the header file.
 *
 */
const tcoap_option_data * tcoap_find_option_by_number(const tcoap_option_data * options, const uint16_t opt_num)
{
    do {

        if (options->num > opt_num) {
            break;
        }

        if (options->num == opt_num) {
            return options;
        }

        options = options->next;

    } while (options != NULL);

    return NULL;
}


/**
 * @brief See description in the header file.
 *
 */
void tcoap_extract_int_value(const uint8_t * const src, uint8_t * const dst, const uint32_t len)
{
    uint32_t idx = 0;

    switch(len) {

        case 4:
            dst[idx++] = src[3];
            //no break

        case 3:
            dst[idx++] = src[2];
            //no break

        case 2:
            dst[idx++] = src[1];
            //no break

        case 1:
            dst[idx] = src[0];
            //no break

        default:
            break;
    }
}

