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
#include <windows.h> 

using namespace std;

// ==========================================
// GLOBAL DISPLAY & QOL SETTINGS
// ==========================================
int gameWidth = 116;    // Default UI Width
bool wideMap = true;    // True = "P . " False = "P."
bool sfxEnabled = true; // QoL: Sound Effects Toggle
int textSpeed = 2;      // QoL: 0 = Instant, 1 = Fast, 2 = Normal

// ==========================================
// AUDIO SFX FUNCTIONS
// ==========================================
void sfxFootstep() { if(sfxEnabled) Beep(150, 40); }
void sfxError() { if(sfxEnabled) { Beep(200, 100); Beep(150, 150); } }
void sfxPickup() { if(sfxEnabled) { Beep(800, 100); Beep(1200, 150); } }
void sfxHurt() { if(sfxEnabled) { Beep(400, 100); Beep(300, 100); Beep(200, 200); } }
void sfxShadowSpotted() { if(sfxEnabled) { Beep(2000, 50); Beep(2500, 50); Beep(2000, 100); } }
void sfxMenuSelect() { if(sfxEnabled) Beep(600, 50); } 
void sfxTrapSnap() { if(sfxEnabled) { Beep(100, 50); Beep(80, 200); } }
void sfxDrink() { if(sfxEnabled) { Beep(900, 50); Beep(1000, 50); Beep(1100, 100); } }
void sfxHeartbeatSlow() { if(sfxEnabled) Beep(250, 150); }
void sfxHeartbeatFast() { if(sfxEnabled) { Beep(400, 80); Sleep(50); Beep(400, 80); } }
void sfxQTEAlert() { if(sfxEnabled) { Beep(1500, 100); Beep(1800, 100); } }
void sfxFlare() { if(sfxEnabled) { Beep(1200, 200); Beep(1000, 400); } }

// ==========================================
// ENGINE OPTIMIZATION & UI FORMATTING
// ==========================================

// Hides the blinking terminal cursor for a cleaner game aesthetic
void hideCursor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false; 
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

// Resets printer position to 0,0 to prevent infinite terminal scrolling
void resetCursor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {0, 0};
    SetConsoleCursorPosition(hConsole, pos); 
}

// Draws a horizontal divider line across the console, using the specified character (default is '=')
void drawDivider(char c = '=') {
    cout << string(gameWidth, c) << "\n";
}

// Draws a centered title with dividers above and below for emphasis
void drawCenteredTitle(const string& title) {
    drawDivider('=');
    int padding = (gameWidth - title.length()) / 2;
    cout << string(padding > 0 ? padding : 0, ' ') << title << "\n";
    drawDivider('=');
}

// ==========================================
// CORE STRUCTS
// ==========================================

// Basic struct to represent X,Y coordinates for various entities and items in the game. This is used for the player, shadow, key, generator, warp gates, holes, and more.
struct Position { int x; int y; };
struct MapItem { Position pos; string name; char symbol; bool active; };
struct Hallucination { Position pos; bool active; };
struct LightPillar { Position pos; int health; bool active; }; 

// ==========================================
// GAME STATE MANAGEMENT
// ==========================================

// Enumeration for managing the different states of the game, such as being in the main menu, actively playing, viewing story or lore, being caught by the shadow, escaping a stage, exiting the game, or being in a game over state.
enum class GameState { MAIN_MENU, PLAYING, STORY, LORE_LIBRARY, CAUGHT, ESCAPED, EXIT, GAMEOVER };

// ==========================================
// GAME VARIABLES
// ==========================================

// Stage and player state variables
const int MAX_STAGE = 10;
const int stageLengths[11] = { 0, 10, 15, 21, 25, 30, 35, 40, 45, 50, 55 }; 
const int maxFloors[11]    = { 0,  0,  1,  2,  3,  4,  4,  5,  5,  6,  6 }; 

// Game Arrays mapping out specific entity and structure locations
int stairPos[11][7];          
Position keyPositions[11];    
Position generatorPos[11];     
Position warpGates[11][2];    
Position holes[11][3];        
MapItem stageItems[11][12]; 
Position lockers[11][4];       
LightPillar pillars[11][7][3]; 

// ==========================================
// MASSIVE LORE DATABASE
// ==========================================

// An array to track which lore entries the player has unlocked, allowing for a persistent lore collection that encourages exploration and replayability. Each entry corresponds to a specific piece of lore that can be discovered throughout the game.
bool loreUnlocked[30] = {false};
const string loreTitles[30] = {
    "1. The Architect's Fall", "2. Nature of the Shadow", "3. The Hallucinations", "4. The Light is Fading", 
    "5. The Scent of Fear", "6. The Missing Walls", "7. The Safe Zones", "8. Final Transmission",
    "9. The Warping Halls", "10. Cracks in the Floor", "11. The Locked Doors", "12. Generator Hum",
    "13. Stolen Memories", "14. The Phantoms", "15. Empty Lockers", "16. Dead Flashlights",
    "17. The Regret", "18. A Way Out", "19. The Evolving Threat", "20. The Last Stand",
    "21. Sector 7 Breach", "22. The Experiment", "23. Blood on the Steel", "24. Fading Sanity",
    "25. The Bear Traps", "26. Energy Reserves", "27. Endless Sprint", "28. The Abyss Stares Back",
    "29. Do Not Trust The Voices", "30. Iridescent Protocol"
};

// The actual lore text corresponding to each title, providing backstory, hints, and world-building details that enrich the player's experience and understanding of the game's setting and mechanics.
const string loreText[30] = {
    "We built the Spire to touch the stars, but we dug too deep. The dark seeped in.",
    "It hunts by sound. It mimics our fears. If you throw something, it will chase the noise.",
    "When the mind breaks in the dark, you will see it everywhere. Trust only the light.",
    "Batteries... I need more batteries. When the beam dies, it finds you immediately.",
    "It can smell your terror. Do not stay in one place. It will follow your path.",
    "The walls of the labyrinth have vanished. It is completely open now. Nowhere to hide.",
    "If you find a glowing pillar, stand in it. The Shadow fears its warmth. But it won't last forever.",
    "If you are reading this, I have escaped the Spire. Do not give up.",
    "The layout changes every time I blink. The architecture is alive.",
    "I saw him fall. The floor gave way without warning. Watch your step.",
    "The keys to the exits are never unguarded. It knows where they are.",
    "The hum of the generator is the only sound that brings me comfort.",
    "I don't remember my own name anymore. Just the cold and the dark.",
    "They look like the Shadow, but they fade in the light. Phantoms of my own mind.",
    "The rust in these lockers smells like blood, but it keeps me safe.",
    "I found a pile of dead batteries today. We are not the first ones here.",
    "We should never have opened the seal beneath the basement. We doomed ourselves.",
    "Floor by floor, it gets darker. But the only way out is up.",
    "If you shine your flashlight on it, it gets enraged and sprints. Turn it off and run!",
    "I only have one stone left. I can hear it breathing. This is it.",
    "Sector 7 was the first to fall. The researchers didn't even have time to scream.",
    "We thought we were studying a new element. We didn't realize it was studying us.",
    "I found a crowbar covered in rust and something darker. It didn't save the last guy.",
    "The whispers are getting louder. I need to find a light pillar before I lose it completely.",
    "I found some old bear traps in the maintenance closet. Maybe they can slow it down.",
    "Energy drinks. Pure caffeine and taurine. I need them to keep sprinting.",
    "My lungs are burning. If I stop running, it will catch me. Just one more floor.",
    "If you stare into the dark long enough, you realize it is not empty. It is waiting.",
    "The voices sound like my family. Do not follow them. They lead to the holes.",
    "Initiate Iridescent Protocol. Total containment. No one leaves alive."
};

char terrain3D[MAX_STAGE + 1][7][60];
int scentMap[MAX_STAGE + 1][7][60]; 
bool skipAnim = false; 

// ==========================================
// FUNCTION PROTOTYPES
// ==========================================
void skippableSleep(int ms);
void typeText(const string& text, int delay = 20);
void bootSequence();
void shutdownSequence(); 
void printTitleScreen();
void printGameOverScreen();
void printVictoryScreen();
void playIntroCinematic();
void playOutroCinematic();
void drawBestiaryArt(int type);
void generateTrapsForStage(int stage);
void generateMap();
void applyMove(Position& pos, int dx, int dy); 
bool canGoUp(const Position& pos, int stage);         
bool canGoDown(const Position& pos, int stage);       
bool isWall(int stage, int f, int x);
bool isSafeZone(int stage, int f, int x);
void drawUI(int stage, const Position& player, const Position& shadow, bool shadowActive, bool hasKey, bool genActive, int hp, int maxHP, int battery, int sanity, int stamina, bool isSprinting, bool isHiding, const Position& soundDistraction, const vector<string>& logs, const vector<string>& inv, bool flashlightActive, int elapsedSecs, bool isNightmare, Hallucination* fakes, bool showPrompt = true);
int getClosestStair(int stage, int floor, int currentX, int stageLength); 
void processShadowAI(const Position& player, Position& shadow, int stage, bool isHiding, Position& soundDistraction, bool hasSight, int& shadowStunTimer, int& shadowFearTimer); 
void processHallucinations(Hallucination* fakes, int stage);
void decayScentMap(int stage);
void bubbleSortScores(int* scores, int* times, int size); 
void drawLoreLibrary();
void drawArchive(); 
void handleThrowingMechanic(Position& player, Position& soundDistraction, vector<string>& logs, int stage);
void handlePlaceTrap(Position& player, vector<string>& logs, int stage, vector<string>& inv);
bool holdBreathQTE();
bool hackGeneratorQTE();

// ==========================================
// FILE I/O
// ==========================================

// NEW: Simple file I/O functions to save and load player progress in the Lore Library. This allows players to keep their unlocked lore entries between sessions, encouraging exploration and replayability.
void loadGameData() {
    ifstream inFile("spire_archive.sav");
    if (inFile.is_open()) {
        for(int i = 0; i < 30; i++) inFile >> loreUnlocked[i];
        inFile.close();
    }
}

// Saves the current state of unlocked lore entries to a file, allowing progress to persist across game sessions. This is called whenever the player unlocks a new lore entry, ensuring their discoveries are not lost.
void saveGameData() {
    ofstream outFile("spire_archive.sav");
    if (outFile.is_open()) {
        for(int i = 0; i < 30; i++) outFile << loreUnlocked[i] << " ";
        outFile.close();
    }
}

// ==========================================
// MAIN GAME LOOP
// ==========================================

// The main function initializes the game, handles the main menu and difficulty selection, manages the game loop, and processes state transitions between playing, game over, and stage clear screens. It also ensures that player and shadow states are reset appropriately when starting new stages or returning to the main menu.
int main() {
    hideCursor(); 
    srand(time(0)); 
    loadGameData(); 
    bootSequence();
    
    // Initial display setup
    system("cls");
    string startupPad((gameWidth - 40) / 2, ' ');
    cout << "\n" << startupPad << "\033[33;5m[SYSTEM NOTE]\033[0m\n";
    cout << startupPad << "For the best visual experience, please MAXIMIZE your console window now.\n";
    cout << startupPad << "Press any key to continue...\n";
    while(_kbhit()) _getch(); 
    _getch();
    
    // Generate the initial map layout for all stages before entering the main game loop. This allows for faster loading times between stages and ensures that all stage data is ready when needed.
    generateMap(); 
    
    int* stageScores = new int[MAX_STAGE + 1]; 
    int* stageTimes = new int[MAX_STAGE + 1];
    for(int i = 0; i <= MAX_STAGE; i++) { *(stageScores + i) = 0; *(stageTimes + i) = 0; }
    
    int currentStage = 1;
    GameState state = GameState::MAIN_MENU; 
    
    Position player = {0, 0};
    Position shadow = {-1, 0};
    Position soundDistraction = {-1, -1}; 
    
    Hallucination fakes[3]; 
    for(int i=0; i<3; i++) fakes[i].active = false;

    // State variables for various mechanics and player conditions, such as whether the shadow is active, if the player has the key or generator activated, flashlight status, hiding and sprinting states, health and resource levels, and timers for shadow behavior. These variables are crucial for managing the complex interactions and challenges the player faces in each stage.
    bool shadowActive = false;
    bool hasKey = false;
    bool generatorActive = false;
    bool flashlightActive = true; 
    bool isHiding = false;
    bool isSprinting = false;
    
    // Counters for player movement and stage progression, allowing for tracking the number of moves taken in total and within the current stage. This is important for scoring, pacing, and providing feedback to the player on their performance.
    int totalMoves = 0; 
    int stageMoves = 0; 
    
    // Difficulty modifiers and player condition variables that affect gameplay mechanics such as health, shadow behavior, and resource management. These allow for a customizable experience based on the player's chosen difficulty level, adding replayability and catering to different playstyles.
    bool isNightmare = false; // Nightmare mode with increased difficulty and unique mechanics
    bool isBeginner = false; // Beginner mode with reduced difficulty and more forgiving mechanics
    int maxHP = 3; // Player health points, which determine how many hits from the shadow the player can take before being caught. This is a core mechanic that adds tension and challenge, as players must manage their health carefully to survive.
    int playerHP = maxHP; // Player health points, which determine how many hits from the shadow the player can take before being caught. This is a core mechanic that adds tension and challenge, as players must manage their health carefully to survive.
    int batteryLevel = 100; // NEW: Sanity level that depletes over time and with certain actions, adding a layer of psychological horror and increasing the challenge as the player must manage not only their physical health but also their mental state to survive the shadow's pursuit.
    int sanityLevel = 100; // NEW: Stamina level for sprinting mechanic, which depletes as the player sprints and regenerates when not sprinting. This adds a layer of resource management to the sprinting mechanic, preventing it from being overused and encouraging strategic use of sprinting to escape the shadow or navigate quickly.
    int staminaLevel = 100; // NEW: Variable to control after how many player moves the shadow becomes active in each stage, allowing for a gradual increase in tension and giving players a chance to prepare before the shadow starts hunting them.
    int shadowSpawnTurn = 2; // NEW: Variable to control after how many player moves the shadow becomes active in each stage, allowing for a gradual increase in tension and giving players a chance to prepare before the shadow starts hunting them.
    int shadowStunTimer = 0; // NEW: Timer for how long the shadow remains stunned after being hit with a flare, adding a strategic element to using flares as a defensive tool against the shadow.
    int shadowFearTimer = 0; // NEW: Fear mechanic for Flares
    
    // Boss escape phase variable to manage the final confrontation with the shadow in the last stage, where the player must escape rather than just survive. This adds a climactic challenge and a change of pace in the final stage, providing a memorable conclusion to the game.
    bool bossEscapePhase = false;
    
    // Timer variable to track how long the player has been on the current stage, which is used for scoring and providing feedback on performance. This adds an element of time-based challenge and encourages players to improve their speed in addition to their survival skills.
    time_t stageStartTime = time(0);
    int finalStageTime = 0;
    
    // Vectors for managing the event log and player inventory, allowing for dynamic tracking of recent events and items collected. The event log provides feedback and narrative context to the player, while the inventory allows for item management and strategic use of resources found throughout the stages.
    vector<string> gameLogs; 
    vector<string> inventory; 
    
    // Main game loop that continues until the player chooses to exit. This loop manages the different states of the game, including the main menu, active gameplay, story and lore viewing, being caught by the shadow, escaping a stage, and game over conditions. It processes player input, updates game state, and handles transitions between different screens and stages.
    while (state != GameState::EXIT) {
        if (state == GameState::MAIN_MENU) {
            printTitleScreen();
            string mPad((gameWidth - 25) / 2, ' '); 
            cout << mPad << "1. Start Game\n";
            cout << mPad << "2. How to Play\n";
            cout << mPad << "3. Lore Library\n";
            cout << mPad << "4. The Archive\n";
            cout << mPad << "5. Credits\n";
            cout << mPad << "6. Options (Settings)\n";
            cout << mPad << "Q. Quit\n";
            drawDivider('-');
            cout << mPad << "Choice: ";

            char choice;
            do { choice = _getch(); } while (choice < '1' || (choice > '6' && choice != 'q' && choice != 'Q'));
            sfxMenuSelect();

            if (choice == '1') {
                system("cls");
                drawCenteredTitle("SELECT DIFFICULTY");
                string diffPad((gameWidth - 40) / 2, ' ');
                cout << diffPad << "\033[32m1. Beginner Mode\033[0m\n";
                cout << diffPad << "   - 5 HP, Slow Sanity Drain, Very Forgiving Shadow\n\n";
                cout << diffPad << "2. Normal Mode\n";
                cout << diffPad << "   - 3 HP, Normal Sanity Drain, Forgiving Shadow\n\n";
                cout << diffPad << "\033[31m3. NIGHTMARE MODE\033[0m\n";
                cout << diffPad << "   - \033[31;5m1 HP, Instant Shadow, Aggressive Scent Tracking\033[0m\n\n";
                cout << diffPad << "Q. Cancel (Return to Main Menu)\n";
                drawDivider('=');
                cout << diffPad << "Choice: ";
                
                char diffChoice;
                do { diffChoice = _getch(); } while (diffChoice != '1' && diffChoice != '2' && diffChoice != '3' && diffChoice != 'q' && diffChoice != 'Q');
                sfxMenuSelect();
                if (diffChoice == 'q' || diffChoice == 'Q') continue; 
                
                playIntroCinematic();
                
                currentStage = 1; totalMoves = 0; inventory.clear();
                generateMap(); system("cls"); 
                
                if (diffChoice == '3') {
                    isNightmare = true; isBeginner = false; maxHP = 1; shadowSpawnTurn = 0; 
                    gameLogs.push_back("\033[31;5mNIGHTMARE MODE INITIATED. 1 HP. GOOD LUCK.\033[0m");
                } else if (diffChoice == '1') {
                    isNightmare = false; isBeginner = true; maxHP = 5; shadowSpawnTurn = 5; 
                    gameLogs.push_back("\033[32mBEGINNER MODE INITIATED. 5 HP. Take your time.\033[0m");
                } else {
                    isNightmare = false; isBeginner = false; maxHP = 3; shadowSpawnTurn = 2; 
                    gameLogs.push_back("\033[32mGame Initialized. Power the Generator and find the Key!\033[0m");
                }
                
                playerHP = maxHP; batteryLevel = 100; sanityLevel = 100; staminaLevel = 100;
                stageStartTime = time(0); 
                state = GameState::PLAYING; 
                
            } else if (choice == '2') {
                system("cls");
                drawCenteredTitle("HOW TO PLAY");
                string pad((gameWidth - 70) / 2, ' ');
                cout << pad << "GOAL: Navigate the floors. Restore power via Generator (G) to unlock the Key (K), then Escape.\n\n";
                cout << pad << "SURVIVAL MECHANICS:\n";
                cout << pad << " - Sprinting: Press 'R' to toggle Sprint. Moves 2 tiles but drains Stamina!\n";
                cout << pad << " - Battery: Drains as you walk. Find (b) to recharge.\n";
                cout << pad << " - Sanity: Drains in the dark. Spawns hallucinations!\n";
                cout << pad << " - Enrage: Shining your Flashlight on the Shadow makes it SPRINT.\n";
                cout << pad << " - Safe Zones (I): Glowing pillars restore Sanity and block the Shadow.\n\n";
                cout << pad << "ENTITIES & TRAPS:\n";
                cout << pad << " \033[32mP\033[0m = Player | \033[31mS\033[0m = Shadow | \033[36m#\033[0m = Ladder | \033[33;1mI\033[0m = Light Pillar\n";
                cout << pad << " \033[33mG\033[0m = Generator | \033[33m~\033[0m = Crumbling Floor | \033[31mO\033[0m = Hole | \033[37m^\033[0m = Bear Trap\n\n";
                cout << pad << "ITEMS:\n";
                cout << pad << " \033[32m+\033[0m = Potion | \033[36mb\033[0m = Battery | \033[37m*\033[0m = Stone | \033[34m?\033[0m = Lore\n";
                cout << pad << " \033[35mE\033[0m = Energy | \033[37m^\033[0m = Trap    | \033[31mF\033[0m = Flare (Fears the Shadow)\n\n";
                cout << pad << "CONTROLS: W/A/S/D = Move | 1/2/3 = Use Item | F = Flashlight\n";
                cout << pad << " H = Hide | T = Throw | L = Listen | R = Toggle Sprint\n";
                drawDivider('=');
                cout << pad << "Press any key to return...\n";
                _getch(); sfxMenuSelect();
            } else if (choice == '3') { drawLoreLibrary(); } 
              else if (choice == '4') { drawArchive(); } 
              else if (choice == '5') {
                system("cls");
                drawCenteredTitle("CREDITS");
                string pad((gameWidth - 60) / 2, ' ');
                cout << pad << "Asfa Izzat: Lead Director, Programmer\n";
                cout << pad << "Sofea Sholihah: Assistant Director, Documentation\n";
                cout << pad << "Tan Kai Xue: Programmer, Idea\n";
                cout << pad << "Tan Kai Xuan: Programmer\n\n";
                cout << pad << "Remembering Za'im Arsyad due to his termination from PASUM\n\n";
                cout << pad << "Language: C++\n";
                cout << pad << "Lecturer: En. Amirul Mohamad Khairi, Dr. Nur Amirah, Pn. Fatin Nabila\n";
                cout << pad << "Instructor: Mr. Abdullah Mohammad Khan\n\n";
                cout << pad << "Gameplay inspiration: Honkai: Star Rail\n";
                cout << pad << "Game theme inspiration: Resident Evil, Darkwood, Utahon no Tatari, Exit 8\n";
                cout << pad << "Path to Iridescent Git repository:\n";
                cout << pad << "https://github.com/rokurooooo01/Path-of-Iridescent-test-\n";
                drawDivider('=');
                cout << pad << "Press any key to return...\n";
                _getch(); sfxMenuSelect();
            } else if (choice == '6') {
                bool inOptions = true;
                while (inOptions) {
                    system("cls");
                    drawCenteredTitle("OPTIONS MENU");
                    string optPad((gameWidth - 40) / 2, ' ');
                    
                    string sizeStr = (gameWidth == 80) ? "Compact (80 col)" : (gameWidth == 116) ? "Standard (116 col)" : "Ultrawide (140 col)";
                    string sfxStr = sfxEnabled ? "\033[32mON\033[0m" : "\033[31mOFF\033[0m";
                    string speedStr = (textSpeed == 0) ? "Instant" : (textSpeed == 1) ? "Fast" : "Normal";

                    cout << optPad << "1. Display Size : [" << sizeStr << "]\n";
                    cout << optPad << "2. Sound Effects: [" << sfxStr << "]\n";
                    cout << optPad << "3. Text Speed   : [" << speedStr << "]\n\n";
                    cout << optPad << "Q. Back to Main Menu\n";
                    drawDivider('-');
                    cout << optPad << "Choice: ";
                    
                    char optChoice = _getch();
                    sfxMenuSelect();
                    
                    if (optChoice == '1') {
                        if (gameWidth == 80) { gameWidth = 116; wideMap = true; }
                        else if (gameWidth == 116) { gameWidth = 140; wideMap = true; }
                        else { gameWidth = 80; wideMap = false; }
                    } else if (optChoice == '2') {
                        sfxEnabled = !sfxEnabled;
                    } else if (optChoice == '3') {
                        textSpeed = (textSpeed + 1) % 3;
                    } else if (optChoice == 'q' || optChoice == 'Q') {
                        inOptions = false;
                    }
                }
            }
            else { state = GameState::EXIT; }
        }
        
        while (state == GameState::PLAYING) {
            int currentElapsed = difftime(time(0), stageStartTime);
            drawUI(currentStage, player, shadow, shadowActive, hasKey, generatorActive, playerHP, maxHP, batteryLevel, sanityLevel, staminaLevel, isSprinting, isHiding, soundDistraction, gameLogs, inventory, flashlightActive, currentElapsed, isNightmare, fakes);
            
            while (gameLogs.size() > 5) gameLogs.erase(gameLogs.begin()); 

            Position oldPos = player; 
            char input = _getch();
            bool validMove = false;
            bool actionTaken = false;
            
            int moveDist = isSprinting ? 2 : 1;

            if (!isHiding) {
                switch (input) {
                    case 'd': case 'D':
                        for(int m=0; m<moveDist; m++) {
                            if (player.x < stageLengths[currentStage] && !isWall(currentStage, player.y, player.x + 1)) { applyMove(player, 1, 0); validMove = true; } 
                            else { if(m==0) { gameLogs.push_back("Wall ahead!"); sfxError(); } break; }
                        }
                        if(validMove) sfxFootstep(); break;
                    case 'a': case 'A':
                        for(int m=0; m<moveDist; m++) {
                            if (player.x > 0 && !isWall(currentStage, player.y, player.x - 1)) { applyMove(player, -1, 0); validMove = true; } 
                            else { if(m==0) { gameLogs.push_back("Cannot move further left."); sfxError(); } break; }
                        }
                        if(validMove) sfxFootstep(); break;
                    case 'w': case 'W':
                        if (canGoUp(player, currentStage)) { applyMove(player, 0, 1); gameLogs.push_back("Climbed UP."); validMove = true; sfxFootstep(); } 
                        else { gameLogs.push_back("No ladder leading up here!"); sfxError(); } break;
                    case 's': case 'S':
                        if (canGoDown(player, currentStage)) { applyMove(player, 0, -1); gameLogs.push_back("Climbed DOWN."); validMove = true; sfxFootstep(); } 
                        else { gameLogs.push_back("No ladder leading down here!"); sfxError(); } break;
                    case 'r': case 'R':
                        isSprinting = !isSprinting; actionTaken = true; sfxMenuSelect();
                        if (isSprinting) gameLogs.push_back("\033[33mSPRINT MODE ENGAGED. Drains stamina!\033[0m");
                        else gameLogs.push_back("Sprint disabled. Walking normally."); break;
                    case 'f': case 'F':
                        flashlightActive = !flashlightActive; actionTaken = true; sfxFootstep(); 
                        if (flashlightActive) gameLogs.push_back("CLICK! Flashlight ON.");
                        else gameLogs.push_back("CLICK! Flashlight OFF. Darkness consumes you..."); break;
                    case 'h': case 'H': {
                        bool onLocker = false;
                        for(int l=0; l<4; l++) if(player.x == lockers[currentStage][l].x && player.y == lockers[currentStage][l].y) onLocker = true;
                        if (onLocker) { isHiding = true; actionTaken = true; isSprinting = false; sfxFootstep(); gameLogs.push_back("\033[36mYou slip into the locker and hold your breath...\033[0m"); } 
                        else { gameLogs.push_back("You must be standing on a Cabinet (C) to hide!"); sfxError(); } break;
                    }
                    case 'l': case 'L': {
                        actionTaken = true;
                        int dx = shadow.x - player.x; int dy = shadow.y - player.y; int dist = abs(dx) + abs(dy) * 10;
                        if (!shadowActive) { gameLogs.push_back("\033[36mSilence... for now.\033[0m"); } 
                        else if (dist <= 3 && shadow.y == player.y) { gameLogs.push_back("\033[31;5mIT IS IN THE ROOM WITH YOU!\033[0m"); sfxShadowSpotted(); } 
                        else if (dy > 0) gameLogs.push_back("\033[33mYou hear heavy footsteps from the floor above...\033[0m");
                        else if (dy < 0) gameLogs.push_back("\033[33mYou hear scratching from the floor below...\033[0m");
                        else if (dx > 0) gameLogs.push_back("\033[33mA chilling breeze comes from the East...\033[0m");
                        else gameLogs.push_back("\033[33mA chilling breeze comes from the West...\033[0m");
                        break;
                    }
                    case 't': case 'T': {
                        int rockIdx = -1;
                        for(size_t i=0; i<inventory.size(); i++) if(inventory[i] == "Stone") rockIdx = i;
                        if (rockIdx != -1) {
                            handleThrowingMechanic(player, soundDistraction, gameLogs, currentStage);
                            inventory.erase(inventory.begin() + rockIdx); actionTaken = true; validMove = true; sfxFootstep(); 
                        } else { gameLogs.push_back("You don't have any Stones to throw!"); sfxError(); } break;
                    }
                    case '1': case '2': case '3': {
                        int slot = (input - '0') - 1; 
                        if (slot < inventory.size()) {
                            if (inventory[slot] == "Health Potion") {
                                if (playerHP < maxHP) { playerHP++; gameLogs.push_back("\033[32mGULP! Used Health Potion. HP +1.\033[0m"); inventory.erase(inventory.begin() + slot); actionTaken = true; sfxDrink(); } 
                                else { gameLogs.push_back("HP is already full!"); sfxError(); }
                            } else if (inventory[slot] == "Battery") {
                                batteryLevel = 100; gameLogs.push_back("\033[36mRELOAD! Flashlight battery restored to 100%.\033[0m");
                                inventory.erase(inventory.begin() + slot); actionTaken = true; flashlightActive = true; sfxPickup();
                            } else if (inventory[slot] == "Energy Drink") {
                                staminaLevel += 50; if(staminaLevel > 100) staminaLevel = 100; gameLogs.push_back("\033[35mGULP! Energy Drink used. Stamina +50.\033[0m");
                                inventory.erase(inventory.begin() + slot); actionTaken = true; sfxDrink();
                            } else if (inventory[slot] == "Bear Trap") {
                                handlePlaceTrap(player, gameLogs, currentStage, inventory); actionTaken = true; sfxTrapSnap();
                            } else if (inventory[slot] == "Signal Flare") {
                                shadowFearTimer = 4; // Shadow runs away for 4 turns!
                                inventory.erase(inventory.begin() + slot); actionTaken = true; sfxFlare();
                                gameLogs.push_back("\033[31;1mFLARE IGNITED! The blinding light terrifies the Shadow!\033[0m");
                            } else if (inventory[slot] == "Stone") { gameLogs.push_back("Press 'T' to throw the Stone and distract the Shadow!"); sfxError(); }
                        } else { gameLogs.push_back("That backpack slot is empty!"); sfxError(); } break;
                    }
                    case '9':
                        hasKey = true; playerHP = 99; player.x = stageLengths[currentStage]; player.y = maxFloors[currentStage];
                        if (currentStage == MAX_STAGE) { player.x = 0; player.y = 0; }
                        actionTaken = true; validMove = true; sfxPickup(); gameLogs.push_back("\033[35;5m[DEV MODE] Cheat activated.\033[0m"); break;
                    default: continue; 
                }
            } else {
                if (input == 'h' || input == 'H') { isHiding = false; actionTaken = true; sfxFootstep(); gameLogs.push_back("\033[36mYou slowly push the locker door open and step out.\033[0m"); } 
                else if (input == 'f' || input == 'F') { flashlightActive = !flashlightActive; actionTaken = true; sfxFootstep(); } 
                else { gameLogs.push_back("You are hiding! Press 'H' to exit the locker."); continue; }
            }

            if (validMove || actionTaken) {
                totalMoves++; if (validMove) stageMoves++; 
                if (validMove) scentMap[currentStage][player.y][player.x] = 20; 
                
                if (validMove && bossEscapePhase && currentStage == MAX_STAGE) {
                    terrain3D[currentStage][oldPos.y][oldPos.x] = 'O';
                    gameLogs.push_back("\033[31;5mThe Spire is collapsing into the void behind you!\033[0m");
                }
                
                if (isSprinting && validMove) {
                    staminaLevel -= 15;
                    if (staminaLevel <= 0) {
                        staminaLevel = 0; isSprinting = false;
                        gameLogs.push_back("\033[31mExhausted! You can no longer sprint.\033[0m");
                    }
                } else if (!isSprinting && staminaLevel < 100) {
                    staminaLevel += 5; if(staminaLevel > 100) staminaLevel = 100;
                }
                
                if (flashlightActive && batteryLevel > 0) {
                    if (isNightmare) batteryLevel -= 2; 
                    else if (isBeginner) { if (stageMoves % 3 == 0) batteryLevel -= 1; } 
                    else { if (stageMoves % 2 == 0) batteryLevel -= 1; }
                }
                if (batteryLevel <= 0) { batteryLevel = 0; flashlightActive = false; }
                
                bool onPillar = false;
                for(int p=0; p<3; p++) {
                    if (pillars[currentStage][player.y][p].active && pillars[currentStage][player.y][p].pos.x == player.x) {
                        onPillar = true; sanityLevel += 15; if (sanityLevel > 100) sanityLevel = 100;
                        pillars[currentStage][player.y][p].health--;
                        if (pillars[currentStage][player.y][p].health <= 0) { pillars[currentStage][player.y][p].active = false; gameLogs.push_back("\033[33;5mThe Light Pillar burns out and dies...\033[0m"); } 
                        else { gameLogs.push_back("\033[33mBasking in the Light Pillar. Sanity restored.\033[0m"); }
                    }
                }
                
                int distToShadow = abs(player.x - shadow.x) + (abs(player.y - shadow.y) * 10);
                if (!onPillar) {
                    if (!flashlightActive || distToShadow <= 5) {
                        if (isNightmare) sanityLevel -= 3; else if (isBeginner) sanityLevel -= 1; else sanityLevel -= 2;
                    } else if (isHiding) sanityLevel -= 1; else if (sanityLevel < 100) sanityLevel += 1; 
                }
                if (sanityLevel <= 0) sanityLevel = 0;
                
                if (validMove && terrain3D[currentStage][oldPos.y][oldPos.x] == '~') {
                    terrain3D[currentStage][oldPos.y][oldPos.x] = 'O'; sfxHurt();
                    gameLogs.push_back("\033[33mCRACK! The floor crumbled into a hole behind you!\033[0m");
                }

                // NEW: GENERATOR HACKING MINIGAME
                if (!generatorActive && player.x == generatorPos[currentStage].x && player.y == generatorPos[currentStage].y) {
                    drawUI(currentStage, player, shadow, shadowActive, hasKey, generatorActive, playerHP, maxHP, batteryLevel, sanityLevel, staminaLevel, isSprinting, isHiding, soundDistraction, gameLogs, inventory, flashlightActive, currentElapsed, isNightmare, fakes, false);
                    
                    if (hackGeneratorQTE()) {
                        generatorActive = true; sfxPickup();
                        gameLogs.push_back("\033[33;5m[ GENERATOR OVERRIDDEN ] Power restored to the lower floors!\033[0m");
                        gameLogs.push_back("\033[36mThe Key is now accessible. Find it.\033[0m");
                    } else {
                        gameLogs.push_back("\033[31;5m[ OVERRIDE FAILED ] The alarm triggered!\033[0m");
                        sfxError();
                        shadowActive = true;
                        shadow.y = player.y;
                        shadow.x = (player.x > stageLengths[currentStage]/2) ? 0 : stageLengths[currentStage];
                    }
                }

                if (generatorActive && !hasKey && player.x == keyPositions[currentStage].x && player.y == keyPositions[currentStage].y) {
                    hasKey = true; sfxPickup();
                    if (currentStage == MAX_STAGE) {
                        bossEscapePhase = true; shadowActive = true; 
                        shadow.x = player.x > 3 ? player.x - 3 : 0; shadow.y = player.y; 
                        sfxShadowSpotted();
                        gameLogs.push_back("\033[31;5m[!] THE SHADOW HAS AWAKENED FULLY! THE EXIT IS LOCKED!\033[0m");
                        gameLogs.push_back("\033[33;5m[!] NEW OBJECTIVE: RUN BACK TO THE STARTING LADDER (0,0) TO ESCAPE!\033[0m");
                    } else { gameLogs.push_back("\033[33mSUCCESS: Picked up the Key! Exit unlocked.\033[0m"); }
                }

                for (int i = 0; i < 12; i++) {
                    if (stageItems[currentStage][i].active && player.x == stageItems[currentStage][i].pos.x && player.y == stageItems[currentStage][i].pos.y) {
                        if (stageItems[currentStage][i].name == "Lore Fragment") {
                            int unlockIdx = rand() % 30; loreUnlocked[unlockIdx] = true; saveGameData(); 
                            stageItems[currentStage][i].active = false; sfxPickup();
                            gameLogs.push_back("\033[34mFound a torn journal page! Added to Library and Saved.\033[0m");
                        }
                        else if (inventory.size() < 3) {
                            inventory.push_back(stageItems[currentStage][i].name); stageItems[currentStage][i].active = false; sfxPickup();
                            gameLogs.push_back("Looted: " + stageItems[currentStage][i].name);
                        } else { if(validMove) gameLogs.push_back("Backpack is full! Left " + stageItems[currentStage][i].name + " behind."); }
                    }
                }

                if (player.x == warpGates[currentStage][0].x && player.y == warpGates[currentStage][0].y) {
                    player.x = warpGates[currentStage][1].x; player.y = warpGates[currentStage][1].y; sfxPickup();
                    gameLogs.push_back("\033[35mZAP! You used the Warp Gate!\033[0m");
                } else if (player.x == warpGates[currentStage][1].x && player.y == warpGates[currentStage][1].y) {
                    player.x = warpGates[currentStage][0].x; player.y = warpGates[currentStage][0].y; sfxPickup();
                    gameLogs.push_back("\033[35mZAP! You used the Warp Gate!\033[0m");
                }

                bool fell = false;
                for (int h = 0; h < 3; h++) if (player.x == holes[currentStage][h].x && player.y == holes[currentStage][h].y) fell = true;
                if (terrain3D[currentStage][player.y][player.x] == 'O') fell = true;
                
                // NEW: If the player falls into a hole, they lose 1 HP and are respawned at the starting point of the stage. The Shadow is temporarily deactivated to give the player a moment to recover, but this also resets the Shadow's position and stage move counter, adding tension as the Shadow can reappear suddenly after a fall.
                if (fell) {
                    playerHP--; player = {0, 0}; shadowActive = false; shadow = {-1, 0}; stageMoves = 0; isHiding = false; isSprinting = false; sfxHurt();
                    gameLogs.push_back("\033[31mAHHH! You fell in a hole! Lost 1 HP and respawned.\033[0m");
                    if (playerHP <= 0) state = GameState::GAMEOVER;
                }

                if (state == GameState::PLAYING) {
                    if (!shadowActive && stageMoves >= shadowSpawnTurn) {
                        shadowActive = true; shadow.x = 0; shadow.y = 0; sfxShadowSpotted();
                        gameLogs.push_back("\033[31mWARNING: THE SHADOW IS HUNTING YOU!\033[0m");
                    } else if (shadowActive) {
                        
                        // Check for bear trap stun
                        if (shadowStunTimer > 0) {
                            shadowStunTimer--;
                            if (shadowStunTimer == 0) gameLogs.push_back("\033[31mThe Shadow breaks free from the trap!\033[0m");
                            else gameLogs.push_back("\033[33mThe Shadow is trapped and thrashing!\033[0m");
                        } else {
                            bool shadowHasSight = (shadow.y == player.y && abs(shadow.x - player.x) <= 5 && flashlightActive);
                            bool isEnraged = shadowHasSight && flashlightActive;
                            if (isEnraged && stageMoves % 2 == 0) gameLogs.push_back("\033[31;5mTHE LIGHT ENRAGES IT! IT IS SPRINTING!\033[0m");
                            
                            int shadowMoves = isEnraged ? 2 : 1; 
                            
                            if (shadowFearTimer > 0) {
                                shadowFearTimer--;
                                shadowMoves = 2; // Runs away fast!
                            }
                            
                            for(int sm = 0; sm < shadowMoves; sm++) {
                                if(shadowStunTimer == 0) processShadowAI(player, shadow, currentStage, isHiding, soundDistraction, shadowHasSight, shadowStunTimer, shadowFearTimer); 
                            }
                            
                            if (shadow.y == player.y && abs(shadow.x - player.x) > (flashlightActive ? 5 : 1) && shadowStunTimer == 0) {
                                string dir = (shadow.x < player.x) ? "West" : "East";
                                gameLogs.push_back("\033[31m...you hear dragging footsteps to the " + dir + ".\033[0m");
                            }
                        }
                        
                        // NEW: Heartbeat sound cues based on Shadow proximity
                        if (!isHiding && shadow.y == player.y && shadowFearTimer == 0) {
                            int distToShadow = abs(player.x - shadow.x);
                            if (distToShadow <= 4) sfxHeartbeatFast();
                            else if (distToShadow <= 10) sfxHeartbeatSlow();
                        }
                        
                        // NEW: Hiding QTE when Shadow is very close
                        if (isHiding && shadow.y == player.y && abs(player.x - shadow.x) <= 1 && shadowFearTimer == 0) {
                            drawUI(currentStage, player, shadow, shadowActive, hasKey, generatorActive, playerHP, maxHP, batteryLevel, sanityLevel, staminaLevel, isSprinting, isHiding, soundDistraction, gameLogs, inventory, flashlightActive, currentElapsed, isNightmare, fakes, false);
                            bool success = holdBreathQTE();
                            if (success) {
                                gameLogs.push_back("\033[32mYou held your breath. It didn't hear you and moved away.\033[0m");
                                shadow.x = (shadow.x > player.x) ? min(shadow.x + 3, stageLengths[currentStage]) : max(shadow.x - 3, 0);
                            } else {
                                gameLogs.push_back("\033[31;5mYOU GASPED! IT RIPPED OPEN THE LOCKER!\033[0m");
                                isHiding = false;
                                playerHP--;
                                sfxHurt();
                                shadow.x = (shadow.x > player.x) ? min(shadow.x + 2, stageLengths[currentStage]) : max(shadow.x - 2, 0); 
                                if (playerHP <= 0) state = GameState::GAMEOVER;
                            }
                        }
                    }

                    // NEW: Hallucination mechanics based on Sanity level. When Sanity drops below 40, fake entities appear randomly on the map, mimicking real items or pillars. They disappear when sanity is restored above 40.
                    if (sanityLevel < 40) {
                        processHallucinations(fakes, currentStage);
                        if(rand()%10 == 0) gameLogs.push_back("\033[31;5mTHEY ARE IN THE WALLS THEY ARE IN THE WALLS\033[0m");
                    } else { for(int i=0; i<3; i++) fakes[i].active = false; }
                }
                decayScentMap(currentStage);
            }
            
            // Check for Shadow encounter after all movements and actions are processed
            if (shadowActive && player.x == shadow.x && player.y == shadow.y && !isHiding && shadowStunTimer == 0 && shadowFearTimer == 0) {
                playerHP--; sfxHurt();
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
                        *(stageScores + currentStage) = stageMoves; *(stageTimes + currentStage) = difftime(time(0), stageStartTime); 
                        finalStageTime = *(stageTimes + currentStage); state = GameState::ESCAPED; sfxPickup();
                    } else { gameLogs.push_back("The door is locked! Find the Key (K) first!"); sfxError(); }
                }
            }
        }

        // Handle Game Over and Stage Clear screens with options to restart, proceed, or return to menu
        if (state == GameState::GAMEOVER) {
            finalStageTime = difftime(time(0), stageStartTime); 
            drawUI(currentStage, player, shadow, shadowActive, hasKey, generatorActive, playerHP, maxHP, batteryLevel, sanityLevel, staminaLevel, isSprinting, isHiding, soundDistraction, gameLogs, inventory, flashlightActive, finalStageTime, isNightmare, fakes, false);
            
            printGameOverScreen();
            string gText = "[!] ZERO HP. THE DARKNESS CONSUMED YOU.";
            int gPad = (gameWidth - gText.length()) / 2;
            cout << "\n" << string(gPad > 0 ? gPad : 0, ' ') << "\033[31m" << gText << "\033[0m\n";
            
            string mPad((gameWidth - 37) / 2, ' ');
            cout << mPad << "1. Restart Entire Game (From Stage 1)\n";
            cout << mPad << "2. Retry Current Stage (Reset Stats)\n";
            cout << mPad << "Q. Main Menu (Quit)\n";
            drawDivider('-');
            cout << mPad << "Choice: ";
            
            char choice; do { choice = _getch(); } while (choice != '1' && choice != '2' && choice != 'q' && choice != 'Q'); sfxMenuSelect();
            if (choice == '1') { currentStage = 1; totalMoves = 0; playerHP = maxHP; inventory.clear(); bossEscapePhase = false; sanityLevel = 100; batteryLevel = 100; generateMap(); stageStartTime = time(0); state = GameState::PLAYING; } 
            else if (choice == '2') { playerHP = maxHP; inventory.clear(); sanityLevel = 100; batteryLevel = 100; bossEscapePhase = false; stageStartTime = time(0); state = GameState::PLAYING; } 
            else state = GameState::MAIN_MENU; 
        } 
        else if (state == GameState::ESCAPED) {
            system("cls"); 
            drawUI(currentStage, player, shadow, shadowActive, hasKey, generatorActive, playerHP, maxHP, batteryLevel, sanityLevel, staminaLevel, isSprinting, isHiding, soundDistraction, gameLogs, inventory, flashlightActive, finalStageTime, isNightmare, fakes, false);
            
            if (currentStage < MAX_STAGE) {
                string cText = "[+] STAGE " + to_string(currentStage) + " CLEARED IN " + to_string(stageMoves) + " MOVES & " + to_string(finalStageTime) + " SECONDS!";
                int cPad = (gameWidth - cText.length()) / 2;
                cout << "\n" << string(cPad > 0 ? cPad : 0, ' ') << "\033[32m" << cText << "\033[0m\n";
                
                string mPad((gameWidth - 28) / 2, ' ');
                cout << mPad << "1. Proceed to Stage " << currentStage + 1 << "\n";
                cout << mPad << "2. Replay Stage " << currentStage << "\n";
                if (currentStage > 1) cout << mPad << "3. Return to Stage " << currentStage - 1 << "\n";
                cout << mPad << "Q. Main Menu (Quit)\n";
                drawDivider('-');
                cout << mPad << "Choice: ";
                
                char choice; do { choice = _getch(); } while (choice != '1' && choice != '2' && choice != '3' && choice != 'q' && choice != 'Q'); sfxMenuSelect();
                if (choice == '1') { currentStage++; state = GameState::PLAYING; } 
                else if (choice == '2') { state = GameState::PLAYING; } 
                else if (choice == '3' && currentStage > 1) { currentStage--; state = GameState::PLAYING; } 
                else state = GameState::MAIN_MENU; 
            } else {
                playOutroCinematic();
                printVictoryScreen();
                
                string vText = "[!!!] YOU ESCAPED ALL 10 STAGES! YOU WIN!";
                int vPad = (gameWidth - vText.length()) / 2;
                cout << "\n" << string(vPad > 0 ? vPad : 0, ' ') << "\033[32m" << vText << "\033[0m\n";
                
                string rText = "=== FINAL REPORT CARD ===";
                cout << string((gameWidth - rText.length()) / 2, ' ') << rText << "\n";
                
                bubbleSortScores(stageScores, stageTimes, MAX_STAGE + 1);
                
                string mPad((gameWidth - 21) / 2, ' ');
                cout << "\n" << mPad << "1. Replay Final Stage\n";
                cout << mPad << "Q. Main Menu (Quit)\n";
                drawDivider('-');
                cout << mPad << "Choice: ";
                
                char choice; do { choice = _getch(); } while (choice != '1' && choice != 'q' && choice != 'Q'); sfxMenuSelect();
                if (choice == '1') { state = GameState::PLAYING; } else state = GameState::MAIN_MENU; 
            }
        }
        
        // Reset player and shadow states when starting a new stage or returning to main menu, ensuring a consistent starting point for each attempt. This also prevents any unintended carryover of positions or states that could affect gameplay.
        if (state == GameState::PLAYING) {
            player = {0, 0}; shadow = {-1, 0}; soundDistraction = {-1,-1}; shadowActive = false; 
            hasKey = false; generatorActive = false; stageMoves = 0; isHiding = false; isSprinting = false;    
            for(int i=0; i<3; i++) fakes[i].active = false;
            bossEscapePhase = false; shadowStunTimer = 0; shadowFearTimer = 0;
            for(int f=0; f<7; f++) for(int x=0; x<60; x++) scentMap[currentStage][f][x] = 0;
            generateTrapsForStage(currentStage); 
            gameLogs.clear(); gameLogs.push_back("Entering Stage " + to_string(currentStage) + ". Trust no shadows.");
            stageStartTime = time(0); 
        }
    }

    delete[] stageScores; 
    delete[] stageTimes; 
    shutdownSequence(); 
    return 0;
}

// ==========================================
// CINEMATICS & ASCII ART HELPER
// ==========================================

// Helper to print centered text based on current gameWidth
void printCenteredLine(const string& line, int rawLength) {
    int pad = (gameWidth - rawLength) / 2;
    cout << string(pad > 0 ? pad : 0, ' ') << line << "\n";
}

// Helper to draw a horizontal divider line
void skippableSleep(int ms) {
    if (skipAnim) return; int intervals = ms / 10;
    for (int i = 0; i < intervals; i++) {
        if (_kbhit()) { _getch(); skipAnim = true; return; }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Types out text with a delay between characters, allowing for skipping
void typeText(const string& text, int delay) {
    if (skipAnim || textSpeed == 0) { cout << text << endl; return; }
    int actualDelay = (textSpeed == 1) ? delay / 2 : delay;
    
    for (char c : text) {
        if (_kbhit()) { _getch(); skipAnim = true; }
        if (skipAnim) cout << c; else { cout << c << flush; std::this_thread::sleep_for(std::chrono::milliseconds(actualDelay)); }
    }
    cout << endl;
}

// Intro cinematic sequence with skippable text and timed delays
void playIntroCinematic() {
    skipAnim = false; system("cls"); cout << "\033[36mPress [ANY KEY] to skip...\n\n\033[0m";
    typeText("They told me the Spire was abandoned.", 40); skippableSleep(500);
    typeText("They said the generator on the bottom floor just needed a restart.", 40); skippableSleep(500);
    typeText("But when the heavy steel doors locked behind me...", 50); skippableSleep(800);
    cout << "\033[31m"; typeText("I realized I wasn't alone.", 60); cout << "\033[0m"; skippableSleep(1000);
    typeText("Something is breathing in the dark. I need to find the keys. I need to get out.", 40); skippableSleep(1000);
}

// Outro cinematic sequence with skippable text and timed delays
void playOutroCinematic() {
    skipAnim = false; system("cls"); cout << "\033[36mPress [ANY KEY] to skip...\n\n\033[0m";
    typeText("The final door screeched open. Real sunlight hit my face.", 40); skippableSleep(500);
    typeText("I looked back into the abyss of the Spire.", 40); skippableSleep(800);
    typeText("The Shadow stood at the edge of the light. Watching.", 60); skippableSleep(1000);
    cout << "\033[32m"; typeText("I survived. But I will never forget the dark.", 50); cout << "\033[0m"; skippableSleep(1500);
}

// Prints the title screen with ASCII art and stylized text
void printTitleScreen() {
    system("cls"); cout << "\033[36m";
    drawDivider('=');
    printCenteredLine("  ____       _______ _    _   _______ ____  ", 48);
    printCenteredLine(" |  _ \\   /\\|__   __| |  | | |__   __/ __ \\ ", 48);
    printCenteredLine(" | |_) | /  \\  | |  | |__| |    | | | |  | |", 48);
    printCenteredLine(" |  __/ / /\\ \\ | |  |  __  |    | | | |  | |", 48);
    printCenteredLine(" | |   / ____ \\| |  | |  | |    | | | |__| |", 48);
    printCenteredLine(" |_|  /_/    \\_\\_|  |_|  |_|    |_|  \\____/ ", 48);
    printCenteredLine("                                            ", 48);
    printCenteredLine("  _____ _____  _____ _____  ______  _____  _____ ______ _   _ _______ ", 74);
    printCenteredLine(" |_   _|  __ \\|_   _|  __ \\|  ____|/ ____|/ ____|  ____| \\ | |__   __|", 74);
    printCenteredLine("   | | | |__) | | | | |  | | |__  | (___ | |    | |__  |  \\| |  | |   ", 74);
    printCenteredLine("   | | |  _  /  | | | |  | |  __|  \\___ \\| |    |  __| | . ` |  | |   ", 74);
    printCenteredLine("  _| |_| | \\ \\ _| |_| |__| | |____ ____) | |____| |____| |\\  |  | |   ", 74);
    printCenteredLine(" |_____|_|  \\_\\_____|_____/|______|_____/ \\_____|______|_| \\_|  |_|   ", 74);
    drawDivider('=');
    printCenteredLine("D E M O", 7);
    drawDivider('=');
    cout << "\033[0m"; 
}

// Prints the game over screen with ASCII art and stylized text
void printGameOverScreen() {
    cout << "\033[31m";
    drawDivider('=');
    printCenteredLine("  _____          __  __ ______    ______      ________ ____  ", 61);
    printCenteredLine(" / ____|   /\\   |  \\/  |  ____|  / __ \\ \\    / /  ____|  _ \\ ", 61);
    printCenteredLine("| |  __   /  \\  | \\  / | |__    | |  | \\ \\  / /| |__  | |_) |", 61);
    printCenteredLine("| | |_ | / /\\ \\ | |\\/| |  __|   | |  | |\\ \\/ / |  __| |  _ < ", 61);
    printCenteredLine("| |__| |/ ____ \\| |  | | |____  | |__| | \\  /  | |____| | \\ \\", 61);
    printCenteredLine(" \\_____/_/    \\_\\_|  |_|______|  \\____/   \\/   |______|_|  \\_\\", 62);
    drawDivider('=');
    cout << "\033[0m";
}

// Prints the victory screen with ASCII art and stylized text
void printVictoryScreen() {
    cout << "\033[32m";
    drawDivider('=');
    printCenteredLine(" __     __ ____  _    _  __          _______ _   _ _ ", 53);
    printCenteredLine(" \\ \\   / // __ \\| |  | | \\ \\        / /_   _| \\ | | |", 53);
    printCenteredLine("  \\ \\_/ /| |  | | |  | |  \\ \\  /\\  / /  | | |  \\| | |", 53);
    printCenteredLine("   \\   / | |  | | |  | |   \\ \\/  \\/ /   | | | . ` | |", 53);
    printCenteredLine("    | |  | |__| | |__| |    \\  /\\  /   _| |_| |\\  |_|", 53);
    printCenteredLine("    |_|   \\____/ \\____/      \\/  \\/   |_____|_| \\_(_)", 53);
    drawDivider('=');
    cout << "\033[0m";
}

// Draws simple ASCII art for the bestiary entries based on type
void drawBestiaryArt(int type) {
    if(type == 1) {
        cout << "\033[31m";
        printCenteredLine("  .---.  ", 9);
        printCenteredLine(" /     \\ ", 9);
        printCenteredLine("| () () |", 9);
        printCenteredLine(" \\  ^  / ", 9);
        printCenteredLine("  |||||  ", 9);
        printCenteredLine("  |||||  ", 9);
        cout << "\033[0m";
    } else if(type == 2) {
        cout << "\033[36m";
        printCenteredLine(" _________", 10);
        printCenteredLine("/        /|", 11);
        printCenteredLine("/________/ |", 12);
        printCenteredLine("|        | |", 12);
        printCenteredLine("|   []   | |", 12);
        printCenteredLine("|        | /", 12);
        printCenteredLine("|________|/", 11);
        cout << "\033[0m";
    }
}

// ==========================================
// MINIGAMES & LOGIC HELPERS
// ==========================================

// Quick Time Event for holding breath while hiding in lockers
bool hackGeneratorQTE() {
    system("cls");
    drawCenteredTitle("GENERATOR TERMINAL OVERRIDE");
    string pad((gameWidth - 55) / 2, ' ');
    cout << "\n" << pad << "Enter the bypass sequence before the system locks out!\n\n";
    
    string seq = "";
    char keys[] = {'w', 'a', 's', 'd'};
    for(int i=0; i<5; i++) seq += keys[rand()%4];
    
    cout << pad << "SEQUENCE: \033[36m" << seq << "\033[0m\n\n";
    cout << pad << "INPUT: ";
    
    auto start = chrono::steady_clock::now();
    string input = "";
    
    while(input.length() < 5) {
        if (chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start).count() >= 5) {
            return false; 
        }
        if (_kbhit()) {
            char c = _getch();
            if (c == seq[input.length()]) {
                input += c;
                cout << c;
                if(sfxEnabled) Beep(1000, 50);
            } else {
                return false; 
            }
        }
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    return true;
}

// ==========================================
// OS BOOT SEQUENCE
// ==========================================

// Simulates a BIOS boot sequence with skippable text and timed delays, including fake hardware checks and warnings
void bootSequence() {
    skipAnim = false; system("cls"); cout << "\033[32mPress [ANY KEY] to skip startup sequence...\n\n";
    typeText("Phoenix - AwardBIOS v6.00PG, An Energy Star Ally", 20); typeText("Copyright (C) 1984-2007, Phoenix Technologies, LTD", 20); cout << "\n";
    typeText("ASUS A8N-SLI ACPI BIOS Revision 1009", 20); skippableSleep(800); cout << "\n";
    typeText("Main Processor : AMD Athlon(tm) 64 Processor 3200+", 20); cout << "Memory Testing : "; skippableSleep(1500); typeText("1048576K OK", 10); cout << "\n";
    typeText("Detecting IDE drives...", 30); skippableSleep(1200); 
    typeText("Primary Master   : ST3160827AS 3.42", 15); typeText("Primary Slave    : None", 15); typeText("Secondary Master : HL-DT-ST DVD-RW GWA-4164B", 15); typeText("Secondary Slave  : None", 15); cout << "\n"; skippableSleep(1000);
    typeText("Loading SpireOS Kernel v2.4.1...", 30); skippableSleep(800);
    typeText("Mounting VFS root filesystem... OK", 15); typeText("Starting Background Services:", 15); skippableSleep(400);
    typeText("  [ OK ] ACPI Daemon", 15); skippableSleep(300); typeText("  [ OK ] NetManager", 15); skippableSleep(300); typeText("  [ OK ] Containment Protocol", 15); skippableSleep(1000);
    typeText("Loading SpireOS Sector 1-10...", 40); skippableSleep(1000); typeText("Initializing Mainframe...", 30); skippableSleep(1200);
    cout << "\033[31m"; typeText("WARNING: BIOS Error 0x88. Containment failed.", 40); sfxError(); skippableSleep(600);
    typeText("CRITICAL: Sector 7 breach detected.", 30); skippableSleep(500); typeText("CRITICAL: Unknown entity signatures detected.", 30); sfxShadowSpotted(); skippableSleep(800); cout << "\033[0m\n";
    if (!skipAnim) { typeText("Press any key to boot recovery mode...", 20); while(!_kbhit()) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); } _getch(); }
}

// Simulates a shutdown sequence with skippable text and timed delays, including fake saving processes and warnings about unidentified entities
void shutdownSequence() {
    skipAnim = false; system("cls"); cout << "\033[32mPress [ANY KEY] to skip shutdown sequence...\n\n";
    typeText("Initiating SpireOS shutdown sequence...", 20); skippableSleep(400);
    cout << "Saving local data..."; skippableSleep(600); typeText(" OK", 5); skippableSleep(200);
    cout << "Flushing NVRAM..."; skippableSleep(500); typeText(" OK", 5); skippableSleep(300);
    typeText("Terminating active processes...", 15); skippableSleep(700);
    typeText("Stopping ACPI Daemon... OK", 10); typeText("Stopping Containment Protocol... FAILED", 15); sfxError();
    typeText("Unmounting drive C: ... OK", 20); skippableSleep(400);
    cout << "\033[31m"; typeText("WARNING: Unidentified entities remain in Sectors 1-10.", 30); sfxShadowSpotted(); skippableSleep(500);
    cout << "\033[32m"; typeText("System halted.", 40); skippableSleep(1000); cout << "\nIt is now safe to turn off your computer.\033[0m\n";
    if (!skipAnim) skippableSleep(1500);
}

// ==========================================
// MAP & ENTITY GENERATION
// ==========================================

// Checks if a given position in the current stage is a wall or a safe zone (like a pillar)
bool isWall(int stage, int f, int x) { return (terrain3D[stage][f][x] == 'X'); }
bool isSafeZone(int stage, int f, int x) { for(int p=0; p<3; p++) if(pillars[stage][f][p].active && pillars[stage][f][p].pos.x == x) return true; return false; }

// Generates traps, items, lockers, and pillars for a given stage, ensuring they are placed in valid positions and do not overlap with key locations or each other
void generateTrapsForStage(int stage) {
    int rightmostLadder = 2;
    if (maxFloors[stage] > 0) { for (int x = 0; x < stageLengths[stage]; x++) if (terrain3D[stage][maxFloors[stage] - 1][x] == '#') rightmostLadder = x; }
    generatorPos[stage] = { 2 + (rand() % (stageLengths[stage] - 3)), maxFloors[stage] };

    if (maxFloors[stage] >= 1) {
        warpGates[stage][0] = { 2 + (rand() % (stageLengths[stage] - 4)), 0 }; 
        int range = rightmostLadder - 2; int topWarpX = (range > 0) ? (2 + (rand() % range)) : 1; warpGates[stage][1] = { topWarpX, maxFloors[stage] };
    } else { warpGates[stage][0] = {-1, -1}; warpGates[stage][1] = {-1, -1}; }

    if (stage > 1) {
        for (int h = 0; h < 3; h++) {
            int hFloor, hX; bool validPos = false; int attempts = 0; 
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

        for (int i = 0; i < 12; i++) {
            int iFloor = (maxFloors[stage] > 0) ? rand() % (maxFloors[stage] + 1) : 0;
            int iX = 2 + (rand() % (stageLengths[stage] - 3));
            if (i == 0) stageItems[stage][i] = { {iX, iFloor}, "Health Potion", '+', true };
            else if (i == 1 || i == 2) stageItems[stage][i] = { {iX, iFloor}, "Battery", 'b', true };
            else if (i == 3 || i == 4) stageItems[stage][i] = { {iX, iFloor}, "Stone", '*', true };
            else if (i == 5 || i == 6) stageItems[stage][i] = { {iX, iFloor}, "Lore Fragment", '?', true };
            else if (i == 7 || i == 8) stageItems[stage][i] = { {iX, iFloor}, "Energy Drink", 'E', true };
            else if (i == 9 || i == 10) stageItems[stage][i] = { {iX, iFloor}, "Bear Trap", '^', true };
            else if (i == 11) stageItems[stage][i] = { {iX, iFloor}, "Signal Flare", 'F', true }; // NEW ITEM
            else stageItems[stage][i] = { {-1, -1}, "Empty", '.', false }; 
        }
        
        for (int l = 0; l < 4; l++) lockers[stage][l] = {2 + (rand() % (stageLengths[stage] - 3)), (maxFloors[stage] > 0) ? rand() % (maxFloors[stage] + 1) : 0};
        for (int f = 0; f <= maxFloors[stage]; f++) {
            for (int p = 0; p < 3; p++) { pillars[stage][f][p] = { {2 + (rand() % (stageLengths[stage] - 3)), f}, 5, true }; }
        }
    } else {
        for (int h = 0; h < 3; h++) holes[stage][h] = {-1, -1};
        for (int i = 0; i < 12; i++) stageItems[stage][i] = { {-1, -1}, "Empty", '.', false };
        for (int l = 0; l < 4; l++) lockers[stage][l] = {-1, -1};
        for (int f = 0; f <= maxFloors[stage]; f++) for (int p = 0; p < 3; p++) pillars[stage][f][p] = { {-1, -1}, 0, false };
    }
}

// Generates the map layout for all stages, including walls, stairs, water hazards, key positions, and traps, ensuring a variety of challenges and valid placements for each stage
void generateMap() {
    for(int s = 0; s <= MAX_STAGE; s++) {
        for(int f = 0; f < 7; f++) { stairPos[s][f] = -1; for(int x = 0; x < 60; x++) { terrain3D[s][f][x] = '.'; scentMap[s][f][x] = 0; } }
    }
    for (int stage = 1; stage <= MAX_STAGE; stage++) {
        if (stage > 1) {
            for (int floor = 0; floor < maxFloors[stage]; floor++) {
                int numStairs = (floor >= 2) ? 2 : 1; 
                if (numStairs == 1) terrain3D[stage][floor][2 + (rand() % (stageLengths[stage] - 4))] = '#';
                else {
                    int midPoint = stageLengths[stage] / 2;
                    terrain3D[stage][floor][2 + (rand() % (midPoint - 3))] = '#';
                    terrain3D[stage][floor][midPoint + 1 + (rand() % (midPoint - 3))] = '#';
                }
            }
        }
        if (stage > 2) {
            for(int c = 0; c < 4; c++) {
                int cX = 2 + (rand() % (stageLengths[stage] - 3));
                if (terrain3D[stage][1][cX] == '.') terrain3D[stage][1][cX] = '~';
            }
        }
        keyPositions[stage] = { 2 + (rand() % (stageLengths[stage] - 3)), (maxFloors[stage] > 0) ? rand() % 2 : 0 };
        generateTrapsForStage(stage);
    }
}

// ==========================================
// PHYSICS & ENGINE LOGIC
// ==========================================

// Applies a movement delta to the player's position, updating their coordinates based on input
void applyMove(Position& pos, int dx, int dy) { pos.x += dx; pos.y += dy; }
bool canGoUp(const Position& pos, int stage) { if (pos.y >= maxFloors[stage]) return false; return (terrain3D[stage][pos.y][pos.x] == '#'); }
bool canGoDown(const Position& pos, int stage) { if (pos.y <= 0) return false; return (terrain3D[stage][pos.y - 1][pos.x] == '#'); }

// Updates the scent map for the current stage, leaving a trail that decays over time, which can be used by the shadow to track the player's movements
void decayScentMap(int stage) {
    for(int f=0; f<=maxFloors[stage]; f++) for(int x=0; x<=stageLengths[stage]; x++) if (scentMap[stage][f][x] > 0) scentMap[stage][f][x]--; 
}

// Draws the user interface, including health, battery, sanity, stamina bars, compass hints, inventory, and a mini-map that reflects the player's current vision based on hiding or flashlight status
void drawUI(int stage, const Position& player, const Position& shadow, bool shadowActive, bool hasKey, bool genActive, int hp, int maxHP, int battery, int sanity, int stamina, bool isSprinting, bool isHiding, const Position& soundDistraction, const vector<string>& logs, const vector<string>& inv, bool flashlightActive, int elapsedSecs, bool isNightmare, Hallucination* fakes, bool showPrompt) {
    resetCursor(); 
    string out; out.reserve(8192);
    
    string batBar = ""; for(int i=0; i<10; i++) batBar += (battery >= i*10) ? "|" : " ";
    string sanBar = ""; for(int i=0; i<10; i++) sanBar += (sanity >= i*10) ? "|" : " ";
    string stamBar = ""; for(int i=0; i<10; i++) stamBar += (stamina >= i*10) ? "|" : " ";
    string batColor = (battery > 20) ? "\033[32m" : "\033[31m";
    string sanColor = (sanity > 40) ? "\033[32m" : "\033[31;5m";
    string stamColor = (stamina > 20) ? "\033[36m" : "\033[31m";

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

    string divEq = string(gameWidth, '=') + "\n";
    string divDash = string(gameWidth, '-') + "\n";

    out += divEq;
    out += " HP: \033[31m"; 
    for(int i=0; i<maxHP; i++) out += (i < hp ? "♥ " : "♡ ");
    out += "\033[0m | BATTERY: " + batColor + "[" + batBar + "] " + to_string(battery) + "%\033[0m\n";
    out += " SANITY: " + sanColor + "[" + sanBar + "] " + to_string(sanity) + "%\033[0m | STAMINA: " + stamColor + "[" + stamBar + "] " + to_string(stamina) + "%\033[0m\n";
    out += divDash;
    out += " STAGE: " + to_string(stage) + " / " + to_string(MAX_STAGE) + " | TIME: " + to_string(elapsedSecs) + "s | COMPASS: " + compass + "\n";
    
    if(isNightmare) out += " MODE: \033[31;5m[ NIGHTMARE ]\033[0m | SPRINT: " + string(isSprinting ? "\033[33mON\033[0m" : "OFF") + "\n";
    else if(maxHP == 5) out += " MODE: \033[32m[ BEGINNER ]\033[0m | SPRINT: " + string(isSprinting ? "\033[33mON\033[0m" : "OFF") + "\n"; 
    else out += " MODE: NORMAL | SPRINT: " + string(isSprinting ? "\033[33mON\033[0m" : "OFF") + "\n";
    
    out += " BACKPACK: ";
    if (inv.empty()) out += "(Empty)";
    else for (size_t i = 0; i < inv.size(); i++) out += "[" + to_string(i + 1) + ": " + inv[i] + "] ";
    out += "\n" + divDash;
    
    if (isHiding) out += " MAP: [\033[36mHIDING IN CABINET - VISION RESTRICTED\033[0m]\n";
    else if (!flashlightActive) out += " MAP: [\033[31mPITCH BLACK\033[0m]\n";
    else out += " MAP: [FLASHLIGHT ON]\n";
    
    for (int y = maxFloors[stage]; y >= 0; y--) {
        out += " F" + to_string(y) + " | ";
        for (int x = 0; x <= stageLengths[stage]; x++) {
            
            int distX = abs(player.x - x); int distY = abs(player.y - y);
            bool inVision = true;
            if (isHiding) inVision = (distX <= 1 && distY == 0);
            else if (!flashlightActive) inVision = (distX <= 1 && distY == 0);
            else inVision = (distX <= 5 && distY <= 1); 
            
            if (!inVision) { out += (wideMap ? "  " : " "); } 
            else {
                bool isHole = false; for(int h=0; h<3; h++) if (holes[stage][h].x == x && holes[stage][h].y == y) isHole = true;
                bool isWarp = (warpGates[stage][0].x == x && warpGates[stage][0].y == y) || (warpGates[stage][1].x == x && warpGates[stage][1].y == y);
                bool isLocker = false; for(int l=0; l<4; l++) if (lockers[stage][l].x == x && lockers[stage][l].y == y) isLocker = true;
                bool isFake = false; for(int f=0; f<3; f++) if (fakes[f].active && fakes[f].pos.x == x && fakes[f].pos.y == y) isFake = true;
                bool isPillar = false; for(int p=0; p<3; p++) if (pillars[stage][y][p].active && pillars[stage][y][p].pos.x == x) isPillar = true;
                
                int itemIdx = -1;
                for(int i=0; i<12; i++) if (stageItems[stage][i].active && stageItems[stage][i].pos.x == x && stageItems[stage][i].pos.y == y) itemIdx = i;
                
                string tile = "";
                if (player.x == x && player.y == y) tile = (isHiding ? "\033[36mP\033[0m" : "\033[32mP\033[0m"); 
                else if (shadowActive && shadow.x == x && shadow.y == y) tile = "\033[31mS\033[0m"; 
                else if (isFake) tile = "\033[31;5mS\033[0m"; 
                else if (soundDistraction.x == x && soundDistraction.y == y) tile = "\033[33;5m!\033[0m"; 
                else if (!genActive && x == generatorPos[stage].x && y == generatorPos[stage].y) tile = "\033[33;5mG\033[0m"; 
                else if (genActive && !hasKey && x == keyPositions[stage].x && y == keyPositions[stage].y) tile = "\033[33mK\033[0m"; 
                else if (stage == MAX_STAGE && hasKey && x == 0 && y == 0) tile = "\033[35mE\033[0m"; 
                else if (x == stageLengths[stage] && y == maxFloors[stage]) tile = ((stage==MAX_STAGE && hasKey) ? "\033[31m#\033[0m" : (hasKey ? "\033[35mE\033[0m" : "L")); 
                else if (isPillar) tile = "\033[33;1mI\033[0m"; 
                else if (isLocker) tile = "\033[36mC\033[0m";
                else if (isWarp) tile = "\033[35mW\033[0m"; 
                else if (isHole) tile = "\033[31mO\033[0m"; 
                else if (itemIdx != -1) {
                    char s = stageItems[stage][itemIdx].symbol;
                    if (s == '+') tile = "\033[32m+\033[0m"; else if (s == 'b') tile = "\033[36mb\033[0m"; 
                    else if (s == '*') tile = "\033[37m*\033[0m"; else if (s == '?') tile = "\033[34m?\033[0m";
                    else if (s == 'E') tile = "\033[35mE\033[0m"; else if (s == '^') tile = "\033[37m^\033[0m";
                    else if (s == 'F') tile = "\033[31mF\033[0m"; // Flare
                }
                else if (terrain3D[stage][y][x] == 'X') tile = "\033[37mX\033[0m"; 
                else if (terrain3D[stage][y][x] == '#') tile = "\033[36m#\033[0m"; 
                else if (y > 0 && terrain3D[stage][y-1][x] == '#') tile = "\033[36m#\033[0m"; 
                else if (terrain3D[stage][y][x] == '~') tile = "\033[33m~\033[0m"; 
                else if (terrain3D[stage][y][x] == '^') tile = "\033[37m^\033[0m"; 
                else if (terrain3D[stage][y][x] == 'O') tile = "\033[31mO\033[0m"; 
                else tile = "."; 
                
                out += tile + (wideMap ? " " : ""); 
            }
        }
        out += "\n";
    }
    out += divDash;
    
    // Dynamic Log Padding
    for (size_t i = 0; i < logs.size(); i++) {
        string line = " > " + logs[i]; 
        while(line.length() < gameWidth - 1) line += " "; 
        out += line + "\n";
    }
    for (size_t i = logs.size(); i < 5; i++) {
        out += string(gameWidth - 1, ' ') + "\n";
    }
    
    out += divEq;
    if (showPrompt) out += "Action: "; 
    
    // Bottom Pad: Wipe out residual text
    out += string(gameWidth, ' ') + "\n";
    out += string(gameWidth, ' ') + "\n";
    out += string(gameWidth, ' ') + "\n";
    
    cout << out;
}

// Finds the closest stair position on the current floor to the shadow's current X coordinate, which is used for pathfinding when the shadow is trying to move between floors
int getClosestStair(int stage, int floor, int currentX, int stageLength) {
    int targetX = currentX; int minDist = 999;
    for (int x = 0; x <= stageLength; x++) { if (terrain3D[stage][floor][x] == '#') { if (abs(currentX - x) < minDist) { minDist = abs(currentX - x); targetX = x; } } } return targetX;
}

// Processes the Shadow's AI behavior, including movement towards the player, reacting to sound distractions, and handling traps. The Shadow will prioritize chasing the player if it has line of sight, otherwise it will follow the scent map or move randomly. It also reacts to the new Flare item by running away from the player for a short duration.
void processShadowAI(const Position& player, Position& shadow, int stage, bool isHiding, Position& soundDistraction, bool hasSight, int& shadowStunTimer, int& shadowFearTimer) {
    auto canMoveTo = [&](int f, int x) { return !isWall(stage, f, x) && !isSafeZone(stage, f, x); };
    
    if (terrain3D[stage][shadow.y][shadow.x] == '^') {
        terrain3D[stage][shadow.y][shadow.x] = '.'; 
        shadowStunTimer = 3; sfxTrapSnap(); return; 
    }
    
    // NEW: Flare Fear Mechanic - Shadow runs AWAY
    if (shadowFearTimer > 0) {
        if (shadow.x < player.x && canMoveTo(shadow.y, shadow.x-1)) shadow.x--;
        else if (shadow.x > player.x && canMoveTo(shadow.y, shadow.x+1)) shadow.x++;
        return;
    }

    // Sound Distraction Logic in case if you gonna fuck yourself up lmfao
    if (soundDistraction.x != -1) {
        if (shadow.y < soundDistraction.y) {
            int tx = getClosestStair(stage, shadow.y, shadow.x, stageLengths[stage]);
            if (shadow.x < tx && canMoveTo(shadow.y, shadow.x+1)) shadow.x++;
            else if (shadow.x > tx && canMoveTo(shadow.y, shadow.x-1)) shadow.x--;
            else shadow.y++; 
        } 
        else if (shadow.y > soundDistraction.y) {
            int tx = getClosestStair(stage, shadow.y - 1, shadow.x, stageLengths[stage]);
            if (shadow.x < tx && canMoveTo(shadow.y, shadow.x+1)) shadow.x++;
            else if (shadow.x > tx && canMoveTo(shadow.y, shadow.x-1)) shadow.x--;
            else shadow.y--; 
        } 
        else {
            if (shadow.x < soundDistraction.x && canMoveTo(shadow.y, shadow.x+1)) shadow.x++;
            else if (shadow.x > soundDistraction.x && canMoveTo(shadow.y, shadow.x-1)) shadow.x--;
            else { soundDistraction.x = -1; soundDistraction.y = -1; }
        }
        return;
    }
    
    // If Shadow can see the player and player isn't hiding, move towards them directly to fuck your ass up even more
    if (hasSight && !isHiding) {
        if (shadow.x < player.x && canMoveTo(shadow.y, shadow.x+1)) shadow.x++;
        else if (shadow.x > player.x && canMoveTo(shadow.y, shadow.x-1)) shadow.x--;
        return;
    }
    
    int bestScent = 0; int targetX = shadow.x; int targetY = shadow.y;
    if (shadow.x < stageLengths[stage] && canMoveTo(shadow.y, shadow.x+1)) { if (scentMap[stage][shadow.y][shadow.x+1] > bestScent) { bestScent = scentMap[stage][shadow.y][shadow.x+1]; targetX = shadow.x+1; } }
    if (shadow.x > 0 && canMoveTo(shadow.y, shadow.x-1)) { if (scentMap[stage][shadow.y][shadow.x-1] > bestScent) { bestScent = scentMap[stage][shadow.y][shadow.x-1]; targetX = shadow.x-1; targetY = shadow.y; } }
    if (canGoUp(shadow, stage)) { if (scentMap[stage][shadow.y+1][shadow.x] > bestScent) { bestScent = scentMap[stage][shadow.y+1][shadow.x]; targetX = shadow.x; targetY = shadow.y+1; } }
    if (canGoDown(shadow, stage)) { if (scentMap[stage][shadow.y-1][shadow.x] > bestScent) { bestScent = scentMap[stage][shadow.y-1][shadow.x]; targetX = shadow.x; targetY = shadow.y-1; } }
    
    if (bestScent > 0) { shadow.x = targetX; shadow.y = targetY; } else {
        int r = rand() % 3;
        if (r == 0 && shadow.x < stageLengths[stage] && canMoveTo(shadow.y, shadow.x+1)) shadow.x++;
        else if (r == 1 && shadow.x > 0 && canMoveTo(shadow.y, shadow.x-1)) shadow.x--;
    }
}

// NEW: Hallucination Logic - Randomly spawns fake shadows that move around and can be mistaken for the real one, but they don't interact with the player at all
void processHallucinations(Hallucination* fakes, int stage) {
    for(int i=0; i<3; i++) {
        if(!fakes[i].active) {
            if (rand() % 4 == 0) { fakes[i].active = true; fakes[i].pos = {rand() % stageLengths[stage], (maxFloors[stage]>0)?rand()%(maxFloors[stage]+1):0}; }
        } else {
            int dir = rand() % 2;
            if (dir == 0 && fakes[i].pos.x < stageLengths[stage]) fakes[i].pos.x++;
            else if (dir == 1 && fakes[i].pos.x > 0) fakes[i].pos.x--;
            if (rand() % 5 == 0) fakes[i].active = false;
        }
    }
}

// NEW: Bear Trap Mechanic - Player can place a trap on the floor which will stun the Shadow for a few turns if it steps on it. Consumes a trap item from inventory.
void handlePlaceTrap(Position& player, vector<string>& logs, int stage, vector<string>& inv) {
    if (terrain3D[stage][player.y][player.x] == '.') {
        terrain3D[stage][player.y][player.x] = '^';
        int trapIdx = -1;
        for(size_t i=0; i<inv.size(); i++) if(inv[i] == "Bear Trap") trapIdx = i;
        if (trapIdx != -1) inv.erase(inv.begin() + trapIdx);
        logs.push_back("\033[33mBear Trap placed! The Shadow will be stunned if it steps here.\033[0m");
    } else { logs.push_back("Cannot place a trap here!"); }
}

// NEW: Sound Distraction Mechanic - Player can throw a stone to create a noise distraction at a chosen location within a certain range, which will attract the Shadow's attention. Consumes a stone item from inventory.
void handleThrowingMechanic(Position& player, Position& soundDistraction, vector<string>& logs, int stage) {
    system("cls");
    drawCenteredTitle("THROW STONE");
    string mPad((gameWidth - 40) / 2, ' ');
    cout << mPad << "Choose Direction to Throw (W/A/S/D): ";
    char dir; do { dir = _getch(); } while (dir != 'w' && dir != 'a' && dir != 's' && dir != 'd'); sfxMenuSelect();
    cout << "\n" << mPad << "Choose Distance (1-5 tiles): ";
    char distC; do { distC = _getch(); } while (distC < '1' || distC > '5'); sfxMenuSelect();
    
    int dist = distC - '0'; Position target = player;
    if (dir == 'w') { if (target.y < maxFloors[stage]) target.y += 1; } 
    else if (dir == 's') { if (target.y > 0) target.y -= 1; }
    else if (dir == 'a') { for(int i=0; i<dist; i++) if(!isWall(stage, target.y, target.x-1) && target.x>0) target.x--; } 
    else if (dir == 'd') { for(int i=0; i<dist; i++) if(!isWall(stage, target.y, target.x+1) && target.x<stageLengths[stage]) target.x++; }
    
    soundDistraction = target;
    logs.push_back("\033[33;5mCLACK! Stone landed at X:" + to_string(target.x) + " F:" + to_string(target.y) + "\033[0m");
}

// ==========================================
// MISC & UI
// ==========================================

// NEW: Performance Grading System - After completing a stage, player is graded based on their moves and time taken. Encourages replayability and optimization.
char calculateGrade(int moves, int time) {
    int score = moves + (time * 2);
    if (score < 150) return 'S';
    else if (score < 250) return 'A';
    else if (score < 400) return 'B';
    else if (score < 600) return 'C';
    else return 'D';
}

// NEW: Bubble Sort for Performance Ranking - Sorts the player's performance across all stages and displays a ranked list with grades.
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
    
    // Display Ranked Performance
    printCenteredLine("--- Performance Ranked (Best to Worst by Moves) ---", 51);
    cout << "\n";
    for(int i = 1; i < size; i++) {
        if (sortedScores[i] > 0) {
            char grade = calculateGrade(sortedScores[i], sortedTimes[i]);
            string color = (grade == 'S') ? "\033[33;5m" : (grade == 'A') ? "\033[32m" : (grade == 'D') ? "\033[31m" : "\033[0m";
            string rText = "Rank " + to_string(i) + ": Stage " + to_string(stageIds[i]) + " -> " + to_string(sortedScores[i]) + " moves | " + to_string(sortedTimes[i]) + "s | Grade: " + color + grade + "\033[0m";
            
            int rPad = (gameWidth - 55) / 2; 
            cout << string(rPad > 0 ? rPad : 0, ' ') << rText << "\n";
        }
    }
    delete[] sortedScores; 
    delete[] sortedTimes; 
    delete[] stageIds;
}

// NEW: Hold Breath QTE - During certain intense moments, player must successfully complete a quick-time event to hold their breath and avoid detection by the Shadow. Adds tension and interactivity.
bool holdBreathQTE() {
    sfxQTEAlert();
    string qPad((gameWidth - 55) / 2, ' ');
    cout << "\n" << qPad << "\033[31;5m[!] IT IS RIGHT OUTSIDE! PRESS 'SPACE' TO HOLD YOUR BREATH!\033[0m\n";
    auto start = chrono::steady_clock::now();
    while (chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() < 2000) {
        if (_kbhit()) {
            char c = _getch();
            if (c == ' ') return true;
        }
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    return false;
}

// NEW: Lore Library - A dedicated section in the main menu where players can read the lore entries they've unlocked by finding fragments in the maze. Provides backstory and world-building.
void drawLoreLibrary() {
    system("cls");
    drawCenteredTitle("LORE LIBRARY");
    int unlockedCount = 0;
    string pad((gameWidth - 60) / 2, ' ');
    for(int i=0; i<30; i++) {
        if (loreUnlocked[i]) { cout << pad << "\033[34m" << loreTitles[i] << "\033[0m\n" << pad << "  \"" << loreText[i] << "\"\n\n"; unlockedCount++; } 
        else { cout << pad << "??? - [Find Fragments in the maze to unlock]\n\n"; }
    }
    if (unlockedCount == 30) {
        drawDivider('=');
        drawCenteredTitle("\033[35m[CLASSIFIED FILE UNLOCKED]: THE TRUE PURPOSE\033[0m");
        cout << pad << "  \"The Spire was never a research facility. It is a containment\n";
        cout << pad << "   vessel for humanity's collective fears. The 'Shadow' is not\n";
        cout << pad << "   an invader. It is the Spire's antibody. And you... you are\n";
        cout << pad << "   the infection it is trying to eradicate. Wake up.\"\n";
    }
    drawDivider('=');
    cout << pad << "Press any key to return...\n";
    _getch(); sfxMenuSelect();
}

// NEW: Archive Section - A bestiary-like section in the main menu that provides detailed information about the Shadow, the lockers, and the pillars. Helps players understand the mechanics and lore better.
void drawArchive() {
    system("cls");
    drawCenteredTitle("T H E   A R C H I V E");
    string pad((gameWidth - 60) / 2, ' ');
    drawBestiaryArt(1);
    cout << pad << "\033[31m ENTRY 01: THE SHADOW [S]\033[0m\n";
    cout << pad << " > Description: A manifestation of the abyss.\n";
    cout << pad << " > Behavior: Relentlessly hunts by sound and scent. \n";
    cout << pad << "   If it loses sight of you, it will sniff the floor\n";
    cout << pad << "   to trace your footsteps. It becomes ENRAGED by light.\n";
    cout << pad << " > Threat Level: LETHAL\n\n";
    drawBestiaryArt(2);
    cout << pad << "\033[36m ENTRY 02: THE FORGOTTEN [C]\033[0m\n";
    cout << pad << " > Description: The rusted containment lockers.\n";
    cout << pad << " > Behavior: Entering one masks your presence entirely,\n";
    cout << pad << "   causing the Shadow to lose your scent. However,\n";
    cout << pad << "   claustrophobia drains sanity rapidly.\n";
    cout << pad << " > Threat Level: SAFE (Temporary)\n\n";
    cout << pad << "\033[33;1m ENTRY 03: THE PILLARS [I]\033[0m\n";
    cout << pad << " > Description: Strange, glowing obelisks found on each floor.\n";
    cout << pad << " > Behavior: The Shadow fears this light. Standing within it\n";
    cout << pad << "   will rapidly restore your sanity and physically block the\n";
    cout << pad << "   entity. It burns out quickly once used.\n";
    cout << pad << " > Threat Level: SANCTUARY\n\n";
    drawDivider('=');
    cout << pad << "Press any key to return to the Main Menu...\n";
    _getch(); sfxMenuSelect();
}
