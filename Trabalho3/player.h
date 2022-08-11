#ifndef PLAYER_H
#define PLAYER_H

#include <arpa/inet.h>

/** @brief Estrutura do jogador */
struct player {
  /** endereço em que o jogador reside */
  struct sockaddr_in addr;
  /** socket que o jogador reside */
  int sockfd;
  /** rivais imediatos do jogador */
  struct player *prev, *next;
  /** posição do jogador (0 a 3) */
  unsigned position;
};

/**
 * @brief Inicializa jogador e o aloca à uma porta
 *
 * @param position posição do jogador (0 a 3)
 * @return jogador inicializado
 */
struct player player_create(const unsigned position);

/**
 * @brief Libera jogador alocado a uma porta
 *
 * @param player jogador a ser liberado
 */
void player_cleanup(struct player player);

/**
 * @brief Envia mensagem para o próximo jogador
 * 
 * @param player jogador corrente
 * @param baton bastão
 * @param nbytes qtd de bytes a ser enviado
 * @return < 0 em casa de falha
 */
int player_send_to_next(struct player *player, const char *baton, size_t nbytes);

/**
 * @brief Envia mensagem para o jogador anterior
 * 
 * @param player jogador corrente
 * @param baton bastão
 * @param nbytes qtd de bytes a ser recebido
 * @return < 0 em casa de falha
 */
int player_recv_from_prev(struct player *player, char *baton, size_t nbytes);

#endif /* PLAYER_H */
