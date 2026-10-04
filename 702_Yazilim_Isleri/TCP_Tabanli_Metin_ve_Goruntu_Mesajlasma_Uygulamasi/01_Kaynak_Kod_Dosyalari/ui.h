#ifndef UI_H
#define UI_H

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <mutex>
#include <filesystem>
#include <cstdlib>
#include <algorithm>

namespace ui {
inline std::mutex out_mutex;

inline constexpr const char* RESET   = "\033[0m";
inline constexpr const char* BOLD    = "\033[1m";
inline constexpr const char* DIM     = "\033[2m";
inline constexpr const char* CYAN    = "\033[36m";
inline constexpr const char* GREEN   = "\033[32m";
inline constexpr const char* YELLOW  = "\033[33m";
inline constexpr const char* RED     = "\033[31m";
inline constexpr const char* MAGENTA = "\033[35m";
inline constexpr const char* BLUE    = "\033[34m";
inline constexpr const char* WHITE   = "\033[97m";

inline void clear() {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << "\033[2J\033[H" << std::flush;
}

inline void banner(const std::string& role, const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << CYAN << BOLD
              << "╔════════════════════════════════════════════════════╗\n"
              << "║       TCP METİN + GÖRÜNTÜ MESAJLAŞMA SİSTEMİ     ║\n"
              << "╠════════════════════════════════════════════════════╣\n"
              << "║ Rol      : " << std::left << std::setw(39) << role << "║\n"
              << "║ Uç Nokta : " << std::left << std::setw(39) << endpoint << "║\n"
              << "╚════════════════════════════════════════════════════╝\n"
              << RESET;
    std::cout << DIM << "Komutlar: !yardim  !durum  !temizle  !resim <dosya>  cikis\n\n" << RESET;
}

inline void status(const std::string& text) {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << GREEN << BOLD << "[✓] " << RESET << text << "\n";
}
inline void info(const std::string& text) {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << CYAN << "[i] " << RESET << text << "\n";
}
inline void warn(const std::string& text) {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << YELLOW << "[!] " << RESET << text << "\n";
}
inline void error(const std::string& text) {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cerr << RED << BOLD << "[X] " << RESET << text << "\n";
}

inline void prompt() {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << MAGENTA << BOLD << "Sen > " << RESET << std::flush;
}

inline void message_box(const std::string& text) {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::string shown = text;
    if (shown.size() > 70) shown = shown.substr(0, 67) + "...";
    std::cout << "\n" << BLUE << "┌─ GELEN MESAJ ─────────────────────────────────────┐\n"
              << "│ " << RESET << shown << "\n"
              << BLUE << "└───────────────────────────────────────────────────┘\n" << RESET;
}

inline std::string shell_quote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
}

inline void image_card(const std::string& path, uint32_t bytes, uint32_t checksum) {
    {
        std::lock_guard<std::mutex> lock(out_mutex);
        std::cout << "\n" << GREEN << "┌─ GELEN GÖRSEL ────────────────────────────────────┐\n" << RESET
                  << "  Dosya : " << path << "\n"
                  << "  Boyut : " << bytes << " bayt\n"
                  << "  CRC32 : 0x" << std::hex << std::uppercase << checksum << std::dec << "  "
                  << GREEN << "DOĞRULANDI ✓" << RESET << "\n"
                  << GREEN << "└───────────────────────────────────────────────────┘\n" << RESET;
    }

    if (std::system("command -v chafa >/dev/null 2>&1") == 0) {
        std::string cmd = "chafa --colors full --symbols block --size 58x24 " + shell_quote(path);
        std::system(cmd.c_str());
    } else {
        warn("Terminal görsel önizlemesi için 'chafa' kurulu değil.");
    }
}

inline void progress(const std::string& label, uint64_t done, uint64_t total) {
    if (total == 0) total = 1;
    const int width = 24;
    int percent = static_cast<int>((done * 100) / total);
    if (percent > 100) percent = 100;
    int fill = percent * width / 100;
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << "\r" << CYAN << label << " [" << RESET;
    for (int i = 0; i < width; ++i) std::cout << (i < fill ? "█" : "░");
    std::cout << CYAN << "] " << std::setw(3) << percent << "%" << RESET << std::flush;
    if (done >= total) std::cout << "\n";
}

inline void help() {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << YELLOW << BOLD << "\nKOMUTLAR\n" << RESET
              << "  normal metin          Karşı tarafa metin gönderir\n"
              << "  !resim <dosya>        JPG/PNG görsel gönderir\n"
              << "  !durum                Bağlantı durumunu gösterir\n"
              << "  !temizle              Terminali temizler\n"
              << "  !yardim               Bu menüyü gösterir\n"
              << "  cikis                  Programı kapatır\n\n";
}
}

#endif
