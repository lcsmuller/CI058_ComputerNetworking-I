#include <bits/stdc++.h>
#include "player.h"

using namespace std;
using die = pair<int, bool>;

#define NUMDICE 5
class game
{
  private:
    vector<die> dice = vector<die>(NUMDICE, make_pair(0, false));
    vector<bool> locked = vector<bool>(NUMDICE, false);

    bool prompt_roll();
    void roll();

  public:
    game(/* args */);
    ~game();

    int get_target();
    void print_dice();
    int get_value();
    void print_bet();
    void play_round(unsigned bet);
    void print_balance();

    int coins = 5;
    int targetValue = 0;
};

game::game(/* args */) {}

game::~game() {}

void
game::print_balance()
{
    cout << "Moedas: " << coins << "\n";
}

void
game::roll()
{
    for (int i = 0; i < NUMDICE; i++) {
        if (!dice[i].second) {
            dice[i].first = rand() % 6 + 1;
        }
    }
    sort(dice.begin(), dice.end());
    locked = vector<bool>(NUMDICE, false); // reseta todas as locks
}

void
game::print_dice()
{
    cout << "Dados: (números entre [] estão trancados)\n";
    for (int i = 0; i < NUMDICE; i++) {
        if (dice[i].second) cout << "[";
        cout << dice[i].first;
        if (dice[i].second) cout << "]";
        cout << " ";
    }
    cout << '\n';
}

// retorna o "valor" dos dados. como não tem uma regra única, vai ficar
// caso-a-caso e bem feio.
int
game::get_value()
{
    multiset<int> m;
    for (int i = 0; i < NUMDICE; i++) {
        m.insert(dice[i].first);
    }
    for (int i = 1; i <= 6; i++) {
        if (m.count(i) == 5) return 15; // quinteto
        if (m.count(i) == 4) return 10; // quadra

        for (int j = 1; j <= 6; j++) {
            if (i == j) continue;
            if (m.count(i) == 3 && m.count(j) == 2) return 5; // full house
            if (m.count(i) == 2 && m.count(j) == 2) return 4; // dois pares
        }
    }
    // sabemos que não houve 2 pares, full house, quadra ou quinteto;
    // falta ver sequências, 1 trio e 1 par
    int diff;
    bool flushFlag = true;
    for (int i = 1; i < NUMDICE; i++) {
        diff = dice[i].first - dice[i - 1].first;
        if (diff != 1) flushFlag = false;
    }
    if (flushFlag) return 7; // sequência

    for (int i = 1; i <= 6; i++) {
        if (m.count(i) == 3) return 3; // um trio
        if (m.count(i) == 2) return 2; // um par
    }

    return 0;
}

void
game::print_bet()
{
    int val = game::get_value();
    switch (val) {
    case 15:
        cout << "Quinteto!\n";
        break;
    case 10:
        cout << "Quadra!\n";
        break;
    case 7:
        cout << "Sequência!\n";
        break;
    case 5:
        cout << "Full House!\n";
        break;
    case 4:
        cout << "Dois pares!\n";
        break;
    case 3:
        cout << "Um trio!\n";
        break;
    case 2:
        cout << "Um par!\n";
        break;

    default:
        cout << "Nenhuma combinação...\n";
        break;
    }
    if (val == targetValue) {
        cout << "Aposta sucedida!! +" << val << " moedas!\n";
    }
    else {
        cout << "Aposta não sucedida...\n";
    }
}

bool
valid_prompt(char c)
{
    return (c == 'y' || c == 'Y' || c == 'n' || c == 'N');
}

bool
yes_prompt(char c)
{
    return (c == 'y' || c == 'Y');
}

bool
no_prompt(char c)
{
    return (c == 'n' || c == 'N');
}

bool
game::prompt_roll()
{
    bool rolling = false;
    char c = '\0', d = '\0', e = '\0';
    int toLock = -1;

    while (!valid_prompt(c)) {
        cout << "Quer jogar os dados de novo? (Y/N)\n";
        cin >> c;
        if (yes_prompt(c)) {
            rolling = true;
            while (!valid_prompt(d)) {
                cout << "Quer trancar um dado? (Y/N)\n";
                cin >> d;
                if (yes_prompt(d))
                    while (toLock < 0 || toLock >= NUMDICE) {
                        cout << "Digite o índice do dado a ser trancado (1-"
                             << NUMDICE << ")\n";
                        cin >> e;
                        toLock = e - '0';
                        toLock--;
                        if (0 <= toLock && toLock < NUMDICE) {
                            if (dice[toLock].second) {
                                cout << "Dado já trancado\n";
                                toLock = -1;
                            }
                            else
                                dice[toLock].second = true;
                        }
                    }
            }
        }
    }
    return rolling;
}

int
game::get_target()
{
    int targetIndex = -1;
    char in = '\0';

    while (targetIndex < 1 || targetIndex > 7) {
        cout << "Em qual combinação quer apostar? (Digite o índice)\n"
             << "1: Um par\n"
             << "2: Um trio\n"
             << "3: Dois pares\n"
             << "4: Full House\n"
             << "5: Sequência\n"
             << "6: Quadra\n"
             << "7: Quinteto\n";
        cin >> in;
        if (in - '0' < 1 || in - '0' > 7) {
            cout << "Entrada inválida\n";
            targetIndex = -1;
        }
        else {
            targetIndex = in - '0';
        }
    }

    switch (targetIndex) {
    case 1:
        targetValue = 2;
        break;
    case 2:
        targetValue = 3;
        break;
    case 3:
        targetValue = 4;
        break;
    case 4:
        targetValue = 5;
        break;
    case 5:
        targetValue = 7;
        break;
    case 6:
        targetValue = 10;
        break;
    case 7:
        targetValue = 15;
        break;
    default:
        targetValue = 0;
        break;
    }
    return targetIndex;
}

void
game::play_round(unsigned bet)
{
    for (int i = 0; i < NUMDICE; i++)
        dice[i].second = 0;
    coins -= bet;
    roll();
    print_dice();
    bool rolling = false;
    rolling = prompt_roll();
    if (rolling) { // segunda jogada
        roll();
        print_dice();
        rolling = prompt_roll();
        if (rolling) { // terceira jogada
            roll();
            print_dice();
        }
    }
    print_bet();
    if (get_value() == targetValue) {
        coins += targetValue;
    }
}

/** @brief Índices do bastão e seus significados */
enum baton_indexes {
    /** tipo da aposta */
    BATON_BET_TYPE = 0,
    /** posição do jogador inicial da partida (0 a 3) */
    BATON_INITIAL_PLAYER = 1,
    /** posição do jogador com maior a maior aposta da partida (0 a 3) */
    BATON_BET_LEADER = 2,
    /** maior quantidade apostada na partida */
    BATON_BET_AMOUNT = 3,
    /** quantidade total de índices para o bastão */
    BATON_MAX,
};

/** indica fim de jogo */
#define END_GAME -1
/** indica novo jogo */
#define NEW_GAME -2

void
baton_update(char baton[BATON_INITIAL_PLAYER],
             char bet_type,
             char initial_player_pos,
             char lead_player_pos,
             char bet_amount)
{
    baton[BATON_BET_TYPE] = bet_type;
    baton[BATON_INITIAL_PLAYER] = initial_player_pos;
    baton[BATON_BET_LEADER] = lead_player_pos;
    baton[BATON_BET_AMOUNT] = bet_amount;
}

int
main(int argc, char *argv[])
{
    char baton[BATON_MAX] = { 0 };
    struct player *player;
    game g;

    srand(time(NULL));

    if (argc <= 1) {
        fputs("Error: Insira o número do jogador entre 1 a 4!\n", stderr);
        return EXIT_FAILURE;
    }

    const int currPlayerPos = (int)strtoul(argv[1], NULL, 10) - 1;
    if (currPlayerPos < 0 || currPlayerPos > 3) {
        fputs("Error: O número do jogador deve ser entre 1 a 4!\n", stderr);
        return EXIT_FAILURE;
    }
    player = player_create(currPlayerPos);

    int cycle = 1; // ciclo corrente
    while (1) {
        if (g.coins <= 0 || baton[BATON_BET_TYPE] == END_GAME) {
            baton_update(baton, END_GAME, baton[BATON_INITIAL_PLAYER],
                         baton[BATON_BET_LEADER], 1);
            if (player_send_to_next(player, baton, sizeof(baton)) < 0)
                perror("Não foi possível enviar ao próximo jogador ");
            break;
        }

        if (currPlayerPos == baton[BATON_INITIAL_PLAYER]) { // origem
            if (cycle == 1) { // inicia jogada da partida
                g.get_target();
                cout << "\n";

                baton_update(baton, g.targetValue, baton[BATON_INITIAL_PLAYER],
                             baton[BATON_BET_LEADER], 1);
                ++cycle;
            }
            else if (cycle == 2) { // aguarda lider jogar dado da aposta
                if (player_recv_from_prev(player, baton, sizeof(baton)) < 0) {
                    perror("Não foi possível receber do jogador anterior ");
                    break;
                }
                ++cycle;
            }
            else if (cycle == 3) { // inicia proxima partida
                if (player_recv_from_prev(player, baton, sizeof(baton)) < 0) {
                    perror("Não foi possível receber do jogador anterior ");
                    break;
                }

                const int incr =
                    (currPlayerPos < 3) ? 1 : -baton[BATON_INITIAL_PLAYER];
                baton_update(baton, NEW_GAME,
                             baton[BATON_INITIAL_PLAYER] + incr,
                             baton[BATON_INITIAL_PLAYER] + incr, 0);
                cycle = 1;
            }

            if (baton[BATON_BET_TYPE] == END_GAME) continue;

            if (player_send_to_next(player, baton, sizeof(baton)) < 0) {
                perror("Não foi possível enviar ao próximo jogador ");
                break;
            }
        }
        else { // jogador
            if (player_recv_from_prev(player, baton, sizeof(baton)) < 0) {
                perror("Não foi possível receber do jogador anterior ");
                break;
            }

            if (baton[BATON_BET_TYPE] == END_GAME) continue;

            if (cycle == 3) {
                cycle = 1;

                // jogador é a nova origem, volta para inicio do loop
                if (currPlayerPos == baton[BATON_INITIAL_PLAYER]) continue;
            }

            if (cycle == 1) {
                char c;
                while (1) {
                    cout << "Deseja jogar? (Y/N)\n";
                    cin >> c;
                    if (no_prompt(c))
                        break;
                    else if (yes_prompt(c)) {
                        g.get_target();
                        cout << "\n";

                        baton_update(
                            baton, g.targetValue, baton[BATON_INITIAL_PLAYER],
                            currPlayerPos, baton[BATON_BET_AMOUNT] + 1);

                        break;
                    }
                }
            }
            else if (cycle == 2 && currPlayerPos == baton[BATON_BET_LEADER]) {
                // lider faz sua jogada
                g.print_balance();
                g.play_round(baton[BATON_BET_AMOUNT]);
            }

            if (player_send_to_next(player, baton, sizeof(baton)) < 0) {
                perror("Não foi possível enviar ao próximo jogador ");
                break;
            }

            ++cycle;
        }
    }

    cout << "Fim de jogo!\n";

    return EXIT_SUCCESS;
}