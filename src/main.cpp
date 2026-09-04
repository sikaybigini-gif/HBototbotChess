#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

// NÖBET: UZUN KORİDOR
// A dependency-free, original C++17 terminal horror adventure.
// It intentionally uses its own names, text, and mechanics rather than
// copying Roblox, DOORS, or any other game's source code or assets.

namespace terminal {

bool isInteractive() {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

bool useColor() {
    const char* noColor = std::getenv("NO_COLOR");
    return isInteractive() && noColor == nullptr;
}

std::string paint(const std::string& text, const std::string& code) {
    if (!useColor()) {
        return text;
    }
    return "\033[" + code + "m" + text + "\033[0m";
}

void clear() {
    if (isInteractive()) {
        std::cout << "\033[2J\033[H";
    }
}

void bootAnimation() {
    if (!isInteractive()) {
        return;
    }
    const std::vector<std::string> frames = {
        "[■□□□□□□□□□] bağlantı kuruluyor",
        "[■■■■□□□□□□] bina haritası okunuyor",
        "[■■■■■■■□□□] kapılar numaralandırılıyor",
        "[■■■■■■■■■■] nöbet başlıyor",
    };
    for (const std::string& frame : frames) {
        clear();
        std::cout << paint("\n        NÖBET // UZUN KORİDOR\n\n", "1;36");
        std::cout << paint("        " + frame + "\n", "0;37") << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}

void doorTransition() {
    if (!isInteractive()) {
        return;
    }
    const std::vector<std::string> frames = {
        "       |              |",
        "       |      ..      |",
        "       |    .    .    |",
        "       |  .        .  |",
    };
    for (const std::string& frame : frames) {
        clear();
        std::cout << paint("\n\n              KAPI AÇILIYOR\n\n", "1;33");
        std::cout << paint(frame + "\n", "0;36") << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(70));
    }
}

void dangerFlash() {
    if (!isInteractive()) {
        return;
    }
    const std::vector<std::string> frames = {
        "\n\n              !!  !!  !!\n              BİR ŞEY YAKLAŞIYOR\n",
        "\n\n              !!       !!\n              SAKLAN\n",
    };
    for (const std::string& frame : frames) {
        clear();
        std::cout << paint(frame, "1;31") << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(110));
    }
}

} // namespace terminal

enum class RoomKind {
    Lobby,
    Corridor,
    GuestRoom,
    Archive,
    Workshop,
    Infirmary,
    Figure,
    Elevator,
};

enum class Item {
    BrassKey,
    Lockpick,
    Bandage,
    Tonic,
    Lighter,
    Adrenaline,
    Fuse,
    Coin,
};

enum class Phase {
    Explore,
    Chase,
    Won,
    Dead,
};

enum class Difficulty {
    Relaxed,
    Standard,
    Nightmare,
};

enum class Threat {
    None,
    Rattle,
    Whisper,
    Figure,
};

enum class PuzzleType {
    None,
    FuseBox,
    NumberLock,
};

struct Room {
    int number = 0;
    RoomKind kind = RoomKind::Corridor;
    std::string name;
    std::string description;
    std::string clue;
    bool dark = false;
    bool lit = false;
    bool locked = false;
    bool hasCloset = true;
    bool hasShop = false;
    bool visited = false;
    bool searched = false;
    bool eventSeen = false;
    bool clueRead = false;
    PuzzleType puzzle = PuzzleType::None;
    bool puzzleSolved = false;
    bool falseDoor = false;
    bool falseDoorSeen = false;
    bool figureRoom = false;
    bool figureCleared = false;
    std::vector<int> puzzleCode;
    std::vector<Item> loot;
};

struct Player {
    int health = 100;
    int sanity = 100;
    std::map<Item, int> inventory;
};

std::string lowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::vector<std::string> tokenize(const std::string& line) {
    std::istringstream stream(line);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token) {
        tokens.push_back(lowerAscii(token));
    }
    return tokens;
}

std::optional<int> parseInt(const std::string& value) {
    try {
        std::size_t used = 0;
        const int result = std::stoi(value, &used);
        if (used != value.size()) {
            return std::nullopt;
        }
        return result;
    } catch (...) {
        return std::nullopt;
    }
}

std::string itemName(Item item) {
    switch (item) {
    case Item::BrassKey:
        return "Pirinç anahtar";
    case Item::Lockpick:
        return "Maymuncuk";
    case Item::Bandage:
        return "Bandaj";
    case Item::Tonic:
        return "Sakinleştirici";
    case Item::Lighter:
        return "Çakmak";
    case Item::Adrenaline:
        return "Adrenalin";
    case Item::Fuse:
        return "Sigorta";
    case Item::Coin:
        return "Jeton";
    }
    return "Bilinmeyen eşya";
}

std::string roomKindName(RoomKind kind) {
    switch (kind) {
    case RoomKind::Lobby:
        return "Giriş holü";
    case RoomKind::Corridor:
        return "Uzun koridor";
    case RoomKind::GuestRoom:
        return "Misafir odası";
    case RoomKind::Archive:
        return "Arşiv";
    case RoomKind::Workshop:
        return "Bakım odası";
    case RoomKind::Infirmary:
        return "Revir";
    case RoomKind::Figure:
        return "Kör nöbetçi salonu";
    case RoomKind::Elevator:
        return "Servis asansörü";
    }
    return "Bilinmeyen oda";
}

std::string threatName(Threat threat) {
    switch (threat) {
    case Threat::Rattle:
        return "Gürültü";
    case Threat::Whisper:
        return "Fısıltı";
    case Threat::Figure:
        return "KÖR NÖBETÇİ";
    case Threat::None:
        return "Yok";
    }
    return "Yok";
}

std::string puzzleName(PuzzleType puzzle) {
    switch (puzzle) {
    case PuzzleType::FuseBox:
        return "Sigorta paneli";
    case PuzzleType::NumberLock:
        return "Sayı kilidi";
    case PuzzleType::None:
        return "Yok";
    }
    return "Yok";
}

std::string difficultyName(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Relaxed:
        return "Rahat";
    case Difficulty::Standard:
        return "Standart";
    case Difficulty::Nightmare:
        return "Kabus";
    }
    return "Standart";
}

std::string progressBar(int value, int width = 20) {
    value = std::clamp(value, 0, 100);
    const int filled = value * width / 100;
    std::string bar = "[";
    for (int i = 0; i < width; ++i) {
        bar += i < filled ? '#' : '-';
    }
    bar += ']';
    return bar;
}

std::string doorLabel(int number) {
    std::ostringstream output;
    output << std::setw(2) << std::setfill('0') << number;
    return output.str();
}

bool isFigureDoor(int number) {
    return number == 50 || number == 100;
}

int firstChaseDoor(int targetDoors) {
    int door = std::clamp(targetDoors / 2, 5, targetDoors - 2);
    if (isFigureDoor(door)) {
        door = std::max(5, door - 5);
    }
    return door;
}

int secondChaseDoor(int targetDoors, int firstDoor) {
    int door = std::clamp((targetDoors * 3) / 4, firstDoor + 3, targetDoors - 1);
    if (isFigureDoor(door)) {
        door = std::max(firstDoor + 3, door - 5);
    }
    return door;
}

std::optional<Difficulty> parseDifficulty(const std::string& value) {
    const std::string normalized = lowerAscii(value);
    if (normalized == "easy" || normalized == "relaxed" || normalized == "rahat") {
        return Difficulty::Relaxed;
    }
    if (normalized == "normal" || normalized == "standard" || normalized == "standart") {
        return Difficulty::Standard;
    }
    if (normalized == "hard" || normalized == "nightmare" || normalized == "kabus") {
        return Difficulty::Nightmare;
    }
    return std::nullopt;
}

enum class Sound {
    Door,
    Key,
    Pickup,
    Lighter,
    Danger,
    Hide,
    Chase,
    Hit,
    Puzzle,
    Error,
    Ambience,
    Victory,
    Death,
};

const char* soundFile(Sound sound) {
    switch (sound) {
    case Sound::Door:
        return "door.wav";
    case Sound::Key:
        return "key.wav";
    case Sound::Pickup:
        return "pickup.wav";
    case Sound::Lighter:
        return "lighter.wav";
    case Sound::Danger:
        return "danger.wav";
    case Sound::Hide:
        return "hide.wav";
    case Sound::Chase:
        return "chase.wav";
    case Sound::Hit:
        return "hit.wav";
    case Sound::Puzzle:
        return "puzzle.wav";
    case Sound::Error:
        return "error.wav";
    case Sound::Ambience:
        return "ambience.wav";
    case Sound::Victory:
        return "victory.wav";
    case Sound::Death:
        return "death.wav";
    }
    return "door.wav";
}

class AudioSystem {
public:
    explicit AudioSystem(bool enabled)
        : enabled_(enabled && std::getenv("NO_SOUND") == nullptr), player_(enabled_ ? detectPlayer() : "") {}

    void play(Sound sound) const {
        if (!enabled_) {
            return;
        }

        // The bell gives immediate feedback even on a machine without an audio
        // player. It is only emitted in a real terminal, never in redirected logs.
        if (terminal::isInteractive()) {
            std::cout << '\a' << std::flush;
        }
        if (player_.empty()) {
            return;
        }

        const std::string path = "assets/sfx/" + std::string(soundFile(sound));
#ifdef _WIN32
        const std::string command =
            "powershell -NoProfile -Command \"(New-Object Media.SoundPlayer '" + path + "').PlaySync()\"";
#else
        const std::string command = player_ + " \"" + path + "\" >/dev/null 2>&1 &";
#endif
        // All command parts above are fixed strings; no user input is included.
        (void)std::system(command.c_str());
    }

private:
    bool enabled_ = true;
    std::string player_;

    static std::string detectPlayer() {
#ifdef _WIN32
        return "powershell";
#else
        if (std::system("command -v paplay >/dev/null 2>&1") == 0) {
            return "paplay";
        }
        if (std::system("command -v aplay >/dev/null 2>&1") == 0) {
            return "aplay -q";
        }
        if (std::system("command -v afplay >/dev/null 2>&1") == 0) {
            return "afplay";
        }
        if (std::system("command -v ffplay >/dev/null 2>&1") == 0) {
            return "ffplay -nodisp -autoexit -loglevel quiet";
        }
        return "";
#endif
    }
};

class Game {
public:
    Game(int targetDoors, std::uint64_t seed, bool audioEnabled, Difficulty difficulty)
        : targetDoors_(std::clamp(targetDoors, 12, 100)),
          seed_(seed),
          rng_(static_cast<std::mt19937::result_type>(seed)),
          audio_(audioEnabled),
          difficulty_(difficulty) {
        chaseDoor_ = firstChaseDoor(targetDoors_);
        secondChaseDoor_ = secondChaseDoor(targetDoors_, chaseDoor_);
        buildRooms();
        player_.inventory[Item::Lighter] = 1;
        player_.inventory[Item::Bandage] = 1;
        player_.inventory[Item::Coin] = 2;
        rooms_[1].visited = true;
    }

    void run() {
        terminal::bootAnimation();
        say("NÖBET başladı. Binadan çıkmak için servis asansörüne ulaş.");
        say("Bu, Roblox veya başka bir oyunun kopyası değil; C++ ile yazılmış özgün bir konsol korku macerasıdır.");
        say("Renkli ASCII sahneler, prosedürel odalar ve özgün WAV ses ipuçları aktif. Sessiz mod: `--no-audio`.");
        say("İpucu: Önce `yardım` yaz. Oyun Türkçe komutların yanında İngilizce kısa komutları da kabul eder.");

        while (running_ && phase_ != Phase::Won && phase_ != Phase::Dead) {
            terminal::clear();
            render();
            std::cout << terminal::paint("\n> ", "1;37");
            std::string line;
            if (!std::getline(std::cin, line)) {
                running_ = false;
                break;
            }
            processCommand(line);
        }

        terminal::clear();
        render();
        if (phase_ == Phase::Won || phase_ == Phase::Dead) {
            std::cout << "\n";
            std::cout << terminal::paint("Devam etmek için Enter'a basın...", "2;37");
            std::string ignored;
            std::getline(std::cin, ignored);
        }
    }

private:
    int targetDoors_ = 30;
    int chaseDoor_ = 15;
    int secondChaseDoor_ = 22;
    std::uint64_t seed_ = 0;
    std::mt19937 rng_;
    AudioSystem audio_;
    Difficulty difficulty_ = Difficulty::Standard;
    std::vector<Room> rooms_;
    Player player_;
    int currentRoom_ = 1;
    int turns_ = 0;
    bool running_ = true;
    bool hidden_ = false;
    Phase phase_ = Phase::Explore;
    Threat threat_ = Threat::None;
    int threatTurns_ = 0;
    int figureNoise_ = 0;
    int chaseStage_ = 0;
    std::vector<std::string> chasePattern_;
    std::size_t chaseStep_ = 0;
    std::vector<std::string> notices_;

    static std::uint64_t mix(std::uint64_t value) {
        value += 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31U);
    }

    Room& currentRoom() {
        return rooms_[static_cast<std::size_t>(currentRoom_)];
    }

    const Room& currentRoom() const {
        return rooms_[static_cast<std::size_t>(currentRoom_)];
    }

    int count(Item item) const {
        const auto found = player_.inventory.find(item);
        return found == player_.inventory.end() ? 0 : found->second;
    }

    void add(Item item, int amount = 1) {
        if (amount > 0) {
            player_.inventory[item] += amount;
        }
    }

    bool take(Item item, int amount = 1) {
        if (amount <= 0 || count(item) < amount) {
            return false;
        }
        auto found = player_.inventory.find(item);
        found->second -= amount;
        if (found->second == 0) {
            player_.inventory.erase(found);
        }
        return true;
    }

    bool rollPercent(int percentage) {
        percentage = std::clamp(percentage, 0, 100);
        return static_cast<int>(rng_() % 100U) < percentage;
    }

    void say(const std::string& message) {
        notices_.push_back(message);
        constexpr std::size_t maxNotices = 14;
        if (notices_.size() > maxNotices) {
            notices_.erase(notices_.begin(), notices_.begin() + static_cast<std::ptrdiff_t>(notices_.size() - maxNotices));
        }
    }

    void buildRooms() {
        rooms_.assign(static_cast<std::size_t>(targetDoors_ + 1), Room{});

        for (int number = 1; number <= targetDoors_; ++number) {
            Room room;
            room.number = number;

            if (number == 1) {
                room.kind = RoomKind::Lobby;
                room.name = "Kayıt Masası";
                room.description = "Tozlu resepsiyon masasının arkasında durmuş bir saat var. Akrep ve yelkovan aynı yerde takılı kalmış.";
                room.hasCloset = false;
            } else if (isFigureDoor(number)) {
                room.kind = RoomKind::Figure;
                room.figureRoom = true;
                room.name = number == 50 ? "Sessiz salon" : "Son nöbet";
                room.description = number == 50
                                       ? "Kapı 50'nin ardında kör bir nöbetçi dolaşıyor. Görmüyor; nefes, metal ve acele ayak seslerini duyuyor."
                                       : "Kapı 100'ün ardında son nöbet başlıyor. Kör nöbetçi ışığı değil, en küçük sesi izliyor.";
                room.hasCloset = true;
                room.dark = true;
            } else if (number == targetDoors_) {
                room.kind = RoomKind::Elevator;
                room.name = "Servis Asansörü";
                room.description = "Paslı bir asansör kapısı titreşerek bekliyor. Üzerindeki kırmızı lamba hâlâ yanıyor.";
                room.hasCloset = false;
            } else {
                const std::uint64_t roomSeed = mix(seed_ ^ (static_cast<std::uint64_t>(number) * 0x632be59bd9b4e019ULL));
                const int kindRoll = static_cast<int>(roomSeed % 5U);
                switch (kindRoll) {
                case 0:
                    room.kind = RoomKind::Corridor;
                    room.name = "Kadife koridor";
                    room.description = "Duvar kâğıtlarının altında ince bir uğultu dolaşıyor. Halı, ayak seslerini yutuyor.";
                    break;
                case 1:
                    room.kind = RoomKind::GuestRoom;
                    room.name = "Terk edilmiş oda";
                    room.description = "Yatak örtüsü sanki az önce düzeltilmiş. Pencerenin ardında hiçbir şehir ışığı görünmüyor.";
                    break;
                case 2:
                    room.kind = RoomKind::Archive;
                    room.name = "Tozlu arşiv";
                    room.description = "Raflarda numarasız dosyalar ve kimsenin göndermediği mektuplar var.";
                    room.clue = "Bir dosyanın kenarında şu cümle yazıyor: Kapı sesi geldiğinde ışığa değil, saklanacak yere güven.";
                    break;
                case 3:
                    room.kind = RoomKind::Workshop;
                    room.name = "Bakım odası";
                    room.description = "Bakır borular duvarların içinde öksürüyor. Zeminde alet izleri ve eski yağ lekeleri var.";
                    break;
                default:
                    room.kind = RoomKind::Infirmary;
                    room.name = "Kapalı revir";
                    room.description = "Metal dolapların camları çatlamış. Bir muayene lambası arada bir kendi kendine yanıyor.";
                    break;
                }

                room.hasCloset = room.kind != RoomKind::Workshop || number % 2 == 0;
                room.hasShop = number % 10 == 0 || (room.kind == RoomKind::Infirmary && number % 2 == 0);
                room.dark = number % 8 == 0 || (room.kind == RoomKind::Workshop && number % 3 == 0);
                room.locked = number > 1 && number < targetDoors_ && number % 6 == 0;
                if (!room.locked && number != chaseDoor_ && number != secondChaseDoor_ && number % 9 == 0) {
                    room.puzzle = PuzzleType::FuseBox;
                } else if (!room.locked && number != chaseDoor_ && number != secondChaseDoor_ && number % 11 == 0) {
                    room.puzzle = PuzzleType::NumberLock;
                }

                std::mt19937 local(static_cast<std::mt19937::result_type>(roomSeed));
                if (room.puzzle == PuzzleType::NumberLock) {
                    const int code = 100 + static_cast<int>(local() % 900U);
                    room.puzzleCode = {code / 100, (code / 10) % 10, code % 10};
                    room.clue = "Kapının yanındaki silinmiş etikette üç rakam seçiliyor: " + std::to_string(code) + ".";
                } else if (room.puzzle == PuzzleType::FuseBox) {
                    room.clue = "Duvar paneli bir sigorta istiyor. Yedek sigortalar genellikle bakım odalarında olur.";
                }
                room.falseDoor = number > 3 && number < targetDoors_ && number % 17 == 0 &&
                                 number != chaseDoor_ && room.puzzle == PuzzleType::None && !room.locked;
                const int lootRoll = static_cast<int>(local() % 100U);
                if (room.puzzle == PuzzleType::FuseBox ||
                    (room.kind == RoomKind::Workshop && lootRoll % 5 == 0)) {
                    room.loot.push_back(Item::Fuse);
                }
                if (lootRoll < 82) {
                    room.loot.insert(room.loot.end(), 1 + static_cast<int>(local() % 3U), Item::Coin);
                }
                if (lootRoll % 7 == 0 || number % 11 == 4) {
                    room.loot.push_back(Item::Lockpick);
                }
                if (room.kind == RoomKind::Infirmary && lootRoll % 3 != 0) {
                    room.loot.push_back(Item::Bandage);
                }
                if (room.kind == RoomKind::Workshop && lootRoll % 4 == 0) {
                    room.loot.push_back(Item::Adrenaline);
                }
                if (number % 13 == 7) {
                    room.loot.push_back(Item::Tonic);
                }
            }

            rooms_[static_cast<std::size_t>(number)] = std::move(room);
        }

        // A locked door's key is hidden in the room immediately before it.
        // The player can return to search it if they missed the clue.
        for (int number = 2; number <= targetDoors_; ++number) {
            if (rooms_[static_cast<std::size_t>(number)].locked) {
                rooms_[static_cast<std::size_t>(number - 1)].loot.push_back(Item::BrassKey);
            }
        }
    }

    void renderRoomArt(const Room& room) const {
        const bool unlit = room.dark && !room.lit;
        std::vector<std::string> art;

        if (unlit && room.kind != RoomKind::Figure) {
            art = {
                "      . . . . . . . . . . . . . . . . . . .",
                "     /                                       \\",
                "    |               [  ?  ]                  |",
                "    |            .-----------.                |",
                "    |            |     ?     |                |",
                "    |            '-----------'                |",
                "    |       . . . . . . . . . . . .          |",
            };
        } else {
            switch (room.kind) {
            case RoomKind::Lobby:
                art = {
                    "      .------------------------------------.",
                    "     /       o                 o            \\",
                    "    |       /|\\               /|\\           |",
                    "    |    .--------------------------------.    |",
                    "    |    |          RESEPSİYON           |    |",
                    "    |    '--------------------------------'    |",
                    "    '------------------------------------------'",
                };
                break;
            case RoomKind::Corridor:
                art = {
                    "      .------------------------------------.",
                    "     /       *       *       *       *        \\",
                    "    |          .-----------------.            |",
                    "    |          |                 |            |",
                    "    |          |    İLERİ        |            |",
                    "    |          |                 |            |",
                    "    '----------'-----------------'------------'",
                };
                break;
            case RoomKind::GuestRoom:
                art = {
                    "      .------------------------------------.",
                    "     |      .--------.             _______    |",
                    "     |      | pencere|            /       \\   |",
                    "     |      '--------'           /  YATAK  \\  |",
                    "     |                         '-----------'  |",
                    "     |       .---------.                       |",
                    "     '-------|  DOLAP  |----------------------'",
                };
                break;
            case RoomKind::Archive:
                art = {
                    "      .------------------------------------.",
                    "     |  [====] [====] [====] [====]         |",
                    "     |  [====] [====] [====] [====]         |",
                    "     |  [====] [====] [====] [====]         |",
                    "     |       o                 o             |",
                    "     |             .---------.               |",
                    "     '-------------|  MASA   |---------------'",
                };
                break;
            case RoomKind::Workshop:
                art = {
                    "      .------------------------------------.",
                    "     |  ===\\        ______        /===      |",
                    "     |      \\______/      \\______/          |",
                    "     |        |       ⚙       |              |",
                    "     |        '---------------'              |",
                    "     |       _/|    ALET    |\\_             |",
                    "     '------'------------------'-------------'",
                };
                break;
            case RoomKind::Infirmary:
                art = {
                    "      .------------------------------------.",
                    "     |          +       +                   |",
                    "     |          |       |       .---.       |",
                    "     |      .---+-------+---.   | o |       |",
                    "     |      |    MUAYENE   |   '---'       |",
                    "     |      '---------------'               |",
                    "     '---------------------------------------'",
                };
                break;
            case RoomKind::Figure:
                art = {
                    "      .------------------------------------.",
                    "     |        .       .       .             |",
                    "     |             .-^^^-.                  |",
                    "     |            /  ___  \\                 |",
                    "     |           |    ^    |                |",
                    "     |           |  /|\\  |                |",
                    "     |          /|   |   |\\               |",
                    "     '---------/_____|_____\\--------------'",
                };
                break;
            case RoomKind::Elevator:
                art = {
                    "      .------------------------------------.",
                    "     |              SERVİS                  |",
                    "     |          .------------.              |",
                    "     |          |  ASANSÖR   |              |",
                    "     |          |      >     |              |",
                    "     |          '------------'              |",
                    "     '---------------------------------------'",
                };
                break;
            }
        }

        std::string color = "0;36";
        if (room.kind == RoomKind::GuestRoom || room.kind == RoomKind::Infirmary) {
            color = "0;35";
        } else if (room.kind == RoomKind::Workshop) {
            color = "0;33";
        } else if (room.kind == RoomKind::Figure) {
            color = "1;31";
        } else if (room.kind == RoomKind::Elevator) {
            color = "1;32";
        }
        if (unlit) {
            color = "1;35";
        }
        if (phase_ == Phase::Chase || threat_ != Threat::None) {
            color = "1;31";
        }

        const std::string plate = room.number == targetDoors_
                                      ? "[ EXIT ]"
                                      : "[ DOOR " + doorLabel(room.number) + " ]";
        std::cout << terminal::paint("   " + plate + "\n", color);
        for (const std::string& line : art) {
            std::cout << terminal::paint("   " + line + "\n", color);
        }
    }

    void renderMap() const {
        const int firstRoom = std::max(1, currentRoom_ - 5);
        const int lastRoom = std::min(targetDoors_, currentRoom_ + 5);
        std::cout << terminal::paint("Rota ", "1;36");
        for (int number = firstRoom; number <= lastRoom; ++number) {
            const std::string label = "[" + doorLabel(number) + "]";
            if (number == currentRoom_) {
                std::cout << terminal::paint(label, "1;33");
            } else if (rooms_[static_cast<std::size_t>(number)].visited) {
                std::cout << terminal::paint(label, "0;36");
            } else {
                std::cout << terminal::paint("[??]", "2;37");
            }
            if (number != lastRoom) {
                std::cout << terminal::paint("──", "2;37");
            }
        }
        if (firstRoom > 1) {
            std::cout << "  ...";
        }
        if (lastRoom < targetDoors_) {
            std::cout << "  ...";
        }
        std::cout << "\n";
    }

    void render() const {
        const Room& room = currentRoom();
        const std::string title = " NÖBET // UZUN KORİDOR ";
        std::cout << terminal::paint("╔══════════════════════════════════════════════════════════════════════╗\n", "1;36");
        std::cout << terminal::paint("║", "1;36") << std::left << std::setw(68) << title << terminal::paint("║\n", "1;36");
        std::cout << terminal::paint("╚══════════════════════════════════════════════════════════════════════╝\n", "1;36");

        if (phase_ == Phase::Won) {
            renderEnding(true);
            return;
        }
        if (phase_ == Phase::Dead) {
            renderEnding(false);
            return;
        }

        std::cout << terminal::paint("Kapı ", "1;33") << std::right << std::setfill('0') << std::setw(2) << currentRoom_;
        std::cout << std::setfill(' ') << " / " << targetDoors_;
        std::cout << "    " << terminal::paint(room.name, "1;37") << "\n";
        std::cout << "Can      " << progressBar(player_.health) << " " << std::setw(3) << player_.health;
        std::cout << "    Akıl " << progressBar(player_.sanity) << " " << std::setw(3) << player_.sanity << "\n";
        std::cout << "Eşyalar  " << inventorySummary() << "\n";
        std::cout << "Mod      " << difficultyName(difficulty_) << "\n";
        std::cout << "────────────────────────────────────────────────────────────────────────\n";
        renderMap();
        renderRoomArt(room);

        if (phase_ == Phase::Chase) {
            std::cout << terminal::paint("KAÇIŞ " + std::to_string(chaseStage_) + "/2 // ARKANA BAKMA\n", "1;31");
            std::cout << "Koridorun sonundaki bir sonraki hareket: "
                      << terminal::paint(displayMove(chasePattern_[chaseStep_]), "1;33") << "\n";
            std::cout << "Sıra " << (chaseStep_ + 1) << "/" << chasePattern_.size()
                      << ". `sol`, `sağ`, `atla` veya `eğil` yaz.\n";
        } else {
            std::cout << terminal::paint(roomKindName(room.kind), "2;37") << "\n";
            if (room.dark && !room.lit) {
                std::cout << terminal::paint("Oda karanlık. Aramak için `çakmak kullan` gerekir.\n", "1;35");
            } else if (room.dark) {
                std::cout << terminal::paint("Çakmağın titrek ışığı duvarları aydınlatıyor.\n", "0;33");
            }
            std::cout << room.description << "\n";
            if (room.locked) {
                std::cout << terminal::paint("İlerideki kapı kilitli; bir anahtar veya maymuncuk gerek.\n", "1;33");
            }
            if (room.puzzle != PuzzleType::None && !room.puzzleSolved) {
                if (room.puzzle == PuzzleType::FuseBox) {
                    std::cout << terminal::paint("Kapının yanında kırmızı bir sigorta paneli var: `kullan sigorta`.\n", "1;33");
                } else {
                    std::cout << terminal::paint("Kapının yanında üç haneli bir sayı kilidi var: `çöz <kod>`.\n", "1;33");
                }
            }
            if (room.hasShop) {
                std::cout << terminal::paint("Eski bir servis arabası burada çalışıyor: `satın al bandaj|tonik|maymuncuk`.\n", "1;32");
            }
            if (room.falseDoor && room.falseDoorSeen) {
                std::cout << terminal::paint("Bu kapının yankısı yok; kolu ikinci denemede dikkatle çevir.\n", "1;31");
            }
            if (room.figureRoom && !room.figureCleared) {
                std::cout << terminal::paint("KÖR NÖBETÇİ: görmüyor ama en küçük sesi duyuyor. Sessizce `saklan` yaz.\n", "1;31");
            } else if (room.figureRoom) {
                std::cout << terminal::paint("Nöbetçi sesini kaybetti; salon şimdilik sessiz.\n", "0;32");
            }
            if (threat_ == Threat::Figure) {
                std::cout << terminal::paint("SES İZİ: " + std::to_string(figureNoise_) + "/3 — jetonun varsa `kullan jeton` ile başka yöne ses at.\n", "1;31");
            } else if (threat_ != Threat::None) {
                std::cout << terminal::paint("TEHLİKE: " + threatName(threat_) + " — saklanmak için `saklan` yaz.\n", "1;31");
            }
            if (hidden_) {
                std::cout << terminal::paint("Dolabın içindesin. Güvenli olduğunda `çık` yaz.\n", "1;32");
            }
        }

        std::cout << "────────────────────────────────────────────────────────────────────────\n";
        const std::size_t first = notices_.size() > 8 ? notices_.size() - 8 : 0;
        for (std::size_t i = first; i < notices_.size(); ++i) {
            std::cout << "• " << notices_[i] << "\n";
        }
        std::cout << "\n" << terminal::paint("Komutlar: yardım | bak | ara | kapıyı aç | çöz | saklan | çanta | çıkış", "2;37") << "\n";
    }

    void renderEnding(bool won) const {
        std::cout << "\n";
        if (won) {
            std::cout << terminal::paint("  ███████╗██╗███╗   ██╗ █████╗ ██╗     \n", "1;32");
            std::cout << terminal::paint("  ╚══███╔╝██║████╗  ██║██╔══██╗██║     \n", "1;32");
            std::cout << terminal::paint("    ███╔╝ ██║██╔██╗ ██║███████║██║     \n", "1;32");
            std::cout << terminal::paint("   ███╔╝  ██║██║╚██╗██║██╔══██║██║     \n", "1;32");
            std::cout << terminal::paint("  ███████╗██║██║ ╚████║██║  ██║███████╗\n", "1;32");
            std::cout << terminal::paint("  ╚══════╝╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝\n", "1;32");
            std::cout << "\nAsansör kapandı. Binanın soğuk havası geride kaldı.\n";
        } else {
            std::cout << terminal::paint("  ██████╗  ██████╗ ██╗   ██╗███████╗\n", "1;31");
            std::cout << terminal::paint("  ██╔══██╗██╔═══██╗██║   ██║██╔════╝\n", "1;31");
            std::cout << terminal::paint("  ██║  ██║██║   ██║██║   ██║█████╗  \n", "1;31");
            std::cout << terminal::paint("  ██║  ██║██║   ██║╚██╗ ██╔╝██╔══╝  \n", "1;31");
            std::cout << terminal::paint("  ██████╔╝╚██████╔╝ ╚████╔╝ ███████╗\n", "1;31");
            std::cout << terminal::paint("  ╚═════╝  ╚═════╝   ╚═══╝  ╚══════╝\n", "1;31");
            std::cout << "\nKoridor seni yuttu. Bir sonraki nöbetçiye yalnızca sessizlik kaldı.\n";
        }
        std::cout << "\nİlerleme: " << currentRoom_ << "/" << targetDoors_ << " kapı\n";
        std::cout << "Toplam tur: " << turns_ << "\n";
        std::cout << "Tohum: " << seed_ << "\n";
        const std::size_t first = notices_.size() > 3 ? notices_.size() - 3 : 0;
        for (std::size_t i = first; i < notices_.size(); ++i) {
            std::cout << "• " << notices_[i] << "\n";
        }
    }

    std::string inventorySummary() const {
        std::ostringstream output;
        bool first = true;
        const std::vector<Item> order = {Item::BrassKey, Item::Lockpick, Item::Bandage, Item::Tonic,
                                         Item::Lighter, Item::Adrenaline, Item::Fuse, Item::Coin};
        for (const Item item : order) {
            const int amount = count(item);
            if (amount == 0) {
                continue;
            }
            if (!first) {
                output << ", ";
            }
            first = false;
            output << itemName(item) << " x" << amount;
        }
        if (first) {
            return "boş";
        }
        return output.str();
    }

    void showHelp() {
        say("`bak`: odayı tekrar incele. `ara`: odayı ve dolapları ara.");
        say("`kapıyı aç` / `ileri`: sonraki kapıya geç. `geri`: geldiğin odaya dön.");
        say("`saklan`: dolaba gir. `çık`: saklandığın yerden çık. `bekle`: zamanı ilerlet.");
        say("`çanta`: eşyaları göster. `durum`: can ve akıl değerlerini göster.");
        say("`kullan <eşya>`: bandaj, sakinleştirici, maymuncuk veya çakmak kullan.");
        say("`dinle`: kapının arkasındaki sesleri kontrol et; yankısız kapılara dikkat et.");
        say("`dinlen`: revire denk gelirsen can/akıl toparla.");
        say("`çöz <kod>`: sayı kilidini aç; `kullan sigorta`: sigorta panelini çalıştır.");
        say("`satın al <eşya>`: servis arabasında jeton karşılığı yardım malzemesi al.");
        say("`kaydet` / `yükle`: oyunu sakla. Kısa komutlar: help, look, search, open, hide, leave, use, quit.");
    }

    void showInventory() {
        say("Çanta: " + inventorySummary() + ".");
        if (count(Item::Lighter) > 0) {
            say("Çakmak tekrar tekrar kullanılabilir; karanlık odaları aydınlatır.");
        }
    }

    void showStatus() {
        say("Can " + std::to_string(player_.health) + "/100, akıl " + std::to_string(player_.sanity) + "/100, tur " + std::to_string(turns_) + ".");
        say("Amaç: " + std::to_string(targetDoors_ - currentRoom_ + 1) + " kapı daha ilerideki servis asansörüne ulaşmak.");
    }

    void look() {
        const Room& room = currentRoom();
        say(room.description);
        if (room.number < targetDoors_ && rooms_[static_cast<std::size_t>(room.number + 1)].locked) {
            say("İlerideki kilidin metal yuvası ışığı yakalıyor. Anahtarın yakında olmalı.");
        }
        if (room.hasCloset) {
            say("Duvar boyunca içine sığabileceğin eski bir dolap var.");
        }
        if (room.searched) {
            say("Bu odayı zaten aradın.");
        } else {
            say("Burada gözden kaçmış bir şey olabilir; `ara` komutunu dene.");
        }
    }

    void searchRoom() {
        if (hidden_) {
            say("Dolabın içinden eşya arayamazsın. Önce `çık`.");
            return;
        }
        Room& room = currentRoom();
        if (room.dark && !room.lit) {
            if (count(Item::Lighter) == 0) {
                disturb(12, "Karanlıkta el yordamıyla ararken zihnin dağılıyor.");
            } else {
                say("Karanlıkta hiçbir ayrıntı seçemiyorsun. Çakmağı kullanmayı dene.");
                return;
            }
        }
        if (room.searched) {
            say("Çekmeceler boş; burada daha önce her şeyi topladın.");
            return;
        }

        room.searched = true;
        if (room.loot.empty()) {
            say("Rafların ve çekmecelerin içi boş. Yalnızca nemli kâğıt kokusu kaldı.");
        } else {
            std::map<Item, int> found;
            for (const Item item : room.loot) {
                ++found[item];
                add(item);
            }
            room.loot.clear();
            audio_.play(Sound::Pickup);
            std::ostringstream message;
            message << "Buldukların: ";
            bool first = true;
            for (const auto& [item, amount] : found) {
                if (!first) {
                    message << ", ";
                }
                first = false;
                message << itemName(item) << " x" << amount;
            }
            message << ".";
            say(message.str());
        }

        if (!room.clue.empty() && !room.clueRead) {
            room.clueRead = true;
            say(room.clue);
        }
        if (rollPercent(12)) {
            disturb(8, "Bir çekmece kapanmadan önce içeriden sana benzeyen bir ses geliyor.");
        }
    }

    void listen() {
        Room& room = currentRoom();
        if (room.falseDoor && !room.falseDoorSeen) {
            room.falseDoorSeen = true;
            audio_.play(Sound::Danger);
            say("Kapının arkasında hiç yankı yok. Levha doğru görünse de bu bir yanıltıcı kapı; önce dikkat et.");
            return;
        }
        if (room.number == targetDoors_) {
            say("Asansör boşluğundan ince bir motor sesi geliyor. Çıkış gerçekten burada olabilir.");
            return;
        }
        if (rooms_[static_cast<std::size_t>(room.number + 1)].locked) {
            say("Kapının arkasından tek bir metal tıkırtı geliyor; kilidin anahtarı çok uzakta değil.");
        } else if (room.dark && !room.lit) {
            say("Karanlıkta bir şey sürünüyor gibi, ama sesin yönünü ayırt edemiyorsun.");
        } else {
            say("İleride havalandırmanın ritmik sesi var. Başka hiçbir şey duymuyorsun.");
        }
    }

    void buyItem(const std::vector<std::string>& tokens) {
        if (!currentRoom().hasShop) {
            say("Bu odada çalışan bir servis arabası yok.");
            return;
        }
        const std::size_t itemIndex = tokens.size() > 1 && tokens[1] == "al" ? 2 : 1;
        if (tokens.size() <= itemIndex) {
            say("Satın alabileceğin eşyalar: bandaj (3), tonik (4), maymuncuk (5), sigorta (2) jeton.");
            return;
        }
        const auto item = itemFromWord(tokens[itemIndex]);
        if (!item.has_value() || (*item != Item::Bandage && *item != Item::Tonic &&
                                  *item != Item::Lockpick && *item != Item::Fuse)) {
            say("Bu servis arabasında yalnızca bandaj, tonik, maymuncuk veya sigorta var.");
            return;
        }

        int price = 0;
        switch (*item) {
        case Item::Bandage:
            price = 3;
            break;
        case Item::Tonic:
            price = 4;
            break;
        case Item::Lockpick:
            price = 5;
            break;
        case Item::Fuse:
            price = 2;
            break;
        default:
            break;
        }
        if (!take(Item::Coin, price)) {
            say("Yeterli jetonun yok. Bu eşya " + std::to_string(price) + " jeton.");
            return;
        }
        add(*item);
        audio_.play(Sound::Pickup);
        say(itemName(*item) + " aldın. Ödenen: " + std::to_string(price) + " jeton.");
    }

    void rest() {
        if (hidden_) {
            say("Dolapta uyumaya çalışmak güvenli değil. Önce `çık`.");
            return;
        }
        if (currentRoom().kind != RoomKind::Infirmary) {
            say("Burada dinlenmek için temiz bir yer yok. Revir bulmalısın.");
            return;
        }
        if (threat_ != Threat::None) {
            say("Tehlike varken dinlenemezsin; saklanmalısın.");
            return;
        }
        const int oldHealth = player_.health;
        const int oldSanity = player_.sanity;
        player_.health = std::min(100, player_.health + 12);
        player_.sanity = std::min(100, player_.sanity + 8);
        audio_.play(Sound::Ambience);
        say("Muayene yatağında kısa bir nefes aldın. Can +" + std::to_string(player_.health - oldHealth) +
            ", akıl +" + std::to_string(player_.sanity - oldSanity) + ".");
    }

    void openDoor() {
        if (hidden_) {
            say("Dolabın içinden kapı açamazsın. Önce `çık`.");
            return;
        }
        Room& room = currentRoom();
        if (threat_ == Threat::Figure) {
            say("Kör nöbetçi hâlâ burada. Kapıya uzanırsan çıkardığın sesi duyar; önce `saklan`.");
            return;
        }
        if (room.number == targetDoors_) {
            phase_ = Phase::Won;
            audio_.play(Sound::Victory);
            say("Asansör düğmesine bastın. Kapılar kapanıyor...");
            return;
        }
        if (room.falseDoor && !room.falseDoorSeen) {
            room.falseDoorSeen = true;
            audio_.play(Sound::Danger);
            disturb(10, "Kapının kolu elinin altında soğuk bir nabız gibi atıyor.");
            hurt(12, "Yanıltıcı kapının geri tepmesi");
            if (phase_ == Phase::Dead) {
                return;
            }
            say("Kapı gerçek değilmiş. Dikkat ederek tekrar denersen geçebilirsin.");
            return;
        }
        if (room.puzzle != PuzzleType::None && !room.puzzleSolved) {
            say(puzzleName(room.puzzle) + " çözülmeden kapı açılmıyor.");
            return;
        }
        if (room.locked) {
            if (count(Item::BrassKey) > 0) {
                take(Item::BrassKey);
                room.locked = false;
                audio_.play(Sound::Key);
                say("Pirinç anahtar kilitte dönüyor. Kapı açıldı; anahtar içeride kırıldı.");
            } else if (count(Item::Lockpick) > 0) {
                say("Bu kapıyı maymuncukla açabilirsin: `kullan maymuncuk`.");
                return;
            } else {
                say("Kilit dönmüyor. Önceki odayı tekrar aramak için `geri` yaz.");
                return;
            }
        }

        ++currentRoom_;
        audio_.play(Sound::Door);
        terminal::doorTransition();
        Room& next = currentRoom();
        if (!next.visited) {
            next.visited = true;
            say("Kapı " + std::to_string(next.number) + " açıldı: " + next.name + ".");
            enterRoomEvent(next);
        } else {
            say("Daha önce geçtiğin " + next.name + " odasına geri döndün.");
        }
    }

    void goBack() {
        if (hidden_) {
            say("Dolabın içinden geri dönemezsin. Önce `çık`.");
            return;
        }
        if (threat_ == Threat::Figure) {
            say("Kör nöbetçi geri dönüş sesini de duyar. Sessizce `saklan`.");
            return;
        }
        if (currentRoom_ <= 1) {
            say("Resepsiyonun gerisine giden bir yol yok.");
            return;
        }
        --currentRoom_;
        say("Sessizce geri döndün: " + currentRoom().name + ".");
    }

    void enterRoomEvent(Room& room) {
        if (room.figureRoom && !room.figureCleared) {
            room.eventSeen = true;
            threat_ = Threat::Figure;
            threatTurns_ = 3;
            figureNoise_ = 0;
            audio_.play(Sound::Danger);
            terminal::dangerFlash();
            say(room.number == 50
                    ? "Kapı 50: kör nöbetçi salona girdi. Görmüyor, fakat en küçük sesi bile duyuyor."
                    : "Kapı 100: son nöbetçi uyandı. Işığa değil, çıkardığın sese geliyor.");
            say("Sessiz kalmak için hemen `saklan` yaz. Kapıya uzanmak veya oyalanmak onu üzerine çeker.");
            return;
        }
        if (room.eventSeen || room.number == targetDoors_) {
            return;
        }
        room.eventSeen = true;
        audio_.play(Sound::Ambience);

        if (room.number == chaseDoor_ && chaseStage_ == 0) {
            chaseStage_ = 1;
            startChase();
            return;
        }
        if (room.number == secondChaseDoor_ && chaseStage_ == 1) {
            chaseStage_ = 2;
            startChase();
            return;
        }

        int chance = room.dark ? 25 : 16;
        if (difficulty_ == Difficulty::Relaxed) {
            chance -= 7;
        } else if (difficulty_ == Difficulty::Nightmare) {
            chance += 9;
        }
        if (rollPercent(chance)) {
            if (rollPercent(55)) {
                threat_ = Threat::Rattle;
                say("Koridorda ağır bir şey sürükleniyor. Kapı kolu titriyor; iki tur içinde saklan.");
            } else {
                threat_ = Threat::Whisper;
                say("Duvarın içinden fısıltılar yükseliyor. Seni bulmadan bir dolaba gir.");
            }
            audio_.play(Sound::Danger);
            terminal::dangerFlash();
            threatTurns_ = 2;
        } else if (room.kind == RoomKind::Archive && rollPercent(35)) {
            disturb(5, "Dosyaların arasındaki tarihler birbirini tutmuyor.");
        }
    }

    void hide() {
        if (hidden_) {
            say("Zaten bir dolabın içindesin.");
            return;
        }
        if (!currentRoom().hasCloset) {
            say("Bu odada saklanacak sağlam bir yer yok.");
            if (threat_ != Threat::None) {
                advanceThreat(threat_);
            }
            return;
        }

        hidden_ = true;
        audio_.play(Sound::Hide);
        if (threat_ != Threat::None) {
            const Threat encountered = threat_;
            int hideFailureChance = encountered == Threat::Figure ? 22 : 15;
            if (difficulty_ == Difficulty::Relaxed) {
                hideFailureChance = encountered == Threat::Figure ? 11 : 7;
            } else if (difficulty_ == Difficulty::Nightmare) {
                hideFailureChance = encountered == Threat::Figure ? 32 : 25;
            }
            if (rollPercent(hideFailureChance)) {
                if (encountered == Threat::Figure) {
                    hidden_ = false;
                    ++figureNoise_;
                    say("Kör nöbetçi nefesini duydu; kapak kapanmadan geri çekildin!");
                    hurt(44, "Kör nöbetçinin duyduğu ses");
                    if (phase_ == Phase::Dead) {
                        return;
                    }
                    threatTurns_ = 2;
                    say("Hâlâ burada. Daha sessiz bir hamleyle tekrar saklan.");
                    return;
                }
                say("Dolabın kapısı tam kapanmıyor. Dışarıdaki şey seni fark etti!");
                hurt(28, "Yakalanma");
            } else if (encountered == Threat::Figure) {
                say("Karanlıkta kıpırdamadan kaldın. Kör nöbetçi sesi bulamayınca uzaklaştı.");
            } else {
                say("Dolabın içinde nefesini tuttun. Ağır adımlar geçip gitti.");
            }
            threat_ = Threat::None;
            threatTurns_ = 0;
            if (encountered == Threat::Figure) {
                figureNoise_ = 0;
                currentRoom().figureCleared = true;
                say("Salon yeniden sessiz. Şimdi kapıya ilerleyebilirsin.");
            }
        } else {
            say("Dolaba girdin. Bir süre bekleyebilirsin.");
        }
    }

    void leaveHide() {
        if (!hidden_) {
            say("Şu anda saklanmıyorsun.");
            return;
        }
        hidden_ = false;
        say("Dolaptan çıktın.");
    }

    std::optional<Item> itemFromWord(const std::string& word) const {
        const std::string value = lowerAscii(word);
        if (value == "key" || value == "anahtar" || value == "pirinc" || value == "pirinç") {
            return Item::BrassKey;
        }
        if (value == "lockpick" || value == "maymuncuk" || value == "pick") {
            return Item::Lockpick;
        }
        if (value == "bandage" || value == "bandaj") {
            return Item::Bandage;
        }
        if (value == "tonic" || value == "sakinlestirici" || value == "sakinleştirici" || value == "ilac" || value == "ilaç") {
            return Item::Tonic;
        }
        if (value == "lighter" || value == "cakmak" || value == "çakmak") {
            return Item::Lighter;
        }
        if (value == "adrenaline" || value == "adrenalin") {
            return Item::Adrenaline;
        }
        if (value == "fuse" || value == "sigorta") {
            return Item::Fuse;
        }
        if (value == "coin" || value == "jeton") {
            return Item::Coin;
        }
        return std::nullopt;
    }

    void solvePuzzle(const std::vector<std::string>& tokens) {
        if (hidden_) {
            say("Dolabın içinden paneli çözemezsin. Önce `çık`.");
            return;
        }
        Room& room = currentRoom();
        if (room.puzzle == PuzzleType::None) {
            say("Bu odada çözülmesi gereken bir panel yok.");
            return;
        }
        if (room.puzzleSolved) {
            say("Panel zaten çözüldü; kapı mekanizması hazır.");
            return;
        }
        if (room.puzzle == PuzzleType::FuseBox) {
            say("Panelin kapağı sıkışmış. Bir sigortayı yerleştirmek için `kullan sigorta` yaz.");
            return;
        }
        if (tokens.size() < 2) {
            say("Üç haneli kodu yaz: `çöz 314`.");
            return;
        }

        std::string expected;
        for (const int digit : room.puzzleCode) {
            expected += static_cast<char>('0' + digit);
        }
        if (tokens[1] == expected) {
            room.puzzleSolved = true;
            audio_.play(Sound::Puzzle);
            say("Rakamlar yeşile döndü. Sayı kilidi açıldı.");
        } else {
            audio_.play(Sound::Error);
            disturb(6, "Sayı kilidi yanlış kodda keskin bir ses çıkarıyor.");
            say("Kod yanlış. Odanın ipucunu `ara` komutuyla bulmayı dene.");
        }
    }

    void useItem(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            say("Kullanılacak eşyayı yaz: `kullan bandaj`, `kullan çakmak` gibi.");
            return;
        }
        const auto item = itemFromWord(tokens[1]);
        if (!item.has_value()) {
            say("Bu eşyayı tanımıyorum. `çanta` ile çantana bak.");
            return;
        }

        switch (*item) {
        case Item::Bandage:
            if (!take(Item::Bandage)) {
                say("Çantanda bandaj yok.");
            } else {
                player_.health = std::min(100, player_.health + 30);
                say("Bandajı sardın. Canın 30 arttı.");
            }
            break;
        case Item::Tonic:
            if (!take(Item::Tonic)) {
                say("Çantanda sakinleştirici yok.");
            } else {
                player_.sanity = std::min(100, player_.sanity + 25);
                say("Sakinleştiricinin acı tadı düşüncelerini toparlıyor.");
            }
            break;
        case Item::Adrenaline:
            if (!take(Item::Adrenaline)) {
                say("Çantanda adrenalin yok.");
            } else {
                player_.health = std::min(100, player_.health + 15);
                player_.sanity = std::max(0, player_.sanity - 5);
                say("Adrenalin damarlarını yakıyor. Canın 15 arttı; aklın biraz sarsıldı.");
            }
            break;
        case Item::Lighter:
            if (count(Item::Lighter) == 0) {
                say("Çakmağın yok.");
            } else if (!currentRoom().dark) {
                say("Çakmağı yakıyorsun, ama bu oda zaten aydınlık.");
            } else {
                currentRoom().lit = true;
                audio_.play(Sound::Lighter);
                say("Çakmağı yaktın. Gölgelere saklanmış ayrıntılar görünür oldu.");
            }
            break;
        case Item::Lockpick:
            if (!take(Item::Lockpick)) {
                say("Çantanda maymuncuk yok.");
            } else if (!currentRoom().locked) {
                add(Item::Lockpick);
                say("Burada açılacak bir kilit yok; maymuncuğu geri koydun.");
            } else {
                currentRoom().locked = false;
                say("Maymuncuk kırıldı, ama kilit açıldı.");
            }
            break;
        case Item::BrassKey:
            if (currentRoom().locked && take(Item::BrassKey)) {
                currentRoom().locked = false;
                audio_.play(Sound::Key);
                say("Anahtarı kilide soktun. İlerideki kapı artık açık.");
            } else {
                say("Anahtar ancak bulunduğun odanın kilitli kapısında işe yarar.");
            }
            break;
        case Item::Fuse:
            if (!take(Item::Fuse)) {
                say("Çantanda sigorta yok.");
            } else if (currentRoom().puzzle != PuzzleType::FuseBox || currentRoom().puzzleSolved) {
                add(Item::Fuse);
                say("Bu odada sigorta takılacak bir panel yok; sigortayı geri aldın.");
            } else {
                currentRoom().puzzleSolved = true;
                audio_.play(Sound::Puzzle);
                say("Sigortayı panele taktın. Lambalar bir an parladı ve kapı mekanizması açıldı.");
            }
            break;
        case Item::Coin:
            if (threat_ != Threat::Figure) {
                say("Jetonları servis arabasında harcayabilirsin; burada şimdilik sakla.");
            } else if (!take(Item::Coin)) {
                say("Çantanda dikkat dağıtacak jeton yok.");
            } else {
                figureNoise_ = 0;
                threatTurns_ = std::min(3, threatTurns_ + 1);
                audio_.play(Sound::Danger);
                say("Jetonu karanlığa yuvarladın. Kör nöbetçi sesi başka yöne çevirdi; şimdi saklan.");
            }
            break;
        }
    }

    int tunedDamage(int amount) const {
        if (difficulty_ == Difficulty::Relaxed) {
            return std::max(1, amount * 3 / 4);
        }
        if (difficulty_ == Difficulty::Nightmare) {
            return std::max(1, amount * 4 / 3);
        }
        return amount;
    }

    int tunedDisturbance(int amount) const {
        if (difficulty_ == Difficulty::Relaxed) {
            return std::max(1, amount * 3 / 4);
        }
        if (difficulty_ == Difficulty::Nightmare) {
            return std::max(1, amount * 5 / 4);
        }
        return amount;
    }

    void advanceThreat(Threat expected) {
        if (threat_ != expected || threat_ == Threat::None) {
            return;
        }
        if (threat_ == Threat::Figure && !hidden_) {
            ++figureNoise_;
        }
        --threatTurns_;
        if (threatTurns_ > 0) {
            if (threat_ == Threat::Figure && !hidden_) {
                say("Kör nöbetçi çıkardığın sesi izliyor; " + std::to_string(threatTurns_) + " hamle kaldı.");
            } else {
                say("Tehlike yaklaşıyor; çok az vaktin kaldı.");
            }
            return;
        }

        if (threat_ == Threat::Rattle) {
            hurt(35, "Gürültünün çarpması");
            disturb(8, "Gürültü zihninde uzun süre yankılanıyor.");
        } else if (threat_ == Threat::Whisper) {
            hurt(16, "Fısıltının gölgesi");
            disturb(18, "Fısıltılar düşüncelerinin arasına yerleşiyor.");
        } else {
            if (!hidden_) {
                hurt(52, "Kör nöbetçinin duyduğu ses");
                disturb(14, "Nöbetçinin ayak sesleri zihninde yankılanıyor.");
            } else {
                say("Nöbetçi çevrende dolaştı ama seni duyamadı.");
            }
            threat_ = Threat::None;
            threatTurns_ = 0;
            currentRoom().figureCleared = true;
            if (phase_ != Phase::Dead) {
                say("Kör nöbetçi sesin izini kaybedip karanlığa çekildi.");
            }
            return;
        }
        threat_ = Threat::None;
        threatTurns_ = 0;
    }

    void hurt(int amount, const std::string& reason) {
        const int actualDamage = tunedDamage(amount);
        player_.health = std::max(0, player_.health - actualDamage);
        audio_.play(Sound::Hit);
        say(reason + ": " + std::to_string(actualDamage) + " hasar aldın.");
        if (player_.health <= 0) {
            phase_ = Phase::Dead;
            audio_.play(Sound::Death);
            say("Gözlerin kapanıyor.");
        }
    }

    void disturb(int amount, const std::string& reason) {
        const int actualDisturbance = tunedDisturbance(amount);
        player_.sanity = std::max(0, player_.sanity - actualDisturbance);
        say(reason + " Akıl -" + std::to_string(actualDisturbance) + ".");
        if (player_.sanity == 0 && phase_ == Phase::Explore) {
            say("Panik nefesini kesiyor; duvarlar hareket ediyor gibi.");
            hurt(20, "Panik");
            if (phase_ != Phase::Dead) {
                player_.sanity = 15;
            }
        }
    }

    std::string canonicalMove(const std::string& word) const {
        if (word == "sol" || word == "left") {
            return "sol";
        }
        if (word == "sag" || word == "sağ" || word == "right") {
            return "sag";
        }
        if (word == "atla" || word == "jump") {
            return "atla";
        }
        if (word == "egil" || word == "eğil" || word == "duck" || word == "slide") {
            return "egil";
        }
        return "";
    }

    std::string displayMove(const std::string& move) const {
        if (move == "sol") {
            return "SOL";
        }
        if (move == "sag") {
            return "SAĞ";
        }
        if (move == "atla") {
            return "ATLA";
        }
        return "EĞİL";
    }

    void startChase() {
        phase_ = Phase::Chase;
        chaseStep_ = 0;
        chasePattern_.clear();
        const std::vector<std::string> moves = {"sol", "sag", "atla", "egil"};
        int chaseLength = 6;
        if (difficulty_ == Difficulty::Relaxed) {
            chaseLength = 4;
        } else if (difficulty_ == Difficulty::Nightmare) {
            chaseLength = 8;
        }
        for (int i = 0; i < chaseLength; ++i) {
            chasePattern_.push_back(moves[static_cast<std::size_t>(rng_() % moves.size())]);
        }
        audio_.play(Sound::Chase);
        terminal::dangerFlash();
        say(chaseStage_ == 1
                ? "Işıklar söndü! Arkanda maskesi olmayan bir gölge belirdi."
                : "Gölge geri döndü; bu kez koridor daha dar ve çıkış daha uzakta.");
        say("Bu bir kovalamaca: ekranda gösterilen hareketi zamanında yaz.");
    }

    void handleChase(const std::vector<std::string>& tokens) {
        if (tokens.empty()) {
            return;
        }
        const std::string& command = tokens[0];
        if (command == "yardım" || command == "help") {
            say("Kovalamacada yalnızca ekranda gösterilen hareketi yaz: sol, sağ, atla veya eğil.");
            return;
        }
        if (command == "çıkış" || command == "quit" || command == "q") {
            say("Nöbet yarıda bırakıldı.");
            running_ = false;
            return;
        }
        if (command == "durum" || command == "status") {
            say("Can " + std::to_string(player_.health) + "/100. Sıra " + std::to_string(chaseStep_ + 1) + "/" + std::to_string(chasePattern_.size()) + ".");
            return;
        }

        const std::string move = canonicalMove(command);
        if (move.empty()) {
            say("Hareket tanınmadı. Şimdi `" + displayMove(chasePattern_[chaseStep_]) + "` yapmalısın.");
            return;
        }
        if (move == chasePattern_[chaseStep_]) {
            say("Doğru! Gölge duvara çarptı.");
        } else {
            hurt(22, "Kovalamacada yanlış yöne sapma");
            if (phase_ == Phase::Dead) {
                return;
            }
        }

        ++chaseStep_;
        if (chaseStep_ >= chasePattern_.size()) {
            phase_ = Phase::Explore;
            say("Dar bir servis kapısından geçip gölgeyi atlattın.");
        }
    }

    void saveGame() {
        if (phase_ != Phase::Explore || threat_ != Threat::None || hidden_) {
            say("Tehlike veya kovalamaca sırasında kayıt yapılamaz.");
            return;
        }

        std::ofstream file("nightshift.save", std::ios::trunc);
        if (!file) {
            say("Kayıt dosyası oluşturulamadı.");
            return;
        }
        file << "NIGHTSHIFT_SAVE 5\n";
        file << targetDoors_ << ' ' << seed_ << ' ' << currentRoom_ << ' '
             << player_.health << ' ' << player_.sanity << ' ' << turns_ << ' ' << chaseStage_ << '\n';
        for (int item = static_cast<int>(Item::BrassKey); item <= static_cast<int>(Item::Coin); ++item) {
            file << count(static_cast<Item>(item)) << ' ';
        }
        file << '\n';
        for (int number = 1; number <= targetDoors_; ++number) {
            const Room& room = rooms_[static_cast<std::size_t>(number)];
            file << room.visited << ' ' << room.searched << ' ' << room.lit << ' '
                 << room.locked << ' ' << room.eventSeen << ' ' << room.clueRead << ' '
                 << room.puzzleSolved << ' ' << room.falseDoorSeen << ' ' << room.figureCleared << '\n';
        }
        say("Oyun `nightshift.save` dosyasına kaydedildi.");
    }

    void loadGame() {
        std::ifstream file("nightshift.save");
        if (!file) {
            say("`nightshift.save` bulunamadı.");
            return;
        }
        std::string header;
        int version = 0;
        if (!(file >> header >> version) || header != "NIGHTSHIFT_SAVE" || version < 1 || version > 5) {
            say("Kayıt dosyasının biçimi tanınmıyor.");
            return;
        }

        int savedTarget = 0;
        std::uint64_t savedSeed = 0;
        int savedRoom = 1;
        int savedHealth = 100;
        int savedSanity = 100;
        int savedTurns = 0;
        int savedChaseStage = 0;
        if (!(file >> savedTarget >> savedSeed >> savedRoom >> savedHealth >> savedSanity >> savedTurns) ||
            (version >= 4 && !(file >> savedChaseStage)) ||
            savedTarget != targetDoors_ || savedSeed != seed_ || savedRoom < 1 || savedRoom > targetDoors_) {
            say("Kayıt başka bir haritaya ait; bu oyunda yüklenemez.");
            return;
        }
        if (version < 4) {
            savedChaseStage = savedRoom > secondChaseDoor_ ? 2 : (savedRoom > chaseDoor_ ? 1 : 0);
        }

        std::map<Item, int> savedInventory;
        for (int item = static_cast<int>(Item::BrassKey); item <= static_cast<int>(Item::Coin); ++item) {
            int amount = 0;
            if (!(file >> amount) || amount < 0 || amount > 999) {
                say("Kayıt dosyası bozuk.");
                return;
            }
            savedInventory[static_cast<Item>(item)] = amount;
        }

        for (int number = 1; number <= targetDoors_; ++number) {
            int visited = 0;
            int searched = 0;
            int lit = 0;
            int locked = 0;
            int eventSeen = 0;
            int clueRead = 0;
            int puzzleSolved = 0;
            int falseDoorSeen = 0;
            int figureCleared = 0;
            if (!(file >> visited >> searched >> lit >> locked >> eventSeen >> clueRead) ||
                (version >= 2 && !(file >> puzzleSolved)) ||
                (version >= 3 && !(file >> falseDoorSeen)) ||
                (version >= 5 && !(file >> figureCleared))) {
                say("Kayıt dosyası eksik.");
                return;
            }
            Room& room = rooms_[static_cast<std::size_t>(number)];
            room.visited = visited != 0;
            room.searched = searched != 0;
            room.lit = lit != 0;
            room.locked = locked != 0;
            room.eventSeen = eventSeen != 0;
            room.clueRead = clueRead != 0;
            room.puzzleSolved = version >= 2 && puzzleSolved != 0;
            room.falseDoorSeen = version >= 3 && falseDoorSeen != 0;
            room.figureCleared = version >= 5 && figureCleared != 0;
            if (room.searched) {
                room.loot.clear();
            }
        }

        player_.inventory.clear();
        for (const auto& [item, amount] : savedInventory) {
            if (amount > 0) {
                player_.inventory[item] = amount;
            }
        }
        player_.health = std::clamp(savedHealth, 0, 100);
        player_.sanity = std::clamp(savedSanity, 0, 100);
        turns_ = std::max(0, savedTurns);
        currentRoom_ = savedRoom;
        chaseStage_ = std::clamp(savedChaseStage, 0, 2);
        threat_ = Threat::None;
        threatTurns_ = 0;
        hidden_ = false;
        phase_ = Phase::Explore;
        say("Kayıt yüklendi. Kapı " + std::to_string(currentRoom_) + " konumundasın.");
    }

    void processCommand(const std::string& line) {
        const std::vector<std::string> tokens = tokenize(line);
        if (tokens.empty()) {
            return;
        }
        const std::string& command = tokens[0];

        if (command == "çıkış" || command == "quit" || command == "q") {
            say("Nöbet yarıda bırakıldı.");
            running_ = false;
            return;
        }

        if (phase_ == Phase::Chase) {
            handleChase(tokens);
            return;
        }
        if (phase_ != Phase::Explore) {
            return;
        }

        const Threat threatBefore = threat_;
        bool consumesTime = true;

        if (command == "yardım" || command == "help" || command == "?") {
            showHelp();
            consumesTime = false;
        } else if (command == "bak" || command == "look" || command == "incele") {
            look();
            consumesTime = false;
        } else if (command == "ara" || command == "search" || command == "inceleoda") {
            searchRoom();
        } else if (command == "kapıyı" || command == "kapi" || command == "open" || command == "aç" || command == "ac" || command == "ileri" || command == "go" || command == "move" || command == "door") {
            openDoor();
        } else if (command == "geri" || command == "back" || command == "dön" || command == "don") {
            goBack();
        } else if (command == "saklan" || command == "hide" || command == "dolap") {
            hide();
            consumesTime = false;
        } else if (command == "çık" || command == "cik" || command == "leave" || command == "getout") {
            leaveHide();
            consumesTime = false;
        } else if (command == "çanta" || command == "canta" || command == "inventory" || command == "inv") {
            showInventory();
            consumesTime = false;
        } else if (command == "durum" || command == "status") {
            showStatus();
            consumesTime = false;
        } else if (command == "kullan" || command == "use") {
            useItem(tokens);
        } else if (command == "dinle" || command == "listen") {
            listen();
        } else if (command == "dinlen" || command == "rest") {
            rest();
        } else if (command == "çöz" || command == "coz" || command == "solve" || command == "kod" || command == "panel") {
            solvePuzzle(tokens);
        } else if (command == "satın" || command == "satin" || command == "buy" || command == "shop") {
            buyItem(tokens);
        } else if (command == "bekle" || command == "wait") {
            say("Bir an bekledin. Binanın sesi değişti.");
        } else if (command == "kaydet" || command == "save") {
            saveGame();
            consumesTime = false;
        } else if (command == "yükle" || command == "yukle" || command == "load") {
            loadGame();
            consumesTime = false;
        } else {
            say("Bu komutu anlayamadım. `yardım` yazarak seçenekleri görebilirsin.");
            consumesTime = false;
        }

        if (consumesTime) {
            ++turns_;
        }
        if (threatBefore != Threat::None && consumesTime && threat_ == threatBefore && phase_ == Phase::Explore) {
            advanceThreat(threatBefore);
        }
    }
};

int main(int argc, char** argv) {
    int doors = 30;
    bool audioEnabled = true;
    Difficulty difficulty = Difficulty::Standard;
    std::uint64_t seed = static_cast<std::uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help" || argument == "-h") {
            std::cout << "NÖBET // Uzun Koridor\n\n"
                      << "Kullanım: nightshift [--doors SAYI] [--seed SAYI]\n"
                      << "  --doors SAYI  Haritadaki kapı sayısı (12-100, varsayılan 30)\n"
                      << "  --seed SAYI   Aynı prosedürel haritayı yeniden üret\n"
                      << "  --difficulty  rahat|standart|kabus (varsayılan standart)\n"
                      << "  --no-audio     Ses ipuçlarını kapat\n";
            return 0;
        }
        if (argument == "--doors" && index + 1 < argc) {
            if (const auto value = parseInt(argv[++index]); value.has_value()) {
                doors = *value;
            }
            continue;
        }
        if (argument == "--seed" && index + 1 < argc) {
            try {
                seed = std::stoull(argv[++index]);
            } catch (...) {
                std::cerr << "Geçersiz seed değeri.\n";
                return 2;
            }
            continue;
        }
        if (argument == "--difficulty" && index + 1 < argc) {
            const auto value = parseDifficulty(argv[++index]);
            if (!value.has_value()) {
                std::cerr << "Geçersiz zorluk. rahat, standart veya kabus kullan.\n";
                return 2;
            }
            difficulty = *value;
            continue;
        }
        if (argument == "--no-audio") {
            audioEnabled = false;
            continue;
        }
        std::cerr << "Bilinmeyen seçenek: " << argument << " (yardım için --help)\n";
        return 2;
    }

    Game game(doors, seed, audioEnabled, difficulty);
    game.run();
    return 0;
}
