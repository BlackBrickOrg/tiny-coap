/**
 * tcoap_utils.h
 *
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 * Copyright 2017 Serge Maslyakov. All rights reserved.
 *
 */


#ifndef __TCOAP_UTILS_H
#define __TCOAP_UTILS_H


#include "tcoap.h"


#ifdef __cplusplus
extern "C" {
#endif


#define TCOAP_CHECK_STATUS(h,s)      ((h)->statuses_mask & (s))
#define TCOAP_SET_STATUS(h,s)        ((h)->statuses_mask |= (s))
#define TCOAP_RESET_STATUS(h,s)      ((h)->statuses_mask &= ~(s))

#define TCOAP_CHECK_RESP(m,s)        ((m) & (s))
#define TCOAP_SET_RESP(m,s)          ((m) |= (s))
#define TCOAP_RESET_RESP(m,s)        ((m) = ~(s))



typedef enum {

     TCOAP_UNKNOWN         = (int) 0x0000,
     TCOAP_ALL_STATUSES    = (int) 0xffff,

     TCOAP_SENDING_PACKET  = (int) 0x0001,
     TCOAP_WAITING_RESP    = (int) 0x0002,

     TCOAP_DEBUG_ON        = (int) 0x0080

} tcoap_handle_status;


typedef enum {

    TCOAP_RESP_EMPTY            = (int) 0x00000000,

    TCOAP_RESP_ACK              = (int) 0x00000001,
    TCOAP_RESP_PIGGYBACKED      = (int) 0x00000002,
    TCOAP_RESP_NRST             = (int) 0x00000004,
    TCOAP_RESP_SEPARATE         = (int) 0x00000008,

    TCOAP_RESP_SUCCESS_CODE     = (int) 0x00000010,
    TCOAP_RESP_FAILURE_CODE     = (int) 0x00000020,
    TCOAP_RESP_TCP_SIGNAL_CODE  = (int) 0x00000020,

    TCOAP_RESP_NEED_SEND_ACK    = (int) 0x00000100,

    TCOAP_RESP_INVALID_PACKET   = (int) 0x80000000

} tcoap_parsing_result;



/**
 * @brief Encoding options and add it to the packet
 *
 * @param buf - pointer on packet buffer
 * @param option - pointer on first element of linked list of options. Must not be NULL.
 *
 * @return length of data that was added to the buffer
 */
uint32_t encoding_options(uint8_t * const buf, const tcoap_option_data * option);


/**
 * @brief Decoding options from response
 *
 * @param response - incoming packet
 * @param option - pointer on first element of linked list
 * @param opt_start_idx - index of options in the incoming packet
 * @param payload_start_idx - pointer on variable for storing idx of payload in the incoming packet
 *
 * @return status of operations
 */
tcoap_error decoding_options(const tcoap_data * const response,
        tcoap_option_data * option,
        const uint32_t const opt_start_idx,
        uint32_t * const payload_start_idx);


/**
 * @brief Add payload to the packet
 *
 * @param buf - pointer on packet buffer
 * @param payload - data with payload
 *
 * @return length of data that was added to the buffer
 */
uint32_t fill_payload(uint8_t * const buf, const tcoap_data * const payload);


#ifdef  __cplusplus
}
#endif


#endif /* __TCOAP_UTILS_H */
