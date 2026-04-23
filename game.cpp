#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

struct Player {
    int hp = 30;
    int maxHp = 30;
    int attack = 6;
    int potions = 1;
    int gold = 0;
    vector<string> inventory{"rusty_sword"};
};

struct Enemy {
    string name;
    int hp;
    int attack;
    int goldReward;
};

struct Room {
    string name;
    string description;
    map<string, int> exits;
    bool hasPotion = false;
    bool hasTreasure = false;
    bool hasEnemy = false;
    Enemy enemy{"", 0, 0, 0};
};

static mt19937 rng(random_device{}());

string toLowerCopy(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(tolower(c));
    });
    return s;
}

vector<string> splitWords(const string& line) {
    vector<string> words;
    istringstream iss(line);
    string word;
    while (iss >> word) {
        words.push_back(toLowerCopy(word));
    }
    return words;
}

Enemy randomEnemy() {
    uniform_int_distribution<int> dist(0, 2);
    int choice = dist(rng);
    if (choice == 0) return {"slime", 10, 3, 4};
    if (choice == 1) return {"goblin", 14, 4, 7};
    return {"skeleton", 18, 5, 10};
}

class Game {
public:
    Game() {
        buildWorld();
    }

    void run() {
        printIntro();
        string line;
        while (true) {
            if (player.hp <= 0) {
                cout << "\nYou have fallen. Game over.\n";
                break;
            }

            cout << "\n> ";
            if (!getline(cin, line)) {
                cout << "\nGoodbye.\n";
                break;
            }

            if (!handleCommand(line)) {
                break;
            }
        }
    }

private:
    vector<Room> rooms;
    Player player;
    int currentRoom = 0;
    bool inCombat = false;

    void printIntro() {
        cout << "=== FuzzQuest ===\n";
        cout << "A tiny terminal adventure game for ClusterFuzzLite experiments.\n";
        cout << "Type 'help' for commands.\n\n";
        describeRoom();
    }

    void buildWorld() {
        rooms.clear();

        Room village;
        village.name = "Village";
        village.description = "A quiet village with a fountain and a worn path leading outward.";
        village.exits["north"] = 1;
        village.hasPotion = true;

        Room forest;
        forest.name = "Forest";
        forest.description = "Tall trees surround you. The air is damp and uneasy.";
        forest.exits["south"] = 0;
        forest.exits["east"] = 2;
        forest.exits["north"] = 3;
        forest.hasEnemy = true;
        forest.enemy = randomEnemy();

        Room cave;
        cave.name = "Cave";
        cave.description = "A dim cave glittering faintly with minerals.";
        cave.exits["west"] = 1;
        cave.hasTreasure = true;

        Room ruins;
        ruins.name = "Ruins";
        ruins.description = "Broken stone arches and strange carvings hint at a forgotten past.";
        ruins.exits["south"] = 1;
        ruins.hasEnemy = true;
        ruins.enemy = {"ruin_guardian", 22, 6, 15};

        rooms.push_back(village);
        rooms.push_back(forest);
        rooms.push_back(cave);
        rooms.push_back(ruins);
    }

    void describeRoom() {
        const Room& room = rooms.at(currentRoom);
        cout << "\n[" << room.name << "]\n";
        cout << room.description << "\n";

        if (room.hasPotion) {
            cout << "You see a healing potion here.\n";
        }
        if (room.hasTreasure) {
            cout << "Something valuable glints on the ground.\n";
        }
        if (room.hasEnemy && room.enemy.hp > 0) {
            cout << "A " << room.enemy.name << " is here, watching you.\n";
        }

        cout << "Exits:";
        for (const auto& [dir, _] : room.exits) {
            cout << ' ' << dir;
        }
        cout << "\n";
    }

    bool handleCommand(const string& rawInput) {
        vector<string> words = splitWords(rawInput);
        if (words.empty()) {
            cout << "You hesitate.\n";
            return true;
        }

        const string& cmd = words[0];

        if (cmd == "quit" || cmd == "exit") {
            cout << "Thanks for playing.\n";
            return false;
        }
        if (cmd == "help") {
            printHelp();
            return true;
        }
        if (cmd == "look") {
            describeRoom();
            return true;
        }
        if (cmd == "status") {
            printStatus();
            return true;
        }
        if (cmd == "inventory") {
            printInventory();
            return true;
        }
        if (cmd == "move" || cmd == "go") {
            if (words.size() < 2) {
                cout << "Move where?\n";
                return true;
            }
            movePlayer(words[1]);
            return true;
        }
        if (cmd == "take") {
            if (words.size() < 2) {
                cout << "Take what?\n";
                return true;
            }
            takeItem(words[1]);
            return true;
        }
        if (cmd == "drink") {
            drinkPotion();
            return true;
        }
        if (cmd == "attack") {
            attackEnemy();
            return true;
        }
        if (cmd == "save") {
            string filename = words.size() >= 2 ? words[1] : "savegame.txt";
            saveGame(filename);
            return true;
        }
        if (cmd == "load") {
            string filename = words.size() >= 2 ? words[1] : "savegame.txt";
            loadGame(filename);
            return true;
        }
        if (cmd == "shop") {
            shop();
            return true;
        }
        if (cmd == "rest") {
            rest();
            return true;
        }

        cout << "Unknown command: " << cmd << "\n";
        return true;
    }

    void printHelp() {
        cout << "Commands:\n";
        cout << "  help                Show commands\n";
        cout << "  look                Describe current room\n";
        cout << "  move/go <dir>       Move north/south/east/west\n";
        cout << "  take <item>         Pick up potion or treasure\n";
        cout << "  attack              Attack current enemy\n";
        cout << "  drink               Use a potion\n";
        cout << "  inventory           View inventory\n";
        cout << "  status              Show player stats\n";
        cout << "  shop                Buy a potion for 8 gold in the Village\n";
        cout << "  rest                Recover 5 HP in the Village\n";
        cout << "  save [file]         Save game\n";
        cout << "  load [file]         Load game\n";
        cout << "  quit                Exit game\n";
    }

    void printStatus() {
        cout << "HP: " << player.hp << '/' << player.maxHp
             << " | Attack: " << player.attack
             << " | Potions: " << player.potions
             << " | Gold: " << player.gold << "\n";
    }

    void printInventory() {
        cout << "Inventory:";
        if (player.inventory.empty()) {
            cout << " empty\n";
            return;
        }
        for (const auto& item : player.inventory) {
            cout << ' ' << item;
        }
        cout << "\n";
    }

    void movePlayer(const string& direction) {
        if (inCombat) {
            cout << "You cannot flee so easily; defeat the enemy first.\n";
            return;
        }

        Room& room = rooms.at(currentRoom);
        auto it = room.exits.find(direction);
        if (it == room.exits.end()) {
            cout << "You cannot go " << direction << ".\n";
            return;
        }

        currentRoom = it->second;
        Room& newRoom = rooms.at(currentRoom);
        inCombat = newRoom.hasEnemy && newRoom.enemy.hp > 0;
        describeRoom();
    }

    void takeItem(const string& item) {
        Room& room = rooms.at(currentRoom);

        if (item == "potion") {
            if (room.hasPotion) {
                room.hasPotion = false;
                player.potions++;
                player.inventory.push_back("potion");
                cout << "You picked up a potion.\n";
            } else {
                cout << "There is no potion here.\n";
            }
            return;
        }

        if (item == "treasure") {
            if (room.hasTreasure) {
                room.hasTreasure = false;
                player.gold += 12;
                player.inventory.push_back("gem");
                cout << "You found treasure worth 12 gold.\n";
            } else {
                cout << "There is no treasure here.\n";
            }
            return;
        }

        cout << "You cannot take that.\n";
    }

    void drinkPotion() {
        if (player.potions <= 0) {
            cout << "You have no potions.\n";
            return;
        }

        player.potions--;
        auto it = find(player.inventory.begin(), player.inventory.end(), "potion");
        if (it != player.inventory.end()) {
            player.inventory.erase(it);
        }

        int healAmount = 10;
        player.hp = min(player.maxHp, player.hp + healAmount);
        cout << "You drink a potion and recover " << healAmount << " HP.\n";
        printStatus();
    }

    void attackEnemy() {
        Room& room = rooms.at(currentRoom);
        if (!(room.hasEnemy && room.enemy.hp > 0)) {
            cout << "There is nothing to attack here.\n";
            inCombat = false;
            return;
        }

        room.enemy.hp -= player.attack;
        cout << "You strike the " << room.enemy.name << " for " << player.attack << " damage.\n";

        if (room.enemy.hp <= 0) {
            cout << "You defeated the " << room.enemy.name << " and earned " << room.enemy.goldReward << " gold.\n";
            player.gold += room.enemy.goldReward;
            inCombat = false;
            return;
        }

        player.hp -= room.enemy.attack;
        cout << "The " << room.enemy.name << " hits you for " << room.enemy.attack << " damage.\n";
        printStatus();
    }

    void shop() {
        if (currentRoom != 0) {
            cout << "There is no shop here.\n";
            return;
        }
        if (player.gold < 8) {
            cout << "You need 8 gold to buy a potion.\n";
            return;
        }
        player.gold -= 8;
        player.potions++;
        player.inventory.push_back("potion");
        cout << "You bought a potion.\n";
    }

    void rest() {
        if (currentRoom != 0) {
            cout << "This is not a safe place to rest.\n";
            return;
        }
        player.hp = min(player.maxHp, player.hp + 5);
        cout << "You rest at the village and recover 5 HP.\n";
        printStatus();
    }

    void saveGame(const string& filename) {
        ofstream out(filename);
        if (!out) {
            cout << "Could not save game to " << filename << "\n";
            return;
        }

        out << currentRoom << '\n';
        out << player.hp << ' ' << player.maxHp << ' ' << player.attack << ' '
            << player.potions << ' ' << player.gold << '\n';
        out << player.inventory.size() << '\n';
        for (const auto& item : player.inventory) {
            out << item << '\n';
        }

        for (const auto& room : rooms) {
            out << room.hasPotion << ' ' << room.hasTreasure << ' ' << room.hasEnemy << ' '
               << room.enemy.name << ' ' << room.enemy.hp << ' ' << room.enemy.attack << ' '
               << room.enemy.goldReward << '\n';
        }

        cout << "Game saved to " << filename << "\n";
    }

    void loadGame(const string& filename) {
        ifstream in(filename);
        if (!in) {
            cout << "Could not load game from " << filename << "\n";
            return;
        }

        Game fresh;
        *this = fresh;

        size_t inventorySize = 0;
        in >> currentRoom;
        in >> player.hp >> player.maxHp >> player.attack >> player.potions >> player.gold;
        in >> inventorySize;
        in.ignore(numeric_limits<streamsize>::max(), '\n');

        player.inventory.clear();
        for (size_t i = 0; i < inventorySize; ++i) {
            string item;
            getline(in, item);
            if (!item.empty()) {
                player.inventory.push_back(item);
            }
        }

        for (auto& room : rooms) {
            in >> room.hasPotion >> room.hasTreasure >> room.hasEnemy
               >> room.enemy.name >> room.enemy.hp >> room.enemy.attack >> room.enemy.goldReward;
        }

        Room& room = rooms.at(currentRoom);
        inCombat = room.hasEnemy && room.enemy.hp > 0;
        cout << "Game loaded from " << filename << "\n";
        describeRoom();
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}
