#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

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

string joinWords(const vector<string>& words, size_t start = 0) {
    string out;
    for (size_t i = start; i < words.size(); ++i) {
        if (!out.empty()) out += ' ';
        out += words[i];
    }
    return out;
}

vector<string> wrapText(const string& text, size_t width) {
    vector<string> lines;
    istringstream iss(text);
    string word;
    string current;

    while (iss >> word) {
        if (current.empty()) {
            current = word;
        } else if (current.size() + 1 + word.size() <= width) {
            current += ' ' + word;
        } else {
            lines.push_back(current);
            current = word;
        }
    }

    if (!current.empty()) lines.push_back(current);
    if (lines.empty()) lines.push_back("");
    return lines;
}

struct Item {
    string name;
    string type;
    int heal = 0;
    int attackBonus = 0;
    int defenseBonus = 0;
    int value = 0;
    bool consumable = false;
};

struct Enemy {
    string name;
    int hp = 0;
    int maxHp = 0;
    int attack = 0;
    int defense = 0;
    int goldReward = 0;
    int xpReward = 0;
    string dropItem;
    bool boss = false;
};

struct Quest {
    string name;
    string description;
    string targetEnemy;
    int targetCount = 0;
    int progress = 0;
    int goldReward = 0;
    int xpReward = 0;
    bool accepted = false;
    bool completed = false;
    bool turnedIn = false;
};

struct Room {
    string name;
    string description;
    map<string, int> exits;
    vector<string> groundItems;
    bool visited = false;
    bool hasMerchant = false;
    bool hasInn = false;
    bool hasQuestGiver = false;
    bool hasForge = false;
    bool hasEnemy = false;
    Enemy enemy;
};

struct Player {
    int hp = 35;
    int maxHp = 35;
    int baseAttack = 5;
    int baseDefense = 1;
    int gold = 10;
    int level = 1;
    int xp = 0;
    int xpToNext = 20;
    vector<string> inventory{"small potion", "rusty sword", "traveler cloak"};
    string weapon = "rusty sword";
    string armor = "traveler cloak";
};

map<string, Item> buildItemDatabase() {
    map<string, Item> items;
    items["small potion"] = {"small potion", "consumable", 12, 0, 0, 6, true};
    items["large potion"] = {"large potion", "consumable", 22, 0, 0, 14, true};
    items["forest herb"] = {"forest herb", "consumable", 6, 0, 0, 3, true};
    items["rusty sword"] = {"rusty sword", "weapon", 0, 2, 0, 5, false};
    items["iron sword"] = {"iron sword", "weapon", 0, 4, 0, 15, false};
    items["crystal blade"] = {"crystal blade", "weapon", 0, 7, 0, 30, false};
    items["traveler cloak"] = {"traveler cloak", "armor", 0, 0, 1, 5, false};
    items["chain vest"] = {"chain vest", "armor", 0, 0, 3, 18, false};
    items["guardian mail"] = {"guardian mail", "armor", 0, 0, 5, 36, false};
    items["wolf pelt"] = {"wolf pelt", "loot", 0, 0, 0, 8, false};
    items["goblin charm"] = {"goblin charm", "loot", 0, 0, 0, 10, false};
    items["ancient core"] = {"ancient core", "quest", 0, 0, 0, 0, false};
    return items;
}

Enemy makeEnemy(const string& kind) {
    if (kind == "slime") return {"slime", 12, 12, 4, 0, 5, 7, "small potion", false};
    if (kind == "wolf") return {"wolf", 16, 16, 6, 1, 8, 10, "wolf pelt", false};
    if (kind == "goblin") return {"goblin", 20, 20, 7, 1, 11, 14, "goblin charm", false};
    if (kind == "bandit") return {"bandit", 24, 24, 8, 2, 15, 18, "large potion", false};
    if (kind == "guardian") return {"guardian", 40, 40, 10, 3, 25, 30, "ancient core", true};
    return {"rat", 8, 8, 3, 0, 3, 5, "forest herb", false};
}

class Game {
public:
    Game() : items(buildItemDatabase()) {
        buildWorld();
        buildQuests();
        addLog("Welcome to FuzzQuest: Echoes of the Hollow.");
        addLog("Type 'help' to view commands.");
        rooms[currentRoom].visited = true;
    }

    void run() {
        string line;
        while (true) {
            renderUI();
            cout << "> ";
            if (!getline(cin, line)) {
                cout << "\nFarewell, adventurer.\n";
                break;
            }
            if (!handleCommand(line)) {
                break;
            }
            if (player.hp <= 0) {
                renderUI();
                cout << "\nYou collapse from your wounds. Game over.\n";
                break;
            }
        }
    }

private:
    vector<Room> rooms;
    Player player;
    map<string, Item> items;
    vector<Quest> quests;
    vector<string> logLines;
    int currentRoom = 0;
    bool inCombat = false;
    int turnCounter = 0;

    void buildWorld() {
        rooms.clear();

        Room village;
        village.name = "Village";
        village.description = "Lanterns glow over a peaceful square. A merchant, inn, and quest board stand nearby.";
        village.exits["north"] = 1;
        village.exits["east"] = 4;
        village.hasMerchant = true;
        village.hasInn = true;
        village.hasQuestGiver = true;
        village.groundItems.push_back("forest herb");

        Room forest;
        forest.name = "Forest Edge";
        forest.description = "Tall trees sway in the wind. Tracks lead deeper into the woods.";
        forest.exits["south"] = 0;
        forest.exits["north"] = 2;
        forest.exits["east"] = 5;
        forest.hasEnemy = true;
        forest.enemy = makeEnemy("wolf");
        forest.groundItems.push_back("forest herb");

        Room deepForest;
        deepForest.name = "Deep Forest";
        deepForest.description = "The forest grows darker and the air feels heavy. Twigs snap somewhere nearby.";
        deepForest.exits["south"] = 1;
        deepForest.exits["east"] = 3;
        deepForest.hasEnemy = true;
        deepForest.enemy = makeEnemy("goblin");

        Room ruins;
        ruins.name = "Ruins";
        ruins.description = "Ancient stone arches loom overhead. A terrible guardian protects the center.";
        ruins.exits["west"] = 2;
        ruins.hasEnemy = true;
        ruins.enemy = makeEnemy("guardian");
        ruins.hasForge = true;

        Room market;
        market.name = "Market Road";
        market.description = "A row of stalls and smithing anvils line the road. Traders call out their prices.";
        market.exits["west"] = 0;
        market.exits["north"] = 5;
        market.hasMerchant = true;
        market.hasForge = true;

        Room pass;
        pass.name = "Stone Pass";
        pass.description = "A narrow pass between weathered rocks. Footprints and dropped gear suggest danger.";
        pass.exits["west"] = 1;
        pass.exits["south"] = 4;
        pass.hasEnemy = true;
        pass.enemy = makeEnemy("bandit");
        pass.groundItems.push_back("small potion");

        rooms.push_back(village);
        rooms.push_back(forest);
        rooms.push_back(deepForest);
        rooms.push_back(ruins);
        rooms.push_back(market);
        rooms.push_back(pass);
    }

    void buildQuests() {
        quests.clear();
        quests.push_back({
            "Wolf Hunt",
            "The village wants the nearby wolves thinned out. Defeat 2 wolves.",
            "wolf",
            2,
            0,
            18,
            16,
            false,
            false,
            false
        });
    }

    int attackStat() const {
        int bonus = 0;
        auto it = items.find(player.weapon);
        if (it != items.end()) bonus += it->second.attackBonus;
        return player.baseAttack + bonus;
    }

    int defenseStat() const {
        int bonus = 0;
        auto it = items.find(player.armor);
        if (it != items.end()) bonus += it->second.defenseBonus;
        return player.baseDefense + bonus;
    }

    void addLog(const string& message) {
        vector<string> wrapped = wrapText(message, 54);
        for (const auto& line : wrapped) {
            logLines.push_back(line);
        }
        if (logLines.size() > 200) {
            logLines.erase(logLines.begin(), logLines.begin() + 50);
        }
    }

    string roomSummary() const {
        const Room& room = rooms.at(currentRoom);
        string out = room.description;

        if (!room.groundItems.empty()) {
            out += " Items here: ";
            for (size_t i = 0; i < room.groundItems.size(); ++i) {
                if (i) out += ", ";
                out += room.groundItems[i];
            }
            out += ".";
        }

        if (room.hasMerchant) out += " Merchant available.";
        if (room.hasInn) out += " Inn available.";
        if (room.hasQuestGiver) out += " Quest board available.";
        if (room.hasForge) out += " Forge available.";
        if (room.hasEnemy && room.enemy.hp > 0) {
            out += " Enemy present: " + room.enemy.name + " (" + to_string(room.enemy.hp) + "/" + to_string(room.enemy.maxHp) + " HP).";
        }

        out += " Exits: ";
        bool first = true;
        for (const auto& [dir, _] : room.exits) {
            if (!first) out += ", ";
            out += dir;
            first = false;
        }
        out += ".";
        return out;
    }

    void printHorizontalBorder(int width) {
        cout << '+' << string(width - 2, '-') << "+\n";
    }

    void printPanelLine(const string& left, const string& right, int leftWidth, int rightWidth) {
        cout << '|'
             << left.substr(0, max(0, leftWidth))
             << string(max(0, leftWidth - static_cast<int>(left.substr(0, max(0, leftWidth)).size())), ' ')
             << '|'
             << right.substr(0, max(0, rightWidth))
             << string(max(0, rightWidth - static_cast<int>(right.substr(0, max(0, rightWidth)).size())), ' ')
             << "|\n";
    }

    vector<string> buildLeftPanel() const {
        vector<string> out;
        const Room& room = rooms.at(currentRoom);
        out.push_back(" LOCATION: " + room.name);
        out.push_back("");
        vector<string> roomLines = wrapText(roomSummary(), 54);
        out.insert(out.end(), roomLines.begin(), roomLines.end());
        out.push_back("");
        out.push_back(" ACTIVE QUESTS");
        bool anyQuest = false;
        for (const auto& q : quests) {
            if (q.accepted && !q.turnedIn) {
                anyQuest = true;
                string status = q.completed ? "completed" : to_string(q.progress) + "/" + to_string(q.targetCount);
                vector<string> qLines = wrapText("- " + q.name + " [" + status + "]", 54);
                out.insert(out.end(), qLines.begin(), qLines.end());
            }
        }
        if (!anyQuest) out.push_back("No active quests.");
        out.push_back("");
        out.push_back(" CONTROLS");
        out.push_back("look, map, status, inventory");
        out.push_back("go <dir>, take <item>, use <item>");
        out.push_back("equip <item>, attack, rest");
        out.push_back("shop, quests, turnin, forge");
        out.push_back("save [file], load [file], quit");
        return out;
    }

    vector<string> buildRightPanel() const {
        vector<string> out;
        out.push_back(" HERO");
        out.push_back("HP: " + to_string(player.hp) + "/" + to_string(player.maxHp));
        out.push_back("LVL: " + to_string(player.level) + "   XP: " + to_string(player.xp) + "/" + to_string(player.xpToNext));
        out.push_back("ATK: " + to_string(attackStat()) + "   DEF: " + to_string(defenseStat()));
        out.push_back("GOLD: " + to_string(player.gold));
        out.push_back("WPN: " + player.weapon);
        out.push_back("ARM: " + player.armor);
        out.push_back("");
        out.push_back(" INVENTORY");
        if (player.inventory.empty()) {
            out.push_back("empty");
        } else {
            for (size_t i = 0; i < player.inventory.size() && i < 8; ++i) {
                out.push_back("- " + player.inventory[i]);
            }
            if (player.inventory.size() > 8) out.push_back("...");
        }
        out.push_back("");
        out.push_back(" ENCOUNTER");
        const Room& room = rooms.at(currentRoom);
        if (room.hasEnemy && room.enemy.hp > 0) {
            out.push_back(room.enemy.name + " HP " + to_string(room.enemy.hp) + "/" + to_string(room.enemy.maxHp));
            out.push_back(string("ATK ") + to_string(room.enemy.attack) + " DEF " + to_string(room.enemy.defense));
            if (room.enemy.boss) out.push_back("Boss enemy");
        } else {
            out.push_back("No enemy present");
        }
        return out;
    }

    void renderUI() {
        cout << string(40, '\n');
        cout << "==================== FUZZQUEST TERMINAL UI ====================\n";
        printHorizontalBorder(80);
        auto left = buildLeftPanel();
        auto right = buildRightPanel();
        size_t maxLines = max(left.size(), right.size());
        left.resize(maxLines, "");
        right.resize(maxLines, "");
        for (size_t i = 0; i < maxLines; ++i) {
            printPanelLine(left[i], right[i], 56, 21);
        }
        printHorizontalBorder(80);
        cout << "+------------------------------------------------------------------------------+\n";
        cout << "| LOG                                                                          |\n";
        size_t start = logLines.size() > 8 ? logLines.size() - 8 : 0;
        for (size_t i = start; i < logLines.size(); ++i) {
            string msg = logLines[i];
            cout << '|' << msg.substr(0, 78)
                 << string(78 - min<size_t>(78, msg.size()), ' ') << "|\n";
        }
        for (size_t i = logLines.size() - start; i < 8; ++i) {
            cout << '|' << string(78, ' ') << "|\n";
        }
        cout << "+------------------------------------------------------------------------------+\n";
    }

    bool handleCommand(const string& rawInput) {
        vector<string> words = splitWords(rawInput);
        if (words.empty()) {
            addLog("You pause, unsure what to do.");
            return true;
        }

        const string& cmd = words[0];

        if (cmd == "quit" || cmd == "exit") {
            addLog("You decide to end your journey.");
            return false;
        }
        if (cmd == "help") {
            addLog("Try commands like: look, go north, take forest herb, attack, use small potion, shop.");
            return true;
        }
        if (cmd == "look") {
            addLog(roomSummary());
            return true;
        }
        if (cmd == "map") {
            addLog("Map: Village -> Forest Edge -> Deep Forest -> Ruins. Village -> Market Road -> Stone Pass.");
            return true;
        }
        if (cmd == "status") {
            addLog("HP " + to_string(player.hp) + "/" + to_string(player.maxHp) + ", ATK " + to_string(attackStat()) + ", DEF " + to_string(defenseStat()) + ", GOLD " + to_string(player.gold) + ".");
            return true;
        }
        if (cmd == "inventory") {
            if (player.inventory.empty()) addLog("Inventory is empty.");
            else addLog("Inventory: " + joinWords(player.inventory));
            return true;
        }
        if (cmd == "quests") {
            showQuests();
            return true;
        }
        if (cmd == "turnin") {
            turnInQuest();
            return true;
        }
        if (cmd == "go" || cmd == "move") {
            if (words.size() < 2) {
                addLog("Go where?");
                return true;
            }
            movePlayer(words[1]);
            advanceTurn();
            return true;
        }
        if (cmd == "take") {
            if (words.size() < 2) {
                addLog("Take what?");
                return true;
            }
            takeItem(joinWords(words, 1));
            advanceTurn();
            return true;
        }
        if (cmd == "use" || cmd == "drink") {
            if (cmd == "drink" && words.size() == 1) {
                useItem("small potion");
            } else {
                useItem(joinWords(words, 1));
            }
            advanceTurn();
            return true;
        }
        if (cmd == "equip") {
            if (words.size() < 2) {
                addLog("Equip what?");
                return true;
            }
            equipItem(joinWords(words, 1));
            return true;
        }
        if (cmd == "attack") {
            attackEnemy();
            advanceTurn();
            return true;
        }
        if (cmd == "shop") {
            shop();
            return true;
        }
        if (cmd == "rest") {
            rest();
            advanceTurn();
            return true;
        }
        if (cmd == "forge") {
            forgeUpgrade();
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

        addLog("Unknown command: " + cmd);
        return true;
    }

    void movePlayer(const string& direction) {
        if (inCombat) {
            addLog("You cannot leave while an enemy blocks your path.");
            return;
        }

        Room& room = rooms.at(currentRoom);
        auto it = room.exits.find(direction);
        if (it == room.exits.end()) {
            addLog("You cannot go " + direction + ".");
            return;
        }

        currentRoom = it->second;
        rooms[currentRoom].visited = true;
        Room& newRoom = rooms.at(currentRoom);
        inCombat = newRoom.hasEnemy && newRoom.enemy.hp > 0;
        addLog("You travel to " + newRoom.name + ".");
        addLog(newRoom.description);
        if (inCombat) {
            addLog("A " + newRoom.enemy.name + " confronts you.");
        }
    }

    void takeItem(const string& itemName) {
        Room& room = rooms.at(currentRoom);
        auto it = find(room.groundItems.begin(), room.groundItems.end(), itemName);
        if (it == room.groundItems.end()) {
            addLog("That item is not here.");
            return;
        }
        player.inventory.push_back(*it);
        addLog("You picked up " + *it + ".");
        room.groundItems.erase(it);
    }

    void useItem(const string& itemName) {
        auto invIt = find(player.inventory.begin(), player.inventory.end(), itemName);
        if (invIt == player.inventory.end()) {
            addLog("You do not have " + itemName + ".");
            return;
        }
        auto dbIt = items.find(itemName);
        if (dbIt == items.end()) {
            addLog("That item seems unusable.");
            return;
        }
        const Item& item = dbIt->second;
        if (!item.consumable) {
            addLog("You cannot use that right now.");
            return;
        }
        player.hp = min(player.maxHp, player.hp + item.heal);
        addLog("You use " + item.name + " and recover " + to_string(item.heal) + " HP.");
        player.inventory.erase(invIt);
    }

    void equipItem(const string& itemName) {
        auto invIt = find(player.inventory.begin(), player.inventory.end(), itemName);
        if (invIt == player.inventory.end()) {
            addLog("You do not have " + itemName + ".");
            return;
        }
        auto dbIt = items.find(itemName);
        if (dbIt == items.end()) {
            addLog("That item cannot be equipped.");
            return;
        }
        const Item& item = dbIt->second;
        if (item.type == "weapon") {
            player.weapon = item.name;
            addLog("You equip " + item.name + ".");
        } else if (item.type == "armor") {
            player.armor = item.name;
            addLog("You equip " + item.name + ".");
        } else {
            addLog("That is not equippable.");
        }
    }

    void attackEnemy() {
        Room& room = rooms.at(currentRoom);
        if (!(room.hasEnemy && room.enemy.hp > 0)) {
            inCombat = false;
            addLog("There is nothing to attack here.");
            return;
        }

        int dealt = max(1, attackStat() - room.enemy.defense);
        room.enemy.hp -= dealt;
        addLog("You hit the " + room.enemy.name + " for " + to_string(dealt) + " damage.");

        if (room.enemy.hp <= 0) {
            addLog("You defeated the " + room.enemy.name + ".");
            player.gold += room.enemy.goldReward;
            addXP(room.enemy.xpReward);
            addLog("You gain " + to_string(room.enemy.goldReward) + " gold and " + to_string(room.enemy.xpReward) + " XP.");
            if (!room.enemy.dropItem.empty()) {
                room.groundItems.push_back(room.enemy.dropItem);
                addLog("The enemy dropped " + room.enemy.dropItem + ".");
            }
            updateQuestProgress(room.enemy.name);
            if (room.enemy.boss) {
                addLog("You have defeated the Guardian and cleared the Ruins. Victory is yours.");
            }
            inCombat = false;
            return;
        }

        int incoming = max(1, room.enemy.attack - defenseStat());
        player.hp -= incoming;
        addLog("The " + room.enemy.name + " hits you for " + to_string(incoming) + " damage.");
    }

    void addXP(int amount) {
        player.xp += amount;
        while (player.xp >= player.xpToNext) {
            player.xp -= player.xpToNext;
            player.level++;
            player.xpToNext += 10;
            player.maxHp += 6;
            player.hp = player.maxHp;
            player.baseAttack += 1;
            player.baseDefense += 1;
            addLog("Level up! You are now level " + to_string(player.level) + ". Stats increased and HP restored.");
        }
    }

    void updateQuestProgress(const string& enemyName) {
        for (auto& q : quests) {
            if (q.accepted && !q.completed && q.targetEnemy == enemyName) {
                q.progress++;
                addLog("Quest progress for '" + q.name + "': " + to_string(q.progress) + "/" + to_string(q.targetCount) + ".");
                if (q.progress >= q.targetCount) {
                    q.completed = true;
                    addLog("Quest complete: " + q.name + ". Return to the Village to turn it in.");
                }
            }
        }
    }

    void showQuests() {
        if (rooms[currentRoom].hasQuestGiver) {
            bool offered = false;
            for (auto& q : quests) {
                if (!q.accepted) {
                    q.accepted = true;
                    addLog("Quest accepted: " + q.name + " - " + q.description);
                    offered = true;
                }
            }
            if (!offered) addLog("No new quests are available.");
            return;
        }

        bool any = false;
        for (const auto& q : quests) {
            if (q.accepted && !q.turnedIn) {
                any = true;
                addLog(q.name + ": " + q.description + " Progress " + to_string(q.progress) + "/" + to_string(q.targetCount) + ".");
            }
        }
        if (!any) addLog("You have no active quests.");
    }

    void turnInQuest() {
        if (!rooms[currentRoom].hasQuestGiver) {
            addLog("You need to be at the Village quest board to turn in quests.");
            return;
        }
        bool turned = false;
        for (auto& q : quests) {
            if (q.accepted && q.completed && !q.turnedIn) {
                q.turnedIn = true;
                player.gold += q.goldReward;
                addXP(q.xpReward);
                addLog("Quest turned in: " + q.name + ". Rewards: " + to_string(q.goldReward) + " gold, " + to_string(q.xpReward) + " XP.");
                turned = true;
            }
        }
        if (!turned) addLog("You have no completed quests to turn in.");
    }

    void shop() {
        const Room& room = rooms.at(currentRoom);
        if (!room.hasMerchant) {
            addLog("There is no merchant here.");
            return;
        }
        addLog("Merchant offers: small potion (6g), large potion (14g), iron sword (15g), chain vest (18g).");
        if (player.gold >= 6 && find(player.inventory.begin(), player.inventory.end(), "small potion") == player.inventory.end()) {
            player.gold -= 6;
            player.inventory.push_back("small potion");
            addLog("The merchant sells you a small potion.");
        } else {
            addLog("You browse the wares but buy nothing automatically. Edit shop logic or add buy <item> later if needed.");
        }
    }

    void rest() {
        const Room& room = rooms.at(currentRoom);
        if (!room.hasInn) {
            addLog("This place is too dangerous to rest.");
            return;
        }
        player.hp = min(player.maxHp, player.hp + 12);
        addLog("You rest at the inn and recover 12 HP.");
    }

    void forgeUpgrade() {
        const Room& room = rooms.at(currentRoom);
        if (!room.hasForge) {
            addLog("There is no forge here.");
            return;
        }
        bool hasCharm = find(player.inventory.begin(), player.inventory.end(), "goblin charm") != player.inventory.end();
        bool hasCore = find(player.inventory.begin(), player.inventory.end(), "ancient core") != player.inventory.end();

        if (player.weapon == "rusty sword" && hasCharm) {
            eraseOne("goblin charm");
            if (find(player.inventory.begin(), player.inventory.end(), "iron sword") == player.inventory.end()) {
                player.inventory.push_back("iron sword");
            }
            player.weapon = "iron sword";
            addLog("The forge reforges your rusty sword into an iron sword using the goblin charm.");
            return;
        }
        if (player.weapon == "iron sword" && hasCore) {
            eraseOne("ancient core");
            if (find(player.inventory.begin(), player.inventory.end(), "crystal blade") == player.inventory.end()) {
                player.inventory.push_back("crystal blade");
            }
            player.weapon = "crystal blade";
            addLog("The forge transforms your iron sword into a crystal blade using the ancient core.");
            return;
        }
        addLog("You lack the materials for any forge upgrade right now.");
    }

    void eraseOne(const string& itemName) {
        auto it = find(player.inventory.begin(), player.inventory.end(), itemName);
        if (it != player.inventory.end()) player.inventory.erase(it);
    }

    void saveGame(const string& filename) {
        ofstream out(filename);
        if (!out) {
            addLog("Could not save to " + filename + ".");
            return;
        }

        out << currentRoom << '\n';
        out << player.hp << ' ' << player.maxHp << ' ' << player.baseAttack << ' ' << player.baseDefense << ' '
            << player.gold << ' ' << player.level << ' ' << player.xp << ' ' << player.xpToNext << '\n';
        out << player.weapon << '\n' << player.armor << '\n';
        out << player.inventory.size() << '\n';
        for (const auto& item : player.inventory) out << item << '\n';

        out << quests.size() << '\n';
        for (const auto& q : quests) {
            out << q.name << '\n' << q.description << '\n' << q.targetEnemy << '\n';
            out << q.targetCount << ' ' << q.progress << ' ' << q.goldReward << ' ' << q.xpReward << ' '
               << q.accepted << ' ' << q.completed << ' ' << q.turnedIn << '\n';
        }

        out << rooms.size() << '\n';
        for (const auto& room : rooms) {
            out << room.groundItems.size() << '\n';
            for (const auto& item : room.groundItems) out << item << '\n';
            out << room.visited << ' ' << room.hasEnemy << ' ' << room.enemy.name << ' ' << room.enemy.hp << ' '
               << room.enemy.maxHp << ' ' << room.enemy.attack << ' ' << room.enemy.defense << ' '
               << room.enemy.goldReward << ' ' << room.enemy.xpReward << ' ' << room.enemy.dropItem << ' ' << room.enemy.boss << '\n';
        }

        addLog("Game saved to " + filename + ".");
    }

    void loadGame(const string& filename) {
        ifstream in(filename);
        if (!in) {
            addLog("Could not load from " + filename + ".");
            return;
        }

        Game fresh;
        *this = fresh;

        size_t inventorySize = 0, questSize = 0, roomSize = 0;
        in >> currentRoom;
        in >> player.hp >> player.maxHp >> player.baseAttack >> player.baseDefense
           >> player.gold >> player.level >> player.xp >> player.xpToNext;
        in.ignore(numeric_limits<streamsize>::max(), '\n');
        getline(in, player.weapon);
        getline(in, player.armor);
        in >> inventorySize;
        in.ignore(numeric_limits<streamsize>::max(), '\n');
        player.inventory.clear();
        for (size_t i = 0; i < inventorySize; ++i) {
            string item;
            getline(in, item);
            if (!item.empty()) player.inventory.push_back(item);
        }

        in >> questSize;
        in.ignore(numeric_limits<streamsize>::max(), '\n');
        quests.clear();
        for (size_t i = 0; i < questSize; ++i) {
            Quest q;
            getline(in, q.name);
            getline(in, q.description);
            getline(in, q.targetEnemy);
            in >> q.targetCount >> q.progress >> q.goldReward >> q.xpReward
               >> q.accepted >> q.completed >> q.turnedIn;
            in.ignore(numeric_limits<streamsize>::max(), '\n');
            quests.push_back(q);
        }

        in >> roomSize;
        in.ignore(numeric_limits<streamsize>::max(), '\n');
        for (size_t i = 0; i < roomSize && i < rooms.size(); ++i) {
            size_t groundCount = 0;
            in >> groundCount;
            in.ignore(numeric_limits<streamsize>::max(), '\n');
            rooms[i].groundItems.clear();
            for (size_t j = 0; j < groundCount; ++j) {
                string item;
                getline(in, item);
                if (!item.empty()) rooms[i].groundItems.push_back(item);
            }
            in >> rooms[i].visited >> rooms[i].hasEnemy >> rooms[i].enemy.name >> rooms[i].enemy.hp
               >> rooms[i].enemy.maxHp >> rooms[i].enemy.attack >> rooms[i].enemy.defense
               >> rooms[i].enemy.goldReward >> rooms[i].enemy.xpReward >> rooms[i].enemy.dropItem >> rooms[i].enemy.boss;
            in.ignore(numeric_limits<streamsize>::max(), '\n');
        }

        Room& room = rooms.at(currentRoom);
        inCombat = room.hasEnemy && room.enemy.hp > 0;
        addLog("Game loaded from " + filename + ".");
    }

    void advanceTurn() {
        turnCounter++;
        if (turnCounter % 4 == 0 && !inCombat) {
            maybeRespawnEnemy();
        }
    }

    void maybeRespawnEnemy() {
        uniform_int_distribution<int> roomDist(1, static_cast<int>(rooms.size()) - 1);
        int idx = roomDist(rng);
        Room& room = rooms[idx];
        if (room.hasEnemy && room.enemy.hp > 0) return;
        if (room.name == "Ruins") return;

        vector<string> pool = {"slime", "wolf", "goblin", "bandit"};
        uniform_int_distribution<int> enemyDist(0, static_cast<int>(pool.size()) - 1);
        room.hasEnemy = true;
        room.enemy = makeEnemy(pool[enemyDist(rng)]);
        addLog("You hear rumors that a " + room.enemy.name + " has appeared near " + room.name + ".");
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}
