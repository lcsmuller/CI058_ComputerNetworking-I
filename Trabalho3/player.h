#ifndef PLAYER_H
#define PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa jogador e o aloca à uma porta
 *
 * @param position posição do jogador (0 a 3)
 * @return jogador inicializado
 */
struct player *player_create(const unsigned position);

/**
 * @brief Libera jogador alocado a uma porta
 *
 * @param player jogador a ser liberado
 */
void player_cleanup(struct player *player);

/**
 * @brief Envia mensagem para o próximo jogador
 *
 * @param player jogador corrente
 * @param baton bastão
 * @param nbytes qtd de bytes a ser enviado
 * @param flags sendto() flags
 */
void player_send_to_next(struct player *player,
                         const char *baton,
                         size_t nbytes,
                         int flags);

/**
 * @brief Envia mensagem para o jogador anterior
 *
 * @param player jogador corrente
 * @param baton bastão
 * @param nbytes qtd de bytes a ser recebido
 * @param flags recvfrom() flags
 */
void player_recv_from_prev(struct player *player,
                           char *baton,
                           size_t nbytes,
                           int flags);

/**
 * @brief Recebe posição do jogador corrente (0 a 3)
 *
 * @param player jogador corrente
 * @return posição do jogador
 */
unsigned player_get_position(struct player *player);

#ifdef __cplusplus
}
#endif

#endif /* PLAYER_H */
