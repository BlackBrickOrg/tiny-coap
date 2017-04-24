/**
 * tcoap_tcp.c
 *
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 * Copyright 2017 Serge Maslyakov. All rights reserved.
 *
 */


#include "tcoap_tcp.h"
#include "tcoap_utils.h"



#define TCOAP_MIN_TCP_HEADER_LEN     2u

#define TCOAP_TCP_LEN_1BYTE          13
#define TCOAP_TCP_LEN_2BYTES         14
#define TCOAP_TCP_LEN_4BYTES         15

#define TCOAP_TCP_LEN_MIN            13
#define TCOAP_TCP_LEN_MED            269
#define TCOAP_TCP_LEN_MAX            65805


/**
 * Auxiliary data structures
 *
 */
typedef union {

    struct {
        uint8_t tkl  : 4;         /* length of Token */
        uint8_t len  : 4;         /* length of Options & Payload */
    } fields;

    uint8_t byte;
} tcoap_tcp_len_header;


typedef struct {

    tcoap_tcp_len_header len_header;

    uint8_t code;
    uint32_t data_len;

} tcoap_tcp_header;



static void asemble_request(tcoap_handle * const handle, tcoap_data * const request, const tcoap_request_descriptor * const reqd);
static uint32_t parse_response(const tcoap_data * const request, const tcoap_data * const response, uint32_t * const options_shift);
static uint32_t extract_payload_length(tcoap_tcp_header * const header, const uint8_t * const buf);
static void shift_data(uint8_t * dst, const uint8_t *src, uint32_t len);



/**
 * @brief See description in the header file.
 *
 */
tcoap_error tcoap_send_coap_request_tcp(tcoap_handle * const handle, const tcoap_request_descriptor * const reqd)
{
    tcoap_error err;
    uint32_t resp_mask;
    uint32_t option_start_idx;
    tcoap_result_data result;

    /* assembling packet */
    asemble_request(handle, &handle->request, reqd);

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

    /* waiting response if needed */
    resp_mask = TCOAP_RESP_EMPTY;
    if (reqd->response_callback != NULL) {

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
            tcoap_debug_print_packet(handle, "coap << ", handle->response.buf, handle->response.len);
        }

        /* parsing incoming packet */
        resp_mask = parse_response(&handle->request, &handle->response, &option_start_idx);

        if (TCOAP_CHECK_RESP(resp_mask, TCOAP_RESP_INVALID_PACKET)) {

            tcoap_tx_signal(handle, TCOAP_WRONG_PACKET_DID_RECEIVE);
            err = TCOAP_NO_RESP_ERROR;

            return err;
        } else if (TCOAP_CHECK_RESP(resp_mask, TCOAP_RESP_NRST)) {

            tcoap_tx_signal(handle, TCOAP_NRST_DID_RECEIVE);
            err = TCOAP_NRST_ANSWER;

            return err;
        }

        /* We are using the same request buffer for storing incoming options, bcoz
         * outgoing packet is not needed already. It allows us to save ram-memory.
         */
        err = decoding_options(&handle->response,
                (tcoap_option_data *)handle->request.buf,
                option_start_idx,
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

        /* response_code_idx = option_start_idx - (handle->response.buf[0] & 0x0f) - 1 */
        result.resp_code = handle->response.buf[option_start_idx - (handle->response.buf[0] & 0x0f) - 1];
        result.options = err == TCOAP_NO_OPTIONS_ERROR ? NULL : (tcoap_option_data *)handle->request.buf;

        reqd->response_callback(reqd, &result);

        /* debug support */
        if (TCOAP_CHECK_STATUS(handle, TCOAP_DEBUG_ON)) {
            tcoap_debug_print_options(handle, "coap opt << ", result.options);
            tcoap_debug_print_payload(handle, "coap pld << ", &result.payload);
        }
    }

    return err;
}


/**
 * @brief Assemble CoAP over TCP request.
 *
 * @param handle - coap handle
 * @param request - data struct for storing request
 * @param reqd - descriptor of request
 *
 */
static void asemble_request(tcoap_handle * const handle, tcoap_data * const request, const tcoap_request_descriptor * const reqd)
{
    uint32_t options_shift;
    uint32_t options_len;
    tcoap_tcp_len_header header;

/**
  * CoAP over TCP has a header with variable length. Therefore we should calculate
  * length of Options & Payload before assembling the header.
  * At first we will try to predict length of header.
  * We should be shift a data, if we will predict wrong length of header.
  *
  */
    options_len = 0;
    options_shift = TCOAP_MIN_TCP_HEADER_LEN + reqd->tkl;

    if (reqd->payload.len > 10) {
        options_shift += 1;
    }

    /* assemble options */
    if (reqd->options != NULL) {
        options_len += encoding_options(request->buf + options_shift, reqd->options);
    }

    /* assemble header */
    request->len = options_len + (reqd->payload.len ? reqd->payload.len + 1 : 0);
    header.fields.tkl = reqd->tkl;

    if (request->len < TCOAP_TCP_LEN_MIN) {

        header.fields.len = request->len;

        request->buf[0] = header.byte;
        request->buf[1] = reqd->code;

        /* check on shift data */
        if (options_shift > (TCOAP_MIN_TCP_HEADER_LEN + reqd->tkl)) {
            shift_data(request->buf + reqd->tkl + TCOAP_MIN_TCP_HEADER_LEN, request->buf + options_shift, options_len);
        }

        request->len = 2;

    } else if (request->len < TCOAP_TCP_LEN_MED) {

        header.fields.len = TCOAP_TCP_LEN_1BYTE;

        request->buf[0] = header.byte;
        request->buf[1] = request->len - TCOAP_TCP_LEN_MIN;

        /* check on shift data */
        if (options_shift > reqd->tkl + TCOAP_MIN_TCP_HEADER_LEN + 1) {

            shift_data(request->buf + reqd->tkl + TCOAP_MIN_TCP_HEADER_LEN + 1,
                    request->buf + options_shift, options_len);

        } else if (options_shift < reqd->tkl + TCOAP_MIN_TCP_HEADER_LEN + 1) {

            shift_data(request->buf + options_len + reqd->tkl + TCOAP_MIN_TCP_HEADER_LEN,
                    request->buf + options_shift + options_len - 1, options_len);
        }

        request->buf[2] = reqd->code;
        request->len = 3;

    } else if (request->len < TCOAP_TCP_LEN_MAX) {

        header.fields.len = TCOAP_TCP_LEN_2BYTES;
        request->buf[0] = header.byte;

        /* check on shift data */
        if (options_shift > reqd->tkl + TCOAP_MIN_TCP_HEADER_LEN + 2) {

            shift_data(request->buf + reqd->tkl + TCOAP_MIN_TCP_HEADER_LEN + 2,
                    request->buf + options_shift, options_len);

        } else if (options_shift < reqd->tkl + TCOAP_MIN_TCP_HEADER_LEN + 2) {

            shift_data(request->buf + options_len + reqd->tkl + TCOAP_MIN_TCP_HEADER_LEN + 1,
                    request->buf + options_shift + options_len - 1, options_len);
        }

        request->buf[1] = (request->len - TCOAP_TCP_LEN_MED) >> 8;
        request->buf[2] = (request->len - TCOAP_TCP_LEN_MED);
        request->buf[3] = reqd->code;
        request->len = 4;

    } else {

        header.fields.len = TCOAP_TCP_LEN_4BYTES;
        request->buf[0] = header.byte;

        /* check on shift data */
        if (options_shift > reqd->tkl + TCOAP_MIN_TCP_HEADER_LEN + 4) {

            shift_data(request->buf + reqd->tkl + TCOAP_MIN_TCP_HEADER_LEN + 1,
                    request->buf + options_shift, options_len);

        } else if (options_shift < reqd->tkl + TCOAP_MIN_TCP_HEADER_LEN + 4) {

            shift_data(request->buf + options_len + reqd->tkl + TCOAP_MIN_TCP_HEADER_LEN + 3,
                    request->buf + options_shift + options_len - 1, options_len);
        }

        request->buf[1] = (request->len - TCOAP_TCP_LEN_MAX) >> 24;
        request->buf[2] = (request->len - TCOAP_TCP_LEN_MAX) >> 16;
        request->buf[3] = (request->len - TCOAP_TCP_LEN_MAX) >> 8;
        request->buf[4] = (request->len - TCOAP_TCP_LEN_MAX);
        request->buf[5] = reqd->code;
        request->len = 6;
    }

    /* assemble token */
    if (reqd->tkl) {
        tcoap_fill_token(handle, request->buf + request->len, reqd->tkl);
        request->len += reqd->tkl;
    }

    request->len += options_len;

    /* assemble payload */
    if (reqd->payload.len) {
        request->len += fill_payload(request->buf + request->len, &reqd->payload);
    }
}


/**
 * @brief Parse CoAP response
 *
 * @param request - pointer on outgoing packet
 * @param response - pointer on incoming packet
 * @param options_shift - in this variable will be stored start of options index
 *
 * @return bit mask of parsing results, see 'tcoap_parsing_result'
 */
static uint32_t parse_response(const tcoap_data * const request, const tcoap_data * const response, uint32_t * const options_shift)
{
    tcoap_tcp_header resp_header;
    tcoap_tcp_header req_header;

    uint32_t resp_mask;
    uint32_t resp_idx;
    uint32_t req_idx;

    /* checking header */
    if (response->len > 1) {
        resp_mask = TCOAP_RESP_SEPARATE;
        resp_idx = 0;
        req_idx = 0;

        resp_header.len_header.byte = response->buf[resp_idx++];
        req_header.len_header.byte = request->buf[req_idx++];

        /* fast checking tkl */
        if (resp_header.len_header.fields.tkl != req_header.len_header.fields.tkl) {
            goto return_err_label;
        }

        resp_idx += extract_payload_length(&resp_header, response->buf + resp_idx);
        req_idx += extract_payload_length(&req_header, request->buf + req_idx);

        /* check length */
        if ((resp_header.data_len + resp_header.len_header.fields.tkl + resp_idx + 1) >= response->len) {
            goto return_err_label;
        }

        /* get code */
        resp_header.code = response->buf[resp_idx++];

        /* check code */
        if (TCOAP_EXTRACT_CLASS(resp_header.code) != TCOAP_SUCCESS_CLASS
                && TCOAP_EXTRACT_CLASS(resp_header.code) != TCOAP_BAD_REQUEST_CLASS
                && TCOAP_EXTRACT_CLASS(resp_header.code) != TCOAP_SERVER_ERR_CLASS
                && TCOAP_EXTRACT_CLASS(resp_header.code) != TCOAP_TCP_SIGNAL_CLASS) {
            goto return_err_label;
        }

        /* check token */
        if (resp_header.len_header.fields.tkl) {
            if (!mem_cmp(response->buf + resp_idx, request->buf + req_idx + 1, resp_header.len_header.fields.tkl)) {
                goto return_err_label;
            }
        }

        if (TCOAP_EXTRACT_CLASS(resp_header.code) == TCOAP_SUCCESS_CLASS) {
            TCOAP_SET_RESP(resp_mask, TCOAP_RESP_SUCCESS_CODE);
        } else if (TCOAP_EXTRACT_CLASS(resp_header.code) == TCOAP_TCP_SIGNAL_CLASS) {
            TCOAP_SET_RESP(resp_mask, TCOAP_RESP_TCP_SIGNAL_CODE);
        } else {
            TCOAP_SET_RESP(resp_mask, TCOAP_RESP_FAILURE_CODE);
        }

        /* packet is valid */
        *options_shift = response->len - resp_header.data_len;
        return resp_mask;
    }

/***********/
return_err_label:
/***********/

    *options_shift = 0;
    resp_mask = TCOAP_RESP_INVALID_PACKET;
    return resp_mask;
}


/**
 * @brief Extract length of data from header for TCP packet
 *
 * @param header - pointer on 'tcoap_tcp_header'
 * @param buf - pointer on packet buffer
 *
 * @return shift for length
 */
static uint32_t extract_payload_length(tcoap_tcp_header * const header, const uint8_t * const buf)
{
    uint32_t idx;

    idx = 0;

    switch (header->len_header.fields.len) {
        case TCOAP_TCP_LEN_1BYTE:
            header->data_len = buf[idx++] + TCOAP_TCP_LEN_MIN;
            break;

        case TCOAP_TCP_LEN_2BYTES:
            header->data_len = buf[idx++];
            header->data_len <<= 8;
            header->data_len |= buf[idx++];
            header->data_len += TCOAP_TCP_LEN_MED;
            break;

        case TCOAP_TCP_LEN_4BYTES:
            header->data_len = buf[idx++];
            header->data_len <<= 8;
            header->data_len |= buf[idx++];
            header->data_len <<= 8;
            header->data_len |= buf[idx++];
            header->data_len <<= 8;
            header->data_len |= buf[idx++];
            header->data_len += TCOAP_TCP_LEN_MAX;
            break;

        default:
            header->data_len = header->len_header.fields.len;
            break;
    }

    return idx;
}


/**
 * @brief Shift the data in the packet if we did predict a wrong length
 *
 * @param dst - pointer on new position of data
 * @param src - pointer on current position of data
 * @param len - length of the shift
 */
static void shift_data(uint8_t * dst, const uint8_t *src, uint32_t len)
{
    if (dst < src) {
        while (len--) *dst++ = *src++;
    } else {
        while (len--) *dst-- = *src--;
    }
}

