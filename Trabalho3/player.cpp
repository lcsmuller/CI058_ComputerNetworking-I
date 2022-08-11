#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include <bits/stdc++.h>

#include "player.h"

#define SERVER_PATH "/tmp/serversocket"
#define PORT 5001

static void
_player_address_load(struct player *player, const unsigned position)
{
  /* "casa" um endereço ao socket referenciado por sockfd */
  memset(&player->addr, 0, sizeof(player->addr));
  player->addr.sin_family = AF_INET;
  player->addr.sin_port = htons(PORT + position);
  player->addr.sin_addr.s_addr = inet_addr("127.0.0.1"); /* localhost */
}

struct player
player_create(const unsigned position)
{
  struct player player = {};
  int opt = 1;

  unlink(SERVER_PATH);

  // cria file descriptor socket
  if ((player.sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket(): ");
    exit(EXIT_FAILURE);
  }
  // configura socket para reutilizar endereço e porta
  if (setsockopt(player.sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                 &opt, sizeof(opt)))
  {
    perror("setsockopt(): ");
    exit(EXIT_FAILURE);
  }

  // "casa" um endereço ao socket referenciado por sockfd
  _player_address_load(&player, position);
  if (bind(player.sockfd, (struct sockaddr *)&player.addr, sizeof(player.addr))
      < 0)
  {
    perror("bind(): ");
    exit(EXIT_FAILURE);
  }
  if (!(player.next = (struct player *)calloc(1, sizeof *player.next))) {
    perror("malloc(): ");
    exit(EXIT_FAILURE);
  }
  if (!(player.prev = (struct player *)calloc(1, sizeof *player.prev))) {
    perror("malloc(): ");
    exit(EXIT_FAILURE);
  }

  // carrega endereço dos rivais imediatos
  _player_address_load(player.next, position == 3 ? position - 3 : position + 1);
  _player_address_load(player.prev, position == 0 ? position + 3 : position - 1);
  player.position = position;

  return player;
}

void
player_cleanup(struct player player)
{
  free(player.next);
  free(player.prev);
  close(player.sockfd);
}

int
player_send_to_next(struct player *player, const char *baton, size_t nbytes)
{
  return sendto(player->sockfd, baton, nbytes, 0,
          (struct sockaddr *)&player->next->addr,
          sizeof(player->next->addr));
}

int
player_recv_from_prev(struct player *player, char *baton, size_t nbytes)
{
  socklen_t size = sizeof(player->prev->addr);
  return recvfrom(player->sockfd, baton, nbytes, 0,
          (struct sockaddr *)&player->prev->addr, &size);
}

