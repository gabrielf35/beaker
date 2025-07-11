#include "../beaker.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ContextVar *find_context_var(TemplateContext *ctx, const char *key) {
  for (int i = 0; i < ctx->count; i++) {
    if (strcmp(ctx->vars[i].key, key) == 0) {
      return &ctx->vars[i];
    }
  }
  return NULL;
}

static void free_string_array_var(ContextVar *var) {
  if (var->value.string_array_data.array_val != NULL) {
    for (int i = 0; i < var->value.string_array_data.array_count; i++) {
      if (var->value.string_array_data.array_val[i] != NULL) {
        free(var->value.string_array_data.array_val[i]);
        var->value.string_array_data.array_val[i] = NULL;
      }
    }
    free(var->value.string_array_data.array_val);
    var->value.string_array_data.array_val = NULL;
  }
}

static void free_array_of_arrays_var(ContextVar *var) {
  if (var->value.string_array_of_arrays_data.array_of_arrays_val != NULL) {
    for (int i = 0; i < var->value.string_array_of_arrays_data.outer_count;
         i++) {
      if (var->value.string_array_of_arrays_data.array_of_arrays_val[i] !=
          NULL) {
        for (int j = 0;
             j < var->value.string_array_of_arrays_data.inner_counts[i]; j++) {
          if (var->value.string_array_of_arrays_data
                  .array_of_arrays_val[i][j] != NULL) {
            free(var->value.string_array_of_arrays_data
                     .array_of_arrays_val[i][j]);
            var->value.string_array_of_arrays_data.array_of_arrays_val[i][j] =
                NULL;
          }
        }
        free(var->value.string_array_of_arrays_data.array_of_arrays_val[i]);
        var->value.string_array_of_arrays_data.array_of_arrays_val[i] = NULL;
      }
    }
    free(var->value.string_array_of_arrays_data.array_of_arrays_val);
    var->value.string_array_of_arrays_data.array_of_arrays_val = NULL;

    free(var->value.string_array_of_arrays_data.inner_counts);
    var->value.string_array_of_arrays_data.inner_counts = NULL;
  }
}

TemplateContext new_context() {
  TemplateContext ctx;
  ctx.count = 0;
  return ctx;
}

void context_set(TemplateContext *ctx, const char *key, const char *value) {
  if (ctx == NULL) {
    fprintf(stderr, "Error: TemplateContext pointer is NULL in context_set.\n");
    return;
  }

  ContextVar *var = find_context_var(ctx, key);
  if (var != NULL) {
    if (var->type == CONTEXT_TYPE_STRING_ARRAY) {
      free_string_array_var(var);
    } else if (var->type == CONTEXT_TYPE_STRING_ARRAY_OF_ARRAYS) {
      free_array_of_arrays_var(var);
    }
    strncpy(var->value.string_val, value, MAX_VALUE_LEN - 1);
    var->value.string_val[MAX_VALUE_LEN - 1] = '\0';
    var->type = CONTEXT_TYPE_STRING;
    return;
  }

  if (ctx->count < MAX_CONTEXT_VARS) {
    ContextVar *new_var = &ctx->vars[ctx->count];
    strncpy(new_var->key, key, MAX_KEY_LEN - 1);
    new_var->key[MAX_KEY_LEN - 1] = '\0';
    strncpy(new_var->value.string_val, value, MAX_VALUE_LEN - 1);
    new_var->value.string_val[MAX_VALUE_LEN - 1] = '\0';
    new_var->type = CONTEXT_TYPE_STRING;
    ctx->count++;
  } else {
    fprintf(stderr, "Warning: TemplateContext is full. Cannot add key '%s'.\n",
            key);
  }
}

void context_set_string_array(TemplateContext *ctx, const char *key,
                              char *values[], int count) {
  if (ctx == NULL) {
    fprintf(stderr, "Error: TemplateContext pointer is NULL in "
                    "context_set_string_array.\n");
    return;
  }
  if (count < 0 || count > MAX_OUTER_ARRAY_ITEMS) {
    fprintf(stderr,
            "Error: Invalid count %d for string array context '%s'. Max %d "
            "allowed.\n",
            count, key, MAX_OUTER_ARRAY_ITEMS);
    return;
  }

  ContextVar *var = find_context_var(ctx, key);
  if (var != NULL) {
    if (var->type == CONTEXT_TYPE_STRING_ARRAY) {
      free_string_array_var(var);
    } else if (var->type == CONTEXT_TYPE_STRING_ARRAY_OF_ARRAYS) {
      free_array_of_arrays_var(var);
    }
  } else {
    if (ctx->count < MAX_CONTEXT_VARS) {
      var = &ctx->vars[ctx->count];
      strncpy(var->key, key, MAX_KEY_LEN - 1);
      var->key[MAX_KEY_LEN - 1] = '\0';
      ctx->count++;
    } else {
      fprintf(stderr,
              "Warning: TemplateContext is full. Cannot add string array key "
              "'%s'.\n",
              key);
      return;
    }
  }

  var->type = CONTEXT_TYPE_STRING_ARRAY;
  var->value.string_array_data.array_val =
      (char **)malloc(sizeof(char *) * count);
  if (var->value.string_array_data.array_val == NULL) {
    perror("Failed to allocate memory for string array pointers");
    return;
  }
  var->value.string_array_data.array_count = count;

  for (int i = 0; i < count; i++) {
    var->value.string_array_data.array_val[i] = strdup(values[i]);
    if (var->value.string_array_data.array_val[i] == NULL) {
      perror("Failed to duplicate string for string array context");
      fprintf(stderr,
              "[ERROR] context_set_string_array: Failed to duplicate value for "
              "item %d of key '%s'.\n",
              i, key);
      for (int j = 0; j < i; j++) {
        if (var->value.string_array_data.array_val[j] != NULL) {
          free(var->value.string_array_data.array_val[j]);
        }
      }
      free(var->value.string_array_data.array_val);
      var->value.string_array_data.array_val = NULL;
      var->value.string_array_data.array_count = 0;
      return;
    }
  }
}

void context_set_array_of_arrays(TemplateContext *ctx, const char *key,
                                 char **values_2d[], int outer_count,
                                 int inner_counts[]) {
  if (ctx == NULL) {
    fprintf(stderr, "Error: TemplateContext pointer is NULL in "
                    "context_set_array_of_arrays.\n");
    return;
  }
  if (outer_count < 0 || outer_count > MAX_OUTER_ARRAY_ITEMS) {
    fprintf(stderr,
            "Error: Invalid outer count %d for array of arrays context '%s'. "
            "Max %d allowed.\n",
            outer_count, key, MAX_OUTER_ARRAY_ITEMS);
    return;
  }

  ContextVar *var = find_context_var(ctx, key);
  if (var != NULL) {
    if (var->type == CONTEXT_TYPE_STRING_ARRAY) {
      free_string_array_var(var);
    } else if (var->type == CONTEXT_TYPE_STRING_ARRAY_OF_ARRAYS) {
      free_array_of_arrays_var(var);
    }
  } else {
    if (ctx->count < MAX_CONTEXT_VARS) {
      var = &ctx->vars[ctx->count];
      strncpy(var->key, key, MAX_KEY_LEN - 1);
      var->key[MAX_KEY_LEN - 1] = '\0';
      ctx->count++;
    } else {
      fprintf(stderr,
              "Warning: TemplateContext is full. Cannot add array of arrays "
              "key '%s'.\n",
              key);
      return;
    }
  }

  var->type = CONTEXT_TYPE_STRING_ARRAY_OF_ARRAYS;
  var->value.string_array_of_arrays_data.array_of_arrays_val =
      (char ***)malloc(sizeof(char **) * outer_count);
  var->value.string_array_of_arrays_data.inner_counts =
      (int *)malloc(sizeof(int) * outer_count);
  if (var->value.string_array_of_arrays_data.array_of_arrays_val == NULL ||
      var->value.string_array_of_arrays_data.inner_counts == NULL) {
    perror("Failed to allocate memory for array of arrays pointers or counts");
    fprintf(stderr,
            "[ERROR] context_set_array_of_arrays: Allocation failed for key "
            "'%s'.\n",
            key);
    free(var->value.string_array_of_arrays_data.array_of_arrays_val);
    free(var->value.string_array_of_arrays_data.inner_counts);
    var->value.string_array_of_arrays_data.array_of_arrays_val = NULL;
    var->value.string_array_of_arrays_data.inner_counts = NULL;
    return;
  }
  var->value.string_array_of_arrays_data.outer_count = outer_count;

  for (int i = 0; i < outer_count; i++) {
    int current_inner_count = inner_counts[i];
    if (current_inner_count < 0 ||
        current_inner_count > MAX_INNER_ARRAY_ITEMS) {
      fprintf(stderr,
              "Error: Invalid inner count %d for item %d in array of arrays "
              "'%s'. Max %d allowed.\n",
              current_inner_count, i, key, MAX_INNER_ARRAY_ITEMS);
      for (int k = 0; k < i; k++) {
        if (var->value.string_array_of_arrays_data.array_of_arrays_val[k] !=
            NULL) {
          for (int l = 0;
               l < var->value.string_array_of_arrays_data.inner_counts[k];
               l++) {
            if (var->value.string_array_of_arrays_data
                    .array_of_arrays_val[k][l] != NULL) {
              free(var->value.string_array_of_arrays_data
                       .array_of_arrays_val[k][l]);
            }
          }
          free(var->value.string_array_of_arrays_data.array_of_arrays_val[k]);
        }
      }
      free(var->value.string_array_of_arrays_data.array_of_arrays_val);
      free(var->value.string_array_of_arrays_data.inner_counts);
      var->value.string_array_of_arrays_data.array_of_arrays_val = NULL;
      var->value.string_array_of_arrays_data.inner_counts = NULL;
      return;
    }

    var->value.string_array_of_arrays_data.array_of_arrays_val[i] =
        (char **)malloc(sizeof(char *) * current_inner_count);
    if (var->value.string_array_of_arrays_data.array_of_arrays_val[i] == NULL) {
      perror("Failed to allocate memory for inner string array pointers");
      fprintf(stderr,
              "[ERROR] context_set_array_of_arrays: Allocation failed for "
              "inner array %d of key '%s'.\n",
              i, key);
      for (int k = 0; k < i; k++) {
        if (var->value.string_array_of_arrays_data.array_of_arrays_val[k] !=
            NULL) {
          for (int l = 0;
               l < var->value.string_array_of_arrays_data.inner_counts[k];
               l++) {
            if (var->value.string_array_of_arrays_data
                    .array_of_arrays_val[k][l] != NULL) {
              free(var->value.string_array_of_arrays_data
                       .array_of_arrays_val[k][l]);
            }
          }
          free(var->value.string_array_of_arrays_data.array_of_arrays_val[k]);
        }
      }
      free(var->value.string_array_of_arrays_data.array_of_arrays_val);
      free(var->value.string_array_of_arrays_data.inner_counts);
      var->value.string_array_of_arrays_data.array_of_arrays_val = NULL;
      var->value.string_array_of_arrays_data.inner_counts = NULL;
      return;
    }
    var->value.string_array_of_arrays_data.inner_counts[i] =
        current_inner_count;

    for (int j = 0; j < current_inner_count; j++) {
      var->value.string_array_of_arrays_data.array_of_arrays_val[i][j] =
          strdup(values_2d[i][j]);
      if (var->value.string_array_of_arrays_data.array_of_arrays_val[i][j] ==
          NULL) {
        perror("Failed to duplicate string for inner array context");
        fprintf(stderr,
                "[ERROR] context_set_array_of_arrays: Failed to duplicate "
                "value for item [%d][%d] of key '%s'.\n",
                i, j, key);
        for (int k = 0; k <= i; k++) {
          if (var->value.string_array_of_arrays_data.array_of_arrays_val[k] !=
              NULL) {
            for (int l = 0;
                 l <
                 (k == i
                      ? j
                      : var->value.string_array_of_arrays_data.inner_counts[k]);
                 l++) {
              if (var->value.string_array_of_arrays_data
                      .array_of_arrays_val[k][l] != NULL) {
                free(var->value.string_array_of_arrays_data
                         .array_of_arrays_val[k][l]);
              }
            }
            free(var->value.string_array_of_arrays_data.array_of_arrays_val[k]);
          }
        }
        free(var->value.string_array_of_arrays_data.array_of_arrays_val);
        free(var->value.string_array_of_arrays_data.inner_counts);
        var->value.string_array_of_arrays_data.array_of_arrays_val = NULL;
        var->value.string_array_of_arrays_data.inner_counts = NULL;
        return;
      }
    }
  }
}

void free_context(TemplateContext *ctx) {
  if (ctx == NULL)
    return;
  for (int i = 0; i < ctx->count; i++) {
    if (ctx->vars[i].type == CONTEXT_TYPE_STRING_ARRAY) {
      free_string_array_var(&ctx->vars[i]);
    } else if (ctx->vars[i].type == CONTEXT_TYPE_STRING_ARRAY_OF_ARRAYS) {
      free_array_of_arrays_var(&ctx->vars[i]);
    }
  }
  ctx->count = 0;
}

static char *render_template_segment(const char *template_segment,
                                     TemplateContext *ctx);

static char *html_escape(const char *input) {
  if (input == NULL)
    return strdup("");

  size_t input_len = strlen(input);
  size_t estimated_len = input_len * 5 + 1;
  char *output = (char *)malloc(estimated_len);
  if (output == NULL) {
    perror("Failed to allocate memory for HTML escape output");
    return NULL;
  }
  output[0] = '\0';
  size_t current_output_len = 0;

  for (size_t i = 0; i < input_len; i++) {
    const char *replacement = NULL;
    switch (input[i]) {
    case '&':
      replacement = "&amp;";
      break;
    case '<':
      replacement = "&lt;";
      break;
    case '>':
      replacement = "&gt;";
      break;
    case '"':
      replacement = "&quot;";
      break;
    case '\'':
      replacement = "&apos;";
      break;
    default:
      break;
    }

    if (replacement) {
      size_t repl_len = strlen(replacement);
      if (current_output_len + repl_len + 1 > estimated_len) {
        estimated_len = (current_output_len + repl_len + 1) * 2;
        char *new_output = (char *)realloc(output, estimated_len);
        if (new_output == NULL) {
          perror("Failed to reallocate memory for HTML escape output");
          free(output);
          return NULL;
        }
        output = new_output;
      }
      strcat(output, replacement);
      current_output_len += repl_len;
    } else {
      if (current_output_len + 1 + 1 > estimated_len) {
        estimated_len = (current_output_len + 1 + 1) * 2;
        char *new_output = (char *)realloc(output, estimated_len);
        if (new_output == NULL) {
          perror("Failed to reallocate memory for HTML escape output");
          free(output);
          return NULL;
        }
        output = new_output;
      }
      output[current_output_len++] = input[i];
      output[current_output_len] = '\0';
    }
  }
  return output;
}

char *render_template(const char *template_file, TemplateContext *ctx) {
  char full_path[MAX_PATH_LEN];
  snprintf(full_path, sizeof(full_path), "%s%s", TEMPLATES_DIR, template_file);

  FILE *fp = fopen(full_path, "r");
  if (fp == NULL) {
    perror("Error opening template file");
    fprintf(stderr, "[ERROR] render_template: Could not open template '%s'.\n",
            full_path);
    return NULL;
  }

  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char *template_content = (char *)malloc(file_size + 1);
  if (template_content == NULL) {
    perror("Error allocating memory for template content");
    fprintf(stderr,
            "[ERROR] render_template: Failed to allocate %ld bytes for "
            "template content.\n",
            file_size + 1);
    fclose(fp);
    return NULL;
  }
  fread(template_content, 1, file_size, fp);
  template_content[file_size] = '\0';
  fclose(fp);

  char *rendered_html = render_template_segment(template_content, ctx);
  free(template_content);
  return rendered_html;
}

static void append_to_buffer(char **buffer, size_t *current_len,
                             size_t *max_len, const char *str_to_add) {
  if (str_to_add == NULL) {
    fprintf(stderr,
            "Warning: Attempted to append NULL string to buffer. Skipping.\n");
    return;
  }

  size_t add_len = strlen(str_to_add);

  if (*current_len + add_len + 1 > *max_len) {
    *max_len = (*current_len + add_len + 1) * 2;
    if (*max_len < BUFFER_SIZE * 2)
      *max_len = BUFFER_SIZE * 2;
    char *new_buffer = (char *)realloc(*buffer, *max_len);
    if (new_buffer == NULL) {
      perror("Failed to reallocate buffer for template rendering");
      fprintf(stderr, "[ERROR] append_to_buffer: Reallocation failed.\n");
      free(*buffer);
      *buffer = NULL;
      exit(EXIT_FAILURE);
    }
    *buffer = new_buffer;
  }

  strcat(*buffer, str_to_add);
  *current_len += add_len;
}

static char *parse_indexed_tag(const char *tag_content, int *index_val) {
  *index_val = -1;

  char *open_bracket = strchr(tag_content, '[');
  if (open_bracket == NULL) {
    char *key_name = strdup(tag_content);
    return key_name;
  }

  char *close_bracket = strchr(open_bracket, ']');
  if (close_bracket == NULL) {
    fprintf(stderr,
            "Template error: Unclosed bracket in tag '%s'. Returning raw tag "
            "content.\n",
            tag_content);
    return strdup(tag_content);
  }

  size_t key_len = open_bracket - tag_content;
  char *key_name = (char *)malloc(key_len + 1);
  if (key_name == NULL) {
    perror("Failed to allocate memory for key_name in parse_indexed_tag");
    fprintf(stderr,
            "[ERROR] parse_indexed_tag: Allocation failed for key_name.\n");
    return NULL;
  }
  strncpy(key_name, tag_content, key_len);
  key_name[key_len] = '\0';

  size_t index_str_len = close_bracket - (open_bracket + 1);
  char *index_str = (char *)malloc(index_str_len + 1);
  if (index_str == NULL) {
    perror("Failed to allocate memory for index_str in parse_indexed_tag");
    fprintf(stderr,
            "[ERROR] parse_indexed_tag: Allocation failed for index_str.\n");
    free(key_name);
    return NULL;
  }
  strncpy(index_str, open_bracket + 1, index_str_len);
  index_str[index_str_len] = '\0';

  *index_val = atoi(index_str);
  free(index_str);
  return key_name;
}

static char *render_template_segment(const char *template_segment,
                                     TemplateContext *ctx) {
  size_t initial_max_len = BUFFER_SIZE;
  char *rendered_buffer = (char *)malloc(initial_max_len);
  if (rendered_buffer == NULL) {
    perror("Failed to allocate initial buffer for template segment rendering");
    fprintf(stderr,
            "[ERROR] render_template_segment: Failed to allocate initial %zu "
            "bytes for rendered_buffer.\n",
            initial_max_len);
    return NULL;
  }
  rendered_buffer[0] = '\0';
  size_t current_len = 0;
  size_t max_len = initial_max_len;

  const char *current_pos = template_segment;
  const char *start_tag;

  while ((start_tag = strstr(current_pos, "{{")) != NULL) {
    size_t text_len = start_tag - current_pos;
    char *text_before_tag = (char *)malloc(text_len + 1);
    if (text_before_tag == NULL) {
      perror("Failed to allocate memory for text_before_tag");
      fprintf(stderr, "[ERROR] render_template_segment: Allocation failed for "
                      "text_before_tag.\n");
      free(rendered_buffer);
      return NULL;
    }
    strncpy(text_before_tag, current_pos, text_len);
    text_before_tag[text_len] = '\0';
    append_to_buffer(&rendered_buffer, &current_len, &max_len, text_before_tag);
    free(text_before_tag);

    const char *end_tag = strstr(start_tag, "}}");
    if (end_tag == NULL) {
      fprintf(stderr, "Template error: Unclosed '{{' tag. Appending remaining "
                      "template content as-is.\n");
      append_to_buffer(&rendered_buffer, &current_len, &max_len, start_tag);
      break;
    }

    size_t tag_content_len = end_tag - (start_tag + 2);
    char *tag_content_raw = (char *)malloc(tag_content_len + 1);
    if (tag_content_raw == NULL) {
      perror("Failed to allocate memory for tag_content_raw");
      fprintf(stderr, "[ERROR] render_template_segment: Allocation failed for "
                      "tag_content_raw.\n");
      free(rendered_buffer);
      return NULL;
    }
    strncpy(tag_content_raw, start_tag + 2, tag_content_len);
    tag_content_raw[tag_content_len] = '\0';

    char *trimmed_tag_content = tag_content_raw;
    while (*trimmed_tag_content != '\0' &&
           (*trimmed_tag_content == ' ' || *trimmed_tag_content == '\t' ||
            *trimmed_tag_content == '\n' || *trimmed_tag_content == '\r')) {
      trimmed_tag_content++;
    }
    if (*trimmed_tag_content == '\0') {
      free(tag_content_raw);
      current_pos = end_tag + 2;
      continue;
    } else {
      char *end_trimmed = trimmed_tag_content + strlen(trimmed_tag_content) - 1;
      while (end_trimmed >= trimmed_tag_content &&
             (*end_trimmed == ' ' || *end_trimmed == '\t' ||
              *end_trimmed == '\n' || *end_trimmed == '\r')) {
        end_trimmed--;
      }
      *(end_trimmed + 1) = '\0';
    }

    if (strncmp(trimmed_tag_content, "for ", 4) == 0) {
      char loop_var[MAX_KEY_LEN];
      char list_var[MAX_KEY_LEN];
      if (sscanf(trimmed_tag_content, "for %s in %s", loop_var, list_var) ==
          2) {
        ContextVar *list_ctx_var = find_context_var(ctx, list_var);
        if (list_ctx_var != NULL) {
          const char *loop_end_tag = strstr(end_tag + 2, "{{endfor}}");
          if (loop_end_tag == NULL) {
            fprintf(stderr, "Template error: '{{for}}' without matching "
                            "'{{endfor}}'. Appending loop tag as-is.\n");
            append_to_buffer(&rendered_buffer, &current_len, &max_len,
                             trimmed_tag_content);
          } else {
            const char *loop_inner_start = end_tag + 2;
            size_t loop_inner_len = loop_end_tag - loop_inner_start;
            char *loop_inner_template = (char *)malloc(loop_inner_len + 1);
            if (loop_inner_template == NULL) {
              perror("Failed to allocate memory for loop_inner_template");
              fprintf(stderr, "[ERROR] render_template_segment: Allocation "
                              "failed for loop_inner_template.\n");
              free(rendered_buffer);
              free(tag_content_raw);
              return NULL;
            }
            strncpy(loop_inner_template, loop_inner_start, loop_inner_len);
            loop_inner_template[loop_inner_len] = '\0';

            if (list_ctx_var->type == CONTEXT_TYPE_STRING_ARRAY_OF_ARRAYS) {
              for (int i = 0;
                   i <
                   list_ctx_var->value.string_array_of_arrays_data.outer_count;
                   i++) {
                TemplateContext loop_ctx = new_context();
                for (int j = 0; j < ctx->count; j++) {
                  if (ctx->vars[j].type == CONTEXT_TYPE_STRING) {
                    context_set(&loop_ctx, ctx->vars[j].key,
                                ctx->vars[j].value.string_val);
                  }
                }
                context_set_string_array(
                    &loop_ctx, loop_var,
                    list_ctx_var->value.string_array_of_arrays_data
                        .array_of_arrays_val[i],
                    list_ctx_var->value.string_array_of_arrays_data
                        .inner_counts[i]);

                char *rendered_loop_item =
                    render_template_segment(loop_inner_template, &loop_ctx);
                if (rendered_loop_item) {
                  append_to_buffer(&rendered_buffer, &current_len, &max_len,
                                   rendered_loop_item);
                  free(rendered_loop_item);
                }
                free_context(&loop_ctx);
              }
            } else if (list_ctx_var->type == CONTEXT_TYPE_STRING_ARRAY) {
              for (int i = 0;
                   i < list_ctx_var->value.string_array_data.array_count; i++) {
                TemplateContext loop_ctx = new_context();
                for (int j = 0; j < ctx->count; j++) {
                  if (ctx->vars[j].type == CONTEXT_TYPE_STRING) {
                    context_set(&loop_ctx, ctx->vars[j].key,
                                ctx->vars[j].value.string_val);
                  }
                }
                context_set(&loop_ctx, loop_var,
                            list_ctx_var->value.string_array_data.array_val[i]);

                char *rendered_loop_item =
                    render_template_segment(loop_inner_template, &loop_ctx);
                if (rendered_loop_item) {
                  append_to_buffer(&rendered_buffer, &current_len, &max_len,
                                   rendered_loop_item);
                  free(rendered_loop_item);
                }
                free_context(&loop_ctx);
              }
            } else {
              fprintf(stderr,
                      "Template error: List variable '%s' (type %d) is not an "
                      "iterable array type for 'for' loop. Appending loop tag "
                      "as-is.\n",
                      list_var, list_ctx_var->type);
              append_to_buffer(&rendered_buffer, &current_len, &max_len,
                               trimmed_tag_content);
            }
            free(loop_inner_template);
            current_pos = loop_end_tag + strlen("{{endfor}}");
            free(tag_content_raw);
            continue;
          }
        } else {
          fprintf(stderr,
                  "Template error: List variable '%s' not found for 'for' "
                  "loop. Appending loop tag as-is.\n",
                  list_var);
          append_to_buffer(&rendered_buffer, &current_len, &max_len,
                           trimmed_tag_content);
        }
      } else {
        fprintf(stderr,
                "Template error: Malformed 'for' loop tag: '%s'. Expected 'for "
                "var in list'. Appending loop tag as-is.\n",
                trimmed_tag_content);
        append_to_buffer(&rendered_buffer, &current_len, &max_len,
                         trimmed_tag_content);
      }
    } else if (strcmp(trimmed_tag_content, "endfor") == 0) {
      fprintf(stderr, "Template error: '{{endfor}}' without matching "
                      "'{{for}}'. Appending endfor tag as-is.\n");
      append_to_buffer(&rendered_buffer, &current_len, &max_len,
                       trimmed_tag_content);
    } else if (strncmp(trimmed_tag_content, "include ", 8) == 0) {
      const char *filename_start = trimmed_tag_content + strlen("include ");
      if (*filename_start == '"') {
        filename_start++;
        char *filename_end = strchr(filename_start, '"');
        if (filename_end != NULL) {
          size_t filename_len = filename_end - filename_start;
          char *included_filename = (char *)malloc(filename_len + 1);
          if (included_filename == NULL) {
            perror("Failed to allocate memory for included filename");
            fprintf(stderr, "[ERROR] render_template_segment: Allocation "
                            "failed for included_filename.\n");
            free(rendered_buffer);
            free(tag_content_raw);
            return NULL;
          }
          strncpy(included_filename, filename_start, filename_len);
          included_filename[filename_len] = '\0';

          char *included_html = render_template(included_filename, ctx);
          if (included_html) {
            append_to_buffer(&rendered_buffer, &current_len, &max_len,
                             included_html);
            free(included_html);
          } else {
            fprintf(
                stderr,
                "Template warning: Failed to render included template '%s'.\n",
                included_filename);
            append_to_buffer(&rendered_buffer, &current_len, &max_len,
                             "<!-- Failed to include ");
            append_to_buffer(&rendered_buffer, &current_len, &max_len,
                             included_filename);
            append_to_buffer(&rendered_buffer, &current_len, &max_len, " -->");
          }
          free(included_filename);
          current_pos = end_tag + 2;
          free(tag_content_raw);
          continue;
        } else {
          fprintf(stderr,
                  "Template error: Malformed include tag '%s'. Missing closing "
                  "quote.\n",
                  trimmed_tag_content);
          append_to_buffer(&rendered_buffer, &current_len, &max_len, "{{");
          append_to_buffer(&rendered_buffer, &current_len, &max_len,
                           trimmed_tag_content);
          append_to_buffer(&rendered_buffer, &current_len, &max_len, "}}");
        }
      } else {
        fprintf(stderr,
                "Template error: Malformed include tag '%s'. Expected quoted "
                "filename.\n",
                trimmed_tag_content);
        append_to_buffer(&rendered_buffer, &current_len, &max_len, "{{");
        append_to_buffer(&rendered_buffer, &current_len, &max_len,
                         trimmed_tag_content);
        append_to_buffer(&rendered_buffer, &current_len, &max_len, "}}");
      }
    } else {
      bool is_safe = false;
      char *processing_tag_content = strdup(trimmed_tag_content);
      if (processing_tag_content == NULL) {
        perror("Failed to duplicate tag content for processing");
        free(rendered_buffer);
        free(tag_content_raw);
        return NULL;
      }

      char *safe_flag_pos_in_processing =
          strstr(processing_tag_content, "|safe");

      if (safe_flag_pos_in_processing != NULL) {
        is_safe = true;
        *safe_flag_pos_in_processing = '\0';
      }

      int index_val = -1;
      char *key_name = parse_indexed_tag(processing_tag_content, &index_val);
      free(processing_tag_content);

      if (key_name == NULL) {
        fprintf(stderr,
                "[ERROR] render_template_segment: parse_indexed_tag failed for "
                "'%s'.\n",
                trimmed_tag_content);
        free(rendered_buffer);
        free(tag_content_raw);
        return NULL;
      }

      const char *value_to_append = NULL;
      ContextVar *var = find_context_var(ctx, key_name);

      if (var != NULL) {
        if (index_val == -1) {
          if (var->type == CONTEXT_TYPE_STRING) {
            value_to_append = var->value.string_val;
          } else {
            fprintf(stderr,
                    "Template warning: Variable '%s' is not a simple string "
                    "(type %d). Not rendering as simple string.\n",
                    key_name, var->type);
            append_to_buffer(&rendered_buffer, &current_len, &max_len, "{{");
            append_to_buffer(&rendered_buffer, &current_len, &max_len,
                             trimmed_tag_content);
            append_to_buffer(&rendered_buffer, &current_len, &max_len, "}}");
          }
        } else {
          if (var->type == CONTEXT_TYPE_STRING_ARRAY) {
            if (index_val >= 0 &&
                index_val < var->value.string_array_data.array_count) {
              value_to_append =
                  var->value.string_array_data.array_val[index_val];
            } else {
              fprintf(stderr,
                      "Template error: Index %d out of bounds for '%s' (count "
                      "%d). Appending tag as-is.\n",
                      index_val, key_name,
                      var->value.string_array_data.array_count);
              append_to_buffer(&rendered_buffer, &current_len, &max_len, "{{");
              append_to_buffer(&rendered_buffer, &current_len, &max_len,
                               trimmed_tag_content);
              append_to_buffer(&rendered_buffer, &current_len, &max_len, "}}");
            }
          } else {
            fprintf(stderr,
                    "Template error: Variable '%s' (type %d) is not a string "
                    "array for indexed access. Appending tag as-is.\n",
                    key_name, var->type);
            append_to_buffer(&rendered_buffer, &current_len, &max_len, "{{");
            append_to_buffer(&rendered_buffer, &current_len, &max_len,
                             trimmed_tag_content);
            append_to_buffer(&rendered_buffer, &current_len, &max_len, "}}");
          }
        }
      } else {
        fprintf(stderr,
                "Template error: Key '%s' not found in context. Appending tag "
                "as-is.\n",
                key_name);
        append_to_buffer(&rendered_buffer, &current_len, &max_len, "{{");
        append_to_buffer(&rendered_buffer, &current_len, &max_len,
                         trimmed_tag_content);
        append_to_buffer(&rendered_buffer, &current_len, &max_len, "}}");
      }

      if (value_to_append != NULL) {
        if (is_safe) {
          append_to_buffer(&rendered_buffer, &current_len, &max_len,
                           value_to_append);
        } else {
          char *escaped_value = html_escape(value_to_append);
          if (escaped_value) {
            append_to_buffer(&rendered_buffer, &current_len, &max_len,
                             escaped_value);
            free(escaped_value);
          } else {
            fprintf(stderr,
                    "[ERROR] Failed to HTML escape value for key '%s'. "
                    "Appending raw.\n",
                    key_name);
            append_to_buffer(&rendered_buffer, &current_len, &max_len,
                             value_to_append);
          }
        }
      }
      free(key_name);
    }
    free(tag_content_raw);
    current_pos = end_tag + 2;
  }

  append_to_buffer(&rendered_buffer, &current_len, &max_len, current_pos);
  return rendered_buffer;
}
