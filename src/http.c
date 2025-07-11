#include "../beaker.h"
#include "beaker_globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void send_response(const char *html) {
  if (current_client_socket == -1) {
    fprintf(stderr, "Error: No client socket set for sending response.\n");
    return;
  }

  char http_response_header[BUFFER_SIZE * 2];
  int content_length = strlen(html);
  char cookie_headers[BUFFER_SIZE] = "";

  for (int i = 0; i < cookies_to_set_count; i++) {
    char single_cookie_header[MAX_VALUE_LEN * 2];
    snprintf(single_cookie_header, sizeof(single_cookie_header),
             "Set-Cookie: %s=%s", cookies_to_set[i].name,
             cookies_to_set[i].value);

    if (strlen(cookies_to_set[i].expires) > 0) {
      strcat(single_cookie_header, "; Expires=");
      strcat(single_cookie_header, cookies_to_set[i].expires);
    }
    if (strlen(cookies_to_set[i].path) > 0) {
      strcat(single_cookie_header, "; Path=");
      strcat(single_cookie_header, cookies_to_set[i].path);
    }
    if (cookies_to_set[i].http_only) {
      strcat(single_cookie_header, "; HttpOnly");
    }
    if (cookies_to_set[i].secure) {
      strcat(single_cookie_header, "; Secure");
    }
    strcat(single_cookie_header, "\r\n");

    strcat(cookie_headers, single_cookie_header);
  }

  snprintf(http_response_header, sizeof(http_response_header),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html; charset=UTF-8\r\n"
           "Content-Length: %d\r\n"
           "%s"
           "Connection: close\r\n"
           "\r\n",
           content_length, cookie_headers);

  if (send(current_client_socket, http_response_header,
           strlen(http_response_header), 0) < 0) {
    perror("Error sending HTTP header");
    fprintf(stderr, "[ERROR] send_response: Failed to send HTTP header.\n");
    return;
  }

  if (send(current_client_socket, html, content_length, 0) < 0) {
    perror("Error sending HTML body");
    fprintf(stderr, "[ERROR] send_response: Failed to send HTML body.\n");
    return;
  }

  cookies_to_set_count = 0;
}

void send_redirect(const char *location) {
  if (current_client_socket == -1) {
    fprintf(stderr, "Error: No client socket set for sending redirect.\n");
    return;
  }

  char http_response_header[BUFFER_SIZE];
  snprintf(http_response_header, sizeof(http_response_header),
           "HTTP/1.1 302 Found\r\n"
           "Location: %s\r\n"
           "Connection: close\r\n"
           "\r\n",
           location);

  if (send(current_client_socket, http_response_header,
           strlen(http_response_header), 0) < 0) {
    perror("Error sending redirect header");
    fprintf(stderr, "[ERROR] send_redirect: Failed to send redirect header.\n");
    return;
  }

  cookies_to_set_count = 0;
}

void set_cookie(const char *name, const char *value, const char *expires,
                const char *path, bool http_only, bool secure) {
  if (cookies_to_set_count < MAX_COOKIES) {
    strncpy(cookies_to_set[cookies_to_set_count].name, name, MAX_KEY_LEN - 1);
    cookies_to_set[cookies_to_set_count].name[MAX_KEY_LEN - 1] = '\0';

    strncpy(cookies_to_set[cookies_to_set_count].value, value,
            MAX_VALUE_LEN - 1);
    cookies_to_set[cookies_to_set_count].value[MAX_VALUE_LEN - 1] = '\0';

    if (expires) {
      strncpy(cookies_to_set[cookies_to_set_count].expires, expires,
              MAX_VALUE_LEN - 1);
      cookies_to_set[cookies_to_set_count].expires[MAX_VALUE_LEN - 1] = '\0';
    } else {
      cookies_to_set[cookies_to_set_count].expires[0] = '\0';
    }

    if (path) {
      strncpy(cookies_to_set[cookies_to_set_count].path, path, MAX_KEY_LEN - 1);
      cookies_to_set[cookies_to_set_count].path[MAX_KEY_LEN - 1] = '\0';
    } else {
      cookies_to_set[cookies_to_set_count].path[0] = '\0';
    }

    cookies_to_set[cookies_to_set_count].http_only = http_only;
    cookies_to_set[cookies_to_set_count].secure = secure;

    cookies_to_set_count++;
  } else {
    fprintf(stderr,
            "Warning: Maximum number of cookies to set reached. Cannot set "
            "cookie '%s'.\n",
            name);
  }
}

char *get_cookie(const char *cookie_name) {
  char *cookie_header_start = strstr(current_request_buffer, "\r\nCookie: ");
  if (cookie_header_start == NULL) {
    return NULL;
  }

  cookie_header_start += strlen("\r\nCookie: ");

  char *cookie_header_end = strstr(cookie_header_start, "\r\n");
  if (cookie_header_end == NULL) {
    cookie_header_end =
        (char *)(current_request_buffer + strlen(current_request_buffer));
  }

  size_t cookie_str_len = cookie_header_end - cookie_header_start;
  char *cookie_str = (char *)malloc(cookie_str_len + 1);
  if (cookie_str == NULL) {
    perror("Failed to allocate memory for cookie string");
    fprintf(stderr, "[ERROR] get_cookie: Allocation failed for cookie_str.\n");
    return NULL;
  }
  strncpy(cookie_str, cookie_header_start, cookie_str_len);
  cookie_str[cookie_str_len] = '\0';

  char *cookie_str_copy = strdup(cookie_str);
  if (cookie_str_copy == NULL) {
    perror("Failed to duplicate cookie string for strtok");
    fprintf(stderr,
            "[ERROR] get_cookie: Duplication failed for cookie_str_copy.\n");
    free(cookie_str);
    return NULL;
  }

  char *token;
  char *saveptr_cookie;
  token = strtok_r(cookie_str_copy, ";", &saveptr_cookie);
  while (token != NULL) {
    while (*token == ' ')
      token++;

    char *equals_sign = strchr(token, '=');
    if (equals_sign != NULL) {
      size_t name_len = equals_sign - token;
      if (name_len == strlen(cookie_name) &&
          strncmp(token, cookie_name, name_len) == 0) {
        char *cookie_value = strdup(equals_sign + 1);
        free(cookie_str);
        free(cookie_str_copy);
        return cookie_value;
      }
    }
    token = strtok_r(NULL, ";", &saveptr_cookie);
  }

  free(cookie_str);
  free(cookie_str_copy);
  return NULL;
}
