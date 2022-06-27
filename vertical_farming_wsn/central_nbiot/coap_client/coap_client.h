#ifndef COAP_CLIENT_H
#define COAP_CLIENT_H

// --- includes ----------------------------------------------------------------
#include <stdint.h>

// --- defines -----------------------------------------------------------------
#define APP_COAP_MAX_MSG_LEN 1280

// --- functions declarations --------------------------------------------------
void coap_client_init(void);
void coap_get(uint8_t *resource, uint16_t resourse_length);
void coap_put(uint8_t *resource, uint16_t resourse_length, uint8_t *payload, uint8_t length);
void coap_observe(uint8_t *resource, uint16_t resourse_length);
int coap_get_socket(void);
uint16_t *get_obs_token(void);
void initialize_observe_renew(void);

#endif // COAP_CLIENT_H