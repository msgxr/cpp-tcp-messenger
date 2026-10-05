#ifndef CONSOLE_H
#define CONSOLE_H

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <poll.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
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

inline void show_list(const Frame& frame, std::atomic<uint16_t>& target) {
    if (frame.data.empty() || frame.data.size() != 1u + 2u * frame.data[0]) {
        ui::error("Sunucudan geçersiz istemci listesi geldi."); return;
    }
    std::ostringstream output;
    output << "Aktif hedefler:";
    bool target_exists = false;
    for (size_t i = 0; i < frame.data[0]; ++i) {
        uint16_t network_id;
        std::memcpy(&network_id, frame.data.data() + 1 + i * 2, 2);
        uint16_t id = ntohs(network_id);
        output << ' ' << id;
        if (id == target.load()) target_exists = true;
    }
    if (!frame.data[0]) output << " yok";
    if (!target_exists) target.store(0);
    ui::info(output.str());
}

inline void client_receive_loop(int sock, std::atomic<bool>& connected,
                                std::atomic<uint16_t>& own_id,
                                std::atomic<uint16_t>& target) {
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
            show_list(frame, target);
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
        ui::prompt();
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
    std::atomic<bool> connected{true};
    std::atomic<uint16_t> own_id{0}, target{0};
    std::thread receiver(client_receive_loop, sock, std::ref(connected), std::ref(own_id), std::ref(target));
    ConsoleInput input;
    std::string line;
    while (connected.load()) {
        ui::prompt();
        if (!input.next(line, connected)) break;
        if (line.empty()) continue;
        if (line == "cikis") break;
        if (line == "!yardim") { ui::help(); continue; }
        if (line == "!temizle") { ui::clear(); ui::banner(role, endpoint); continue; }
        if (line == "!durum") {
            ui::status("Kimlik: " + std::to_string(own_id.load()) +
                       " • hedef: " + (target.load() ? std::to_string(target.load()) : std::string("seçilmedi")));
            continue;
        }
        if (line == "!liste") {
            if (!send_frame(sock, Frame{MSG_LIST_REQUEST, 0, {}, {}})) break;
            continue;
        }
        if (line.rfind("!hedef ", 0) == 0) {
            try {
                size_t used = 0; unsigned long value = std::stoul(line.substr(7), &used);
                if (used != line.size() - 7 || value == 0 || value > 65535 || value == own_id.load()) throw std::out_of_range("id");
                target.store(static_cast<uint16_t>(value));
                ui::status("Hedef istemci " + std::to_string(value) + " olarak seçildi.");
            } catch (...) { ui::error("Kullanım: !hedef <1-65535 arasındaki başka bir kimlik>"); }
            continue;
        }
        if (!target.load()) { ui::error("Önce !liste ve !hedef <kimlik> komutlarını kullanın."); continue; }
        Frame outgoing;
        outgoing.client_id = target.load();
        const bool image_command = line.rfind("!resim ", 0) == 0;
        const std::string candidate = resolve_image_path(image_command ? line.substr(7) : line);
        if (image_command || (fs::is_regular_file(candidate) &&
            [&]{ auto e=fs::path(candidate).extension().string(); std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); }); return e==".jpg"||e==".jpeg"||e==".png"; }())) {
            outgoing.type = MSG_IMAGE;
            if (!load_image(candidate, outgoing)) continue;
        } else {
            outgoing.type = MSG_TEXT;
            outgoing.data = bytes(line);
            if (outgoing.data.size() > MAX_TEXT_SIZE || !valid_utf8(outgoing.data)) {
                ui::error("Metin geçerli UTF-8 olmalı ve 64 KiB sınırını aşmamalı."); continue;
            }
        }
        if (!send_frame(sock, outgoing)) { ui::error("İleti gönderilemedi."); break; }
        ui::sent_card(outgoing.type == MSG_TEXT ? line : "Görsel: " + outgoing.filename);
    }
    connected.store(false);
    shutdown(sock, SHUT_RDWR);
    if (receiver.joinable()) receiver.join();
}

#endif
