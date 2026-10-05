#ifndef CONSOLE_H
#define CONSOLE_H

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <poll.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include "chat_common.h"

inline std::string clean_dropped_path(std::string path) {
    const auto visible = [](unsigned char c){ return !std::isspace(c); };
    path.erase(path.begin(), std::find_if(path.begin(), path.end(), visible));
    path.erase(std::find_if(path.rbegin(), path.rend(), visible).base(), path.end());
    if (path.size() >= 2 && ((path.front() == '"' && path.back() == '"') ||
                            (path.front() == '\'' && path.back() == '\'')))
        path = path.substr(1, path.size() - 2);
    std::replace(path.begin(), path.end(), '\\', '/');
    if (path.size() >= 3 && std::isalpha(static_cast<unsigned char>(path[0])) &&
        path[1] == ':' && path[2] == '/')
        path = "/mnt/" + std::string(1, static_cast<char>(std::tolower(path[0]))) + path.substr(2);
    return path;
}

inline std::string resolve_image_path(const std::string& raw) {
    const fs::path requested(clean_dropped_path(raw));
    if (fs::exists(requested)) return requested.string();
    if (requested.has_parent_path()) return requested.string();
    const fs::path test_dir = "702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/03_Test_ve_Dogrulama_Calismalari/Test_Verileri";
    for (fs::path dir = fs::current_path();; dir = dir.parent_path()) {
        if (fs::exists(dir / test_dir / requested)) return (dir / test_dir / requested).string();
        if (dir == dir.root_path()) break;
    }
    return requested.string();
}

inline bool load_image(const std::string& path, Frame& frame) {
    if (!fs::is_regular_file(path)) { ui::error("Dosya bulunamadı: " + path); return false; }
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) { ui::error("Görüntü açılamadı."); return false; }
    const auto size = input.tellg();
    if (size <= 0 || static_cast<uint64_t>(size) > MAX_IMAGE_SIZE) {
        ui::error("Görüntü boş olamaz ve 10 MiB sınırını aşamaz."); return false;
    }
    frame.filename = fs::path(path).filename().string();
    if (!safe_wire_filename(frame.filename)) { ui::error("Dosya adı geçersiz veya 255 bayttan uzun."); return false; }
    frame.data.resize(static_cast<size_t>(size));
    input.seekg(0);
    input.read(reinterpret_cast<char*>(frame.data.data()), size);
    if (!input || !valid_image_signature(frame.filename, frame.data)) {
        ui::error("Dosya içeriği geçerli JPG/JPEG/PNG imzası taşımıyor."); return false;
    }
    return true;
}

struct RecipientSelection {
    std::mutex mutex;
    std::vector<uint16_t> active;
    std::vector<uint16_t> chosen;
};

inline std::string id_list(const std::vector<uint16_t>& ids) {
    std::ostringstream output;
    for (size_t i = 0; i < ids.size(); ++i) {
        if (i) output << ',';
        output << ids[i];
    }
    return output.str();
}

inline void show_list(const Frame& frame, std::atomic<uint16_t>& target,
                      std::atomic<unsigned int>& active_count,
                      RecipientSelection& selection,
                      const std::atomic<bool>& interface_ready) {
    if (frame.data.empty() || frame.data.size() != 1u + 2u * frame.data[0]) {
        ui::error("Sunucudan geçersiz istemci listesi geldi."); return;
    }
    active_count.store(frame.data[0]);
    std::vector<uint16_t> active_ids;
    std::ostringstream output;
    output << "Alıcılar:";
    for (size_t i = 0; i < frame.data[0]; ++i) {
        uint16_t network_id;
        std::memcpy(&network_id, frame.data.data() + 1 + i * 2, 2);
        uint16_t id = ntohs(network_id);
        active_ids.push_back(id);
        output << " [" << id << ']';
    }
    if (!frame.data[0]) output << " yok";
    if (frame.data[0] == 1) {
        uint16_t network_id = 0;
        std::memcpy(&network_id, frame.data.data() + 1, 2);
        target.store(ntohs(network_id));
        std::lock_guard<std::mutex> lock(selection.mutex);
        selection.active = active_ids;
        selection.chosen = active_ids;
        output << " • otomatik seçildi; mesajınızı yazın";
    } else if (frame.data[0] > 1) {
        std::lock_guard<std::mutex> lock(selection.mutex);
        selection.active = active_ids;
        selection.chosen.erase(
            std::remove_if(selection.chosen.begin(), selection.chosen.end(),
                [&](uint16_t id){ return std::find(active_ids.begin(), active_ids.end(), id) == active_ids.end(); }),
            selection.chosen.end());
        target.store(selection.chosen.size() == 1 ? selection.chosen.front() : 0);
        output << "  |  Seçim: 3 / 2,4 / 0";
    } else {
        std::lock_guard<std::mutex> lock(selection.mutex);
        selection.active.clear();
        selection.chosen.clear();
        target.store(0);
    }
    if (interface_ready.load()) ui::info(output.str());
}

inline void client_receive_loop(int sock, std::atomic<bool>& connected,
                                std::atomic<uint16_t>& own_id,
                                std::atomic<uint16_t>& target,
                                std::atomic<unsigned int>& active_count,
                                RecipientSelection& selection,
                                std::atomic<bool>& interface_ready) {
    while (connected.load()) {
        Frame frame;
        std::string error;
        if (!receive_frame(sock, frame, error)) {
            if (!error.empty()) ui::error(error);
            break;
        }
        if (frame.type == MSG_ID_ASSIGN && frame.client_id == 0 && frame.filename.empty() && frame.data.size() == 2) {
            uint16_t network_id; std::memcpy(&network_id, frame.data.data(), 2);
            own_id.store(ntohs(network_id));
            ui::status("Bağlantı kimliğiniz: " + std::to_string(own_id.load()));
        } else if (frame.type == MSG_LIST_RESPONSE && frame.client_id == 0 && frame.filename.empty()) {
            show_list(frame, target, active_count, selection, interface_ready);
        } else if (frame.type == MSG_ERROR && frame.client_id == 0 && frame.filename.empty() &&
                   frame.data.size() <= MAX_ERROR_SIZE && valid_utf8(frame.data)) {
            ui::error(std::string(frame.data.begin(), frame.data.end()));
        } else if (frame.type == MSG_TEXT && frame.client_id && frame.filename.empty() &&
                   !frame.data.empty() && frame.data.size() <= MAX_TEXT_SIZE && valid_utf8(frame.data)) {
            ui::message_box("İstemci " + std::to_string(frame.client_id) + ": " +
                            std::string(frame.data.begin(), frame.data.end()));
        } else if (frame.type == MSG_IMAGE && frame.client_id && safe_wire_filename(frame.filename) &&
                   valid_image_signature(frame.filename, frame.data)) {
            fs::path saved;
            if (save_received_image(frame, saved, error)) {
                ui::info("Görseli gönderen istemci: " + std::to_string(frame.client_id));
                ui::image_card(saved.string(), static_cast<uint32_t>(frame.data.size()), frame_crc(frame.filename, frame.data));
            } else ui::error(error);
        } else {
            ui::error("Sunucudan geçersiz veya beklenmeyen bir çerçeve geldi.");
            break;
        }
    }
    connected.store(false);
    shutdown(sock, SHUT_RDWR);
}

class ConsoleInput {
    std::string pending;
public:
    bool next(std::string& line, const std::atomic<bool>& connected) {
        while (connected.load()) {
            auto end = pending.find('\n');
            if (end != std::string::npos) {
                line = pending.substr(0, end); pending.erase(0, end + 1);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                return true;
            }
            pollfd input{STDIN_FILENO, POLLIN, 0};
            int ready = poll(&input, 1, 100);
            if (ready < 0 && errno != EINTR) return false;
            if (ready > 0 && (input.revents & (POLLIN | POLLHUP))) {
                char buffer[4096]; ssize_t count = read(STDIN_FILENO, buffer, sizeof(buffer));
                if (count > 0) pending.append(buffer, static_cast<size_t>(count)); else return false;
            }
        }
        return false;
    }
};

inline void run_console(int sock, const std::string& role, const std::string& endpoint) {
    (void)role;
    (void)endpoint;
    std::atomic<bool> connected{true};
    std::atomic<uint16_t> own_id{0}, target{0};
    std::atomic<unsigned int> active_count{0};
    std::atomic<bool> interface_ready{false};
    RecipientSelection selection;
    std::thread receiver(client_receive_loop, sock, std::ref(connected),
                         std::ref(own_id), std::ref(target), std::ref(active_count),
                         std::ref(selection), std::ref(interface_ready));
    while (connected.load() && own_id.load() == 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    unsigned int expected_peers = 1;
    if (const char* expected = std::getenv("TCPMSG_EXPECTED_CLIENTS")) {
        const int parsed = std::atoi(expected);
        if (parsed >= 2 && parsed <= MAX_CLIENTS) expected_peers = static_cast<unsigned int>(parsed - 1);
    }
    for (int attempt = 0; connected.load() && active_count.load() < expected_peers && attempt < 150; ++attempt)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    std::vector<uint16_t> initial_recipients;
    {
        std::lock_guard<std::mutex> lock(selection.mutex);
        initial_recipients = selection.active;
    }
    ui::client_ready(own_id.load(), initial_recipients);
    interface_ready.store(true);
    ConsoleInput input;
    std::string line;
    while (connected.load()) {
        ui::prompt();
        if (!input.next(line, connected)) break;
        if (line.empty()) continue;
        if (line == "cikis") break;
        if (line == "!yardim") { ui::help(); continue; }
        if (line == "!temizle") {
            std::vector<uint16_t> recipients;
            {
                std::lock_guard<std::mutex> lock(selection.mutex);
                recipients = selection.active;
            }
            ui::client_ready(own_id.load(), recipients);
            continue;
        }
        if (line == "!durum") {
            std::vector<uint16_t> chosen;
            {
                std::lock_guard<std::mutex> lock(selection.mutex);
                chosen = selection.chosen;
            }
            ui::status("Kimlik: " + std::to_string(own_id.load()) +
                       " • alıcılar: " + (chosen.empty() ? std::string("seçilmedi") : id_list(chosen)));
            continue;
        }
        if (line == "!liste") {
            if (!send_frame(sock, Frame{MSG_LIST_REQUEST, 0, {}, {}})) break;
            continue;
        }
        const bool selection_input = active_count.load() > 1 &&
            std::all_of(line.begin(), line.end(), [](unsigned char c){
                return std::isdigit(c) || c == ',' || std::isspace(c);
            });
        if (selection_input) {
            try {
                std::string compact;
                for (unsigned char c : line) if (!std::isspace(c)) compact += static_cast<char>(c);
                std::vector<uint16_t> active_ids;
                {
                    std::lock_guard<std::mutex> lock(selection.mutex);
                    active_ids = selection.active;
                }
                std::vector<uint16_t> chosen;
                if (compact == "0") {
                    chosen = active_ids;
                } else {
                    std::istringstream values(compact);
                    std::string item;
                    while (std::getline(values, item, ',')) {
                        if (item.empty()) throw std::out_of_range("id");
                        size_t used = 0;
                        const unsigned long value = std::stoul(item, &used);
                        if (used != item.size() || value > 65535 ||
                            std::find(active_ids.begin(), active_ids.end(), static_cast<uint16_t>(value)) == active_ids.end())
                            throw std::out_of_range("id");
                        if (std::find(chosen.begin(), chosen.end(), static_cast<uint16_t>(value)) == chosen.end())
                            chosen.push_back(static_cast<uint16_t>(value));
                    }
                }
                if (chosen.empty()) throw std::out_of_range("id");
                {
                    std::lock_guard<std::mutex> lock(selection.mutex);
                    selection.chosen = chosen;
                }
                target.store(chosen.size() == 1 ? chosen.front() : 0);
                ui::status("Alıcı seçildi: " + id_list(chosen) + ". Şimdi mesajınızı yazın.");
            } catch (...) {
                ui::error("Tek kişi: 3   Birkaç kişi: 2,4   Herkes: 0");
            }
            continue;
        }
        if (line.rfind("!hedef", 0) == 0) {
            try {
                std::string value_text = line.substr(6);
                value_text.erase(value_text.begin(), std::find_if(value_text.begin(), value_text.end(), [](unsigned char c){ return !std::isspace(c); }));
                value_text.erase(std::find_if(value_text.rbegin(), value_text.rend(), [](unsigned char c){ return !std::isspace(c); }).base(), value_text.end());
                if (value_text.size() >= 2 && value_text.front() == '<' && value_text.back() == '>')
                    value_text = value_text.substr(1, value_text.size() - 2);
                size_t used = 0; unsigned long value = std::stoul(value_text, &used);
                if (used != value_text.size() || value == 0 || value > 65535 || value == own_id.load()) throw std::out_of_range("id");
                target.store(static_cast<uint16_t>(value));
                {
                    std::lock_guard<std::mutex> lock(selection.mutex);
                    selection.chosen = {static_cast<uint16_t>(value)};
                }
                ui::status("Hedef istemci " + std::to_string(value) + " olarak seçildi.");
            } catch (...) { ui::error("Kullanım: !hedef <1-65535 arasındaki başka bir kimlik>"); }
            continue;
        }
        std::string content = line;
        std::vector<uint16_t> message_targets;
        {
            std::lock_guard<std::mutex> lock(selection.mutex);
            message_targets = selection.chosen;
        }
        if (line.front() == '@') {
            try {
                const size_t separator = line.find(' ');
                if (separator == std::string::npos) throw std::out_of_range("mesaj");
                const std::string id_text = line.substr(1, separator - 1);
                size_t used = 0;
                const unsigned long value = std::stoul(id_text, &used);
                if (used != id_text.size() || value == 0 || value > 65535 || value == own_id.load())
                    throw std::out_of_range("id");
                message_targets = {static_cast<uint16_t>(value)};
                content = line.substr(separator + 1);
                if (content.empty()) throw std::out_of_range("mesaj");
            } catch (...) {
                ui::error("Örnek kullanım: @3 merhaba");
                continue;
            }
        }
        if (message_targets.empty()) {
            ui::error("Önce alıcı seçin: tek kişi 3, birkaç kişi 2,4, herkes 0");
            continue;
        }
        Frame outgoing;
        const bool image_command = content.rfind("!resim", 0) == 0;
        const std::string candidate = resolve_image_path(image_command ? content.substr(6) : content);
        if (image_command || (fs::is_regular_file(candidate) &&
            [&]{ auto e=fs::path(candidate).extension().string(); std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); }); return e==".jpg"||e==".jpeg"||e==".png"; }())) {
            outgoing.type = MSG_IMAGE;
            if (!load_image(candidate, outgoing)) continue;
        } else {
            outgoing.type = MSG_TEXT;
            outgoing.data = bytes(content);
            if (outgoing.data.size() > MAX_TEXT_SIZE || !valid_utf8(outgoing.data)) {
                ui::error("Metin geçerli UTF-8 olmalı ve 64 KiB sınırını aşmamalı."); continue;
            }
        }
        bool sent = true;
        for (uint16_t message_target : message_targets) {
            outgoing.client_id = message_target;
            if (!send_frame(sock, outgoing)) { sent = false; break; }
        }
        if (!sent) { ui::error("İleti gönderilemedi."); break; }
        ui::sent_card("İstemci " + id_list(message_targets) + " → " +
                      (outgoing.type == MSG_TEXT ? content : "Görsel: " + outgoing.filename));
    }
    connected.store(false);
    shutdown(sock, SHUT_RDWR);
    if (receiver.joinable()) receiver.join();
}

#endif
