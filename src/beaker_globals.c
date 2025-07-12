#include "beaker_globals.h" 

RouteHandler handlers[MAX_HANDLERS];

int handler_count = 0;

int current_client_socket = -1;

Cookie cookies_to_set[MAX_COOKIES];

int cookies_to_set_count = 0;

char current_request_buffer[BUFFER_SIZE];