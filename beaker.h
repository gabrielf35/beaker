#ifndef BEAKER_H
#define BEAKER_H

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CONTEXT_VARS 32
#define MAX_KEY_LEN 64
#define MAX_VALUE_LEN 256
#define MAX_PATH_LEN 256
#define MAX_HANDLERS 32
#define BUFFER_SIZE 4096
#define MAX_URL_PARAMS 16
#define MAX_COOKIES 10
#define MAX_OUTER_ARRAY_ITEMS 32
#define MAX_INNER_ARRAY_ITEMS 16

#define TEMPLATES_DIR "templates/"
#define STATIC_DIR "static/"

typedef enum {
  CONTEXT_TYPE_STRING,
  CONTEXT_TYPE_STRING_ARRAY,
  CONTEXT_TYPE_STRING_ARRAY_OF_ARRAYS,
} ContextType;

typedef struct {
  char key[MAX_KEY_LEN];
  ContextType type;
  union {
    char string_val[MAX_VALUE_LEN];
    struct {
      char **array_val;
      int array_count;
    } string_array_data;
    struct {
      char ***array_of_arrays_val;
      int outer_count;
      int *inner_counts;
    } string_array_of_arrays_data;
  } value;
} ContextVar;

typedef struct {
  ContextVar vars[MAX_CONTEXT_VARS];
  int count;
} TemplateContext;

typedef struct {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];
} UrlParam;

typedef struct {
  UrlParam params[MAX_URL_PARAMS];
  int count;
} UrlParams;

typedef struct {
  char name[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];
  char expires[MAX_VALUE_LEN];
  char path[MAX_KEY_LEN];
  bool http_only;
  bool secure;
} Cookie;

typedef int (*RequestHandler)(UrlParams *params);

typedef struct {
  char path[MAX_PATH_LEN];
  RequestHandler handler;
} RouteHandler;

TemplateContext new_context();

void context_set(TemplateContext *ctx, const char *key, const char *value);

void context_set_string_array(TemplateContext *ctx, const char *key,
                              char *values[], int count);

void context_set_array_of_arrays(TemplateContext *ctx, const char *key,
                                 char **values_2d[], int outer_count,
                                 int inner_counts[]);

void free_context(TemplateContext *ctx);

char *render_template(const char *template_file, TemplateContext *ctx);

void send_response(const char *html);

void send_redirect(const char *location);

void set_cookie(const char *name, const char *value, const char *expires,
                const char *path, bool http_only, bool secure);

char *get_cookie(const char *cookie_name);

void set_handler(const char *path, RequestHandler handler);

char *parse_request_url(const char *request_buffer, UrlParams *params);

const char *get_mime_type(const char *file_path);

bool serve_static_file(const char *request_path_relative_to_static);

int beaker_run(const char *ip, int port);

#endif
