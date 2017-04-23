/**
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 *
 */


#include "tcoap_udp.h"
#include "tcoap_utils.h"


#define TCOAP_RESPONSE_CODE(buf)     ((buf)[1])


/**
 * @brief CoAP header data struct
 */
typedef struct {

    uint8_t tkl   : 4;        /* length of Token */
    uint8_t type  : 2;        /* type flag */
    uint8_t vers  : 2;        /* protocol version */

    uint8_t code  : 8;        /* request method (value 1--10) or response
                                 code (value 40-255) */
    uint16_t mid;             /* transaction id (network byte order!) */

} __tcoap_udp_header;



static void asemble_request(__tcoap_handle * const handle, __tcoap_data * const request, const __tcoap_request_descriptor * const dr);
static uint32_t parse_response(const __tcoap_data * const request, const __tcoap_data * const response);
static void asemble_ack(__tcoap_data * const ack, const __tcoap_data * const response);
static __tcoap_error waiting_ack(__tcoap_handle * const handle, const __tcoap_data * const request);



/**
 * @brief See description in the header file.
 *
 */
__tcoap_error tcoap_send_coap_request_udp(__tcoap_handle * const handle, const __tcoap_request_descriptor * const dr)
{
    __tcoap_error err;
    uint32_t resp_mask;
    __tcoap_result_data result;

    /* assembling packet */
    asemble_request(handle, &handle->request, dr);

    /* debug support */
    if (TCOAP_CHECK_STATUS(handle, TCOAP_DEBUG_ON)) {
        tcoap_debug_print_packet(handle, "coap >> ", handle->request.buf, handle->request.len);
    }

    /* sending packet */
    tcoap_tx_signal(handle, TCOAP_ROUTINE_PACKET_WILL_START);

    err = tcoap_tx_data(handle, handle->request.buf, handle->request.len);

    if (err != TCOAP_OK) {
        return err;
    }

    /* waiting ack if needed */
    resp_mask = TCOAP_RESP_EMPTY;
    if (dr->type == TCOAP_MESSAGE_CON) {

        TCOAP_SET_STATUS(handle, TCOAP_WAITING_RESP);

        err = waiting_ack(handle, &handle->request);

        TCOAP_RESET_STATUS(handle, TCOAP_WAITING_RESP);

        if (err != TCOAP_OK) {
            return err;
        }

        /* debug support */
        if (TCOAP_CHECK_STATUS(handle, TCOAP_DEBUG_ON)) {
            tcoap_debug_print_packet(handle, "coap << ", handle->response.buf, handle->response.len);
        }

        /* parsing incoming ack packet */
        resp_mask = parse_response(&handle->request, &handle->response);

        if (TCOAP_CHECK_RESP(resp_mask, TCOAP_RESP_ACK)) {

            tcoap_tx_signal(handle, TCOAP_ACK_DID_RECEIVE);

        } else if (TCOAP_CHECK_RESP(resp_mask, TCOAP_RESP_NRST)) {

            tcoap_tx_signal(handle, TCOAP_NRST_DID_RECEIVE);
            err = TCOAP_NRST_ANSWER;

            return err;
        } else if (TCOAP_CHECK_RESP(resp_mask, TCOAP_RESP_INVALID_PACKET)) {

            tcoap_tx_signal(handle, TCOAP_WRONG_PACKET_DID_RECEIVE);
            err = TCOAP_NO_ACK_ERROR;

            return err;
        }
    }

    /* waiting response if needed */
    if (dr->response_callback != NULL) {

        if (dr->type != TCOAP_MESSAGE_CON || !TCOAP_CHECK_RESP(resp_mask, TCOAP_RESP_PIGGYBACKED)) {

            handle->response.len = 0;
            TCOAP_SET_STATUS(handle, TCOAP_WAITING_RESP);

            /* waiting either data arriving or timeout expiring */
            err = tcoap_wait_event(handle, TCOAP_RESP_TIMEOUT_MS);

            TCOAP_RESET_STATUS(handle, TCOAP_WAITING_RESP);

            if (err != TCOAP_OK) {
                return err;
            }

            /* debug support */
            if (TCOAP_CHECK_STATUS(handle, TCOAP_DEBUG_ON)) {
                tcoap_debug_print_packet(handle, "rcv coap << ", handle->response.buf, handle->response.len);
            }

            resp_mask = parse_response(&handle->request, &handle->response);

            if (TCOAP_CHECK_RESP(resp_mask, TCOAP_RESP_INVALID_PACKET)) {

                tcoap_tx_signal(handle, TCOAP_WRONG_PACKET_DID_RECEIVE);
                err = TCOAP_NO_RESP_ERROR;

                return err;
            } else if (TCOAP_CHECK_RESP(resp_mask, TCOAP_RESP_NRST)) {

                tcoap_tx_signal(handle, TCOAP_NRST_DID_RECEIVE);
                err = TCOAP_NRST_ANSWER;

                return err;
            }
        }

        /* We are using the same request buffer for storing incoming options, bcoz
         * outgoing packet is not needed already. It allows us to save ram-memory.
         */
        err = decoding_options(&handle->response,
                (__tcoap_option_data *)handle->request.buf,
                ((handle->response.buf[0] & 0x0F) + 4),
                &handle->request.len);

        if (err == TCOAP_WRONG_OPTIONS_ERROR) {
            return err;
        }

        /* check the payload len */
        if (handle->response.len > handle->request.len) {
            result.payload.len = handle->response.len - handle->request.len;
            result.payload.buf = handle->response.buf + handle->request.len;
        } else {
            result.payload.len = 0;
            result.payload.buf = NULL;
        }

        result.resp_code = TCOAP_RESPONSE_CODE(handle->response.buf);
        result.options = err == TCOAP_NO_OPTIONS_ERROR ? NULL : (__tcoap_option_data *)handle->request.buf;

        dr->response_callback(dr, &result);

        /* debug support */
        if (TCOAP_CHECK_STATUS(handle, TCOAP_DEBUG_ON)) {
            tcoap_debug_print_options(handle, "coap opt << ", result.options);
            tcoap_debug_print_payload(handle, "coap pld << ", &result.payload);
        }

        /* send ACK back if needed */
        if (TCOAP_CHECK_RESP(resp_mask, TCOAP_RESP_NEED_SEND_ACK)) {

            asemble_ack(&handle->request, &handle->response);
            tcoap_tx_signal(handle, TCOAP_TX_ACK_PACKET);

            err = tcoap_tx_data(handle, handle->request.buf, handle->request.len);
        }
    }

    return err;
}


/**
 * @brief Assemble CoAP over UDP request.
 *
 * @param handle - coap handle
 * @param request - data struct for storing request
 * @param dr - descriptor of request
 *
 */
static void asemble_request(__tcoap_handle * const handle, __tcoap_data * const request, const __tcoap_request_descriptor * const dr)
{
    __tcoap_udp_header header;

    request->len = sizeof(__tcoap_udp_header);

    /* assemble header */
    header.vers = TCOAP_DEFAULT_VERSION;
    header.type = dr->type;
    header.code = dr->code;
    header.tkl = dr->tkl;
    header.mid = tcoap_get_message_id(handle);

    /* assemble token */
    if (dr->tkl) {
        tcoap_fill_token(handle, request->buf + request->len, dr->tkl);
        request->len += dr->tkl;
    }

    /* assemble options */
    if (dr->options != NULL) {
        request->len += encoding_options(request->buf + request->len, dr->options);
    }

    /* assemble payload */
    if (dr->payload.len) {
        request->len += fill_payload(request->buf + request->len, &dr->payload);
    }

    /* copy header */
    TCOAP_MEM_COPY(request->buf, &header, sizeof(__tcoap_udp_header));
}


/**
 * @brief Parse CoAP response (it may be either an ACK response or separate response)
 *
 * @param request - pointer on outgoing packet data
 * @param response - pointer on incoming packet data
 *
 * @return bit mask of results parsing (see __tcoap_parsing_result_t)
 */
static uint32_t parse_response(const __tcoap_data * const request, const __tcoap_data * const response)
{
    /**
     * 4.2.  Messages Transmitted Reliably
     * ...
     * ...
     * The Acknowledgement message MUST echo the Message ID of
     * the Confirmable message and MUST carry a response or be Empty (see
     * Sections 5.2.1 and 5.2.2).  The Reset message MUST echo the Message
     * ID of the Confirmable message and MUST be Empty.
     */

    __tcoap_udp_header resp_header;
    __tcoap_udp_header req_header;
    uint32_t resp_mask;

    /* check on header */
    if (response->len > 3) {

        resp_mask = TCOAP_RESP_EMPTY;
        TCOAP_MEM_COPY(&resp_header, response->buf, sizeof(__tcoap_udp_header));
        TCOAP_MEM_COPY(&req_header, request->buf, sizeof(__tcoap_udp_header));

        /* do fast checking */
        if (resp_header.vers != req_header.vers) {
            goto return_err_label;
        }

        /* do checking on the type of message */
        switch (resp_header.type) {

            case TCOAP_MESSAGE_ACK:
                TCOAP_SET_RESP(resp_mask, TCOAP_RESP_ACK);

                if (resp_header.mid != req_header.mid) {
                    goto return_err_label;
                }

                if (resp_header.code != TCOAP_CODE_EMPTY_MSG) {
                    TCOAP_SET_RESP(resp_mask, TCOAP_RESP_PIGGYBACKED);
                } else {
                    if (!resp_header.tkl && response->len == 4) {
                        return resp_mask;
                    } else {
                        goto return_err_label;
                    }
                }
                break;

            case TCOAP_MESSAGE_CON:
                TCOAP_SET_RESP(resp_mask, TCOAP_RESP_SEPARATE);
                TCOAP_SET_RESP(resp_mask, TCOAP_RESP_NEED_SEND_ACK);
                break;

            case TCOAP_MESSAGE_NON:
                TCOAP_SET_RESP(resp_mask, TCOAP_RESP_SEPARATE);
                break;

            case TCOAP_MESSAGE_RST:
                if (resp_header.code == TCOAP_CODE_EMPTY_MSG && !resp_header.tkl && response->len == 4) {
                    TCOAP_SET_RESP(resp_mask, TCOAP_RESP_NRST);
                    return resp_mask;
                } else {
                    goto return_err_label;
                }

            default:
                goto return_err_label;
        }

        /* if it is a separate response (msg id's should not be equals) */
        if (!TCOAP_CHECK_RESP(resp_mask, TCOAP_RESP_ACK)) {
            if (resp_header.mid == req_header.mid) {
                goto return_err_label;
            }
        }

        /* tkl's should be equals */
        if (resp_header.tkl != req_header.tkl) {
            goto return_err_label;
        }

        /* check length of msg */
        if (response->len < (uint32_t)(4 + resp_header.tkl)) {
            goto return_err_label;
        }

        /* check tokens */
        if (!TCOAP_MEM_CMP(response->buf + 4, request->buf + 4, resp_header.tkl)) {
            goto return_err_label;
        }

        /* check code */
        if (TCOAP_EXTRACT_CLASS(resp_header.code) != TCOAP_SUCCESS_CLASS
                && TCOAP_EXTRACT_CLASS(resp_header.code) != TCOAP_BAD_REQUEST_CLASS
                && TCOAP_EXTRACT_CLASS(resp_header.code) != TCOAP_SERVER_ERR_CLASS) {
            goto return_err_label;
        }

        if (TCOAP_EXTRACT_CLASS(resp_header.code) == TCOAP_SUCCESS_CLASS) {
            TCOAP_SET_RESP(resp_mask, TCOAP_RESP_SUCCESS_CODE);
        } else {
            TCOAP_SET_RESP(resp_mask, TCOAP_RESP_FAILURE_CODE);
        }

        /* packet is valid */
        return resp_mask;
    }

/***********/
return_err_label:
/***********/

    resp_mask = TCOAP_RESP_INVALID_PACKET;
    return resp_mask;
}


/**
 * @brief Assemble ACK packet
 *
 * @param ack - data where was stored ACK packet
 * @param response - the response on the basis of which will be assemble ACK packet
 */
static void asemble_ack(__tcoap_data * const ack, const __tcoap_data * const response)
{
    __tcoap_udp_header ack_header;

    /* get header from incoming packet */
    TCOAP_MEM_COPY(&ack_header, response, sizeof(__tcoap_udp_header));

    /* assemble header */
    ack_header.type = TCOAP_MESSAGE_ACK;
    ack_header.code = TCOAP_CODE_EMPTY_MSG;
    ack_header.tkl = 0;

    /* copy header */
    TCOAP_MEM_COPY(ack->buf, &ack_header, sizeof(__tcoap_udp_header));
    ack->len = sizeof(__tcoap_udp_header);
}



/**
 * @brief Waiting ACK functionality (retransmission and etc)
 *
 * @param handle - coap handle
 * @param request - outgoing packet
 *
 * @return result of operation
 */
static __tcoap_error waiting_ack(__tcoap_handle * const handle, const __tcoap_data * const request)
{
    __tcoap_error err;
    uint32_t retransmition;

    retransmition = 0;

    do {

        err = tcoap_wait_event(handle, retransmition * ((TCOAP_ACK_TIMEOUT_MS * TCOAP_ACK_RANDOM_FACTOR) / 100) + TCOAP_ACK_TIMEOUT_MS);

        if (err == TCOAP_TIMEOUT_ERROR) {

            if (retransmition < TCOAP_MAX_RETRANSMIT) {
                /* retransmission */
                tcoap_tx_signal(handle, TCOAP_TX_RETR_PACKET);

                /* debug support */
                if (TCOAP_CHECK_STATUS(handle, TCOAP_DEBUG_ON)) {
                    tcoap_debug_print_packet(handle, "coap retr >> ", handle->request.buf, handle->request.len);
                }

                retransmition++;
                err = tcoap_tx_data(handle, request->buf, request->len);

                if (err != TCOAP_OK) {
                    break;
                }
            } else {
                break;
            }
        } else {
            break;
        }
    } while (1);

    return err;
}

