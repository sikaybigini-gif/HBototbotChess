#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using Socket = SOCKET;
constexpr Socket InvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using Socket = int;
constexpr Socket InvalidSocket = -1;
#endif

// NÖBET: UZUN KORİDOR - CO-OP SERVER
// A small authoritative HTTP server. It intentionally uses original names,
// text, visuals, and mechanics rather than copying Roblox or DOORS code.

namespace net {

void closeSocket(Socket socket) {
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

bool sendAll(Socket socket, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
#ifdef _WIN32
        const int result = send(socket, data.data() + sent, static_cast<int>(data.size() - sent), 0);
#else
        const ssize_t result = send(socket, data.data() + sent, data.size() - sent, 0);
#endif
        if (result <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(result);
    }
    return true;
}

} // namespace net

std::string lowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

int hexValue(char value) {
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    return -1;
}

std::string urlDecode(const std::string& value) {
    std::string decoded;
    decoded.reserve(value.size());
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (value[index] == '+') {
            decoded.push_back(' ');
        } else if (value[index] == '%' && index + 2 < value.size()) {
            const int high = hexValue(value[index + 1]);
            const int low = hexValue(value[index + 2]);
            if (high >= 0 && low >= 0) {
                decoded.push_back(static_cast<char>((high << 4) | low));
                index += 2;
            } else {
                decoded.push_back(value[index]);
            }
        } else {
            decoded.push_back(value[index]);
        }
    }
    return decoded;
}

std::map<std::string, std::string> parseParams(const std::string& encoded) {
    std::map<std::string, std::string> params;
    std::size_t start = 0;
    while (start <= encoded.size()) {
        const std::size_t end = encoded.find('&', start);
        const std::string part = encoded.substr(start, end == std::string::npos ? std::string::npos : end - start);
        const std::size_t equals = part.find('=');
        if (equals == std::string::npos) {
            if (!part.empty()) {
                params[urlDecode(part)] = "";
            }
        } else {
            params[urlDecode(part.substr(0, equals))] = urlDecode(part.substr(equals + 1));
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return params;
}

std::string jsonEscape(const std::string& value) {
    std::ostringstream output;
    for (const unsigned char character : value) {
        switch (character) {
        case '"':
            output << "\\\"";
            break;
        case '\\':
            output << "\\\\";
            break;
        case '\b':
            output << "\\b";
            break;
        case '\f':
            output << "\\f";
            break;
        case '\n':
            output << "\\n";
            break;
        case '\r':
            output << "\\r";
            break;
        case '\t':
            output << "\\t";
            break;
        default:
            if (character < 0x20) {
                output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                       << static_cast<int>(character) << std::dec << std::setfill(' ');
            } else {
                output << character;
            }
            break;
        }
    }
    return output.str();
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

std::string doorLabel(int number) {
    std::ostringstream output;
    output << std::setw(2) << std::setfill('0') << number;
    return output.str();
}

bool endsWith(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

enum class LobbyPhase {
    Waiting,
    Explore,
    Chase,
    Won,
    Dead,
};

enum class Threat {
    None,
    Rattle,
    Whisper,
};

enum class PuzzleType {
    None,
    FuseBox,
    NumberLock,
};

std::string phaseName(LobbyPhase phase) {
    switch (phase) {
    case LobbyPhase::Waiting:
        return "lobby";
    case LobbyPhase::Explore:
        return "explore";
    case LobbyPhase::Chase:
        return "chase";
    case LobbyPhase::Won:
        return "won";
    case LobbyPhase::Dead:
        return "dead";
    }
    return "lobby";
}

std::string threatKey(Threat threat) {
    switch (threat) {
    case Threat::Rattle:
        return "rattle";
    case Threat::Whisper:
        return "whisper";
    case Threat::None:
        return "none";
    }
    return "none";
}

std::string threatText(Threat threat) {
    switch (threat) {
    case Threat::Rattle:
        return "Gürültü";
    case Threat::Whisper:
        return "Fısıltı";
    case Threat::None:
        return "Yok";
    }
    return "Yok";
}

std::string puzzleKey(PuzzleType puzzle) {
    switch (puzzle) {
    case PuzzleType::FuseBox:
        return "fuse";
    case PuzzleType::NumberLock:
        return "number";
    case PuzzleType::None:
        return "none";
    }
    return "none";
}

struct RoomNet {
    int number = 0;
    std::string kind;
    std::string name;
    std::string description;
    std::string clue;
    std::string puzzleCode;
    PuzzleType puzzle = PuzzleType::None;
    bool dark = false;
    bool lit = false;
    bool locked = false;
    bool hasCloset = true;
    bool hasShop = false;
    bool teamDoor = false;
    std::set<std::string> openVotes;
    bool visited = false;
    bool searched = false;
    bool puzzleSolved = false;
    bool falseDoor = false;
    bool falseDoorSeen = false;
    std::vector<std::string> loot;
};

struct PlayerNet {
    std::string id;
    std::string name;
    bool host = false;
    bool ready = false;
    bool hidden = false;
    bool alive = true;
    int health = 100;
    int sanity = 100;
    std::map<std::string, int> items;
};

struct Lobby {
    std::string code;
    std::string hostId;
    int targetDoors = 20;
    std::uint64_t seed = 0;
    LobbyPhase phase = LobbyPhase::Waiting;
    int currentDoor = 1;
    std::vector<RoomNet> rooms;
    std::map<std::string, PlayerNet> players;
    std::vector<std::string> log;
    Threat threat = Threat::None;
    int threatTurns = 0;
    std::vector<std::string> chasePattern;
    std::size_t chaseStep = 0;
    int chaseStage = 0;
    std::uint64_t eventSeq = 0;
    std::string sound;
    std::mt19937 rng;
};

class GameState {
public:
    explicit GameState(int defaultDoors)
        : defaultDoors_(std::clamp(defaultDoors, 12, 50)),
          idCounter_(1),
          rng_(static_cast<std::mt19937::result_type>(
              std::chrono::high_resolution_clock::now().time_since_epoch().count())) {}

    std::string join(const std::map<std::string, std::string>& params) {
        std::lock_guard<std::mutex> guard(mutex_);
        const std::string name = cleanName(params.count("name") ? params.at("name") : "");
        std::string code = params.count("code") ? upperCode(params.at("code")) : "";
        Lobby* lobby = nullptr;

        if (code.empty()) {
            Lobby newLobby;
            newLobby.code = createLobbyCode();
            newLobby.targetDoors = defaultDoors_;
            if (params.count("doors")) {
                if (const auto doors = parseInt(params.at("doors")); doors.has_value()) {
                    newLobby.targetDoors = std::clamp(*doors, 12, 50);
                }
            }
            newLobby.seed = static_cast<std::uint64_t>(rng_());
            newLobby.rng.seed(static_cast<std::mt19937::result_type>(newLobby.seed));
            buildRooms(newLobby);
            auto inserted = lobbies_.emplace(newLobby.code, std::move(newLobby));
            lobby = &inserted.first->second;
            code = lobby->code;
        } else {
            const auto found = lobbies_.find(code);
            if (found == lobbies_.end()) {
                return errorJson("Bu lobi kodu bulunamadı.");
            }
            lobby = &found->second;
            if (lobby->phase != LobbyPhase::Waiting) {
                return errorJson("Bu asansör yolculuğu zaten başladı.");
            }
            if (lobby->players.size() >= 8) {
                return errorJson("Bu asansör en fazla 8 kişi alabilir.");
            }
        }

        PlayerNet player;
        player.id = "p" + std::to_string(idCounter_++);
        player.name = name;
        player.host = lobby->players.empty();
        player.items["lighter"] = 1;
        player.items["bandage"] = 1;
        player.items["coin"] = 2;
        if (player.host) {
            lobby->hostId = player.id;
        }
        const std::string playerId = player.id;
        lobby->players[player.id] = std::move(player);
        addLog(*lobby, name + " asansöre bindi.", "hide.wav");
        return stateJson(*lobby, playerId);
    }

    std::string state(const std::map<std::string, std::string>& params) const {
        std::lock_guard<std::mutex> guard(mutex_);
        const std::string code = upperCode(params.count("lobby") ? params.at("lobby") : "");
        const std::string player = params.count("player") ? params.at("player") : "";
        const auto found = lobbies_.find(code);
        if (found == lobbies_.end()) {
            return errorJson("Lobi bulunamadı.");
        }
        if (!found->second.players.count(player)) {
            return errorJson("Oyuncu oturumu bulunamadı.");
        }
        return stateJson(found->second, player);
    }

    std::string action(const std::map<std::string, std::string>& params) {
        std::lock_guard<std::mutex> guard(mutex_);
        const std::string code = upperCode(params.count("lobby") ? params.at("lobby") : "");
        const std::string playerId = params.count("player") ? params.at("player") : "";
        const std::string command = lowerAscii(params.count("action") ? params.at("action") : "");
        const std::string value = params.count("value") ? params.at("value") : "";
        const auto found = lobbies_.find(code);
        if (found == lobbies_.end()) {
            return errorJson("Lobi bulunamadı.");
        }
        Lobby& lobby = found->second;
        const auto playerFound = lobby.players.find(playerId);
        if (playerFound == lobby.players.end()) {
            return errorJson("Oyuncu oturumu bulunamadı.");
        }
        if (command == "leave_lobby") {
            return leaveLocked(code, playerId);
        }
        if (command == "chat") {
            std::string message = trim(value);
            if (message.empty()) {
                return stateJson(lobby, playerId);
            }
            if (message.size() > 180) {
                message.resize(180);
            }
            addLog(lobby, "[Sohbet] " + playerFound->second.name + ": " + message);
            return stateJson(lobby, playerId);
        }

        if (lobby.phase == LobbyPhase::Waiting) {
            handleLobbyAction(lobby, playerFound->second, command);
            return stateJson(lobby, playerId);
        }
        if (lobby.phase == LobbyPhase::Chase) {
            handleChaseAction(lobby, playerFound->second, command, value);
            return stateJson(lobby, playerId);
        }
        if (lobby.phase != LobbyPhase::Explore) {
            return stateJson(lobby, playerId);
        }

        handleExploreAction(lobby, playerFound->second, command, value);
        return stateJson(lobby, playerId);
    }

    std::string leave(const std::map<std::string, std::string>& params) {
        std::lock_guard<std::mutex> guard(mutex_);
        return leaveLocked(upperCode(params.count("lobby") ? params.at("lobby") : ""),
                           params.count("player") ? params.at("player") : "");
    }

private:
    int defaultDoors_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Lobby> lobbies_;
    std::uint64_t idCounter_;
    std::mt19937 rng_;

    static std::uint64_t mix(std::uint64_t value) {
        value += 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31U);
    }

    static std::string cleanName(std::string name) {
        name = trim(name);
        if (name.empty()) {
            return "Gezgin";
        }
        if (name.size() > 20) {
            name.resize(20);
        }
        for (char& character : name) {
            if (static_cast<unsigned char>(character) < 0x20) {
                character = ' ';
            }
        }
        return name;
    }

    static std::string upperCode(std::string code) {
        code = trim(code);
        std::transform(code.begin(), code.end(), code.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });
        return code;
    }

    std::string createLobbyCode() {
        static constexpr char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
        std::uniform_int_distribution<int> distribution(0, static_cast<int>(sizeof(alphabet) - 2));
        std::string code;
        do {
            code.clear();
            for (int index = 0; index < 4; ++index) {
                code.push_back(alphabet[distribution(rng_)]);
            }
        } while (lobbies_.count(code));
        return code;
    }

    static void addLoot(RoomNet& room, const std::string& item, int amount = 1) {
        for (int index = 0; index < amount; ++index) {
            room.loot.push_back(item);
        }
    }

    void buildRooms(Lobby& lobby) {
        lobby.rooms.assign(static_cast<std::size_t>(lobby.targetDoors + 1), RoomNet{});
        for (int number = 1; number <= lobby.targetDoors; ++number) {
            RoomNet room;
            room.number = number;
            if (number == 1) {
                room.kind = "lobby";
                room.name = "Kayıt Masası";
                room.description = "Tozlu resepsiyon masasındaki saat durmuş. Asansörün kapısı arkanda bekliyor.";
                room.hasCloset = false;
            } else if (number == lobby.targetDoors) {
                room.kind = "elevator";
                room.name = "Servis Asansörü";
                room.description = "Paslı servis asansörü titreşiyor. Kırmızı lamba, çıkışın hâlâ çalıştığını söylüyor.";
                room.hasCloset = false;
            } else {
                const std::uint64_t roomSeed = mix(lobby.seed ^ (static_cast<std::uint64_t>(number) * 0x632be59bd9b4e019ULL));
                switch (static_cast<int>(roomSeed % 5U)) {
                case 0:
                    room.kind = "corridor";
                    room.name = "Kadife Koridor";
                    room.description = "Duvar kâğıtlarının altında ince bir uğultu dolaşıyor. Halı ayak seslerini yutuyor.";
                    break;
                case 1:
                    room.kind = "guest";
                    room.name = "Terk Edilmiş Oda";
                    room.description = "Yatak örtüsü az önce düzeltilmiş gibi. Pencerenin ardında şehir ışığı yok.";
                    break;
                case 2:
                    room.kind = "archive";
                    room.name = "Tozlu Arşiv";
                    room.description = "Numarasız dosyalar raflarda bekliyor. Mektupların hiçbirinde gönderen yok.";
                    room.clue = "Dosyalardan biri şöyle diyor: Kapı sesi geldiğinde ışığa değil, saklanacak yere güven.";
                    break;
                case 3:
                    room.kind = "workshop";
                    room.name = "Bakım Odası";
                    room.description = "Bakır borular duvarların içinde öksürüyor. Zeminde eski yağ izleri var.";
                    break;
                default:
                    room.kind = "infirmary";
                    room.name = "Kapalı Revir";
                    room.description = "Çatlak camlı metal dolaplar ve kendi kendine yanan bir muayene lambası var.";
                    break;
                }

                room.hasCloset = room.kind != "workshop" || number % 2 == 0;
                room.hasShop = number % 10 == 0 || (room.kind == "infirmary" && number % 2 == 0);
                room.dark = number % 8 == 0 || (room.kind == "workshop" && number % 3 == 0);
                room.locked = number > 1 && number < lobby.targetDoors && number % 6 == 0;
                const int firstChase = std::clamp(lobby.targetDoors / 2, 5, lobby.targetDoors - 2);
                const int secondChase = std::clamp((lobby.targetDoors * 3) / 4, firstChase + 3, lobby.targetDoors - 1);
                room.teamDoor = number > 1 && number < lobby.targetDoors && number % 10 == 0 &&
                                number != firstChase && number != secondChase;
                if (!room.locked && number != firstChase && number != secondChase && number % 9 == 0) {
                    room.puzzle = PuzzleType::FuseBox;
                    room.clue = "Duvar paneli bir sigorta istiyor. Yedek sigortalar bakım odalarında olur.";
                    addLoot(room, "fuse");
                } else if (!room.locked && number != firstChase && number != secondChase && number % 11 == 0) {
                    room.puzzle = PuzzleType::NumberLock;
                    const int code = 100 + static_cast<int>(roomSeed % 900U);
                    room.puzzleCode = std::to_string(code);
                    room.clue = "Kapının yanındaki etikette üç rakam seçiliyor: " + room.puzzleCode + ".";
                }
                room.falseDoor = number > 3 && number < lobby.targetDoors && number % 17 == 0 &&
                                 number != firstChase && number != secondChase && room.puzzle == PuzzleType::None && !room.locked;

                std::mt19937 local(static_cast<std::mt19937::result_type>(roomSeed));
                const int lootRoll = static_cast<int>(local() % 100U);
                if (lootRoll < 82) {
                    addLoot(room, "coin", 1 + static_cast<int>(local() % 3U));
                }
                if (lootRoll % 7 == 0 || number % 11 == 4) {
                    addLoot(room, "lockpick");
                }
                if (room.kind == "infirmary" && lootRoll % 3 != 0) {
                    addLoot(room, "bandage");
                }
                if (room.kind == "workshop" && lootRoll % 4 == 0) {
                    addLoot(room, "adrenaline");
                }
                if (number % 13 == 7) {
                    addLoot(room, "tonic");
                }
            }
            lobby.rooms[static_cast<std::size_t>(number)] = std::move(room);
        }

        for (int number = 2; number <= lobby.targetDoors; ++number) {
            if (lobby.rooms[static_cast<std::size_t>(number)].locked) {
                addLoot(lobby.rooms[static_cast<std::size_t>(number - 1)], "key");
            }
        }
        lobby.rooms[1].visited = true;
    }

    static std::string itemLabel(const std::string& item) {
        if (item == "key") {
            return "Pirinç anahtar";
        }
        if (item == "lockpick") {
            return "Maymuncuk";
        }
        if (item == "bandage") {
            return "Bandaj";
        }
        if (item == "tonic") {
            return "Sakinleştirici";
        }
        if (item == "lighter") {
            return "Çakmak";
        }
        if (item == "adrenaline") {
            return "Adrenalin";
        }
        if (item == "fuse") {
            return "Sigorta";
        }
        if (item == "coin") {
            return "Jeton";
        }
        return item;
    }

    static int itemCount(const PlayerNet& player, const std::string& item) {
        const auto found = player.items.find(item);
        return found == player.items.end() ? 0 : found->second;
    }

    static bool takeItem(PlayerNet& player, const std::string& item, int amount = 1) {
        if (itemCount(player, item) < amount) {
            return false;
        }
        auto found = player.items.find(item);
        found->second -= amount;
        if (found->second <= 0) {
            player.items.erase(found);
        }
        return true;
    }

    static void giveItem(PlayerNet& player, const std::string& item, int amount = 1) {
        if (amount > 0) {
            player.items[item] += amount;
        }
    }

    static PlayerNet* findPlayerWithItem(Lobby& lobby, const std::string& item) {
        for (auto& [id, player] : lobby.players) {
            if (player.alive && itemCount(player, item) > 0) {
                return &player;
            }
        }
        return nullptr;
    }

    static bool allAliveHidden(const Lobby& lobby) {
        bool hasAlive = false;
        for (const auto& [id, player] : lobby.players) {
            if (!player.alive) {
                continue;
            }
            hasAlive = true;
            if (!player.hidden) {
                return false;
            }
        }
        return hasAlive;
    }

    void addLog(Lobby& lobby, const std::string& message, const std::string& sound = "") {
        lobby.log.push_back(message);
        constexpr std::size_t maxLog = 32;
        if (lobby.log.size() > maxLog) {
            lobby.log.erase(lobby.log.begin(), lobby.log.begin() + static_cast<std::ptrdiff_t>(lobby.log.size() - maxLog));
        }
        if (!sound.empty()) {
            lobby.sound = sound;
            ++lobby.eventSeq;
        }
    }

    void handleLobbyAction(Lobby& lobby, PlayerNet& player, const std::string& command) {
        if (command == "ready") {
            player.ready = !player.ready;
            addLog(lobby, player.name + (player.ready ? " hazır. " : " hazır değil.") + "");
            return;
        }
        if (command == "start") {
            if (player.id != lobby.hostId) {
                addLog(lobby, "Asansörü yalnızca host çalıştırabilir.");
                return;
            }
            if (lobby.players.empty() || !allReady(lobby)) {
                addLog(lobby, "Herkes hazır olmadan asansör çalışmaz.");
                return;
            }
            lobby.phase = LobbyPhase::Explore;
            lobby.currentDoor = 1;
            lobby.rooms[1].visited = true;
            addLog(lobby, "Asansör kapıları açıldı. Ekip 01 numaralı holde.", "door.wav");
            return;
        }
        addLog(lobby, "Lobide hazır düğmesine veya host isen başlat düğmesine dokun.");
    }

    static bool allReady(const Lobby& lobby) {
        if (lobby.players.empty()) {
            return false;
        }
        for (const auto& [id, player] : lobby.players) {
            if (!player.ready) {
                return false;
            }
        }
        return true;
    }

    void startChase(Lobby& lobby) {
        lobby.phase = LobbyPhase::Chase;
        lobby.chaseStep = 0;
        lobby.chasePattern.clear();
        const std::vector<std::string> moves = {"left", "right", "jump", "duck"};
        const int length = lobby.chaseStage == 1 ? 5 : 7;
        for (int index = 0; index < length; ++index) {
            lobby.chasePattern.push_back(moves[static_cast<std::size_t>(lobby.rng() % moves.size())]);
        }
        addLog(lobby, lobby.chaseStage == 1
                          ? "Işıklar söndü; gölge ekibin arkasında!"
                          : "Gölge geri döndü. Bu koridor daha dar.",
               "chase.wav");
    }

    void enterRoom(Lobby& lobby) {
        RoomNet& room = lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)];
        room.visited = true;
        for (auto& [id, player] : lobby.players) {
            player.hidden = false;
        }
        addLog(lobby, "Ekip kapı " + doorLabel(lobby.currentDoor) + " içinde: " + room.name + ".", "ambience.wav");

        const int firstChase = std::clamp(lobby.targetDoors / 2, 5, lobby.targetDoors - 2);
        const int secondChase = std::clamp((lobby.targetDoors * 3) / 4, firstChase + 3, lobby.targetDoors - 1);
        if (lobby.currentDoor == firstChase && lobby.chaseStage == 0) {
            lobby.chaseStage = 1;
            startChase(lobby);
            return;
        }
        if (lobby.currentDoor == secondChase && lobby.chaseStage == 1) {
            lobby.chaseStage = 2;
            startChase(lobby);
            return;
        }

        const int chance = room.dark ? 24 : 15;
        if (static_cast<int>(lobby.rng() % 100U) < chance) {
            lobby.threat = (lobby.rng() % 2U == 0) ? Threat::Rattle : Threat::Whisper;
            lobby.threatTurns = 3;
            addLog(lobby, lobby.threat == Threat::Rattle
                              ? "Ağır bir sürüklenme sesi geliyor. Üç hamle içinde saklanın."
                              : "Duvarın içinden fısıltılar yükseliyor. Herkes bir dolap bulsun.",
                   "danger.wav");
        }
    }

    void hurt(Lobby& lobby, PlayerNet& player, int amount, const std::string& reason) {
        player.health = std::max(0, player.health - amount);
        addLog(lobby, player.name + ": " + reason + " (-" + std::to_string(amount) + " can).", "hit.wav");
        if (player.health == 0) {
            player.alive = false;
            player.hidden = false;
            addLog(lobby, player.name + " karanlıkta kaldı.", "death.wav");
        }
    }

    void disturb(Lobby& lobby, PlayerNet& player, int amount, const std::string& reason) {
        player.sanity = std::max(0, player.sanity - amount);
        addLog(lobby, player.name + ": " + reason + " (akıl -" + std::to_string(amount) + ").");
        if (player.sanity == 0) {
            hurt(lobby, player, 12, "Panik");
            if (player.alive) {
                player.sanity = 20;
            }
        }
    }

    void checkTeamDead(Lobby& lobby) {
        for (const auto& [id, player] : lobby.players) {
            if (player.alive) {
                return;
            }
        }
        lobby.phase = LobbyPhase::Dead;
        addLog(lobby, "Ekibin tamamı koridorda kayboldu.", "death.wav");
    }

    void tickThreat(Lobby& lobby, Threat expected) {
        if (expected == Threat::None || lobby.threat != expected) {
            return;
        }
        --lobby.threatTurns;
        if (lobby.threatTurns > 0) {
            addLog(lobby, "Tehlike yaklaşıyor; " + std::to_string(lobby.threatTurns) + " hamle kaldı.");
            return;
        }
        for (auto& [id, player] : lobby.players) {
            if (player.alive && !player.hidden) {
                hurt(lobby, player, expected == Threat::Rattle ? 28 : 15,
                     expected == Threat::Rattle ? "Gürültünün çarpması" : "Fısıltının gölgesi");
            }
        }
        lobby.threat = Threat::None;
        lobby.threatTurns = 0;
        checkTeamDead(lobby);
    }

    void handleChaseAction(Lobby& lobby, PlayerNet& player, const std::string& command, const std::string& rawValue) {
        if (!player.alive) {
            addLog(lobby, player.name + " artık hareket edemiyor.");
            return;
        }
        std::string move = command == "move" ? lowerAscii(rawValue) : command;
        if (move == "sol") {
            move = "left";
        } else if (move == "sag" || move == "sağ") {
            move = "right";
        } else if (move == "atla") {
            move = "jump";
        } else if (move == "egil" || move == "eğil") {
            move = "duck";
        }
        if (move != "left" && move != "right" && move != "jump" && move != "duck") {
            addLog(lobby, "Kaçışta ekrandaki hareketi kullan: sol, sağ, atla veya eğil.");
            return;
        }
        if (move != lobby.chasePattern[lobby.chaseStep]) {
            hurt(lobby, player, 20, "Kovalamacada yanlış yöne sapma");
        } else {
            addLog(lobby, player.name + " doğru hamleyi yaptı.");
        }
        ++lobby.chaseStep;
        if (lobby.chaseStep >= lobby.chasePattern.size()) {
            lobby.phase = LobbyPhase::Explore;
            addLog(lobby, "Ekip servis kapısından geçip gölgeyi atlattı.", "door.wav");
        }
        checkTeamDead(lobby);
    }

    void handleExploreAction(Lobby& lobby, PlayerNet& player, const std::string& command, const std::string& rawValue) {
        if (command == "revive") {
            revivePlayer(lobby, player, rawValue);
            return;
        }
        if (!player.alive) {
            addLog(lobby, player.name + " baygın; diğerleri devam edebilir.");
            return;
        }
        const Threat threatBefore = lobby.threat;
        bool consumesTime = true;

        if (command == "look") {
            addLog(lobby, lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)].description);
            consumesTime = false;
        } else if (command == "search") {
            searchRoom(lobby, player);
        } else if (command == "open") {
            openDoor(lobby, player);
        } else if (command == "hide") {
            hidePlayer(lobby, player);
            consumesTime = false;
        } else if (command == "leave_hide") {
            player.hidden = false;
            addLog(lobby, player.name + " dolaptan çıktı.");
            consumesTime = false;
        } else if (command == "listen") {
            listenRoom(lobby);
        } else if (command == "use") {
            useItem(lobby, player, lowerAscii(rawValue));
        } else if (command == "solve" || command == "code") {
            solvePuzzle(lobby, player, trim(rawValue));
        } else if (command == "buy") {
            buyItem(lobby, player, lowerAscii(rawValue));
        } else if (command == "rest") {
            rest(lobby, player);
        } else if (command == "back") {
            goBack(lobby, player);
        } else if (command == "wait") {
            addLog(lobby, player.name + " bir an bekledi.");
        } else {
            addLog(lobby, "Bilinmeyen hareket: " + command + ".");
            consumesTime = false;
        }

        if (consumesTime && threatBefore != Threat::None && lobby.threat == threatBefore && lobby.phase == LobbyPhase::Explore) {
            tickThreat(lobby, threatBefore);
        }
    }

    void revivePlayer(Lobby& lobby, PlayerNet& actor, const std::string& targetId) {
        if (!actor.alive) {
            addLog(lobby, "Baygın oyuncular başkasını canlandıramaz.");
            return;
        }
        PlayerNet* target = nullptr;
        if (!targetId.empty()) {
            const auto found = lobby.players.find(targetId);
            if (found != lobby.players.end()) {
                target = &found->second;
            }
        } else {
            for (auto& [id, player] : lobby.players) {
                if (!player.alive) {
                    target = &player;
                    break;
                }
            }
        }
        if (target == nullptr || target->alive) {
            addLog(lobby, "Canlandırılacak baygın bir ekip arkadaşı yok.");
            return;
        }
        if (!takeItem(actor, "bandage") && !takeItem(actor, "adrenaline")) {
            addLog(lobby, "Canlandırmak için bandaj veya adrenalin gerekiyor.");
            return;
        }
        target->alive = true;
        target->hidden = false;
        target->health = 35;
        target->sanity = 40;
        addLog(lobby, actor.name + ", " + target->name + " adlı oyuncuyu canlandırdı.", "puzzle.wav");
        if (lobby.phase == LobbyPhase::Dead) {
            lobby.phase = LobbyPhase::Explore;
        }
    }

    void searchRoom(Lobby& lobby, PlayerNet& player) {
        RoomNet& room = lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)];
        if (player.hidden) {
            addLog(lobby, "Dolaptan çıkmadan arama yapamazsın.");
            return;
        }
        if (room.dark && !room.lit) {
            addLog(lobby, "Oda karanlık. Önce `kullan lighter` yap.");
            return;
        }
        if (room.searched) {
            addLog(lobby, "Bu odadaki her şey daha önce toplandı.");
            return;
        }
        room.searched = true;
        if (room.loot.empty()) {
            addLog(lobby, player.name + " boş çekmeceler buldu.");
        } else {
            std::map<std::string, int> found;
            for (const std::string& item : room.loot) {
                giveItem(player, item);
                ++found[item];
            }
            room.loot.clear();
            std::ostringstream message;
            message << player.name << " buldu: ";
            bool first = true;
            for (const auto& [item, amount] : found) {
                if (!first) {
                    message << ", ";
                }
                first = false;
                message << itemLabel(item) << " x" << amount;
            }
            addLog(lobby, message.str(), "pickup.wav");
        }
        if (!room.clue.empty()) {
            addLog(lobby, "İpucu: " + room.clue);
        }
    }

    void listenRoom(Lobby& lobby) {
        RoomNet& room = lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)];
        if (room.falseDoor && !room.falseDoorSeen) {
            room.falseDoorSeen = true;
            addLog(lobby, "Bu kapının arkasında hiç yankı yok. Yanıltıcı kapı; dikkatli olun.", "danger.wav");
            return;
        }
        if (room.number == lobby.targetDoors) {
            addLog(lobby, "Asansör boşluğundan zayıf bir motor sesi geliyor.");
        } else if (room.number < lobby.targetDoors && lobby.rooms[static_cast<std::size_t>(room.number + 1)].locked) {
            addLog(lobby, "İlerideki kilit tek bir metal tıkırtısı çıkarıyor.");
        } else if (room.dark && !room.lit) {
            addLog(lobby, "Karanlıkta bir şey sürünüyor, ama yönünü seçemiyorsun.");
        } else {
            addLog(lobby, "Havalandırmanın ritmik sesinden başka bir şey yok.");
        }
    }

    void openDoor(Lobby& lobby, PlayerNet& player) {
        RoomNet& room = lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)];
        if (player.hidden) {
            addLog(lobby, "Dolaptan çıkmadan kapıyı açamazsın.");
            return;
        }
        if (room.number == lobby.targetDoors) {
            lobby.phase = LobbyPhase::Won;
            addLog(lobby, "Servis asansörü çalıştı. Ekip binadan çıktı!", "victory.wav");
            return;
        }
        if (room.falseDoor && !room.falseDoorSeen) {
            room.falseDoorSeen = true;
            disturb(lobby, player, 8, "Kapı kolu elinin altında soğuk bir nabız gibi atıyor.");
            hurt(lobby, player, 10, "Yanıltıcı kapının geri tepmesi");
            addLog(lobby, "Kapı gerçek değilmiş; ikinci denemede açılabilir.", "danger.wav");
            return;
        }
        if (room.puzzle != PuzzleType::None && !room.puzzleSolved) {
            addLog(lobby, puzzleKey(room.puzzle) == "fuse"
                              ? "Kırmızı panel bir sigorta istiyor."
                              : "Üç haneli sayı kilidi çözülmeden kapı açılmıyor.");
            return;
        }
        if (room.locked) {
            PlayerNet* owner = findPlayerWithItem(lobby, "key");
            if (owner == nullptr) {
                owner = findPlayerWithItem(lobby, "lockpick");
                if (owner != nullptr) {
                    takeItem(*owner, "lockpick");
                    room.locked = false;
                    addLog(lobby, owner->name + " maymuncukla kilidi açtı.", "key.wav");
                }
            } else {
                takeItem(*owner, "key");
                room.locked = false;
                addLog(lobby, owner->name + " anahtarı kilitte çevirdi.", "key.wav");
            }
            if (room.locked) {
                addLog(lobby, "Kilit dönmüyor. Önceki odayı arayın.");
                return;
            }
        }
        int alivePlayers = 0;
        for (const auto& [id, member] : lobby.players) {
            alivePlayers += member.alive ? 1 : 0;
        }
        if (room.teamDoor && alivePlayers > 1) {
            room.openVotes.insert(player.id);
            if (room.openVotes.size() < 2) {
                addLog(lobby, player.name + " kapıyı tuttu. Bir ekip arkadaşı daha `kapıyı aç` demeli.");
                return;
            }
            room.openVotes.clear();
            addLog(lobby, "İki oyuncu aynı anda bastı; ekip kapısı açılıyor.");
        }
        ++lobby.currentDoor;
        addLog(lobby, "Kapı " + doorLabel(lobby.currentDoor) + " açıldı.", "door.wav");
        enterRoom(lobby);
    }

    void hidePlayer(Lobby& lobby, PlayerNet& player) {
        const RoomNet& room = lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)];
        if (!room.hasCloset) {
            addLog(lobby, "Bu odada sağlam bir dolap yok.");
            if (lobby.threat != Threat::None) {
                tickThreat(lobby, lobby.threat);
            }
            return;
        }
        player.hidden = true;
        if (lobby.threat != Threat::None && static_cast<int>(lobby.rng() % 100U) < 14) {
            player.hidden = false;
            hurt(lobby, player, 18, "Dolap kapağı tam kapanmadı");
        } else {
            addLog(lobby, player.name + " dolaba saklandı.", "hide.wav");
        }
        if (lobby.threat != Threat::None && allAliveHidden(lobby)) {
            lobby.threat = Threat::None;
            lobby.threatTurns = 0;
            addLog(lobby, "Bütün ekip saklandı; ağır adımlar geçip gitti.");
        }
        checkTeamDead(lobby);
    }

    void useItem(Lobby& lobby, PlayerNet& player, const std::string& item) {
        if (item == "lighter") {
            if (itemCount(player, "lighter") == 0) {
                addLog(lobby, "Çakmağın yok.");
            } else if (!lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)].dark) {
                addLog(lobby, "Bu oda zaten aydınlık.");
            } else {
                lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)].lit = true;
                addLog(lobby, player.name + " çakmağı yaktı.", "lighter.wav");
            }
            return;
        }
        if (item == "bandage") {
            if (!takeItem(player, "bandage")) {
                addLog(lobby, "Çantanda bandaj yok.");
            } else {
                player.health = std::min(100, player.health + 30);
                addLog(lobby, player.name + " bandaj kullandı.");
            }
            return;
        }
        if (item == "tonic") {
            if (!takeItem(player, "tonic")) {
                addLog(lobby, "Çantanda sakinleştirici yok.");
            } else {
                player.sanity = std::min(100, player.sanity + 25);
                addLog(lobby, player.name + " sakinleştirici kullandı.");
            }
            return;
        }
        if (item == "adrenaline") {
            if (!takeItem(player, "adrenaline")) {
                addLog(lobby, "Çantanda adrenalin yok.");
            } else {
                player.health = std::min(100, player.health + 15);
                player.sanity = std::max(0, player.sanity - 5);
                addLog(lobby, player.name + " adrenalin kullandı.");
            }
            return;
        }
        if (item == "lockpick") {
            RoomNet& room = lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)];
            if (!room.locked) {
                addLog(lobby, "Bu odada maymuncuk gerektiren kilit yok.");
            } else if (!takeItem(player, "lockpick")) {
                addLog(lobby, "Çantanda maymuncuk yok.");
            } else {
                room.locked = false;
                addLog(lobby, player.name + " maymuncukla kilidi açtı.", "key.wav");
            }
            return;
        }
        if (item == "fuse") {
            RoomNet& room = lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)];
            if (room.puzzle != PuzzleType::FuseBox || room.puzzleSolved) {
                addLog(lobby, "Burada sigorta takılacak aktif bir panel yok.");
            } else if (!takeItem(player, "fuse")) {
                addLog(lobby, "Çantanda sigorta yok.");
            } else {
                room.puzzleSolved = true;
                addLog(lobby, player.name + " sigortayı panele taktı.", "puzzle.wav");
            }
            return;
        }
        if (item == "key") {
            RoomNet& room = lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)];
            if (room.locked && takeItem(player, "key")) {
                room.locked = false;
                addLog(lobby, player.name + " anahtarı kilitte çevirdi.", "key.wav");
            } else {
                addLog(lobby, "Anahtar yalnızca kilitli kapıda işe yarar.");
            }
            return;
        }
        addLog(lobby, "Bu eşya tanınmadı.");
    }

    void solvePuzzle(Lobby& lobby, PlayerNet& player, const std::string& value) {
        RoomNet& room = lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)];
        if (room.puzzle != PuzzleType::NumberLock) {
            addLog(lobby, "Bu odada sayı kilidi yok.");
            return;
        }
        if (room.puzzleSolved) {
            addLog(lobby, "Sayı kilidi zaten açıldı.");
            return;
        }
        if (trim(value) == room.puzzleCode) {
            room.puzzleSolved = true;
            addLog(lobby, player.name + " sayı kilidini çözdü.", "puzzle.wav");
        } else {
            disturb(lobby, player, 5, "Yanlış kod kilitte tiz bir ses çıkardı");
            addLog(lobby, "Kod yanlış; ipucu için odayı arayın.", "error.wav");
        }
    }

    void buyItem(Lobby& lobby, PlayerNet& player, const std::string& item) {
        const RoomNet& room = lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)];
        if (!room.hasShop) {
            addLog(lobby, "Bu odada servis arabası yok.");
            return;
        }
        int price = 0;
        if (item == "bandage") {
            price = 3;
        } else if (item == "tonic") {
            price = 4;
        } else if (item == "lockpick") {
            price = 5;
        } else if (item == "fuse") {
            price = 2;
        } else {
            addLog(lobby, "Servis arabasında bandaj, tonic, maymuncuk veya sigorta var.");
            return;
        }
        if (!takeItem(player, "coin", price)) {
            addLog(lobby, "Yeterli jeton yok; bu eşya " + std::to_string(price) + " jeton.");
            return;
        }
        giveItem(player, item);
        addLog(lobby, player.name + " " + itemLabel(item) + " aldı.", "pickup.wav");
    }

    void rest(Lobby& lobby, PlayerNet& player) {
        const RoomNet& room = lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)];
        if (room.kind != "infirmary") {
            addLog(lobby, "Dinlenmek için revir bulmalısın.");
            return;
        }
        if (lobby.threat != Threat::None) {
            addLog(lobby, "Tehlike varken dinlenemezsin.");
            return;
        }
        player.health = std::min(100, player.health + 12);
        player.sanity = std::min(100, player.sanity + 8);
        addLog(lobby, player.name + " revire uzanıp nefes aldı.", "ambience.wav");
    }

    void goBack(Lobby& lobby, PlayerNet& player) {
        if (lobby.currentDoor <= 1) {
            addLog(lobby, "Resepsiyonun gerisine giden yol yok.");
            return;
        }
        lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)].openVotes.clear();
        --lobby.currentDoor;
        addLog(lobby, player.name + " önceki odaya geri döndü.", "door.wav");
    }

    std::string leaveLocked(const std::string& code, const std::string& playerId) {
        const auto lobbyFound = lobbies_.find(code);
        if (lobbyFound == lobbies_.end()) {
            return errorJson("Lobi bulunamadı.");
        }
        Lobby& lobby = lobbyFound->second;
        const auto playerFound = lobby.players.find(playerId);
        if (playerFound == lobby.players.end()) {
            return errorJson("Oyuncu bulunamadı.");
        }
        const std::string name = playerFound->second.name;
        const bool wasHost = playerFound->second.host;
        for (RoomNet& room : lobby.rooms) {
            room.openVotes.erase(playerId);
        }
        lobby.players.erase(playerFound);
        if (lobby.players.empty()) {
            lobbies_.erase(lobbyFound);
            return "{\"ok\":true,\"left\":true}";
        }
        if (wasHost) {
            auto nextHost = lobby.players.begin();
            nextHost->second.host = true;
            lobby.hostId = nextHost->first;
        }
        addLog(lobby, name + " lobiden ayrıldı.");
        return "{\"ok\":true,\"left\":true,\"lobby\":\"" + jsonEscape(code) + "\"}";
    }

    static std::string errorJson(const std::string& message) {
        return "{\"ok\":false,\"error\":\"" + jsonEscape(message) + "\"}";
    }

    static void appendJsonString(std::ostringstream& output, const std::string& value) {
        output << '"' << jsonEscape(value) << '"';
    }

    std::string stateJson(const Lobby& lobby, const std::string& me) const {
        std::ostringstream output;
        output << "{\"ok\":true";
        output << ",\"lobby\":\"" << jsonEscape(lobby.code) << "\"";
        output << ",\"me\":\"" << jsonEscape(me) << "\"";
        output << ",\"phase\":\"" << phaseName(lobby.phase) << "\"";
        output << ",\"door\":" << lobby.currentDoor;
        output << ",\"targetDoors\":" << lobby.targetDoors;
        output << ",\"host\":\"" << jsonEscape(lobby.hostId) << "\"";
        output << ",\"threat\":\"" << threatKey(lobby.threat) << "\"";
        output << ",\"threatText\":\"" << jsonEscape(threatText(lobby.threat)) << "\"";
        output << ",\"threatTurns\":" << lobby.threatTurns;
        output << ",\"eventSeq\":" << lobby.eventSeq;
        output << ",\"sound\":\"" << jsonEscape(lobby.sound) << "\"";
        output << ",\"chaseStep\":" << lobby.chaseStep;
        output << ",\"chaseTotal\":" << lobby.chasePattern.size();
        output << ",\"nextMove\":";
        if (lobby.phase == LobbyPhase::Chase && lobby.chaseStep < lobby.chasePattern.size()) {
            appendJsonString(output, lobby.chasePattern[lobby.chaseStep]);
        } else {
            output << "null";
        }

        const RoomNet& room = lobby.rooms[static_cast<std::size_t>(lobby.currentDoor)];
        output << ",\"room\":{";
        output << "\"number\":" << room.number;
        output << ",\"kind\":\"" << jsonEscape(room.kind) << "\"";
        output << ",\"name\":\"" << jsonEscape(room.name) << "\"";
        output << ",\"description\":\"" << jsonEscape(room.description) << "\"";
        output << ",\"clue\":\"" << jsonEscape(room.searched ? room.clue : "") << "\"";
        output << ",\"puzzle\":\"" << puzzleKey(room.puzzle) << "\"";
        output << ",\"puzzleSolved\":" << (room.puzzleSolved ? "true" : "false");
        output << ",\"puzzleCode\":\"" << jsonEscape(room.searched || room.puzzleSolved ? room.puzzleCode : "") << "\"";
        output << ",\"dark\":" << (room.dark ? "true" : "false");
        output << ",\"lit\":" << (room.lit ? "true" : "false");
        output << ",\"locked\":" << (room.locked ? "true" : "false");
        output << ",\"hasCloset\":" << (room.hasCloset ? "true" : "false");
        output << ",\"hasShop\":" << (room.hasShop ? "true" : "false");
        output << ",\"searched\":" << (room.searched ? "true" : "false");
        output << ",\"falseDoorSeen\":" << (room.falseDoorSeen ? "true" : "false");
        output << ",\"teamDoor\":" << (room.teamDoor ? "true" : "false");
        output << ",\"openVotes\":" << room.openVotes.size();
        output << ",\"lootAvailable\":" << (!room.searched && !room.loot.empty() ? "true" : "false");
        output << "}";

        output << ",\"players\":[";
        bool firstPlayer = true;
        for (const auto& [id, player] : lobby.players) {
            if (!firstPlayer) {
                output << ',';
            }
            firstPlayer = false;
            output << "{\"id\":\"" << jsonEscape(id) << "\"";
            output << ",\"name\":\"" << jsonEscape(player.name) << "\"";
            output << ",\"host\":" << (player.host ? "true" : "false");
            output << ",\"ready\":" << (player.ready ? "true" : "false");
            output << ",\"hidden\":" << (player.hidden ? "true" : "false");
            output << ",\"alive\":" << (player.alive ? "true" : "false");
            output << ",\"health\":" << player.health << ",\"sanity\":" << player.sanity;
            output << ",\"items\":{";
            bool firstItem = true;
            for (const auto& [item, amount] : player.items) {
                if (!firstItem) {
                    output << ',';
                }
                firstItem = false;
                output << "\"" << jsonEscape(item) << "\":" << amount;
            }
            output << "}";
            output << "}";
        }
        output << "]";

        output << ",\"log\":[";
        const std::size_t firstLog = lobby.log.size() > 18 ? lobby.log.size() - 18 : 0;
        for (std::size_t index = firstLog; index < lobby.log.size(); ++index) {
            if (index != firstLog) {
                output << ',';
            }
            appendJsonString(output, lobby.log[index]);
        }
        output << "]";
        output << "}";
        return output.str();
    }
};

struct HttpRequest {
    std::string method;
    std::string target;
    std::string path;
    std::string query;
    std::string body;
};

class HttpServer {
public:
    HttpServer(int port, int doors)
        : port_(port), state_(doors), listener_(InvalidSocket), running_(true) {}

    ~HttpServer() {
        if (listener_ != InvalidSocket) {
            net::closeSocket(listener_);
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }

    bool run() {
#ifdef _WIN32
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            std::cerr << "Winsock başlatılamadı.\n";
            return false;
        }
#endif
        listener_ = socket(AF_INET, SOCK_STREAM, 0);
        if (listener_ == InvalidSocket) {
            std::cerr << "Sunucu soketi oluşturulamadı.\n";
            return false;
        }

        int reuse = 1;
#ifdef _WIN32
        setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
#else
        setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(static_cast<std::uint16_t>(port_));
        if (bind(listener_, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
            std::cerr << "Port " << port_ << " bağlanamadı.\n";
            return false;
        }
        if (listen(listener_, 32) != 0) {
            std::cerr << "Sunucu dinleme moduna geçemedi.\n";
            return false;
        }

        std::cout << "NÖBET co-op sunucusu 0.0.0.0:" << port_ << " üzerinde çalışıyor.\n";
        std::cout << "Bu bilgisayarda: http://localhost:" << port_ << "\n";
        std::cout << "Aynı Wi-Fi'daki arkadaşların bu bilgisayarın LAN IP'siyle bağlanabilir.\n";
        while (running_) {
            sockaddr_in clientAddress{};
#ifdef _WIN32
            int clientLength = sizeof(clientAddress);
#else
            socklen_t clientLength = sizeof(clientAddress);
#endif
            const Socket client = accept(listener_, reinterpret_cast<sockaddr*>(&clientAddress), &clientLength);
            if (client == InvalidSocket) {
                if (running_) {
                    continue;
                }
                break;
            }
            std::thread(&HttpServer::handleClient, this, client).detach();
        }
        return true;
    }

private:
    int port_;
    GameState state_;
    Socket listener_;
    std::atomic<bool> running_;

    static std::string contentType(const std::string& path) {
        if (endsWith(path, ".html")) {
            return "text/html; charset=utf-8";
        }
        if (endsWith(path, ".js")) {
            return "text/javascript; charset=utf-8";
        }
        if (endsWith(path, ".css")) {
            return "text/css; charset=utf-8";
        }
        if (endsWith(path, ".json")) {
            return "application/json; charset=utf-8";
        }
        if (endsWith(path, ".svg")) {
            return "image/svg+xml";
        }
        if (endsWith(path, ".wav")) {
            return "audio/wav";
        }
        return "application/octet-stream";
    }

    static bool readRequest(Socket client, HttpRequest& request) {
        std::string raw;
        std::size_t headerEnd = std::string::npos;
        char buffer[4096];
        while (raw.size() < 128 * 1024 && headerEnd == std::string::npos) {
#ifdef _WIN32
            const int received = recv(client, buffer, sizeof(buffer), 0);
#else
            const ssize_t received = recv(client, buffer, sizeof(buffer), 0);
#endif
            if (received <= 0) {
                return false;
            }
            raw.append(buffer, static_cast<std::size_t>(received));
            headerEnd = raw.find("\r\n\r\n");
        }
        if (headerEnd == std::string::npos) {
            return false;
        }

        const std::string header = raw.substr(0, headerEnd);
        std::istringstream lines(header);
        std::string firstLine;
        if (!std::getline(lines, firstLine)) {
            return false;
        }
        firstLine = trim(firstLine);
        std::istringstream first(firstLine);
        first >> request.method >> request.target;
        if (request.method.empty() || request.target.empty()) {
            return false;
        }
        const std::size_t question = request.target.find('?');
        if (question == std::string::npos) {
            request.path = request.target;
        } else {
            request.path = request.target.substr(0, question);
            request.query = request.target.substr(question + 1);
        }

        std::size_t contentLength = 0;
        std::string line;
        while (std::getline(lines, line)) {
            line = trim(line);
            const std::size_t colon = line.find(':');
            if (colon == std::string::npos) {
                continue;
            }
            const std::string key = lowerAscii(trim(line.substr(0, colon)));
            if (key == "content-length") {
                if (const auto parsed = parseInt(trim(line.substr(colon + 1))); parsed.has_value() && *parsed >= 0) {
                    contentLength = static_cast<std::size_t>(*parsed);
                }
            }
        }
        if (contentLength > 128 * 1024) {
            return false;
        }
        const std::size_t bodyStart = headerEnd + 4;
        while (raw.size() < bodyStart + contentLength) {
#ifdef _WIN32
            const int received = recv(client, buffer, sizeof(buffer), 0);
#else
            const ssize_t received = recv(client, buffer, sizeof(buffer), 0);
#endif
            if (received <= 0) {
                return false;
            }
            raw.append(buffer, static_cast<std::size_t>(received));
        }
        request.body = raw.substr(bodyStart, contentLength);
        return true;
    }

    static std::string response(int status, const std::string& type, const std::string& body) {
        std::string statusText = "OK";
        if (status == 400) {
            statusText = "Bad Request";
        } else if (status == 404) {
            statusText = "Not Found";
        } else if (status == 405) {
            statusText = "Method Not Allowed";
        } else if (status == 500) {
            statusText = "Internal Server Error";
        }
        std::ostringstream headers;
        headers << "HTTP/1.1 " << status << ' ' << statusText << "\r\n"
                << "Content-Type: " << type << "\r\n"
                << "Content-Length: " << body.size() << "\r\n"
                << "Cache-Control: no-store\r\n"
                << "Access-Control-Allow-Origin: *\r\n"
                << "Access-Control-Allow-Headers: Content-Type\r\n"
                << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                << "Connection: close\r\n\r\n";
        return headers.str() + body;
    }

    static std::string staticFile(const std::string& path, int& status) {
        std::string relative;
        if (path == "/" || path == "/index.html") {
            relative = "web/index.html";
        } else if (path == "/app.js" || path == "/styles.css" || path == "/manifest.json") {
            relative = "web" + path;
        } else if (path.rfind("/assets/", 0) == 0) {
            relative = "." + path;
        } else {
            status = 404;
            return "Not found";
        }
        if (relative.find("..") != std::string::npos || relative.find('\\') != std::string::npos) {
            status = 404;
            return "Not found";
        }
        std::ifstream file(relative, std::ios::binary);
        if (!file) {
            status = 404;
            return "Not found";
        }
        std::ostringstream content;
        content << file.rdbuf();
        status = 200;
        return content.str();
    }

    void handleClient(Socket client) {
        HttpRequest request;
        if (!readRequest(client, request)) {
            net::closeSocket(client);
            return;
        }

        std::string body;
        int status = 200;
        std::string type = "application/json; charset=utf-8";
        if (request.method == "OPTIONS") {
            body = "{}";
        } else if (request.method == "GET" && request.path == "/api/state") {
            body = state_.state(parseParams(request.query));
        } else if (request.method == "POST" && request.path == "/api/join") {
            body = state_.join(parseParams(request.body));
        } else if (request.method == "POST" && request.path == "/api/action") {
            body = state_.action(parseParams(request.body));
        } else if (request.method == "POST" && request.path == "/api/leave") {
            body = state_.leave(parseParams(request.body));
        } else if (request.method == "GET") {
            type = contentType(request.path);
            body = staticFile(request.path, status);
        } else {
            status = request.method == "POST" ? 404 : 405;
            body = "{\"ok\":false,\"error\":\"İstek desteklenmiyor.\"}";
        }

        net::sendAll(client, response(status, type, body));
        net::closeSocket(client);
    }
};

int main(int argc, char** argv) {
    int port = 8080;
    int doors = 20;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help" || argument == "-h") {
            std::cout << "NÖBET co-op sunucusu\n\n"
                      << "Kullanım: nightshift-server [--port SAYI] [--doors SAYI]\n"
                      << "  --port SAYI   HTTP portu (varsayılan 8080)\n"
                      << "  --doors SAYI   Lobi haritası (12-50, varsayılan 20)\n";
            return 0;
        }
        if (argument == "--port" && index + 1 < argc) {
            if (const auto value = parseInt(argv[++index]); value.has_value()) {
                port = std::clamp(*value, 1024, 65535);
            }
            continue;
        }
        if (argument == "--doors" && index + 1 < argc) {
            if (const auto value = parseInt(argv[++index]); value.has_value()) {
                doors = std::clamp(*value, 12, 50);
            }
            continue;
        }
        std::cerr << "Bilinmeyen seçenek: " << argument << "\n";
        return 2;
    }

    HttpServer server(port, doors);
    return server.run() ? 0 : 1;
}
