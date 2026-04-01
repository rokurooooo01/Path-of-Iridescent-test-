#include <iostream>
#include <string>
#include <vector>
#include <conio.h> 
#include <cstdlib> 
#include <ctime>   
#include <cmath>   
#include <thread>   
#include <chrono> 
#include <fstream>

using namespace std;

struct Position {
    int x;
    int y;
};

struct MapItem {
    Position pos;
    string name;
    char symbol;
    bool active;
};

struct Hallucination {
    Position pos;
    bool active;
};

enum class GameState { MAIN_MENU, PLAYING, STORY, LORE_LIBRARY, CAUGHT, ESCAPED, EXIT, GAMEOVER };

const int MAX_STAGE = 10;
const int stageLengths[11] = { 0, 10, 15, 21, 25, 30, 35, 40, 45, 50, 55 }; 
const int maxFloors[11]    = { 0,  0,  1,  2,  3,  4,  4,  5,  5,  6,  6 }; 

int stairPos[11][7];          
Position keyPositions[11];    
Position generatorPos[11];     
Position warpGates[11][2];    
Position holes[11][3];        
MapItem stageItems[11][8];     
Position lockers[11][4];       

bool loreUnlocked[5] = {false, false, false, false, false};
const string loreTitles[5] = {
    "1. The Architect's Fall", 
    "2. Nature of the Shadow", 
    "3. The Hallucinations", 
    "4. The Light is Fading", 
    "5. The Scent of Fear"
};
const string loreText[5] = {
    "We built the Spire to touch the stars, but we dug too deep. The dark seeped in.",
    "It hunts by sound. It mimics our fears. If you throw something, it will chase the noise.",
    "When the mind breaks in the dark, you will see it everywhere. Trust only the light.",
    "Batteries... I need more batteries. When the beam dies, it finds you immediately.",
    "It can smell your terror. Do not stay in one place. It will follow your path."
};

char terrain3D[MAX_STAGE + 1][7][60];
int scentMap[MAX_STAGE + 1][7][60]; 

void sleepMs(int ms);
void typeText(const string& text, int delay = 20);
void bootSequence();
void shutdownSequence(); 
void printTitleScreen();
void generateTrapsForStage(int stage);
void generateWalls(int stage);
void generateMap();
void applyMove(Position* pos, int dx, int dy); 
bool canGoUp(Position pos, int stage);         
bool canGoDown(Position pos, int stage);       
bool isWall(int stage, int f, int x);
void drawUI(int stage, Position player, Position shadow, bool shadowActive, bool hasKey, bool genActive, int hp, int maxHP, int battery, int sanity, bool isHiding, Position soundDistraction, const vector<string>& logs, const vector<string>& inv, bool flashlightActive, int elapsedSecs, bool isNightmare, Hallucination* fakes);
int getClosestStair(int stage, int floor, int currentX, int stageLength); 
void processShadowAI(Position player, Position* shadow, int stage, bool isHiding, Position* soundDistraction, bool hasSight); 
void processHallucinations(Hallucination* fakes, int stage);
void decayScentMap(int stage);
void bubbleSortScores(int* scores, int* times, int size); 
void drawLoreLibrary();
void drawArchive(); 
void handleThrowingMechanic(Position player, Position* soundDistraction, vector<string>& logs, int stage);

void loadGameData() {
    ifstream inFile("spire_archive.sav");
    if (inFile.is_open()) {
        for(int i = 0; i < 5; i++) {
            inFile >> loreUnlocked[i];
        }
        inFile.close();
    }
}

void saveGameData() {
    ofstream outFile("spire_archive.sav");
    if (outFile.is_open()) {
        for(int i = 0; i < 5; i++) {
            outFile << loreUnlocked[i] << " ";
        }
        outFile.close();
    }
}

int main() {
    srand(time(0)); 
    loadGameData(); 
    bootSequence();
    
    system("cls");
    cout << "\n\033[33;5m[SYSTEM NOTE]\033[0m\n";
    cout << "For the best visual experience, please MAXIMIZE your console window now.\n";
    cout << "Press any key to continue...\n";
    _getch();

    generateMap(); 
    
    int* stageScores = new int[MAX_STAGE + 1]; 
    int* stageTimes = new int[MAX_STAGE + 1];
    for(int i = 0; i <= MAX_STAGE; i++) {
        *(stageScores + i) = 0; 
        *(stageTimes + i) = 0; 
    }
    
    int currentStage = 1;
    GameState state = GameState::MAIN_MENU; 
    
    Position player = {0, 0};
    Position shadow = {-1, 0};
    Position soundDistraction = {-1, -1}; 
    
    Hallucination fakes[3]; 
    for(int i=0; i<3; i++) fakes[i].active = false;

    bool shadowActive = false;
    bool hasKey = false;
    bool generatorActive = false;
    bool flashlightActive = true; 
    bool isHiding = false;
    
    int totalMoves = 0; 
    int stageMoves = 0; 
    
    bool isNightmare = false;
    int maxHP = 3;
    int playerHP = maxHP; 
    int batteryLevel = 100;
    int sanityLevel = 100;
    int shadowSpawnTurn = 2;
    int shadowStunTimer = 0; 
    
    bool bossEscapePhase = false;
    
    time_t stageStartTime = time(0);
    int finalStageTime = 0;
    
    vector<string> gameLogs; 
    vector<string> inventory; 

    while (state != GameState::EXIT) {
        if (state == GameState::MAIN_MENU) {
            printTitleScreen();
            cout << " 1. Start Game\n";
            cout << " 2. How to Play\n";
            cout << " 3. Lore Library (Journals)\n";  
            cout << " 4. The Archive (Monsters)\n";  
            cout << " 5. Credits\n";  
            cout << " Q. Quit\n";
            cout << "--------------------------------------------------------------------------\n";
            cout << " Choice: ";

            char choice;
            do {
                choice = _getch();
            } while (choice != '1' && choice != '2' && choice != '3' && choice != '4' && choice != '5' && choice != 'q' && choice != 'Q');

            if (choice == '1') {
                system("cls");
                cout << "========================================================\n";
                cout << "                   SELECT DIFFICULTY                    \n";
                cout << "========================================================\n";
                cout << " 1. Normal Mode\n";
                cout << "    - 3 HP, Normal Sanity Drain, Forgiving Shadow\n\n";
                cout << " \033[31m2. NIGHTMARE MODE\033[0m\n";
                cout << "    - \033[31;5m1 HP, Instant Shadow, Aggressive Scent Tracking\033[0m\n\n";
                cout << " Q. Cancel (Return to Main Menu)\n";
                cout << "========================================================\n";
                cout << " Choice: ";
                
                char diffChoice;
                do { diffChoice = _getch(); } while (diffChoice != '1' && diffChoice != '2' && diffChoice != 'q' && diffChoice != 'Q');
                if (diffChoice == 'q' || diffChoice == 'Q') continue; 
                
                currentStage = 1; totalMoves = 0; inventory.clear();
                generateMap(); 
                
                if (diffChoice == '2') {
                    isNightmare = true; maxHP = 1; shadowSpawnTurn = 0; 
                    gameLogs.push_back("\033[31;5mNIGHTMARE MODE INITIATED. 1 HP. GOOD LUCK.\033[0m");
                } else {
                    isNightmare = false; maxHP = 3; shadowSpawnTurn = 2; 
                    gameLogs.push_back("\033[32mGame Initialized. Power the Generator and find the Key!\033[0m");
                }
                
                playerHP = maxHP; batteryLevel = 100; sanityLevel = 100;
                stageStartTime = time(0); 
                state = GameState::PLAYING; 
                
            } else if (choice == '2') {
                system("cls");
                cout << "========================================================\n";
                cout << "                     HOW TO PLAY                        \n";
                cout << "========================================================\n";
                cout << " GOAL: Navigate the floors. You must restore power by finding\n";
                cout << " the Generator (G) to unlock the Key (K), then reach the Exit.\n\n";
                cout << " SURVIVAL MECHANICS:\n";
                cout << "  - Battery: Drains as you walk. Find (b) to recharge.\n";
                cout << "  - Sanity System: Drains in the dark. Spawns hallucinations!\n";
                cout << "  - AI Scent Tracking: The Shadow sniffs your footsteps.\n";
                cout << "  - Stealth (C): Stand on a Cabinet and press 'H' to hide.\n";
                cout << "  - Listen: Press 'L' to listen for the Shadow's location.\n\n";
                cout << " ENTITIES & TRAPS:\n";
                cout << "  \033[32mP\033[0m = Player | \033[31mS\033[0m = Shadow | \033[36m#\033[0m = Ladder | \033[37mX\033[0m = Wall Obstacle\n";
                cout << "  \033[33mG\033[0m = Generator | \033[33m~\033[0m = Crumbling Floor | \033[31mO\033[0m = Hole\n\n";
                cout << " ITEMS:\n";
                cout << "  \033[32m+\033[0m = Potion | \033[36mb\033[0m = Battery | \033[37m*\033[0m = Stone | \033[34m?\033[0m = Lore Fragment\n\n";
                cout << " CONTROLS: W/A/S/D = Move | 1/2/3 = Item | F = Flashlight\n";
                cout << "  H = Hide | T = Throw | L = Listen\n";
                cout << "========================================================\n";
                cout << " Press any key to return to the Main Menu...\n";
                _getch();
            } else if (choice == '3') {
                drawLoreLibrary();
            } else if (choice == '4') {
                drawArchive(); 
            } else if (choice == '5') {
                system("cls");
                cout << "========================================================\n";
                cout << "                        CREDITS                         \n";
                cout << "========================================================\n";
                cout << " Asfa Izzat: Lead Director, Programmer\n";
                cout << " Sofea Sholihah: Assistant Director, Documentation\n";
                cout << " Tan Kai Xue: Programmer, Idea\n";
                cout << " Tan Kai Xuan: Programmer\n\n";
                cout << " Remembering Za'im Arsyad due to his termination from PASUM\n\n";
                cout << " Language: C++\n";
                cout << " Lecturer: En. Amirul Mohamad Khairi, Dr. Nur Amirah, Pn. Fatin Nabila\n";
                cout << " Instructor: Mr. Abdullah Mohammad Khan\n\n";
                cout << " Gameplay inspiration: Honkai: Star Rail\n";
                cout << " Game theme inspiration: Resident Evil, Darkwood, Utahon no Tatari, Exit 8\n";
                cout << " Path to Iridescent Git repository:\n";
                cout << " https://github.com/rokurooooo01/Path-of-Iridescent-test-\n";
                cout << "========================================================\n";
                cout << " Press any key to return to the Main Menu...\n";
                _getch();
            } else {
                state = GameState::EXIT;
            }
        }
        
        while (state == GameState::PLAYING) {
            int currentElapsed = difftime(time(0), stageStartTime);
            drawUI(currentStage, player, shadow, shadowActive, hasKey, generatorActive, playerHP, maxHP, batteryLevel, sanityLevel, isHiding, soundDistraction, gameLogs, inventory, flashlightActive, currentElapsed, isNightmare, fakes);
            if (gameLogs.size() > 5) gameLogs.erase(gameLogs.begin()); 

            Position oldPos = player; 
            char input = _getch();
            bool validMove = false;
            bool actionTaken = false;

            if (!isHiding) {
                switch (input) {
                    case 'd': case 'D':
                        if (player.x < stageLengths[currentStage] && !isWall(currentStage, player.y, player.x + 1)) { applyMove(&player, 1, 0); validMove = true; } 
                        else gameLogs.push_back("Wall ahead!"); break;
                    case 'a': case 'A':
                        if (player.x > 0 && !isWall(currentStage, player.y, player.x - 1)) { applyMove(&player, -1, 0); validMove = true; } 
                        else gameLogs.push_back("Cannot move further left."); break;
                    case 'w': case 'W':
                        if (canGoUp(player, currentStage)) { applyMove(&player, 0, 1); gameLogs.push_back("Climbed UP."); validMove = true; } 
                        else gameLogs.push_back("No ladder leading up here!"); break;
                    case 's': case 'S':
                        if (canGoDown(player, currentStage)) { applyMove(&player, 0, -1); gameLogs.push_back("Climbed DOWN."); validMove = true; } 
                        else gameLogs.push_back("No ladder leading down here!"); break;
                    
                    case 'f': case 'F':
                        flashlightActive = !flashlightActive; actionTaken = true;
                        if (flashlightActive) gameLogs.push_back("CLICK! Flashlight ON.");
                        else gameLogs.push_back("CLICK! Flashlight OFF. Darkness consumes you...");
                        break;
                        
                    case 'h': case 'H': {
                        bool onLocker = false;
                        for(int l=0; l<4; l++) {
                            if(player.x == lockers[currentStage][l].x && player.y == lockers[currentStage][l].y) onLocker = true;
                        }
                        if (onLocker) {
                            isHiding = true; actionTaken = true;
                            gameLogs.push_back("\033[36mYou slip into the locker and hold your breath...\033[0m");
                        } else gameLogs.push_back("You must be standing on a Cabinet (C) to hide!");
                        break;
                    }

                    case 'l': case 'L': {
                        actionTaken = true;
                        int dx = shadow.x - player.x;
                        int dy = shadow.y - player.y;
                        int dist = abs(dx) + abs(dy) * 10;
                        
                        if (!shadowActive) {
                            gameLogs.push_back("\033[36mSilence... for now.\033[0m");
                        } else if (dist <= 3 && shadow.y == player.y) {
                            gameLogs.push_back("\033[31;5mIT IS IN THE ROOM WITH YOU!\033[0m");
                        } else if (dy > 0) {
                            gameLogs.push_back("\033[33mYou hear heavy footsteps from the floor above...\033[0m");
                        } else if (dy < 0) {
                            gameLogs.push_back("\033[33mYou hear scratching from the floor below...\033[0m");
                        } else if (dx > 0) {
                            gameLogs.push_back("\033[33mA chilling breeze comes from the East...\033[0m");
                        } else {
                            gameLogs.push_back("\033[33mA chilling breeze comes from the West...\033[0m");
                        }
                        break;
                    }
                    
                    case 't': case 'T': {
                        int rockIdx = -1;
                        for(size_t i=0; i<inventory.size(); i++) {
                            if(inventory[i] == "Stone") rockIdx = i;
                        }
                        if (rockIdx != -1) {
                            handleThrowingMechanic(player, &soundDistraction, gameLogs, currentStage);
                            inventory.erase(inventory.begin() + rockIdx);
                            actionTaken = true; validMove = true; 
                        } else gameLogs.push_back("You don't have any Stones to throw!");
                        break;
                    }

                    case '1': case '2': case '3': {
                        int slot = (input - '0') - 1; 
                        if (slot < inventory.size()) {
                            if (inventory[slot] == "Health Potion") {
                                if (playerHP < maxHP) {
                                    playerHP++; gameLogs.push_back("\033[32mGULP! Used Health Potion. HP +1.\033[0m");
                                    inventory.erase(inventory.begin() + slot); actionTaken = true; 
                                } else gameLogs.push_back("HP is already full!");
                            } else if (inventory[slot] == "Battery") {
                                batteryLevel = 100; gameLogs.push_back("\033[36mRELOAD! Flashlight battery restored to 100%.\033[0m");
                                inventory.erase(inventory.begin() + slot); actionTaken = true; flashlightActive = true;
                            } else if (inventory[slot] == "Stone") {
                                gameLogs.push_back("Press 'T' to throw the Stone and distract the Shadow!");
                            }
                        } else gameLogs.push_back("That backpack slot is empty!");
                        break;
                    }
                    case '9':
                        hasKey = true;
                        playerHP = 99;
                        player.x = stageLengths[currentStage];
                        player.y = maxFloors[currentStage];
                        if (currentStage == MAX_STAGE) { player.x = 0; player.y = 0; }
                        actionTaken = true; 
                        validMove = true;
                        gameLogs.push_back("\033[35;5m[DEV MODE] Cheat activated. Teleporting to exit with Key.\033[0m");
                        break;
                    default: continue; 
                }
            } else {
                if (input == 'h' || input == 'H') {
                    isHiding = false; actionTaken = true;
                    gameLogs.push_back("\033[36mYou slowly push the locker door open and step out.\033[0m");
                } else if (input == 'f' || input == 'F') {
                    flashlightActive = !flashlightActive; actionTaken = true;
                } else {
                    gameLogs.push_back("You are hiding! Press 'H' to exit the locker."); continue;
                }
            }

            if (validMove || actionTaken) {
                totalMoves++; 
                if (validMove) stageMoves++; 
                
                if (validMove) {
                    scentMap[currentStage][player.y][player.x] = 20; 
                }
                
                if (flashlightActive && batteryLevel > 0) batteryLevel -= (isNightmare ? 2 : 1);
                if (batteryLevel <= 0) { batteryLevel = 0; flashlightActive = false; }
                
                int distToShadow = abs(player.x - shadow.x) + (abs(player.y - shadow.y) * 10);
                if (!flashlightActive || distToShadow <= 5) sanityLevel -= (isNightmare ? 3 : 2);
                else if (isHiding) sanityLevel -= 1; 
                else if (sanityLevel < 100) sanityLevel += 1; 
                
                if (sanityLevel <= 0) sanityLevel = 0;
                
                if (validMove && terrain3D[currentStage][oldPos.y][oldPos.x] == '~') {
                    terrain3D[currentStage][oldPos.y][oldPos.x] = 'O'; 
                    gameLogs.push_back("\033[33mCRACK! The floor crumbled into a hole behind you!\033[0m");
                }

                if (!generatorActive && player.x == generatorPos[currentStage].x && player.y == generatorPos[currentStage].y) {
                    generatorActive = true;
                    gameLogs.push_back("\033[33;5m[ GENERATOR ACTIVATED ] Power restored to the lower floors!\033[0m");
                    gameLogs.push_back("\033[36mThe Key is now accessible. Find it.\033[0m");
                }

                if (generatorActive && !hasKey && player.x == keyPositions[currentStage].x && player.y == keyPositions[currentStage].y) {
                    hasKey = true; 
                    if (currentStage == MAX_STAGE) {
                        bossEscapePhase = true; shadowActive = true;
                        shadow.x = player.x - 3; shadow.y = player.y; shadowStunTimer = 0;
                        gameLogs.push_back("\033[31;5m[!] THE SHADOW HAS AWAKENED FULLY! THE EXIT IS LOCKED!\033[0m");
                        gameLogs.push_back("\033[33;5m[!] NEW OBJECTIVE: RUN BACK TO THE STARTING LADDER (0,0) TO ESCAPE!\033[0m");
                    } else { gameLogs.push_back("\033[33mSUCCESS: Picked up the Key! Exit unlocked.\033[0m"); }
                }

                for (int i = 0; i < 8; i++) {
                    if (stageItems[currentStage][i].active && player.x == stageItems[currentStage][i].pos.x && player.y == stageItems[currentStage][i].pos.y) {
                        if (stageItems[currentStage][i].name == "Lore Fragment") {
                            int unlockIdx = rand() % 5;
                            loreUnlocked[unlockIdx] = true;
                            saveGameData(); 
                            stageItems[currentStage][i].active = false;
                            gameLogs.push_back("\033[34mFound a torn journal page! Added to Library and Saved.\033[0m");
                        }
                        else if (inventory.size() < 3) {
                            inventory.push_back(stageItems[currentStage][i].name);
                            stageItems[currentStage][i].active = false; 
                            gameLogs.push_back("Looted: " + stageItems[currentStage][i].name);
                        } else {
                            if(validMove) gameLogs.push_back("Backpack is full! Left " + stageItems[currentStage][i].name + " behind.");
                        }
                    }
                }

                if (player.x == warpGates[currentStage][0].x && player.y == warpGates[currentStage][0].y) {
                    player.x = warpGates[currentStage][1].x; player.y = warpGates[currentStage][1].y;
                    gameLogs.push_back("\033[35mZAP! You used the Warp Gate!\033[0m");
                } else if (player.x == warpGates[currentStage][1].x && player.y == warpGates[currentStage][1].y) {
                    player.x = warpGates[currentStage][0].x; player.y = warpGates[currentStage][0].y;
                    gameLogs.push_back("\033[35mZAP! You used the Warp Gate!\033[0m");
                }

                bool fell = false;
                for (int h = 0; h < 3; h++) if (player.x == holes[currentStage][h].x && player.y == holes[currentStage][h].y) fell = true;
                if (terrain3D[currentStage][player.y][player.x] == 'O') fell = true;
                
                if (fell) {
                    playerHP--; player = {0, 0}; shadowActive = false; shadow = {-1, 0}; stageMoves = 0; isHiding = false;
                    gameLogs.push_back("\033[31mAHHH! You fell in a hole! Lost 1 HP and respawned.\033[0m");
                    if (playerHP <= 0) state = GameState::GAMEOVER;
                }

                if (state == GameState::PLAYING) {
                    if (!shadowActive && stageMoves >= shadowSpawnTurn) {
                        shadowActive = true; shadow.x = 0; shadow.y = 0;
                        gameLogs.push_back("\033[31mWARNING: THE SHADOW IS HUNTING YOU!\033[0m");
                    } else if (shadowActive) {
                        bool shadowHasSight = (shadow.y == player.y && abs(shadow.x - player.x) <= 4 && flashlightActive);
                        processShadowAI(player, &shadow, currentStage, isHiding, &soundDistraction, shadowHasSight); 
                        
                        if (shadow.y == player.y && abs(shadow.x - player.x) > (flashlightActive ? 3 : 1)) {
                            string dir = (shadow.x < player.x) ? "West" : "East";
                            gameLogs.push_back("\033[31m...you hear dragging footsteps to the " + dir + ".\033[0m");
                        }
                    }
                    
                    if (sanityLevel < 40) {
                        processHallucinations(fakes, currentStage);
                        if(rand()%10 == 0) gameLogs.push_back("\033[31;5mTHEY ARE IN THE WALLS THEY ARE IN THE WALLS\033[0m");
                    } else {
                        for(int i=0; i<3; i++) fakes[i].active = false;
                    }
                }
                
                decayScentMap(currentStage);
            }

            if (shadowActive && player.x == shadow.x && player.y == shadow.y && !isHiding) {
                playerHP--;
                if (playerHP > 0) {
                    shadowActive = false; shadow = {-1, 0}; stageMoves = 0; sanityLevel = 100;
                    gameLogs.push_back("\033[31mTHE SHADOW STRUCK YOU! Lost 1 HP.\033[0m");
                } else state = GameState::GAMEOVER;
            } else {
                bool reachedExit = false;
                if (!bossEscapePhase && player.x == stageLengths[currentStage] && player.y == maxFloors[currentStage]) reachedExit = true;
                if (bossEscapePhase && player.x == 0 && player.y == 0) reachedExit = true;

                if (reachedExit) {
                    if (hasKey) {
                        *(stageScores + currentStage) = stageMoves; 
                        *(stageTimes + currentStage) = difftime(time(0), stageStartTime); 
                        finalStageTime = *(stageTimes + currentStage);
                        state = GameState::ESCAPED;
                    }
                    else gameLogs.push_back("The door is locked! Find the Key (K) first!");
                }
            }
        }

        if (state == GameState::GAMEOVER) {
            finalStageTime = difftime(time(0), stageStartTime);
            drawUI(currentStage, player, shadow, shadowActive, hasKey, generatorActive, playerHP, maxHP, batteryLevel, sanityLevel, isHiding, soundDistraction, gameLogs, inventory, flashlightActive, finalStageTime, isNightmare, fakes);
            cout << "\n\033[31m[!] ZERO HP. THE DARKNESS CONSUMED YOU.\033[0m\n";
            cout << "1. Restart Entire Game (From Stage 1)\n";
            cout << "2. Retry Current Stage (Reset Stats)\n";
            cout << "Q. Main Menu (Quit)\nChoice: ";
            
            char choice;
            do { choice = _getch(); } while (choice != '1' && choice != '2' && choice != 'q' && choice != 'Q');
            
            if (choice == '1') {
                currentStage = 1; totalMoves = 0; playerHP = maxHP; inventory.clear();
                bossEscapePhase = false; sanityLevel = 100; batteryLevel = 100;
                generateMap(); stageStartTime = time(0); state = GameState::PLAYING;
            } else if (choice == '2') {
                playerHP = maxHP; inventory.clear(); sanityLevel = 100; batteryLevel = 100;
                bossEscapePhase = false; stageStartTime = time(0); state = GameState::PLAYING;
            } else state = GameState::MAIN_MENU; 
        } 
        else if (state == GameState::ESCAPED) {
            drawUI(currentStage, player, shadow, shadowActive, hasKey, generatorActive, playerHP, maxHP, batteryLevel, sanityLevel, isHiding, soundDistraction, gameLogs, inventory, flashlightActive, finalStageTime, isNightmare, fakes);
            if (currentStage < MAX_STAGE) {
                cout << "\n\033[32m[+] STAGE " << currentStage << " CLEARED IN " << stageMoves << " MOVES & " << finalStageTime << " SECONDS!\033[0m\n";
                cout << "1. Proceed to Stage " << currentStage + 1 << "\n";
                cout << "2. Replay Stage " << currentStage << "\n";
                if (currentStage > 1) cout << "3. Return to Stage " << currentStage - 1 << "\n";
                cout << "Q. Main Menu (Quit)\nChoice: ";
                
                char choice;
                do { choice = _getch(); } while (choice != '1' && choice != '2' && choice != '3' && choice != 'q' && choice != 'Q');
                
                if (choice == '1') { currentStage++; state = GameState::PLAYING; } 
                else if (choice == '2') { state = GameState::PLAYING; } 
                else if (choice == '3' && currentStage > 1) { currentStage--; state = GameState::PLAYING; } 
                else state = GameState::MAIN_MENU; 
            } else {
                cout << "\n\033[32m[!!!] YOU ESCAPED ALL 10 STAGES! YOU WIN!\033[0m\n";
                cout << "=== FINAL REPORT CARD ===\n";
                bubbleSortScores(stageScores, stageTimes, MAX_STAGE + 1);
                
                cout << "\n1. Replay Final Stage\nQ. Main Menu (Quit)\nChoice: ";
                char choice; do { choice = _getch(); } while (choice != '1' && choice != 'q' && choice != 'Q');
                if (choice == '1') state = GameState::PLAYING; else state = GameState::MAIN_MENU; 
            }
        }
        
        if (state == GameState::PLAYING) {
            player = {0, 0}; shadow = {-1, 0}; soundDistraction = {-1,-1}; shadowActive = false; 
            hasKey = false; generatorActive = false; stageMoves = 0; isHiding = false;     
            for(int i=0; i<3; i++) fakes[i].active = false;
            bossEscapePhase = false;
            
            for(int f=0; f<7; f++) for(int x=0; x<60; x++) scentMap[currentStage][f][x] = 0;
            
            generateTrapsForStage(currentStage);
            generateWalls(currentStage);
            gameLogs.clear(); gameLogs.push_back("Entering Stage " + to_string(currentStage) + ". Trust no shadows.");
            stageStartTime = time(0); 
        }
    }

    delete[] stageScores; 
    delete[] stageTimes;
    
    shutdownSequence();
    
    return 0;
}

void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void typeText(const string& text, int delay) {
    for (char c : text) {
        cout << c << flush;
        sleepMs(delay);
    }
    cout << endl;
}

void bootSequence() {
    system("cls");
    cout << "\033[32m";
    typeText("Phoenix - AwardBIOS v6.00PG, An Energy Star Ally", 10);
    typeText("Copyright (C) 1984-2007, Phoenix Technologies, LTD", 10);
    cout << "\n";
    typeText("ASUS A8N-SLI ACPI BIOS Revision 1009", 10);
    cout << "\n";
    typeText("Main Processor : AMD Athlon(tm) 64 Processor 3200+", 10);
    cout << "Memory Testing : ";
    sleepMs(500);
    typeText("1048576K OK", 5);
    cout << "\n";
    typeText("Primary Master   : ST3160827AS 3.42", 10);
    typeText("Primary Slave    : None", 10);
    typeText("Secondary Master : HL-DT-ST DVD-RW GWA-4164B", 10);
    typeText("Secondary Slave  : None", 10);
    cout << "\n";
    sleepMs(800);

    typeText("Loading SpireOS Sector 1-10...", 20);
    sleepMs(500);
    typeText("Initializing Mainframe...", 20);
    sleepMs(800);

    cout << "\033[31m"; 
    typeText("WARNING: BIOS Error 0x88. Containment failed.", 20);
    sleepMs(300);
    typeText("CRITICAL: Unknown entity signatures detected.", 20);
    sleepMs(500);
    cout << "\033[0m\n";
    typeText("Press any key to boot recovery mode...", 10);
    _getch(); 
}

void shutdownSequence() {
    system("cls");
    cout << "\033[32m"; 
    
    typeText("Initiating SpireOS shutdown sequence...", 20);
    sleepMs(400);
    
    cout << "Saving local data...";
    sleepMs(600);
    cout << " OK\n";
    sleepMs(200);
    
    cout << "Flushing NVRAM...";
    sleepMs(500);
    cout << " OK\n";
    sleepMs(300);
    
    typeText("Terminating active processes...", 15);
    sleepMs(700);
    typeText("Unmounting drive C: ... OK", 20);
    sleepMs(400);
    
    cout << "\033[31m"; 
    typeText("WARNING: Unidentified entities remain in Sectors 1-10.", 30);
    sleepMs(500);
    
    cout << "\033[32m"; 
    typeText("System halted.", 40);
    sleepMs(1000);
    
    cout << "\nIt is now safe to turn off your computer.\033[0m\n";
    sleepMs(1500);
}

void printTitleScreen() {
    system("cls");
    cout << "\033[36m";
    cout << "==========================================================================\n";
    cout << "  ____       _______ _    _   _______ ____  \n";
    cout << " |  _ \\   /\\|__   __| |  | | |__   __/ __ \\ \n";
    cout << " | |_) | /  \\  | |  | |__| |    | | | |  | |\n";
    cout << " |  __/ / /\\ \\ | |  |  __  |    | | | |  | |\n";
    cout << " | |   / ____ \\| |  | |  | |    | | | |__| |\n";
    cout << " |_|  /_/    \\_\\_|  |_|  |_|    |_|  \\____/ \n";
    cout << "                                            \n";
    cout << "  _____ _____  _____ _____  ______  _____  _____ ______ _   _ _______ \n";
    cout << " |_   _|  __ \\|_   _|  __ \\|  ____|/ ____|/ ____|  ____| \\ | |__   __|\n";
    cout << "   | | | |__) | | | | |  | | |__  | (___ | |    | |__  |  \\| |  | |   \n";
    cout << "   | | |  _  /  | | | |  | |  __|  \\___ \\| |    |  __| | . ` |  | |   \n";
    cout << "  _| |_| | \\ \\ _| |_| |__| | |____ ____) | |____| |____| |\\  |  | |   \n";
    cout << " |_____|_|  \\_\\_____|_____/|______|_____/ \\_____|______|_| \\_|  |_|   \n";
    cout << "==========================================================================\n";
    cout << "                               D E M O                                    \n";
    cout << "==========================================================================\n";
    cout << "\033[0m"; 
}

void generateWalls(int stage) {
    for(int f=0; f<=maxFloors[stage]; f++) {
        int numWalls = 2 + (stage / 2); 
        for(int w=0; w<numWalls; w++) {
            int wx = 3 + (rand() % (stageLengths[stage] - 5));
            if(terrain3D[stage][f][wx] == '#' || terrain3D[stage][f][wx-1] == '#' || terrain3D[stage][f][wx+1] == '#') continue;
            terrain3D[stage][f][wx] = 'X';
        }
    }
}

bool isWall(int stage, int f, int x) {
    return (terrain3D[stage][f][x] == 'X');
}

void generateTrapsForStage(int stage) {
    int rightmostLadder = 2;
    if (maxFloors[stage] > 0) {
        for (int x = 0; x < stageLengths[stage]; x++) {
            if (terrain3D[stage][maxFloors[stage] - 1][x] == '#') rightmostLadder = x;
        }
    }

    generatorPos[stage] = { 2 + (rand() % (stageLengths[stage] - 3)), maxFloors[stage] };

    if (maxFloors[stage] >= 1) {
        warpGates[stage][0] = { 2 + (rand() % (stageLengths[stage] - 4)), 0 }; 
        int range = rightmostLadder - 2;
        int topWarpX = (range > 0) ? (2 + (rand() % range)) : 1;
        warpGates[stage][1] = { topWarpX, maxFloors[stage] };
    } else {
        warpGates[stage][0] = {-1, -1}; warpGates[stage][1] = {-1, -1};
    }

    if (stage > 2) {
        for (int h = 0; h < 3; h++) {
            int hFloor, hX;
            bool validPos = false;
            int attempts = 0; 
            
            while (!validPos && attempts < 100) {
                attempts++; hFloor = 1; hX = 2 + (rand() % (stageLengths[stage] - 3)); validPos = true; 
                for (int dx = -1; dx <= 1; dx++) {
                    int checkX = hX + dx;
                    if (checkX >= 0 && checkX <= stageLengths[stage]) {
                        if (terrain3D[stage][hFloor][checkX] == '#') validPos = false;
                        if (hFloor > 0 && terrain3D[stage][hFloor - 1][checkX] == '#') validPos = false;
                    }
                }
                if (hFloor == maxFloors[stage] && hX >= rightmostLadder) validPos = false;
                if (keyPositions[stage].y == hFloor && abs(keyPositions[stage].x - hX) <= 1) validPos = false;
                if (generatorPos[stage].y == hFloor && abs(generatorPos[stage].x - hX) <= 1) validPos = false;
                if (hX == warpGates[stage][0].x && hFloor == warpGates[stage][0].y) validPos = false;
                if (hX == warpGates[stage][1].x && hFloor == warpGates[stage][1].y) validPos = false;
            }
            if (validPos) holes[stage][h] = {hX, hFloor}; else holes[stage][h] = {-1, -1}; 
        }
    } else {
        for (int h = 0; h < 3; h++) holes[stage][h] = {-1, -1};
    }

    for (int i = 0; i < 8; i++) {
        int iFloor = (maxFloors[stage] > 0) ? rand() % (maxFloors[stage] + 1) : 0;
        int iX = 2 + (rand() % (stageLengths[stage] - 3));
        if (i == 0) stageItems[stage][i] = { {iX, iFloor}, "Health Potion", '+', true };
        else if (i == 1 || i == 2) stageItems[stage][i] = { {iX, iFloor}, "Battery", 'b', true };
        else if (i == 3 || i == 4) stageItems[stage][i] = { {iX, iFloor}, "Stone", '*', true };
        else if (i == 5) stageItems[stage][i] = { {iX, iFloor}, "Lore Fragment", '?', true };
        else stageItems[stage][i] = { {-1, -1}, "Empty", '.', false }; 
    }

    for (int l = 0; l < 4; l++) {
        int lFloor = (maxFloors[stage] > 0) ? rand() % (maxFloors[stage] + 1) : 0;
        int lX = 2 + (rand() % (stageLengths[stage] - 3));
        lockers[stage][l] = {lX, lFloor};
    }
}

void generateMap() {
    for(int s = 0; s <= MAX_STAGE; s++) {
        for(int f = 0; f < 7; f++) {
            stairPos[s][f] = -1;
            for(int x = 0; x < 60; x++) {
                terrain3D[s][f][x] = '.'; 
                scentMap[s][f][x] = 0; 
            }
        }
    }
    
    for (int stage = 1; stage <= MAX_STAGE; stage++) {
        if (stage > 1) {
            for (int floor = 0; floor < maxFloors[stage]; floor++) {
                int numStairs = (floor >= 2) ? 2 : 1; 
                if (numStairs == 1) {
                    int stairX = 2 + (rand() % (stageLengths[stage] - 4));
                    terrain3D[stage][floor][stairX] = '#';
                } else {
                    int midPoint = stageLengths[stage] / 2;
                    int stairX1 = 2 + (rand() % (midPoint - 3));
                    int stairX2 = midPoint + 1 + (rand() % (midPoint - 3));
                    terrain3D[stage][floor][stairX1] = '#';
                    terrain3D[stage][floor][stairX2] = '#';
                }
            }
        }
        
        if (stage > 2) {
            for(int c = 0; c < 4; c++) {
                int cFloor = 1; 
                int cX = 2 + (rand() % (stageLengths[stage] - 3));
                if (terrain3D[stage][cFloor][cX] == '.') terrain3D[stage][cFloor][cX] = '~';
            }
        }
        
        int keyFloor = (maxFloors[stage] > 0) ? rand() % 2 : 0;
        int keyX = 2 + (rand() % (stageLengths[stage] - 3));
        keyPositions[stage] = { keyX, keyFloor };

        generateTrapsForStage(stage);
    }
}

void applyMove(Position* pos, int dx, int dy) {
    pos->x += dx; pos->y += dy;
}

bool canGoUp(Position pos, int stage) {
    if (pos.y >= maxFloors[stage]) return false; 
    return (terrain3D[stage][pos.y][pos.x] == '#'); 
}

bool canGoDown(Position pos, int stage) {
    if (pos.y <= 0) return false;
    return (terrain3D[stage][pos.y - 1][pos.x] == '#'); 
}

void decayScentMap(int stage) {
    for(int f=0; f<=maxFloors[stage]; f++) {
        for(int x=0; x<=stageLengths[stage]; x++) {
            if (scentMap[stage][f][x] > 0) {
                scentMap[stage][f][x]--; 
            }
        }
    }
}

void drawUI(int stage, Position player, Position shadow, bool shadowActive, bool hasKey, bool genActive, int hp, int maxHP, int battery, int sanity, bool isHiding, Position soundDistraction, const vector<string>& logs, const vector<string>& inv, bool flashlightActive, int elapsedSecs, bool isNightmare, Hallucination* fakes) {
    system("cls");
    
    string batBar = ""; for(int i=0; i<10; i++) batBar += (battery >= i*10) ? "|" : " ";
    string sanBar = ""; for(int i=0; i<10; i++) sanBar += (sanity >= i*10) ? "|" : " ";
    string batColor = (battery > 20) ? "\033[32m" : "\033[31m";
    string sanColor = (sanity > 40) ? "\033[32m" : "\033[31;5m";

    string compass = "";
    if (!hasKey) {
        if (!genActive) compass = "\033[33m[ FIND GENERATOR ]\033[0m";
        else {
            int dist = abs(player.x - keyPositions[stage].x) + abs(player.y - keyPositions[stage].y);
            if (dist <= 2) compass = "\033[31m[ BURNING HOT ]\033[0m";
            else if (dist <= 5) compass = "\033[33m[ WARM ]\033[0m";
            else if (dist <= 10) compass = "\033[36m[ COLD ]\033[0m";
            else compass = "\033[34m[ FREEZING ]\033[0m";
        }
    } else compass = "\033[32m[ KEY FOUND ]\033[0m";

    cout << "========================================================\n";
    cout << " HP: \033[31m"; for(int i=0; i<maxHP; i++) cout << (i < hp ? "♥ " : "♡ ");
    cout << "\033[0m | BATTERY: " << batColor << "[" << batBar << "] " << battery << "%\033[0m\n";
    cout << " SANITY: " << sanColor << "[" << sanBar << "] " << sanity << "%\033[0m | TIME: " << elapsedSecs << "s\n";
    cout << "--------------------------------------------------------\n";
    cout << " STAGE: " << stage << " / " << MAX_STAGE << " | COMPASS: " << compass << "\n";
    if(isNightmare) cout << " MODE: \033[31;5m[ NIGHTMARE ]\033[0m\n";
    
    cout << " BACKPACK: ";
    if (inv.empty()) cout << "(Empty)";
    else for (size_t i = 0; i < inv.size(); i++) cout << "[" << i + 1 << ": " << inv[i] << "] ";
    cout << "\n--------------------------------------------------------\n";
    
    if (isHiding) cout << " MAP: [\033[36mHIDING IN CABINET - VISION RESTRICTED\033[0m]\n";
    else if (!flashlightActive) cout << " MAP: [\033[31mPITCH BLACK\033[0m]\n";
    else cout << " MAP: [FLASHLIGHT ON]\n";
    
    for (int y = maxFloors[stage]; y >= 0; y--) {
        cout << " F" << y << " | ";
        for (int x = 0; x <= stageLengths[stage]; x++) {
            
            bool inVision = true;
            if (isHiding) inVision = (abs(player.x - x) <= 1 && player.y == y);
            else if (!flashlightActive) inVision = (abs(player.x - x) <= 1 && player.y == y);
            else inVision = (abs(player.x - x) <= 3 && abs(player.y - y) <= 1);
            
            if (!inVision) {
                cout << " "; 
            } else {
                bool isHole = false; for(int h=0; h<3; h++) if (holes[stage][h].x == x && holes[stage][h].y == y) isHole = true;
                bool isWarp = (warpGates[stage][0].x == x && warpGates[stage][0].y == y) || (warpGates[stage][1].x == x && warpGates[stage][1].y == y);
                bool isLocker = false; for(int l=0; l<4; l++) if (lockers[stage][l].x == x && lockers[stage][l].y == y) isLocker = true;
                bool isFake = false; for(int f=0; f<3; f++) if (fakes[f].active && fakes[f].pos.x == x && fakes[f].pos.y == y) isFake = true;
                
                int itemIdx = -1;
                for(int i=0; i<8; i++) if (stageItems[stage][i].active && stageItems[stage][i].pos.x == x && stageItems[stage][i].pos.y == y) itemIdx = i;
                
                if (player.x == x && player.y == y) cout << (isHiding ? "\033[36mP\033[0m" : "\033[32mP\033[0m"); 
                else if (shadowActive && shadow.x == x && shadow.y == y) cout << "\033[31mS\033[0m"; 
                else if (isFake) cout << "\033[31;5mS\033[0m"; 
                else if (soundDistraction.x == x && soundDistraction.y == y) cout << "\033[33;5m!\033[0m"; 
                else if (!genActive && x == generatorPos[stage].x && y == generatorPos[stage].y) cout << "\033[33;5mG\033[0m"; 
                else if (genActive && !hasKey && x == keyPositions[stage].x && y == keyPositions[stage].y) cout << "\033[33mK\033[0m"; 
                else if (stage == MAX_STAGE && hasKey && x == 0 && y == 0) cout << "\033[35mE\033[0m"; 
                else if (x == stageLengths[stage] && y == maxFloors[stage]) cout << ((stage==MAX_STAGE && hasKey) ? "\033[31m#\033[0m" : (hasKey ? "\033[35mE\033[0m" : "L")); 
                else if (isLocker) cout << "\033[36mC\033[0m";
                else if (isWarp) cout << "\033[35mW\033[0m"; 
                else if (isHole) cout << "\033[31mO\033[0m"; 
                else if (itemIdx != -1) {
                    char s = stageItems[stage][itemIdx].symbol;
                    if (s == '+') cout << "\033[32m+\033[0m"; else if (s == 'b') cout << "\033[36mb\033[0m"; 
                    else if (s == '*') cout << "\033[37m*\033[0m"; else if (s == '?') cout << "\033[34m?\033[0m";
                }
                else if (terrain3D[stage][y][x] == 'X') cout << "\033[37mX\033[0m"; 
                else if (terrain3D[stage][y][x] == '#') cout << "\033[36m#\033[0m"; 
                else if (y > 0 && terrain3D[stage][y-1][x] == '#') cout << "\033[36m#\033[0m"; 
                else if (terrain3D[stage][y][x] == '~') cout << "\033[33m~\033[0m"; 
                else if (terrain3D[stage][y][x] == 'O') cout << "\033[31mO\033[0m"; 
                else cout << "."; 
            }
        }
        cout << "\n";
    }
    cout << "--------------------------------------------------------\n";
    for (size_t i = 0; i < logs.size(); i++) cout << " > " << logs[i] << "\n";
    cout << "========================================================\n";
    cout << "Action: ";
}

int getClosestStair(int stage, int floor, int currentX, int stageLength) {
    int targetX = currentX;
    int minDist = 999;
    for (int x = 0; x <= stageLength; x++) {
        if (terrain3D[stage][floor][x] == '#') {
            if (abs(currentX - x) < minDist) { minDist = abs(currentX - x); targetX = x; }
        }
    }
    return targetX;
}

void processShadowAI(Position player, Position* shadow, int stage, bool isHiding, Position* soundDistraction, bool hasSight) {
    
    if (soundDistraction->x != -1) {
        if (shadow->y < soundDistraction->y) {
            int tx = getClosestStair(stage, shadow->y, shadow->x, stageLengths[stage]);
            if (shadow->x < tx && !isWall(stage, shadow->y, shadow->x+1)) shadow->x++;
            else if (shadow->x > tx && !isWall(stage, shadow->y, shadow->x-1)) shadow->x--;
            else shadow->y++; 
        } 
        else if (shadow->y > soundDistraction->y) {
            int tx = getClosestStair(stage, shadow->y - 1, shadow->x, stageLengths[stage]);
            if (shadow->x < tx && !isWall(stage, shadow->y, shadow->x+1)) shadow->x++;
            else if (shadow->x > tx && !isWall(stage, shadow->y, shadow->x-1)) shadow->x--;
            else shadow->y--; 
        } 
        else {
            if (shadow->x < soundDistraction->x && !isWall(stage, shadow->y, shadow->x+1)) shadow->x++;
            else if (shadow->x > soundDistraction->x && !isWall(stage, shadow->y, shadow->x-1)) shadow->x--;
            else { soundDistraction->x = -1; soundDistraction->y = -1; }
        }
        return;
    }
    
    if (hasSight && !isHiding) {
        if (shadow->x < player.x && !isWall(stage, shadow->y, shadow->x+1)) shadow->x++;
        else if (shadow->x > player.x && !isWall(stage, shadow->y, shadow->x-1)) shadow->x--;
        return;
    }
    
    int bestScent = 0;
    int targetX = shadow->x;
    int targetY = shadow->y;
    
    if (shadow->x < stageLengths[stage] && !isWall(stage, shadow->y, shadow->x+1)) {
        if (scentMap[stage][shadow->y][shadow->x+1] > bestScent) {
            bestScent = scentMap[stage][shadow->y][shadow->x+1]; targetX = shadow->x+1;
        }
    }
    if (shadow->x > 0 && !isWall(stage, shadow->y, shadow->x-1)) {
        if (scentMap[stage][shadow->y][shadow->x-1] > bestScent) {
            bestScent = scentMap[stage][shadow->y][shadow->x-1]; targetX = shadow->x-1; targetY = shadow->y;
        }
    }
    if (canGoUp(*shadow, stage)) {
        if (scentMap[stage][shadow->y+1][shadow->x] > bestScent) {
            bestScent = scentMap[stage][shadow->y+1][shadow->x]; targetX = shadow->x; targetY = shadow->y+1;
        }
    }
    if (canGoDown(*shadow, stage)) {
        if (scentMap[stage][shadow->y-1][shadow->x] > bestScent) {
            bestScent = scentMap[stage][shadow->y-1][shadow->x]; targetX = shadow->x; targetY = shadow->y-1;
        }
    }
    
    if (bestScent > 0) {
        shadow->x = targetX;
        shadow->y = targetY;
    } else {
        int r = rand() % 3;
        if (r == 0 && shadow->x < stageLengths[stage] && !isWall(stage, shadow->y, shadow->x+1)) shadow->x++;
        else if (r == 1 && shadow->x > 0 && !isWall(stage, shadow->y, shadow->x-1)) shadow->x--;
    }
}

void processHallucinations(Hallucination* fakes, int stage) {
    for(int i=0; i<3; i++) {
        if(!fakes[i].active) {
            if (rand() % 4 == 0) { 
                fakes[i].active = true;
                fakes[i].pos = {rand() % stageLengths[stage], (maxFloors[stage]>0)?rand()%(maxFloors[stage]+1):0};
            }
        } else {
            int dir = rand() % 2;
            if (dir == 0 && fakes[i].pos.x < stageLengths[stage]) fakes[i].pos.x++;
            else if (dir == 1 && fakes[i].pos.x > 0) fakes[i].pos.x--;
            if (rand() % 5 == 0) fakes[i].active = false;
        }
    }
}

void handleThrowingMechanic(Position player, Position* soundDistraction, vector<string>& logs, int stage) {
    system("cls");
    cout << "========================================================\n";
    cout << "                    THROW STONE                         \n";
    cout << "========================================================\n";
    cout << " Choose Direction to Throw (W/A/S/D): ";
    char dir; do { dir = _getch(); } while (dir != 'w' && dir != 'a' && dir != 's' && dir != 'd');
    
    cout << "\n Choose Distance (1-5 tiles): ";
    char distC; do { distC = _getch(); } while (distC < '1' || distC > '5');
    int dist = distC - '0';
    
    Position target = player;
    if (dir == 'w') target.y += 1; 
    if (dir == 'a') {
        for(int i=0; i<dist; i++) if(!isWall(stage, target.y, target.x-1) && target.x>0) target.x--;
    } else if (dir == 'd') {
        for(int i=0; i<dist; i++) if(!isWall(stage, target.y, target.x+1) && target.x<stageLengths[stage]) target.x++;
    }
    
    *soundDistraction = target;
    logs.push_back("\033[33;5mCLACK! Stone landed at X:" + to_string(target.x) + " F:" + to_string(target.y) + "\033[0m");
}

void drawLoreLibrary() {
    system("cls");
    cout << "========================================================\n";
    cout << "                    LORE LIBRARY                        \n";
    cout << "========================================================\n";
    for(int i=0; i<5; i++) {
        if (loreUnlocked[i]) {
            cout << "\033[34m" << loreTitles[i] << "\033[0m\n";
            cout << "  \"" << loreText[i] << "\"\n\n";
        } else {
            cout << "??? - [Find Fragments in the maze to unlock]\n\n";
        }
    }
    cout << "========================================================\n";
    cout << " Press any key to return...\n";
    _getch();
}

void drawArchive() {
    system("cls");
    cout << "========================================================\n";
    cout << "                T H E   A R C H I V E                   \n";
    cout << "========================================================\n";
    cout << "\033[31m ENTRY 01: THE SHADOW [S]\033[0m\n";
    cout << " > Description: A manifestation of the abyss.\n";
    cout << " > Behavior: Relentlessly hunts by sound and scent. \n";
    cout << "   If it loses sight of you, it will sniff the floor\n";
    cout << "   to trace your footsteps.\n";
    cout << " > Threat Level: LETHAL (1 Strike = Death in Nightmare)\n\n";

    cout << "\033[31;5m ENTRY 02: THE PHANTOMS [S]\033[0m\n";
    cout << " > Description: Hallucinations born from low sanity.\n";
    cout << " > Behavior: When your sanity drops below 40%, these\n";
    cout << "   blinking entities spawn to trick your mind. They\n";
    cout << "   flicker in and out of reality. Trust only the light.\n";
    cout << " > Threat Level: PSYCHOLOGICAL\n\n";

    cout << "\033[36m ENTRY 03: THE FORGOTTEN [C]\033[0m\n";
    cout << " > Description: The rusted containment lockers.\n";
    cout << " > Behavior: Entering one masks your presence entirely,\n";
    cout << "   causing the Shadow to lose your scent. However,\n";
    cout << "   claustrophobia drains sanity rapidly.\n";
    cout << " > Threat Level: SAFE (Temporary)\n\n";

    cout << "========================================================\n";
    cout << " Press any key to return to the Main Menu...\n";
    _getch();
}

void bubbleSortScores(int* scores, int* times, int size) {
    int* sortedScores = new int[size]; int* sortedTimes = new int[size]; int* stageIds = new int[size];
    for(int i = 1; i < size; i++) { sortedScores[i] = scores[i]; sortedTimes[i] = times[i]; stageIds[i] = i; }
    for (int i = 1; i < size - 1; i++) {
        for (int j = 1; j < size - i; j++) {
            if (sortedScores[j] > sortedScores[j + 1]) {
                int ts = sortedScores[j]; sortedScores[j] = sortedScores[j + 1]; sortedScores[j + 1] = ts;
                int tt = sortedTimes[j]; sortedTimes[j] = sortedTimes[j + 1]; sortedTimes[j + 1] = tt;
                int tid = stageIds[j]; stageIds[j] = stageIds[j + 1]; stageIds[j + 1] = tid;
            }
        }
    }
    cout << "\n--- Performance Ranked (Best to Worst by Moves) ---\n";
    for(int i = 1; i < size; i++) {
        if (sortedScores[i] > 0) cout << "Rank " << i << ": Stage " << stageIds[i] << " -> " << sortedScores[i] << " moves | " << sortedTimes[i] << " seconds\n";
    }
    delete[] sortedScores; delete[] sortedTimes; delete[] stageIds;
}