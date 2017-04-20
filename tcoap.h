/**
 * Author: Serge Maslyakov, rusoil.9@gmail.com
 *
 * Acknowledgement:
 *
 * 1) libcoap       https://github.com/obgm/libcoap
 * 2) lobaro-coap   https://github.com/Lobaro/lobaro-coap
 * 3) californium   https://github.com/eclipse/californium
 *
 * Aims:
 *
 * Implementation of CoAP for mcu with ram 1-4 kB via GSM/NB-IoT.
 * Assumed that device is client and it initializes data exchange with server.
 *
 * 1. Assemble a packet by provided user data
 * 2. Parse an incoming packet and invoking callbacks to user code
 *
 */


#ifndef __TCOAP_H
#define __TCOAP_H



#include <stdint.h>
#include <stdbool.h>


#ifndef NUUL
#define NULL ((void *)0)
#endif


#define TCOAP_DEFAULT_VERSION           1
#define TCOAP_CODE(CLASS,CODE)          (int)((CLASS<<5)|CODE)
#define TCOAP_EXTRACT_CLASS(c)          (int)((c)>>5)

#ifndef TCOAP_RESP_TIMEOUT_MS
#define TCOAP_RESP_TIMEOUT_MS           9000
#endif /* TCOAP_RESP_TIMEOUT_MS */

#ifndef TCOAP_ACK_TIMEOUT_MS
#define TCOAP_ACK_TIMEOUT_MS            5000
#endif /* TCOAP_ACK_TIMEOUT_MS */

#ifndef TCOAP_MAX_RETRANSMIT
#define TCOAP_MAX_RETRANSMIT            3
#endif /* TCOAP_MAX_RETRANSMIT */

#ifndef TCOAP_ACK_RANDOM_FACTOR
#define TCOAP_ACK_RANDOM_FACTOR         130       /* 1.3 -> 130 to rid from float */
#endif /* TCOAP_ACK_RANDOM_FACTOR */

#ifndef TCOAP_MAX_PDU_SIZE
#define TCOAP_MAX_PDU_SIZE              96        /* maximum size of a CoAP PDU */
#endif /* TCOAP_MAX_PDU_SIZE */


#define TCOAP_TCP_URI_SCHEME            "coap+tcp"
#define TCOAP_TCP_SECURE_URI_SCHEME     "coaps+tcp"

#define TCOAP_TCP_DEFAULT_PORT          5683
#define TCOAP_TCP_DEFAULT_SECURE_PORT   5684


#define TCOAP_UDP_URI_SCHEME            "coap"
#define TCOAP_UDP_SECURE_URI_SCHEME     "coaps"

#define TCOAP_UDP_DEFAULT_PORT          5683
#define TCOAP_UDP_DEFAULT_SECURE_PORT   5684


typedef enum {

    TCOAP_OK = 0,
    TCOAP_BUSY_ERROR,
    TCOAP_PARAM_ERROR,

    TCOAP_NO_FREE_MEM_ERROR,
    TCOAP_TIMEOUT_ERROR,
    TCOAP_NRST_ANSWER,
    TCOAP_NO_ACK_ERROR,
    TCOAP_NO_RESP_ERROR,

    TCOAP_RX_BUFF_FULL_ERROR,
    TCOAP_WRONG_STATE_ERROR,

    TCOAP_NO_OPTIONS_ERROR,
    TCOAP_WRONG_OPTIONS_ERROR

} __tcoap_error;


typedef enum {

    TCOAP_ROUTINE_PACKET_WILL_START = 0,
    TCOAP_ROUTINE_PACKET_DID_FINISH,

    TCOAP_TX_RETR_PACKET,
    TCOAP_TX_ACK_PACKET,

    TCOAP_ACK_DID_RECEIVE,
    TCOAP_NRST_DID_RECEIVE,
    TCOAP_WRONG_PACKET_DID_RECEIVE,

    TCOAP_RESPONSE_BYTE_DID_RECEIVE,
    TCOAP_RESPONSE_TO_LONG_ERROR,
    TCOAP_RESPONSE_DID_RECEIVE

} __tcoap_out_signal;


typedef enum {

    TCOAP_UDP = 0,
    TCOAP_TCP,
    TCOAP_SMS

} __tcoap_transport;


typedef enum {

    TCOAP_MESSAGE_CON = 0,   /* confirmable message (requires ACK/RST) */
    TCOAP_MESSAGE_NON = 1,   /* non-confirmable message (one-shot message) */
    TCOAP_MESSAGE_ACK = 2,   /* used to acknowledge confirmable messages */
    TCOAP_MESSAGE_RST = 3    /* indicates error in received messages */

} __tcoap_udp_message;


typedef enum {

    TCOAP_REQUEST_CLASS = 0,
    TCOAP_SUCCESS_CLASS = 2,
    TCOAP_BAD_REQUEST_CLASS = 4,
    TCOAP_SERVER_ERR_CLASS = 5,

    TCOAP_TCP_SIGNAL_CLASS = 7

} __tcoap_class;


typedef enum {

    TCOAP_CODE_EMPTY_MSG = TCOAP_CODE(0, 0),

    TCOAP_REQ_GET = TCOAP_CODE(TCOAP_REQUEST_CLASS, 1),
    TCOAP_REQ_POST = TCOAP_CODE(TCOAP_REQUEST_CLASS, 2),
    TCOAP_REQ_PUT = TCOAP_CODE(TCOAP_REQUEST_CLASS, 3),
    TCOAP_REQ_DEL = TCOAP_CODE(TCOAP_REQUEST_CLASS, 4),

    TCOAP_RESP_SUCCESS_OK_200 = TCOAP_CODE(TCOAP_SUCCESS_CLASS, 0),
    TCOAP_RESP_SUCCESS_CREATED_201 = TCOAP_CODE(TCOAP_SUCCESS_CLASS, 1),
    TCOAP_RESP_SUCCESS_DELETED_202 = TCOAP_CODE(TCOAP_SUCCESS_CLASS, 2),
    TCOAP_RESP_SUCCESS_VALID_203 = TCOAP_CODE(TCOAP_SUCCESS_CLASS, 3),
    TCOAP_RESP_SUCCESS_CHANGED_204 = TCOAP_CODE(TCOAP_SUCCESS_CLASS, 4),
    TCOAP_RESP_SUCCESS_CONTENT_205 = TCOAP_CODE(TCOAP_SUCCESS_CLASS, 5),

    TCOAP_RESP_ERROR_BAD_REQUEST_400 = TCOAP_CODE(TCOAP_BAD_REQUEST_CLASS, 0),
    TCOAP_RESP_ERROR_UNAUTHORIZED_401 = TCOAP_CODE(TCOAP_BAD_REQUEST_CLASS, 1),
    TCOAP_RESP_BAD_OPTION_402 = TCOAP_CODE(TCOAP_BAD_REQUEST_CLASS, 2),
    TCOAP_RESP_FORBIDDEN_403 = TCOAP_CODE(TCOAP_BAD_REQUEST_CLASS, 3),
    TCOAP_RESP_NOT_FOUND_404 = TCOAP_CODE(TCOAP_BAD_REQUEST_CLASS, 4),
    TCOAP_RESP_METHOD_NOT_ALLOWED_405 = TCOAP_CODE(TCOAP_BAD_REQUEST_CLASS, 5),
    TCOAP_RESP_METHOD_NOT_ACCEPTABLE_406 = TCOAP_CODE(TCOAP_BAD_REQUEST_CLASS, 6),
    TCOAP_RESP_PRECONDITION_FAILED_412 = TCOAP_CODE(TCOAP_BAD_REQUEST_CLASS, 12),
    TCOAP_RESP_REQUEST_ENTITY_TOO_LARGE_413 = TCOAP_CODE(TCOAP_BAD_REQUEST_CLASS, 13),
    TCOAP_RESP_UNSUPPORTED_CONTENT_FORMAT_415 = TCOAP_CODE(TCOAP_BAD_REQUEST_CLASS, 15),

    TCOAP_RESP_INTERNAL_SERVER_ERROR_500 = TCOAP_CODE(TCOAP_SERVER_ERR_CLASS, 0),
    TCOAP_RESP_NOT_IMPLEMENTED_501 = TCOAP_CODE(TCOAP_SERVER_ERR_CLASS, 1),
    TCOAP_RESP_BAD_GATEWAY_502 = TCOAP_CODE(TCOAP_SERVER_ERR_CLASS, 2),
    TCOAP_RESP_SERVICE_UNAVAILABLE_503 = TCOAP_CODE(TCOAP_SERVER_ERR_CLASS, 3),
    TCOAP_RESP_GATEWAY_TIMEOUT_504 = TCOAP_CODE(TCOAP_SERVER_ERR_CLASS, 4),
    TCOAP_RESP_PROXYING_NOT_SUPPORTED_505 = TCOAP_CODE(TCOAP_SERVER_ERR_CLASS, 5),

    TCOAP_TCP_SIGNAL_700 = TCOAP_CODE(TCOAP_TCP_SIGNAL_CLASS, 0),
    TCOAP_TCP_SIGNAL_CSM_701 = TCOAP_CODE(TCOAP_TCP_SIGNAL_CLASS, 1),
    TCOAP_TCP_SIGNAL_PING_702 = TCOAP_CODE(TCOAP_TCP_SIGNAL_CLASS, 2),
    TCOAP_TCP_SIGNAL_PONG_703 = TCOAP_CODE(TCOAP_TCP_SIGNAL_CLASS, 3),
    TCOAP_TCP_SIGNAL_RELEASE_704 = TCOAP_CODE(TCOAP_TCP_SIGNAL_CLASS, 4),
    TCOAP_TCP_SIGNAL_ABORT_705 = TCOAP_CODE(TCOAP_TCP_SIGNAL_CLASS, 5)

} __tcoap_packet_code;


/**
  * Critical = (optnum & 1)
  * UnSafe =   (optnum & 2)
  * NoCacheKey = ((optnum & 0x1e) == 0x1c)
  *
 */
typedef enum {

    TCOAP_IF_MATCH_OPT         = 1,
    TCOAP_URI_HOST_OPT         = 3,
    TCOAP_ETAG_OPT             = 4,
    TCOAP_IF_NON_MATCH_OPT     = 5,
    TCOAP_URI_PORT_OPT         = 7,
    TCOAP_LOCATION_PATH_OPT    = 8,
    TCOAP_URI_PATH_OPT         = 11,
    TCOAP_CONTENT_FORMAT_OPT   = 12,

    TCOAP_MAX_AGE_OPT          = 14,
    TCOAP_URI_QUERY_OPT        = 15,
    TCOAP_ACCEPT_OPT           = 17,
    TCOAP_LOCATION_QUERY_OPT   = 20,
    TCOAP_PROXY_URI_OPT        = 35,
    TCOAP_PROXY_SCHEME_OPT     = 39,
    TCOAP_SIZE1_OPT            = 60

} __tcoap_option;


typedef enum {

    TCOAP_TEXT_PLAIN = 0,   /* default value */
    TCOAP_TEXT_XML = 1,
    TCOAP_TEXT_CSV = 2,
    TCOAP_TEXT_HTML = 3,
    TCOAP_IMAGE_GIF = 21,
    TCOAP_IMAGE_JPEG = 22,
    TCOAP_IMAGE_PNG = 23,
    TCOAP_IMAGE_TIFF = 24,
    TCOAP_AUDIO_RAW = 25,
    TCOAP_VIDEO_RAW = 26,
    TCOAP_APPLICATION_LINK_FORMAT = 40,
    TCOAP_APPLICATION_XML = 41,
    TCOAP_APPLICATION_OCTET_STREAM = 42,
    TCOAP_APPLICATION_RDF_XML = 43,
    TCOAP_APPLICATION_SOAP_XML = 44,
    TCOAP_APPLICATION_ATOM_XML = 45,
    TCOAP_APPLICATION_XMPP_XML = 46,
    TCOAP_APPLICATION_EXI = 47,
    TCOAP_APPLICATION_FASTINFOSET = 48,
    TCOAP_APPLICATION_SOAP_FASTINFOSET = 49,
    TCOAP_APPLICATION_JSON = 50,
    TCOAP_APPLICATION_X_OBIX_BINARY = 51,
    TCOAP_APPLICATION_CBOR = 60

} __tcoap_media_type;


typedef struct __tcoap_option_data {

    uint16_t num;
    uint16_t len;
    uint8_t * value;   /* may be string/int/long */

    struct __tcoap_option_data * next;

} __tcoap_option_data;


typedef struct __tcoap_data {

    uint8_t * buf;
    uint32_t len;

} __tcoap_data;


typedef struct __tcoap_result_data {

    uint8_t resp_code;
    __tcoap_data payload;
    __tcoap_option_data * options;   /* NULL terminated linked list of options */

} __tcoap_result_data;


typedef struct __tcoap_request_descriptor {

    uint8_t type;
    uint8_t code;
    uint16_t tkl;

    __tcoap_data payload;            /* should not be NULL */
    __tcoap_option_data * options;   /* should be NULL if there are no options */

    /**
     * @brief Callback with results of request
     *
     * @param dr - pointer on the request data struct '__tcoap_request_descr'
     * @param result - pointer on result data struct '__tcoap_result_data'
     */
    void (* response_callback) (const struct __tcoap_request_descriptor * const dr, const struct __tcoap_result_data * const result);

} __tcoap_request_descriptor;


typedef struct __tcoap_handle {

    const char * name;
    uint16_t transport;

    uint16_t statuses_mask;

    __tcoap_data request;
    __tcoap_data response;

} __tcoap_handle;



/**
 * @brief Extern API that user should provide
 *
 */
extern __tcoap_error tcoap_tx_data(__tcoap_handle * const handle, const uint8_t *buf, const uint32_t len);
extern __tcoap_error tcoap_wait_event(__tcoap_handle * const handle, const uint32_t timeout_ms);
extern __tcoap_error tcoap_tx_signal(__tcoap_handle * const handle, const __tcoap_out_signal signal);

extern uint16_t tcoap_get_message_id(__tcoap_handle * const handle);
extern __tcoap_error tcoap_fill_token(__tcoap_handle * const handle, uint8_t *token, const uint32_t tkl);

extern void tcoap_debug_print_packet(__tcoap_handle * const handle, const char * msg, uint8_t *data, const uint32_t len);
extern void tcoap_debug_print_options(__tcoap_handle * const handle, const char * msg, const __tcoap_option_data * options);
extern void tcoap_debug_print_payload(__tcoap_handle * const handle, const char * msg, const __tcoap_data * const payload);

extern __tcoap_error tcoap_alloc_mem_block(uint8_t **block, const uint32_t min_len);
extern __tcoap_error tcoap_free_mem_block(uint8_t *block, const uint32_t min_len);

extern void mem_copy(void *dst, const void *src, uint32_t cnt);
extern bool mem_cmp(const void *dst, const void *src, uint32_t cnt);



/**
 * @brief Enable/disable CoAP debug.
 *        Also you should implement 'tcoap_debug_print..' methods in your code.
 *
 */
void tcoap_debug(__tcoap_handle * const handle, const bool enable);


/**
 * @brief Send CoAP request to the server
 *
 * @param handle - coap handle
 * @param dr - descriptor of request
 *
 * @return status of operation
 *
 */
__tcoap_error tcoap_send_coap_request(__tcoap_handle * const handle, const __tcoap_request_descriptor * const dr);


/**
 * @brief Receive a packet step-by-step (sequence of bytes).
 *        You may to use it if you communicate with server over serial port
 *        or you haven't a free mem for cumulative buffer. Detecting of the
 *        end of packet is a user responsibility (through byte-timeout).
 *
 * @param handle - coap handle
 * @param byte - received byte
 *
 * @return status of operation
 *
 */
__tcoap_error tcoap_rx_byte(__tcoap_handle * const handle, const uint8_t byte);


/**
 * @brief Receive whole packet
 *
 * @param handle - coap handle
 * @param buf - pointer on buffer with data
 * @param len - length of data
 *
 * @return status of operation
 *
 */
__tcoap_error tcoap_rx_packet(__tcoap_handle * const handle, const uint8_t * buf, const uint32_t len);


#endif /* __TCOAP_H */
