#ifndef CONSOLE_H
#define CONSOLE_H

#include <atomic>
#include <cerrno>
#include <poll.h>
#include <filesystem>
#include <cctype>
#include <string>
#include <thread>
#include <unistd.h>
#include "chat_common.h"

inline std::string clean_dropped_path(std::string path) {
    const auto not_space = [](unsigned char c) {
        return !std::isspace(c);
    };
    path.erase(path.begin(), std::find_if(path.begin(), path.end(), not_space));
    path.erase(std::find_if(path.rbegin(), path.rend(), not_space).base(), path.end());

    if (path.size() >= 2 &&
        ((path.front() == '"' && path.back() == '"') ||
         (path.front() == '\'' && path.back() == '\''))) {
        path = path.substr(1, path.size() - 2);
    }

    // Windows Explorer can paste a Windows path into a WSL terminal.
    std::replace(path.begin(), path.end(), '\\', '/');
    if (path.size() >= 3 && std::isalpha(static_cast<unsigned char>(path[0])) &&
        path[1] == ':' && path[2] == '/') {
        path = "/mnt/" + std::string(1, static_cast<char>(
            std::tolower(static_cast<unsigned char>(path[0])))) + path.substr(2);
    }
    return path;
}

inline std::string resolve_image_path(const std::string& raw_path) {
    const std::string cleaned = clean_dropped_path(raw_path);
    const fs::path requested(cleaned);
    if (fs::exists(requested)) return requested.string();

    // A bare filename is looked up from the project's standard test folder.
    if (requested.has_parent_path()) return cleaned;
    const fs::path relative_test_dir =
        "702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/"
        "03_Test_ve_Dogrulama_Calismalari/Test_Verileri";
    fs::path directory = fs::current_path();
    for (;;) {
        const fs::path candidate = directory / relative_test_dir / requested;
        if (fs::exists(candidate)) return candidate.string();
        if (directory == directory.root_path() || !directory.has_parent_path()) break;
        directory = directory.parent_path();
    }
    return cleaned;
}

// poll ile beklemek, baglanti kesildiginde Enter tusuna gerek kalmadan cikmayi saglar.
class ConsoleInput {
    std::string pending;
    bool eof = false;
public:
    bool next(std::string& line, const std::atomic<bool>& connected) {
        while (connected.load()) {
            const auto end = pending.find('\n');
            if (end != std::string::npos) {
                line = pending.substr(0, end);
                pending.erase(0, end + 1);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                return true;
            }
            if (eof) {
                if (pending.empty()) return false;
                line.swap(pending);
                return true;
            }
            pollfd input{STDIN_FILENO, POLLIN, 0};
            const int ready = poll(&input, 1, 100);
            if (ready < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            if (ready == 0) continue;
            if (input.revents & (POLLERR | POLLNVAL)) return false;
            if (input.revents & (POLLIN | POLLHUP)) {
                char buffer[4096];
                const ssize_t count = read(STDIN_FILENO, buffer, sizeof(buffer));
                if (count > 0) pending.append(buffer, static_cast<size_t>(count));
                else if (count == 0) eof = true;
                else if (errno != EINTR) return false;
            }
        }
        return false;
    }
};

inline void run_console(int sock, const std::string& role, const std::string& endpoint) {
    std::atomic<bool> connected{true};
    std::thread receiver([&] {
        receive_loop(sock);
        connected.store(false);
        shutdown(sock, SHUT_RDWR);
    });
    ConsoleInput input;
    std::string line;
    while (connected.load()) {
        ui::prompt();
        if (!input.next(line, connected)) break;
        if (!connected.load()) break;
        if (line.empty()) continue;
        if (line == "cikis") break;
        if (line == "!yardim") { ui::help(); continue; }
        if (line == "!durum") { ui::status(role + " aktif; TCP bağlantısı açık."); continue; }
        if (line == "!temizle") { ui::clear(); ui::banner(role, endpoint); continue; }
        if (line == "!resim" || line == "!resim ") {
            ui::error("Kullanım: !resim <dosya yolu>");
            continue;
        }
        if (line.rfind("!resim ", 0) == 0) {
            const std::string path = resolve_image_path(line.substr(7));
            send_image_message(sock, path);
            continue;
        }
        // A path pasted or dragged from Explorer is an image command by itself.
        const std::string dropped_path = resolve_image_path(line);
        if (fs::is_regular_file(dropped_path) && valid_image_extension(dropped_path)) {
            send_image_message(sock, dropped_path);
            continue;
        }
        if (line.size() > MAX_TEXT_SIZE) {
            ui::error("Metin 1 MB'dan büyük olamaz.");
            continue;
        }
        if (!send_text_message(sock, line)) {
            ui::error("Mesaj gönderilemedi; bağlantı kapanmış olabilir.");
            break;
        }
        ui::sent_card(line);
    }
    connected.store(false);
    ui::info("Bağlantı kapatılıyor...");
    shutdown(sock, SHUT_RDWR);
    if (receiver.joinable()) receiver.join();
}

#endif
