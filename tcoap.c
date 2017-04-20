/**
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 */


#include "tcoap.h"

#include "tcoap_udp.h"
#include "tcoap_tcp.h"
#include "tcoap_utils.h"



static __tcoap_error init_coap_driver(__tcoap_handle * const handle, const __tcoap_request_descriptor * const dr);
static void deinit_coap_driver(__tcoap_handle *handle);



/**
 * @brief See description in the header file.
 *
 */
void tcoap_debug(__tcoap_handle * const handle, const bool enable)
{
    if (enable) {
        TCOAP_SET_STATUS(handle, TCOAP_DEBUG_ON);
    } else {
        TCOAP_RESET_STATUS(handle, TCOAP_DEBUG_ON);
    }
}


/**
 * @brief See description in the header file.
 *
 */
__tcoap_error tcoap_send_coap_request(__tcoap_handle * const handle, const __tcoap_request_descriptor * const dr)
{
    __tcoap_error err;

    /* check state */
    if (TCOAP_CHECK_STATUS(handle, TCOAP_SENDING_PACKET)) {
        return TCOAP_BUSY_ERROR;
    }

    TCOAP_SET_STATUS(handle, TCOAP_SENDING_PACKET);
    err = init_coap_driver(handle, dr);

    if (err == TCOAP_OK) {

        switch (handle->transport) {
            case TCOAP_UDP:
                err = tcoap_send_coap_request_udp(handle, dr);
                break;

            case TCOAP_TCP:
                err = tcoap_send_coap_request_tcp(handle, dr);
                break;

            case TCOAP_SMS:
            default:
                /* not supported yet */
                err = TCOAP_PARAM_ERROR;
                break;
        }
    }

    deinit_coap_driver(handle);

    TCOAP_RESET_STATUS(handle, TCOAP_SENDING_PACKET);
    tcoap_tx_signal(handle, TCOAP_ROUTINE_PACKET_DID_FINISH);

    return err;
}


/**
 * @brief See description in the header file.
 *
 */
__tcoap_error tcoap_rx_byte(__tcoap_handle * const handle, const uint8_t byte)
{
    if (TCOAP_CHECK_STATUS(handle, TCOAP_WAITING_RESP)) {

        if (handle->response.len < TCOAP_MAX_PDU_SIZE) {
            handle->response.buf[handle->response.len++] = byte;

            tcoap_tx_signal(handle, TCOAP_RESPONSE_BYTE_DID_RECEIVE);
            return TCOAP_OK;
        }

        return TCOAP_RX_BUFF_FULL_ERROR;
    }

    return TCOAP_WRONG_STATE_ERROR;
}


/**
 * @brief See description in the header file.
 *
 */
__tcoap_error tcoap_rx_packet(__tcoap_handle * const handle, const uint8_t * buf, const uint32_t len)
{
    if (TCOAP_CHECK_STATUS(handle, TCOAP_WAITING_RESP)) {

        mem_copy(handle->response.buf, buf, len < TCOAP_MAX_PDU_SIZE ? len : TCOAP_MAX_PDU_SIZE);
        handle->response.len = len;

        if (len < TCOAP_MAX_PDU_SIZE) {
            tcoap_tx_signal(handle, TCOAP_RESPONSE_DID_RECEIVE);
            return TCOAP_OK;
        }

        return TCOAP_RX_BUFF_FULL_ERROR;
    }

    return TCOAP_WRONG_STATE_ERROR;
}


/**
 * @brief Init CoAP driver
 *
 * @param handle - coap handle
 * @param dr - descriptor of request
 *
 * @return status of operation
 */
static __tcoap_error init_coap_driver(__tcoap_handle * const handle, const __tcoap_request_descriptor * const dr)
{
    __tcoap_error err;

    err = TCOAP_OK;

    handle->request.len = 0;
    handle->response.len = 0;

    if (dr->code == TCOAP_CODE_EMPTY_MSG && dr->tkl) {
        return TCOAP_PARAM_ERROR;
    }

    if (handle->request.buf == NULL) {
        err = tcoap_alloc_mem_block(&handle->request.buf, TCOAP_MAX_PDU_SIZE);

        if (err != TCOAP_OK) {
            return err;
        }
    }

    if (dr->type == TCOAP_MESSAGE_CON || dr->response_callback != NULL) {
        if (handle->response.buf == NULL) {
            err = tcoap_alloc_mem_block(&handle->response.buf, TCOAP_MAX_PDU_SIZE);
        }
    }

    return err;
}


/**
 * @brief Deinit CoAP driver
 *
 * @param handle - coap handle
 *
 */
static void deinit_coap_driver(__tcoap_handle *handle)
{
    if (handle->response.buf != NULL) {
        tcoap_free_mem_block(handle->response.buf, TCOAP_MAX_PDU_SIZE);
        handle->response.buf = NULL;
    }

    if (handle->request.buf != NULL) {
        tcoap_free_mem_block(handle->request.buf, TCOAP_MAX_PDU_SIZE);
        handle->request.buf = NULL;
    }

    handle->request.len = 0;
    handle->response.len = 0;
}


