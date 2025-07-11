#include "../beaker.h"
#include "beaker_globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

void set_handler(const char *path, RequestHandler handler) {
  if (handler_count < MAX_HANDLERS) {
    strncpy(handlers[handler_count].path, path, MAX_PATH_LEN - 1);
    handlers[handler_count].path[MAX_PATH_LEN - 1] = '\0';
    handlers[handler_count].handler = handler;
    handler_count++;
  } else {
    fprintf(stderr,
            "Warning: Maximum number of handlers reached. Cannot register "
            "handler for '%s'.\n",
            path);
  }
}

char *parse_request_url(const char *request_line, UrlParams *params) {
  char method[16];
  char path_with_query_buffer[MAX_PATH_LEN];
  char http_version[16];

  if (sscanf(request_line, "%15s %255s %15s", method, path_with_query_buffer,
             http_version) != 3) {
    fprintf(stderr, "[ERROR] parse_request_url: Malformed request line: '%s'\n",
            request_line);
    return NULL;
  }

  params->count = 0;

  char *working_url_copy = strdup(path_with_query_buffer);
  if (working_url_copy == NULL) {
    perror("Failed to allocate memory for working URL copy");
    return NULL;
  }

  char *query_start = strchr(working_url_copy, '?');
  char *path_only_for_return;

  if (query_start != NULL) {
    *query_start = '\0';
    path_only_for_return = working_url_copy;
    char *query_string = query_start + 1;

    char *token;
    char *saveptr_query;
    token = strtok_r(query_string, "&", &saveptr_query);
    while (token != NULL && params->count < MAX_URL_PARAMS) {
      char *equals_sign = strchr(token, '=');
      if (equals_sign != NULL) {
        size_t key_len = equals_sign - token;
        strncpy(params->params[params->count].key, token, key_len);
        params->params[params->count].key[key_len] = '\0';
        strncpy(params->params[params->count].value, equals_sign + 1,
                MAX_VALUE_LEN - 1);
        params->params[params->count].value[MAX_VALUE_LEN - 1] = '\0';
        params->count++;
      }
      token = strtok_r(NULL, "&", &saveptr_query);
    }
  } else {
    path_only_for_return = working_url_copy;
  }

  char *final_requested_path = strdup(path_only_for_return);
  if (final_requested_path == NULL) {
    perror("Failed to allocate final requested path");
    free(working_url_copy);
    return NULL;
  }

  char *segment;
  int segment_index = 0;
  char *saveptr_path;

  segment = strtok_r(working_url_copy, "/", &saveptr_path);
  while (segment != NULL) {
    if (segment_index > 0) {
      if (params->count < MAX_URL_PARAMS) {
        char param_key[MAX_KEY_LEN];
        snprintf(param_key, sizeof(param_key), "param%d", segment_index - 1);
        strncpy(params->params[params->count].key, param_key, MAX_KEY_LEN - 1);
        params->params[params->count].key[MAX_KEY_LEN - 1] = '\0';
        strncpy(params->params[params->count].value, segment,
                MAX_VALUE_LEN - 1);
        params->params[params->count].value[MAX_VALUE_LEN - 1] = '\0';
        params->count++;
      } else {
        fprintf(stderr,
                "Warning: Max URL parameters reached. Skipping path segment "
                "'%s'.\n",
                segment);
      }
    }
    segment_index++;
    segment = strtok_r(NULL, "/", &saveptr_path);
  }

  free(working_url_copy);

  return final_requested_path;
}

const char *get_mime_type(const char *file_path) {
  const char *ext = strrchr(file_path, '.');
  if (!ext) {
    return "application/octet-stream";
  }
  ext++;

  if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0)
    return "text/html";
  if (strcmp(ext, "css") == 0)
    return "text/css";
  if (strcmp(ext, "js") == 0)
    return "application/javascript";
  if (strcmp(ext, "json") == 0)
    return "application/json";
  if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0)
    return "image/jpeg";
  if (strcmp(ext, "png") == 0)
    return "image/png";
  if (strcmp(ext, "gif") == 0)
    return "image/gif";
  if (strcmp(ext, "ico") == 0)
    return "image/x-icon";
  if (strcmp(ext, "svg") == 0)
    return "image/svg+xml";
  if (strcmp(ext, "pdf") == 0)
    return "application/pdf";
  if (strcmp(ext, "txt") == 0)
    return "text/plain";
  return "application/octet-stream";
}

bool serve_static_file(const char *request_path_relative_to_static) {
  char full_static_path[MAX_PATH_LEN];
  if (strstr(request_path_relative_to_static, "..") != NULL) {
    fprintf(stderr, "Attempted directory traversal: %s\n",
            request_path_relative_to_static);
    const char *forbidden =
        "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
    send(current_client_socket, forbidden, strlen(forbidden), 0);
    return true;
  }

  snprintf(full_static_path, sizeof(full_static_path), "%s%s", STATIC_DIR,
           request_path_relative_to_static);

  FILE *fp = fopen(full_static_path, "rb");
  if (fp == NULL) {
    fprintf(stderr,
            "[ERROR] serve_static_file: File '%s' not found or could not be "
            "opened.\n",
            full_static_path);
    return false;
  }

  struct stat st;
  if (fstat(fileno(fp), &st) < 0) {
    perror("fstat error");
    fprintf(stderr, "[ERROR] serve_static_file: fstat failed for '%s'.\n",
            full_static_path);
    fclose(fp);
    const char *server_error =
        "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
    send(current_client_socket, server_error, strlen(server_error), 0);
    return true;
  }

  long file_size = st.st_size;
  const char *mime_type = get_mime_type(full_static_path);

  char http_header[BUFFER_SIZE];
  snprintf(http_header, sizeof(http_header),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %ld\r\n"
           "Connection: close\r\n"
           "\r\n",
           mime_type, file_size);

  send(current_client_socket, http_header, strlen(http_header), 0);

  char file_buffer[BUFFER_SIZE];
  size_t bytes_read;
  while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), fp)) > 0) {
    send(current_client_socket, file_buffer, bytes_read, 0);
  }

  fclose(fp);
  return true;
}
