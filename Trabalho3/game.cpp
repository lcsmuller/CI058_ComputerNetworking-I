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
}

void game::print_dice() {
    for (int i = 0; i < NUMDICE; i++) {
        cout << dice[i] << ' ';
    }
    cout << '\n';
}

int main () {
    srand(time(NULL)); //randomização
    
    game g;
    g.roll();
    g.print_dice();
}