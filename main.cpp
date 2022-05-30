#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define MAX_PENDING_CONN_COUNT 1

// voy a leer en chunks de 10 bytes los mensajes que me llegan
#define CHUNK_SIZE 10

const char* reset = "\033[0m";
const char* red = "\033[0;31m";
const char* green = "\033[0;32m";
const char* yellow = "\033[0;33m";
const char* blue = "\033[0;34m";
const char* pink = "\033[0;35m";
const char* teal = "\033[0;36m";

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: %s <port>\n", argv[0]);
    return 0;
  }

  int port = atoi(argv[1]);

  printf("%sselected port: %d\n%s", blue, port, reset);

  /*
    crear el tcp socket
  */

  int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (tcp_socket == -1) {
    perror("error creating TCP socket");
    return 1;
  }

  /*
    asociar el tcp socket a localhost:port
  */

  struct sockaddr_in tcp_socket_address;
  memset(&tcp_socket_address, 0, sizeof(tcp_socket_address));
  tcp_socket_address.sin_family = AF_INET;
  tcp_socket_address.sin_port = htons(port);

  if (bind(tcp_socket, (struct sockaddr*)&tcp_socket_address,
           sizeof(tcp_socket_address)) == -1) {
    perror("error binding TCP socket to address");
    return 1;
  }

  /*
    marcar el tcp socket como un socket que acepta conexiones
    pero que no las empieza, además, el tamaño del queue
    donde están las conexiones todavía no aceptabas es 1
  */

  if (listen(tcp_socket, MAX_PENDING_CONN_COUNT) == -1) {
    perror("could not make TCP socket listen for connections");
    return 1;
  }

  /*
    bloquear hasta que venga una conexión
  */

  struct sockaddr_in conn_address;
  socklen_t conn_address_size = sizeof(conn_address);

  int conn =
      accept(tcp_socket, (struct sockaddr*)&conn_address, &conn_address_size);

  printf("%sWaiting for connections...\n%s", yellow, reset);

  /*
    extraer la tupla ip:port
  */

  const char* client_ip = inet_ntoa(conn_address.sin_addr);
  int client_port = ntohs(conn_address.sin_port);

  printf("%sclient at %s:%d connected\n%s", green, client_ip, client_port,
         reset);

  /*
    enviar un mensaje de bienvenida al cliente
  */

  const char* welcome_message = "welcome to my server\n";

  int bytes_sent = send(conn, welcome_message, strlen(welcome_message), 0);

  if (bytes_sent < 0) {
    perror("error sending welcome msg");
    return 1;
  }

  char recv_buffer[CHUNK_SIZE];

  while (1) {
    printf("%sWaiting for message from client...\n%s", teal, reset);

    /*
      bloquear hasta que se reciba un mensaje del cliente
      se recibe solo 10 bytes del mensaje, en la siguiente iteración
      del while se recibirán los siguientes 10 hasta que no quede
      data en el recv buffer del sistema operativo
    */

    int bytes_received = recv(conn, recv_buffer, CHUNK_SIZE, 0);
    if (bytes_received < 0) {
      perror("error receiving msg from client");
      return 1;
    }
    if (bytes_received == 0) {
      printf("%sclient at %s:%d disconnected\n%s", red, client_ip, client_port,
             reset);
      break;
    }

    printf("%s================\n%s", pink, reset);
    for (int i = 0; i < bytes_received; i++) {
      printf("%c", recv_buffer[i]);
    }
    printf("\n%s================\n%s", pink, reset);

    /*
      mandarle reply al cliente
    */

    const char* reply = "i got your message\n";
    int reply_bytes_sent = send(conn, reply, strlen(reply), 0);

    if (reply_bytes_sent < 0) {
      perror("error sending reply");
      return 1;
    }
  }

  printf("%sClosing TCP socket...\n%s", red, reset);
  shutdown(conn, SHUT_RDWR);

  return 0;
}