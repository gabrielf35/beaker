#ifndef BEAKER_GLOBALS_H
#define BEAKER_GLOBALS_H

#include "../beaker.h"

extern RouteHandler handlers[MAX_HANDLERS];
extern int handler_count;

extern int current_client_socket;

extern Cookie cookies_to_set[MAX_COOKIES];
extern int cookies_to_set_count;

extern char current_request_buffer[BUFFER_SIZE];

#endif
