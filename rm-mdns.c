#define _DEFAULT_SOURCE

#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include "mdns.h"

static const char RM_MDNS_HOSTNAME[] = "remarkable-stha.local.";
static const size_t RM_MDNS_HOSTNAME_LENGTH = sizeof(RM_MDNS_HOSTNAME) - 1;

static const char RM_MDNS_WIFI_IFACE[] = "wlan0";

static char namebuffer[256];
static char recvbuffer[2048];
static char sendbuffer[1024];

static int obtain_ipv4_address(struct sockaddr_in *addr) {
  if (addr == nullptr)
    return -1;

  // Request IPv4 address of wlan0.
  struct ifreq ifr = {.ifr_addr.sa_family = AF_INET};
  strncpy(ifr.ifr_name, RM_MDNS_WIFI_IFACE, IFNAMSIZ - 1);

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd == -1) {
    perror("[obtain_ipv4_address] Could not open socket");
    return -1;
  }

  if (ioctl(fd, SIOCGIFADDR, &ifr) == -1) {
    perror("[obtain_ipv4_address] Could not get interface address");
    return -1;
  }

  close(fd);

  memcpy(addr, &ifr.ifr_addr, sizeof(struct sockaddr_in));
  return 0;
}

static int callback(int sock, const struct sockaddr *from, size_t addrlen,
                    mdns_entry_type_t entry, uint16_t query_id, uint16_t rtype,
                    uint16_t rclass, uint32_t ttl, const void *data,
                    size_t size, size_t name_offset, size_t name_length,
                    size_t record_offset, size_t record_length,
                    void *user_data) {
  // We only answer to question types.
  if (entry != MDNS_ENTRYTYPE_QUESTION)
    return 0;

  // Extract the name from the buffer. The native format is one byte for
  // length of the next fragment and afterwards comes the fragment. All
  // name fragments are joined by `.`.
  // NOTE: The function `mdns_string_extract` might change the output parameter
  // offset. Therefore we create a copy.
  size_t offset = name_offset;
  mdns_string_t name =
      mdns_string_extract(data, size, &offset, namebuffer, sizeof(namebuffer));

  // We only anwser for our name.
  if (name.length != RM_MDNS_HOSTNAME_LENGTH ||
      memcmp(name.str, RM_MDNS_HOSTNAME, RM_MDNS_HOSTNAME_LENGTH) != 0)
    return 0;

  // Return A record.
  if (rtype == MDNS_RECORDTYPE_A) {
    uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
    if (unicast)
      printf("WARNING: Unicast bit was set! Unicast response not implemented, "
             "replying via multicast.\n");
    printf("Sending reply for our A record\n");
    fflush(stdout);

    mdns_record_t answer = {
        .name.str = RM_MDNS_HOSTNAME,
        .name.length = RM_MDNS_HOSTNAME_LENGTH,
        .type = MDNS_RECORDTYPE_A,
    };

    if (obtain_ipv4_address(&answer.data.a.addr) == -1) {
      printf("WARNING: Unable to obtain IPv4, skipping reply.\n");
      fflush(stdout);
      return 0;
    }

    mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0,
                                0, 0, 0);
  }

  return 0;
}

static int open_listening_socket() {
  // Open sockets to listen to incoming mDNS queries on port 5353
  struct sockaddr_in sock_addr;
  memset(&sock_addr, 0, sizeof(struct sockaddr_in));
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_addr.s_addr = INADDR_ANY;
  sock_addr.sin_port = htons(MDNS_PORT);

  int socket = mdns_socket_open_ipv4(&sock_addr);
  if (socket == -1) {
    perror("[open_listening_socket] Could not open socket");
    exit(EXIT_FAILURE);
  }

  return socket;
}

static int setup_signal_handler() {
  // Handle SIGINT and SIGQUIT.
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGQUIT);
  sigaddset(&mask, SIGTERM);

  // Block signals so that they are not handled by default handler.
  // Otherwise they would not trigger the signalfd.
  if (sigprocmask(SIG_BLOCK, &mask, nullptr) == -1) {
    perror("[setup_signal_handler] Could not mask signals");
    exit(EXIT_FAILURE);
  }

  // Create new signalfd.
  int sfd = signalfd(-1, &mask, 0);
  if (sfd == -1) {
    perror("[setup_signal_handler] Could not create signalfd");
    exit(EXIT_FAILURE);
  }

  return sfd;
}

static int setup_epoll(int socketfd, int sigfd) {
  struct epoll_event ev;

  // Create epoll instance.
  int epollfd = epoll_create1(0);
  if (epollfd == -1) {
    perror("[setup_epoll] Could not create epollfd");
    exit(EXIT_FAILURE);
  }

  // Add socket read event.
  ev.events = EPOLLIN;
  ev.data.fd = socketfd;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &ev) == -1) {
    perror("[setup_epoll] Could not add socket to epollfd");
    exit(EXIT_FAILURE);
  }

  // Add signalfd read event.
  ev.events = EPOLLIN;
  ev.data.fd = sigfd;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sigfd, &ev) == -1) {
    perror("[setup_epoll] Could not add signalfd to epollfd");
    exit(EXIT_FAILURE);
  }

  return epollfd;
}

int main(int argc, const char *const *argv) {
  int socketfd = open_listening_socket();
  int sigfd = setup_signal_handler();
  int epollfd = setup_epoll(socketfd, sigfd);

  for (;;) {
    struct epoll_event ev;
    if (epoll_wait(epollfd, &ev, 1, -1) == -1) {
      perror("[main] Failure on epoll_wait");
      exit(EXIT_FAILURE);
    }

    if (ev.data.fd == sigfd) {
      printf("Signal received. Shutting down...\n");
      break;
    }

    mdns_socket_listen(socketfd, recvbuffer, sizeof(recvbuffer), callback,
                       nullptr);
  }

  mdns_socket_close(socketfd);
}
