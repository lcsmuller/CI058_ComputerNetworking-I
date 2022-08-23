#include <bits/stdc++.h>

#include "player.h"

int
main(int argc, char *argv[])
{
    unsigned char baton[1024];
    struct player *player;
    unsigned position;

    if (argc <= 1) {
      fputs("Error: Insira o número do jogador de 1 a 4!\n", stderr);
      return EXIT_FAILURE;
    }
    position = (unsigned)strtoul(argv[1], NULL, 10);
    if (position < 1 || position > 4) {
      fputs("Error: O número do jogador deve ser de 1 a 4!\n", stderr);
      return EXIT_FAILURE;
    }

    player = player_create(position - 1);
    if (player_get_position(player) == 0) { // primeiro jogador
      puts("Insira mensagem inicial a ser passada pelo bastão:\t");
      fgets((char *)baton, sizeof(baton), stdin);
      player_send_to_next(player, baton, sizeof(baton));
    }

    while (1) { // loop de jogo
      // aguarda bastão do jogador anterior
      if (player_recv_from_prev(player, baton, sizeof(baton)) < 0) {
        perror("Não foi possível receber do jogador anterior: ");
        break;
      }
      fprintf(stderr, "Rival anterior envia: %s\n", baton);

      /* altera mensagem do bastão */
      puts("Insira nova mensagem a ser passada pelo bastão:\t");
      fgets((char *)baton, sizeof(baton), stdin);

      // passa bastão para o próximo jogador
      if (player_send_to_next(player, baton, sizeof(baton)) < 0) {
        perror("Não foi possível enviar para o próximo jogador: ");
        break;
      }
    }

    player_cleanup(player);

    return EXIT_FAILURE;
}
