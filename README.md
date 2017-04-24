### CoAP client library for embedded devices

Allows to add the CoAP functionality for embedded device.

#### Features

- very small memory consumption (in the common case it may be about of 200 bytes for rx/tx buffers)

- implemented CoAP over UDP [rfc7252](https://tools.ietf.org/html/rfc7252)

- implemented CoAP over TCP [draft-coap-tcp-tls-07](https://tools.ietf.org/html/draft-ietf-core-coap-tcp-tls-07)

- retransmition/acknowledgment functionality

- parsing responses. Received data will be return to user via callback.


#### How to send CoAP request to server

1) Include `tcoap.h` in your code.

```
#include "tcoap.h"

```
  There are several functions in the `tcoap.h` which declared how `external`. You should provide it implementation in your code. See [wiki](https://github.com/Mozilla9/tiny-coap/wiki) for common case of their implementation.


2) Define a `tcoap_handle` object, e.g.

```
tcoap_handle tc_handle = {
        .name = "coap_over_gsm",
        .transport = TCOAP_UDP
};

```


3) Implement a transfer of incoming data from your hardware interface (e.g. serial port) to the `tcoap` via functions `tcoap_rx_byte` or `tcoap_rx_packet`. E.g.

```
void uart1_rx_irq_handler()
{
    uint8_t byte = UART1_DR;    
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
