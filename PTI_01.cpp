#include <iostream>
#include <string>
#include <vector>
#include <conio.h> 
#include <thread>

using namespace std;

struct Position {
    int x;
    int y;
};

enum class GameState { PLAYING, CAUGHT, ESCAPED, EXIT };

const int MAX_STAGE = 5;
const int stageLengths[6] = { 0, 10, 15, 21, 25, 30 }; // Target X to win
const int stairPos[6]     = { 0, -1,  8,  8, 12, 15 }; // X-coordinate of stairs
const int maxFloors[6]    = { 0,  0,  1,  2,  3,  4 }; // Maximum Y-coordinate (floors)

void applyMove(Position* pos, int dx, int dy) {
    pos->x += dx;
    pos->y += dy;
}

bool isAtLadder(const Position& pos, int currentStage) {
    if (currentStage == 1) return false; 
    return (pos.x == stairPos[currentStage]);
}

void drawUI(int stage, Position player, Position shadow, bool shadowActive, const vector<string>& logs) {
    system("cls");
    cout << "========================================================\n";
    cout << "                  PATH OF IRIDESCENT                    \n";
    cout << "========================================================\n";
    cout << " STAGE: " << stage << " | GOAL: Reach X=" << stageLengths[stage] 
         << " on Floor " << maxFloors[stage] << "\n";
    cout << "--------------------------------------------------------\n";
    cout << " PLAYER POSITION : Floor " << player.y << ", Space " << player.x << "\n";
    
    if (shadowActive) {
        cout << " SHADOW POSITION : Floor " << shadow.y << ", Space " << shadow.x << "\n";
    } else {
        cout << " SHADOW POSITION : [Not Spawned Yet]\n";
    }
    cout << "--------------------------------------------------------\n";
    cout << " CONTROLS: W (Up) / S (Down) / A (Left) / D (Right)\n";
    cout << "--------------------------------------------------------\n";
 
    for (size_t i = 0; i < logs.size(); i++) {
        cout << " > " << logs[i] << "\n";
    }
    cout << "========================================================\n";
    cout << "Action: ";
}

void processShadowAI(Position& player, Position& shadow, int stage) {
    if (shadow.x < player.x) shadow.x++;
    else if (shadow.x > player.x) shadow.x--;

    if (isAtLadder(shadow, stage)) {
        if (shadow.y < player.y) shadow.y++;
        else if (shadow.y > player.y) shadow.y--;
    }
}

int main() {
    int currentStage = 1;
    GameState state = GameState::PLAYING;
    
    Position player = {0, 0};
    Position shadow = {-1, 0};
    bool shadowActive = false;
    
    vector<string> gameLogs; 
    gameLogs.push_back("Game Initialized. Make your move.");

    while (state != GameState::EXIT) {

        // the most amazing addition ull ever see
        this_thread::sleep_for(360000ms);
     
        while (state == GameState::PLAYING) {
            drawUI(currentStage, player, shadow, shadowActive, gameLogs);
         
            if (gameLogs.size() > 3) gameLogs.erase(gameLogs.begin());

            char input = _getch();
            bool validMove = false;

            switch (input) {
                case 'd': case 'D':
                    if (player.x < stageLengths[currentStage]) {
                        applyMove(&player, 1, 0); 
                        gameLogs.push_back("Player moved Right.");
                        validMove = true;
                    } else gameLogs.push_back("Wall ahead!");
                    break;
                case 'a': case 'A':
                    if (player.x > 0) {
                        applyMove(&player, -1, 0);
                        gameLogs.push_back("Player moved Left.");
                        validMove = true;
                    } else gameLogs.push_back("Cannot move further left.");
                    break;
                case 'w': case 'W':
                    if (isAtLadder(player, currentStage) && player.y < maxFloors[currentStage]) {
                        applyMove(&player, 0, 1);
                        gameLogs.push_back("Player climbed UP the ladder.");
                        validMove = true;
                    } else gameLogs.push_back("No ladder here or already at top!");
                    break;
                case 's': case 'S':
                    if (isAtLadder(player, currentStage) && player.y > 0) {
                        applyMove(&player, 0, -1);
                        gameLogs.push_back("Player climbed DOWN the ladder.");
                        validMove = true;
                    } else gameLogs.push_back("No ladder here or already at bottom!");
                    break;
                default:
                    gameLogs.push_back("Invalid key pressed.");
                    continue; 
            }

            if (validMove) {
                if (!shadowActive && player.x > 2) {
                    shadowActive = true;
                    shadow.x = 0;
                    shadow.y = 0;
                    gameLogs.push_back("WARNING: THE SHADOW IS HUNTING YOU!");
                } else if (shadowActive) {
                    processShadowAI(player, shadow, currentStage);
                }
            }

            if (shadowActive && player.x == shadow.x && player.y == shadow.y) {
                state = GameState::CAUGHT;
            } else if (player.x == stageLengths[currentStage] && player.y == maxFloors[currentStage]) {
                state = GameState::ESCAPED;
            }
        }

        drawUI(currentStage, player, shadow, shadowActive, gameLogs);
        
        if (state == GameState::CAUGHT) {
            cout << "\n[!] THE SHADOW CAUGHT YOU! YOU DIED.\n";
            cout << "1. Retry Stage\n2. Main Menu (Quit)\nChoice: ";
            char choice = _getch();
            if (choice == '1') {
                player = {0, 0}; shadow = {-1, 0}; shadowActive = false;
                gameLogs.clear(); gameLogs.push_back("Stage Reset.");
                state = GameState::PLAYING;
            } else {
                state = GameState::EXIT;
            }
        } 
        else if (state == GameState::ESCAPED) {
            if (currentStage < MAX_STAGE) {
                cout << "\n[+] STAGE " << currentStage << " CLEARED!\n";
                cout << "1. Proceed to Stage " << currentStage + 1 << "\n2. Main Menu (Quit)\nChoice: ";
                char choice = _getch();
                if (choice == '1') {
                    currentStage++;
                    player = {0, 0}; shadow = {-1, 0}; shadowActive = false;
                    gameLogs.clear(); gameLogs.push_back("Entered New Stage.");
                    state = GameState::PLAYING;
                } else {
                    state = GameState::EXIT;
                }
            } else {
                cout << "\n[!!!] YOU ESCAPED ALL STAGES! YOU WIN!\n";
                cout << "Press any key to exit...";
                _getch();
                state = GameState::EXIT;
            }
        }
    }

    return 0;
}
