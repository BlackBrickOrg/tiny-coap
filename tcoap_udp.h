/**
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 *
 *
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Ver| T |  TKL  |      Code     |          Message ID           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |   Token (if any, TKL bytes) ...
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |   Options (if any) ...
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |1 1 1 1 1 1 1 1|    Payload (if any) ...
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */


#ifndef __TCOAP_UDP_H
#define __TCOAP_UDP_H


#include "tcoap.h"



/**
 * @brief Send a CoAP packet over UDP. Do not use it directly.
 *
 * @param handle - coap handle
 * @param dr - descriptor of request
 *
 * @return status of operation
 */
__tcoap_error tcoap_send_coap_request_udp(__tcoap_handle * const handle, const __tcoap_request_descriptor * const dr);



#endif /* __TCOAP_UDP_H */
