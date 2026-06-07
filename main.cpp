#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>
 
using namespace std;
 
// --- Системы Тегов и Ресурсов [cite: 1494-1511] ---
struct Fire {}; struct Ice {}; struct Poison {}; struct Physical {};
struct ManaTag {}; struct EnergyTag {}; struct RageTag {}; struct HealthTag {};
 
template<typename Tag>
class Resource {
    int val;
public:
    explicit Resource(int v) : val(v < 0 ? 0 : v) {}
    int value() const { return val; }
    void add(int v) { val += v; }
    void consume(int v) { val = (val >= v) ? (val - v) : 0; }
};
 
using Mana = Resource<ManaTag>; using Energy = Resource<EnergyTag>;
using Rage = Resource<RageTag>; using Health = Resource<HealthTag>;
 
// --- Оружие [cite: 1541-1549] ---
class BaseWeapon {
public:
    virtual ~BaseWeapon() = default;
    virtual string getName() const = 0;
    virtual int getCost() const = 0;
    virtual string getDmgType() const = 0;
};
 
template<typename T>
class Weapon : public BaseWeapon {
    string name; string type; int base; double crit; int cost;
public:
    Weapon(string n, string t, int b, double c, int co) : name(n), type(t), base(b), crit(c), cost(co) {}
    string getName() const override { return name; }
    int getCost() const override { return cost; }
    string getDmgType() const override { return type; }
    int getDmg() const { return static_cast<int>(round(base * crit)); }
};
 
// --- Персонажи [cite: 1550-1616] ---
class BaseCharacter {
public:
    virtual ~BaseCharacter() = default;
    virtual string getName() const = 0;
    virtual string getType() const = 0;
    virtual int getHP() const = 0;
    virtual int getResVal() const = 0;
    virtual bool isDefeated() const { return getHP() <= 0; }
    virtual void tryAttack(BaseCharacter& target) = 0;
    // Интерфейс получения урона
    virtual void receiveFire(int d) { (void)d; throw "type"; }
    virtual void receiveIce(int d) { (void)d; throw "type"; }
    virtual void receivePhysical(int d) { (void)d; throw "type"; }
};
 
template<typename H, typename R, typename D>
class Character : public BaseCharacter {
protected:
    string name; H hp; R res; shared_ptr<Weapon<D>> weapon;
public:
    Character(string n, int h, int r, shared_ptr<Weapon<D>> w) : name(n), hp(h), res(r), weapon(w) {}
    string getName() const override { return name; }
    int getHP() const override { return hp.value(); }
    int getResVal() const override { return res.value(); }
};
 
class Mage : public Character<Health, Mana, Fire> {
    double spellPower;
public:
    Mage(string n, int h, int m, shared_ptr<Weapon<Fire>> w, double sp) : Character(n, h, m, w), spellPower(sp) {}
    string getType() const override { return "Mage"; }
    void tryAttack(BaseCharacter& target) override {
        if (res.value() < weapon->getCost()) { cout << "Not enough resource" << endl; return; }
        res.consume(weapon->getCost());
        cout << name << " attacks " << target.getName() << endl;
        // Расчет урона мага (с промежуточным округлением) 
        int incoming = static_cast<int>(round(weapon->getDmg() * spellPower));
        try { target.receiveFire(incoming); } 
        catch(...) { cout << "Cannot attack type " << target.getType() << endl; }
    }
    void receiveIce(int d) override { takeRaw(d, "Ice"); }
    void receivePhysical(int d) override { takeRaw(d, "Physical"); }
    void takeRaw(int d, string t) {
        hp.consume(d); cout << name << " receives " << d << " " << t << " damage" << endl;
        cout << name << " health: " << hp.value() << endl;
        if (isDefeated()) cout << name << " defeated" << endl;
    }
};
 
class Warrior : public Character<Health, Rage, Physical> {
    double armor;
public:
    Warrior(string n, int h, int r, shared_ptr<Weapon<Physical>> w, double a) : Character(n, h, r, w), armor(a) {}
    string getType() const override { return "Warrior"; }
    void tryAttack(BaseCharacter& target) override {
        if (res.value() < weapon->getCost()) { cout << "Not enough resource" << endl; return; }
        res.consume(weapon->getCost());
        cout << name << " attacks " << target.getName() << endl;
        int incoming = weapon->getDmg();
        try { target.receivePhysical(incoming); }
        catch(...) { cout << "Cannot attack type " << target.getType() << endl; }
    }
    void receiveFire(int d) override {
        int finalD = static_cast<int>(round(d * (1.0 - armor)));
        res.add(d - finalD); // Накопление ярости за поглощенный урон
        hp.consume(finalD);
        cout << name << " receives " << finalD << " Fire damage" << endl;
        cout << name << " health: " << hp.value() << endl;
        if (isDefeated()) cout << name << " defeated" << endl;
    }
    void receiveIce(int d) override {
        int finalD = static_cast<int>(round(d * (1.0 - armor)));
        res.add(d - finalD); hp.consume(finalD);
        cout << name << " receives " << finalD << " Ice damage" << endl;
        cout << name << " health: " << hp.value() << endl;
        if (isDefeated()) cout << name << " defeated" << endl;
    }
};
 
class Drone : public Character<Health, Energy, Ice> {
    double shield;
public:
    Drone(string n, int h, int r, shared_ptr<Weapon<Ice>> w, double s) : Character(n, h, r, w), shield(s) {}
    string getType() const override { return "Drone"; }
    void tryAttack(BaseCharacter& target) override {
        if (res.value() < weapon->getCost()) { cout << "Not enough resource" << endl; return; }
        res.consume(weapon->getCost());
        cout << name << " attacks " << target.getName() << endl;
        int incoming = weapon->getDmg();
        try { target.receiveIce(incoming); }
        catch(...) { cout << "Cannot attack type " << target.getType() << endl; }
    }
    // Drone не имеет receive методов для Fire/Physical по условию
};
 
// --- Парсер [cite: 1753-1863] ---
class Arena {
    map<string, shared_ptr<BaseWeapon>> weapons;
    map<string, shared_ptr<BaseCharacter>> chars;
    bool anyOk = false;
    bool toInt(string s, int& r) { char* e; long v = strtol(s.c_str(), &e, 10); if (*e != '\0') return false; r = (int)v; return true; }
    bool toDouble(string s, double& r) { char* e; r = strtod(s.c_str(), &e); return *e == '\0'; }
public:
    void run() {
        string line;
        while (getline(cin, line)) {
            stringstream ss(line); string cmd; if (!(ss >> cmd)) continue;
            if (cmd == "exit") { string ex; if (ss >> ex) { cout << "Invalid command" << endl; continue; } break; }
            vector<string> args; string a; while (ss >> a) args.push_back(a);
            if (cmd == "create_weapon") {
                if (args.size() != 5) { cout << (args.size() < 5 ? "Too few arguments" : "Too many arguments") << endl; continue; }
                if (weapons.count(args[0]) || chars.count(args[0])) { cout << (weapons.count(args[0]) ? "Weapon already exists" : "Name already in use") << endl; continue; }
                int d, co; double cr; if (!toInt(args[2], d) || !toDouble(args[3], cr) || !toInt(args[4], co)) { cout << "Invalid arguments" << endl; continue; }
                if (d <= 0 || cr < 1.0 || co < 0) { cout << "Invalid weapon parameters" << endl; continue; }
                if (args[1] == "Fire") weapons[args[0]] = make_shared<Weapon<Fire>>(args[0], "Fire", d, cr, co);
                else if (args[1] == "Physical") weapons[args[0]] = make_shared<Weapon<Physical>>(args[0], "Physical", d, cr, co);
                else if (args[1] == "Ice") weapons[args[0]] = make_shared<Weapon<Ice>>(args[0], "Ice", d, cr, co);
                else { cout << "Invalid damage type" << endl; continue; }
                anyOk = true;
            } else if (cmd == "create") {
                if (args.size() != 6) { cout << (args.size() < 6 ? "Too few arguments" : "Too many arguments") << endl; continue; }
                if (chars.count(args[1]) || weapons.count(args[1])) { cout << (chars.count(args[1]) ? "Character already exists" : "Name already in use") << endl; continue; }
                if (!weapons.count(args[4])) { cout << "Weapon not found" << endl; continue; }
                int h, r; double ex; if (!toInt(args[2], h) || !toInt(args[3], r) || !toDouble(args[5], ex)) { cout << "Invalid arguments" << endl; continue; }
                if (h <= 0 || r < 0) { cout << "Invalid character parameters" << endl; continue; }
                auto w = weapons[args[4]];
                if (args[0] == "Mage") {
                    auto fw = dynamic_pointer_cast<Weapon<Fire>>(w);
                    if (!fw) { cout << "Incompatible weapon type" << endl; continue; }
                    if (ex <= 0) { cout << "Invalid character parameters" << endl; continue; }
                    chars[args[1]] = make_shared<Mage>(args[1], h, r, fw, ex);
                } else if (args[0] == "Warrior") {
                    auto pw = dynamic_pointer_cast<Weapon<Physical>>(w);
                    if (!pw) { cout << "Incompatible weapon type" << endl; continue; }
                    if (ex < 0 || ex > 0.9) { cout << "Invalid character parameters" << endl; continue; }
                    chars[args[1]] = make_shared<Warrior>(args[1], h, r, pw, ex);
                } else if (args[0] == "Drone") {
                    auto iw = dynamic_pointer_cast<Weapon<Ice>>(w);
                    if (!iw) { cout << "Incompatible weapon type" << endl; continue; }
                    if (ex < 0) { cout << "Invalid character parameters" << endl; continue; }
                    chars[args[1]] = make_shared<Drone>(args[1], h, r, iw, ex);
                } else { cout << "Invalid character type" << endl; continue; }
                anyOk = true;
            } else if (cmd == "attack") {
                if (args.size() != 2) { cout << (args.size() < 2 ? "Too few arguments" : "Too many arguments") << endl; continue; }
                if (args[0] == args[1]) { cout << "Cannot attack self" << endl; continue; }
                if (!chars.count(args[0]) || !chars.count(args[1])) { cout << "Character not found" << endl; continue; }
                auto at = chars[args[0]], df = chars[args[1]];
                if (at->isDefeated()) { cout << "Attacker defeated" << endl; continue; }
                if (df->isDefeated()) { cout << "Defender already defeated" << endl; continue; }
                at->tryAttack(*df); anyOk = true;
            } else cout << "Invalid command" << endl;
        }
        if (!anyOk) cout << "Empty input" << endl;
    }
};
 
int main() { Arena().run(); return 0; }
