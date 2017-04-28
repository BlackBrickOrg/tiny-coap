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
uint16_t tcoap_decode_szx_to_size(const uint8_t szx)
{
    switch (szx)
    {
        case 0:
            return TCOPA_BLOCK_SZX_VAL_0;

        case 1:
            return TCOPA_BLOCK_SZX_VAL_1;

        case 2:
            return TCOPA_BLOCK_SZX_VAL_2;

        case 3:
            return TCOPA_BLOCK_SZX_VAL_3;

        case 4:
            return TCOPA_BLOCK_SZX_VAL_4;

        case 5:
            return TCOPA_BLOCK_SZX_VAL_5;

        case 6:
            return TCOPA_BLOCK_SZX_VAL_6;

        default:
            return TCOPA_BLOCK_SZX_VAL_7;
    }
}


/**
 * @brief See description in the header file.
 *
 */
void tcoap_fill_block2_opt(tcoap_option_data * const option, const tcoap_blockwise_data * const bw, uint8_t * const value)
{
/*
 * Block Option Value
 *
 *  0
 *  0 1 2 3 4 5 6 7
 *  +-+-+-+-+-+-+-+-+
 *  |  NUM  |M| SZX |
 *  +-+-+-+-+-+-+-+-+
 *
 *  0                   1
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |          NUM          |M| SZX |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  0                   1                   2
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                   NUM                 |M| SZX |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

    option->num = TCOAP_BLOCK2_OPT;
    option->value = value;
    option->len = 1;
    option->next = NULL;

    value[0] = (bw->arr[0] & 0x0F);
    value[0] <<= 4;
    value[0] |= bw->fld.block_szx;
    value[0] |= bw->fld.more ? 8 : 0;

    if (bw->fld.num > 15) {
        option->len = 2;

        value[1] = value[0];

        value[0] = (bw->arr[0] >> 4);
        value[0] |= (bw->arr[1] & 0x0F);
    }

    if (bw->fld.num > 4095) {
        option->len = 3;

        value[2] = value[1];
        value[1] = value[0];

        value[0] = (bw->arr[1] >> 4);
        value[0] |= (bw->arr[2] & 0x0F);
    }
}


/**
 * @brief See description in the header file.
 *
 */
void tcoap_extract_block2_from_opt(const tcoap_option_data * const block2, tcoap_blockwise_data * const bw)
{
    bw->fld.num = 0;
    bw->fld.block_szx = 0;
    bw->fld.more = 0;

    switch (block2->len) {
        case 0:
            break;

        case 1:
            bw->fld.num = (block2->value[0] >> 4);
            bw->arr[3] = (block2->value[0] & 0x0F);
            break;

        case 2:
            bw->arr[1] = block2->value[0];
            bw->arr[0] = (block2->value[1] >> 4);

            bw->arr[3] = (block2->value[1] & 0x0F);
            break;

        case 3:
            bw->arr[2] = block2->value[0];
            bw->arr[1] = block2->value[1];
            bw->arr[0] = (block2->value[2] >> 4);

            bw->arr[3] = (block2->value[2] & 0x0F);
            break;

        default:
            break;
    }
}


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


