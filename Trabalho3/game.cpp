#include <bits/stdc++.h>
using namespace std;
using die = pair<int,bool>;

#define NUMDICE 5


class game {
private:
    vector<die> dice = vector<die>(NUMDICE,make_pair(0,false));
    //vector<int> dice = vector<int>(NUMDICE,0);
    vector<bool> locked = vector<bool>(NUMDICE,false);

    bool prompt_roll();
    void roll();
    void get_target();

public:
    game(/* args */);
    ~game();

    void print_dice();
    int get_value();
    void print_bet();
    void play_round();
    void print_balance();
    

    int coins = 5;
    int bet = 1;
    int targetValue = 0;
};

game::game(/* args */) {
}

game::~game() {
}

void game::print_balance() {
    cout << "Moedas: " << coins << "\n";
}

void game::roll() {
    for (int i = 0; i < NUMDICE; i++) {
        if (!dice[i].second) {
            dice[i].first = rand() % 6 + 1;
            
        }
    }
    //dice[0].second = true;
    //dice[2].second = true;
    sort(dice.begin(), dice.end());
    locked = vector<bool>(NUMDICE,false); //reseta todas as locks
}

void game::print_dice() {
    cout << "Dados: (números entre [] estão trancados)\n";
    for (int i = 0; i < NUMDICE; i++) {
        if (dice[i].second)
            cout << "[";
        cout << dice[i].first;
        if (dice[i].second)
            cout << "]";
        cout << " ";

    }
    cout << '\n';
}

//retorna o "valor" dos dados. como não tem uma regra única, vai ficar caso-a-caso
//e bem feio.
int game::get_value() {
    multiset<int> m;
    for (int i = 0; i < NUMDICE; i++) {
        m.insert(dice[i].first);
    }
    for (int i = 1; i <= 6; i++) {
        if (m.count(i) == 5)
            return 15;  //quinteto
        if (m.count(i) == 4)
            return 10;  //quadra

        for (int j = 1; j <= 6; j++) {
            if (i == j) continue;
            if (m.count(i) == 3 && m.count(j) == 2)
                return 5; //full house
            if (m.count(i) == 2 && m.count(j) == 2)
                return 4; //dois pares
        }
    }
    //sabemos que não houve 2 pares, full house, quadra ou quinteto;
    //falta ver sequências, 1 trio e 1 par
    int diff;
    bool flushFlag = true;
    for (int i = 1; i < NUMDICE; i++) {
        diff = dice[i].first - dice[i-1].first;
        if (diff != 1)
            flushFlag = false;
    }
    if (flushFlag)
        return 7; //sequência

    for (int i = 1; i <= 6; i++) {
        if (m.count(i) == 3)
            return 3; //um trio
        if (m.count(i) == 2)
            return 2; //um par
    }

    return 0;
}

void game::print_bet() {
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

bool valid_prompt(char c) {
    return (c == 'y' || c == 'Y' || c == 'n' || c == 'N');
}

bool yes_prompt(char c) {
    return (c == 'y' || c == 'Y');
}

bool no_prompt(char c) {
    return (c == 'n' || c == 'N');
}

bool game::prompt_roll() {
    bool rolling = false;
    char c = '\0',d = '\0', e = '\0';
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
                        cout << "Digite o índice do dado a ser trancado (1-" << NUMDICE <<")\n";
                        cin >> e;
                        toLock = e - '0';
                        toLock--;
                        if (0 <= toLock  && toLock < NUMDICE) {
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

void game::get_target() {
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
}

void game::play_round() {
    for (int i = 0; i < NUMDICE; i++)
        dice[i].second = 0;
    coins -= bet;
    get_target();
    roll();
    print_dice();
    bool rolling = false;
    rolling = prompt_roll();
    if (rolling) { //segunda jogada
        roll();
        print_dice();
        rolling = prompt_roll();
        if (rolling) {  //terceira jogada
            roll();
            print_dice();
        }
    }
    print_bet();
    if (get_value() == targetValue) {
        coins += targetValue;
    }
}

/* TODO: connections, bet escalation
*
*/

int main () {
    srand(time(NULL)); //randomização
    int received = 0; //dado a ser recebido
    bool canPlay = true; //habilitado se este jogador puder jogar
    game g;
    while (g.coins > 0 && received >= 0) {
        if (canPlay) {
            g.print_balance();
            g.play_round();
        }
        cout << "\n";
    }
    cout << "Fim de jogo!\n";
    //g.print_dice();
    //g.print_bet();
}