### CoAP client library for embedded devices in C

Allows to add the CoAP functionality for embedded device.

#### Features

- very small memory consumption (in the common case it may be about of 200 bytes for both rx/tx buffers, you can tune a PDU size)

- implemented CoAP over UDP [rfc7252](https://tools.ietf.org/html/rfc7252)

- implemented CoAP over TCP [draft-coap-tcp-tls-07](https://tools.ietf.org/html/draft-ietf-core-coap-tcp-tls-07)

- retransmition/acknowledgment functionality

- parsing of responses. Received data will be return an user via callback.

- helpers for block-wise mode. The block-wise mode is located on higher layer than this implementation.
  See [wiki](https://github.com/Mozilla9/tiny-coap/wiki/Block-wise-mode-example) for example.


#### How to send CoAP request to server

1) Include `tcoap.h` in your code.

```
#include "tcoap.h"

```
  There are several functions in the `tcoap.h` which are declared how `external`. You should provide it implementation in your code. See [wiki](https://github.com/Mozilla9/tiny-coap/wiki) for common case of their implementation.

```
/**
 * @brief In this function user should implement a transmission given data via 
 *        hardware interface (e.g. serial port)
 * 
 */
extern tcoap_error 
tcoap_tx_data(tcoap_handle * const handle, const uint8_t * buf, const uint32_t len);


/**
 * @brief In this function user should implement a functionality of waiting response. 
 *        This function has to return a control when timeout will expired or 
 *        when response from server will be received.
 */
extern tcoap_error 
tcoap_wait_event(tcoap_handle * const handle, const uint32_t timeout_ms);


/**
 * @brief Through this function the 'tcoap' lib will be notifing about events.
 *        See possible events here 'tcoap_out_signal'.
 */
extern tcoap_error 
tcoap_tx_signal(tcoap_handle * const handle, const tcoap_out_signal signal);


/**
 * @brief In this function user should implement a generating of message id.
 * 
 */
extern uint16_t 
tcoap_get_message_id(tcoap_handle * const handle);


/**
 * @brief In this function user should implement a generating of token.
 * 
 */
extern tcoap_error 
tcoap_fill_token(tcoap_handle * const handle, uint8_t * token, const uint32_t tkl);


/**
 * @brief These functions are using for debug purpose, if user will enable debug mode.
 * 
 */
extern void tcoap_debug_print_packet(tcoap_handle * const handle, const char * msg, uint8_t * data, const uint32_t len);
extern void tcoap_debug_print_options(tcoap_handle * const handle, const char * msg, const tcoap_option_data * options);
extern void tcoap_debug_print_payload(tcoap_handle * const handle, const char * msg, const tcoap_data * const payload);


/**
 * @brief In this function user should implement an allocating block of memory.
 *        In the simple case it may be a static buffer. The 'TCOAP' will make
 *        two calls of this function before starting work (for rx and tx buffer).
 *        So, you should have minimum two separate blocks of memory.
 * 
 */
extern tcoap_error tcoap_alloc_mem_block(uint8_t ** block, const uint32_t min_len);


/**
 * @brief In this function user should implement a freeing block of memory.
 * 
 */
extern tcoap_error tcoap_free_mem_block(uint8_t * block, const uint32_t min_len);

extern void mem_copy(void * dst, const void * src, uint32_t cnt);
extern bool mem_cmp(const void * dst, const void * src, uint32_t cnt);

```

2) Define a `tcoap_handle` object, e.g.

```
tcoap_handle tc_handle = {
        .name = "coap_over_gsm",
        .transport = TCOAP_UDP
};

```


3) Implement a transfer of incoming data from your hardware interface (e.g. serial port) to the `tcoap` either `tcoap_rx_byte` or `tcoap_rx_packet`. E.g.

```
void uart1_rx_irq_handler()
{
    uint8_t byte = UART1->DR;    
    tcoap_rx_byte(&tc_handle, byte);
}

void eth_rx_irq_handler(uint8_t * data, uint32_t len)
{
    tcoap_rx_packet(&tc_handle, data, len);
}

```


4) Send a coap request and get back response data in the provided callback:

```

static void data_resource_response_callback(const struct tcoap_request_descriptor * const reqd, const tcoap_result_data * const result)
{
    // ... check response
}


void send_request_to_data_resource(uint32_t *token_etag, uint8_t * data, uint32_t len)
{
    tcoap_error err;
    tcoap_request_descriptor data_request;

    tcoap_option_data opt_etag;
    tcoap_option_data opt_path;
    tcoap_option_data opt_content;

    /* fill options - we should adhere an order of options */
    opt_content.num = TCOAP_CONTENT_FORMAT_OPT;
    opt_content.value = (uint8_t *)"\x2A";  /* 42 = TCOAP_APPLICATION_OCTET_STREAM */
    opt_content.len = 1;
    opt_content.next = NULL;

    opt_path.num = TCOAP_URI_PATH_OPT;
    opt_path.value = (uint8_t *)"data";
    opt_path.len = 4;
    opt_path.next = &opt_content;

    opt_etag.num = TCOAP_ETAG_OPT;
    opt_etag.value = (uint8_t *)token_etag;
    opt_etag.len = 4;
    opt_etag.next = &opt_path;

    /* fill the request descriptor */
    data_request.payload.buf = data;
    data_request.payload.len = len;

    data_request.code = TCOAP_REQ_POST;
    data_request.tkl = 2;
    data_request.type = tc_handle.transport == TCOAP_UDP ? TCOAP_MESSAGE_CON : TCOAP_MESSAGE_NON;
    data_request.options = &opt_etag;
    
    /* define the callback for response data */
    data_request.response_callback = data_resource_response_callback;

    /* enable debug */
    tcoap_debug(&tc_handle, true);
    
    /* send request */
    err = tcoap_send_coap_request(&tc_handle, &data_request);

    if (err != TCOAP_OK) {
        // error handling
    }
}

```
