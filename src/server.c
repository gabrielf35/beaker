#include "../beaker.h"        
#include "beaker_globals.h"   
#include <arpa/inet.h>        
#include <netinet/in.h>       
#include <stdbool.h>          
#include <stdio.h>            
#include <string.h>           
#include <sys/socket.h>       
#include <unistd.h>           

static int initialize_server_socket(const char *ip, int port, int *server_fd_out,
                                    struct sockaddr_in *address_out) {

  if ((*server_fd_out = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    fprintf(stderr, "[ERROR] initialize_server_socket: Failed to create socket.\n");
    return -1;
  }

  int opt = 1;
  if (setsockopt(*server_fd_out, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setsockopt failed");
    fprintf(stderr, "[ERROR] initialize_server_socket: Failed to set socket options.\n");
    close(*server_fd_out);
    return -1;
  }

  address_out->sin_family = AF_INET;           
  address_out->sin_addr.s_addr = inet_addr(ip); 
  address_out->sin_port = htons(port);         

  if (bind(*server_fd_out, (struct sockaddr *)address_out, sizeof(*address_out)) < 0) {
    perror("bind failed");
    fprintf(stderr, "[ERROR] initialize_server_socket: Failed to bind socket to %s:%d.\n", ip, port);
    close(*server_fd_out);
    return -1;
  }

  if (listen(*server_fd_out, 10) < 0) {
    perror("listen failed");
    fprintf(stderr, "[ERROR] initialize_server_socket: Failed to listen on socket.\n");
    close(*server_fd_out);
    return -1;
  }

  printf("Beaker server listening on %s:%d\n", ip, port);
  return 0;
}

static void handle_client_connection(int new_socket) {
  current_client_socket = new_socket; 
  char buffer[BUFFER_SIZE] = {0};     

  ssize_t bytes_read = read(new_socket, buffer, BUFFER_SIZE - 1);
  if (bytes_read < 0) {
    perror("read failed");
    fprintf(stderr, "[ERROR] handle_client_connection: Failed to read from client socket.\n");
    close(new_socket);
    return;
  }
  buffer[bytes_read] = '\0'; 

  strncpy(current_request_buffer, buffer, BUFFER_SIZE - 1);
  current_request_buffer[BUFFER_SIZE - 1] = '\0';

  char request_line[MAX_PATH_LEN + 64]; 
  char *first_line_end = strstr(buffer, "\r\n"); 

  if (first_line_end == NULL) {
    fprintf(stderr, "[ERROR] handle_client_connection: Invalid HTTP request: No CRLF found.\n");
    const char *bad_request = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    send(new_socket, bad_request, strlen(bad_request), 0);
    close(new_socket);
    return;
  }
  size_t request_line_len = first_line_end - buffer;
  if (request_line_len >= sizeof(request_line)) {
    fprintf(stderr, "[ERROR] handle_client_connection: Request line too long.\n");
    const char *bad_request = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    send(new_socket, bad_request, strlen(bad_request), 0);
    close(new_socket);
    return;
  }
  strncpy(request_line, buffer, request_line_len);
  request_line[request_line_len] = '\0'; 

  UrlParams request_params; 
  char *requested_path = parse_request_url(request_line, &request_params); 

  if (requested_path == NULL) {
    fprintf(stderr, "[ERROR] handle_client_connection: Could not parse request path. Sending 400 Bad Request.\n");
    const char *bad_request = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    send(new_socket, bad_request, strlen(bad_request), 0);
    close(new_socket);
    return;
  }
  printf("Accessing: %s\n", requested_path); 

  bool handled = false; 

  if (strncmp(requested_path, "/static/", strlen("/static/")) == 0) {

    if (serve_static_file(requested_path + strlen("/static/"))) {
      handled = true; 
    }
  }

  if (!handled) {
    int best_match_handler_index = -1; 
    size_t best_match_len = 0;         

    for (int i = 0; i < handler_count; i++) {
      size_t handler_path_len = strlen(handlers[i].path);

      if (strncmp(requested_path, handlers[i].path, handler_path_len) == 0) {

        if (handler_path_len == strlen(requested_path) ||
            requested_path[handler_path_len] == '/') {

          if (handler_path_len > best_match_len) {
            best_match_len = handler_path_len;
            best_match_handler_index = i;
          }
        }
      }
    }

    if (best_match_handler_index != -1) {
      handlers[best_match_handler_index].handler(&request_params);
      handled = true; 
    }
  }

  if (!handled) {
    fprintf(stderr,
            "[WARNING] handle_client_connection: No handler or static file found for path '%s'. Sending 404 Not Found.\n",
            requested_path);
    const char *not_found_html = "<h1>404 Not Found</h1><p>The requested URL "
                                 "was not located on this server.</p>";
    char not_found_response[BUFFER_SIZE];
    snprintf(not_found_response, sizeof(not_found_response),
             "HTTP/1.1 404 Not Found\r\n"
             "Content-Type: text/html; charset=UTF-8\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n%s",
             strlen(not_found_html), not_found_html);
    send(new_socket, not_found_response, strlen(not_found_response), 0);
  }

  free(requested_path); 
  close(new_socket);    
  current_client_socket = -1; 
}

int beaker_run(const char *ip, int port) {
  int server_fd;            
  struct sockaddr_in address; 
  int addrlen = sizeof(address); 

  if (initialize_server_socket(ip, port, &server_fd, &address) != 0) {
    return -1; 
  }

  while (true) {
    int new_socket; 

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
      perror("accept failed");
      fprintf(stderr, "[ERROR] beaker_run: Failed to accept connection.\n");
      continue; 
    }

    handle_client_connection(new_socket);
  }

  close(server_fd); 
  return 0;
}