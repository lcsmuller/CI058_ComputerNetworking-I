#include <bits/stdc++.h>
using namespace std;

#define NUMDICE 5

class game {
private:
    vector<int> dice = vector<int>(NUMDICE,0);
    vector<bool> locked = vector<bool>(NUMDICE,false);

public:
    game(/* args */);
    ~game();
    void roll();
    void print_dice();
    int get_value();
    void print_bet();
};

game::game(/* args */) {
}

game::~game() {
}

void game::roll() {
    for (int i = 0; i < NUMDICE; i++) {
        if (!locked[i]) {
            dice[i] = rand() % 6 + 1;
            
        }
    }
    sort(dice.begin(), dice.end());
    locked = vector<bool>(NUMDICE,false); //reseta todas as locks
}

void game::print_dice() {
    for (int i = 0; i < NUMDICE; i++) {
        cout << dice[i] << ' ';
    }
    cout << '\n';
}

//retorna o "valor" dos dados. como não tem uma regra única, vai ficar caso-a-caso
//e bem feio.
int game::get_value() {
    multiset<int> m;
    for (int i = 0; i < NUMDICE; i++) {
        m.insert(dice[i]);
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
        diff = dice[i] - dice[i-1];
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
    cout << "Valor: " << val << "! ";
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
}

int main () {
    srand(time(NULL)); //randomização
    
    game g;
    g.roll();
    g.print_dice();
    g.print_bet();
}