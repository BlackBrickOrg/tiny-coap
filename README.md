### CoAP client library for embedded devices

Allows to add the CoAP functionality for embedded device.

#### Features

- very small memory consumption (in the common case it may be about of 200 bytes for rx/tx buffers)

- implemented CoAP over UDP [rfc7252](https://tools.ietf.org/html/rfc7252)

- implemented CoAP over TCP [draft-coap-tcp-tls-07](https://tools.ietf.org/html/draft-ietf-core-coap-tcp-tls-07)

- retransmition/acknowledgment functionality

- parsing responses. Received data will be return to user via callback.


#### How to send CoAP request to server

1) There are several functions in `tcoap.h` which declared how `external`. You should provide it implementation in your code.
See [wiki](https://github.com/Mozilla9/tiny-coap/wiki) for common case of their implementation.


2) Define a `tcoap_handle` object, e.g.

```
__tcoap_handle tcoap_handle = {
        .name = "coap_over_gsm",
        .transport = TCOAP_UDP
};

```


3) Implement a transfer of incoming data from your hardware interface (e.g. serail port) to the `tcoap` via  functions `tcoap_rx_byte` or `tcoap_rx_packet`. E.g.

```
void uart_rx_irq_handler(uint8_t byte)
{
    tcoap_rx_byte(&tcoap_handle, byte);
}

void eth_rx_irq_handler(uint8_t * data, uint32_t len)
{
    tcoap_rx_packet(&tcoap_handle, data, len);
}

```


4) Define and store request parameters in the `__tcoap_request_descriptor`.

```
 - type
 - code
 - tkl
 - payload
 - options
 - callback for response

```

5) Send a coap request and get back response data in the provided callback:

```
__tcoap_error err;

err = tcoap_send_coap_request(&tcoap_handle, &tcoap_request);

```
