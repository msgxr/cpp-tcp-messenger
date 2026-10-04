#ifndef UI_H
#define UI_H

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include <sys/ioctl.h>
#include <unistd.h>

namespace ui {
inline std::mutex out_mutex;
inline constexpr const char* RESET = "\033[0m";
inline constexpr const char* BOLD = "\033[1m";
inline constexpr const char* DIM = "\033[2m";
inline constexpr const char* CYAN = "\033[36m";
inline constexpr const char* GREEN = "\033[32m";
inline constexpr const char* YELLOW = "\033[33m";
inline constexpr const char* RED = "\033[31m";
inline constexpr const char* MAGENTA = "\033[35m";
inline constexpr const char* BLUE = "\033[34m";

inline int columns() {
    winsize size{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0)
        return std::max(20, static_cast<int>(size.ws_col));
    return 80;
}

inline size_t utf8_length(const std::string& text) {
    size_t count = 0;
    for (unsigned char c : text) if ((c & 0xc0) != 0x80) ++count;
    return count;
}

// Turkce UTF-8 karakterleri bayt ortasindan kesmeden satirlara ayirir.
inline std::vector<std::string> wrap(const std::string& text, size_t width) {
    std::vector<std::string> lines;
    std::string line;
    size_t used = 0;
    for (size_t i = 0; i < text.size();) {
        const auto c = static_cast<unsigned char>(text[i]);
        if (c == '\n') { lines.push_back(line); line.clear(); used = 0; ++i; continue; }
        if (c == '\r') { ++i; continue; }
        if (used == width) { lines.push_back(line); line.clear(); used = 0; }
        size_t bytes = c < 0x80 ? 1 : c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
        bytes = std::min(bytes, text.size() - i);
        if (c < 32 || c == 127) { line += c == '\t' ? ' ' : '?'; bytes = 1; }
        else line.append(text, i, bytes);
        i += bytes;
        ++used;
    }
    lines.push_back(line);
    return lines;
}

inline void border(const char* left, const char* fill, const char* right, int width) {
    std::cout << left;
    for (int i = 0; i < width; ++i) std::cout << fill;
    std::cout << right << '\n';
}

inline void boxed_line(const std::string& text, int width) {
    for (const auto& line : wrap(text, static_cast<size_t>(width - 2))) {
        std::cout << "║ " << line
                  << std::string(static_cast<size_t>(width - 1) - utf8_length(line), ' ') << "║\n";
    }
}

inline void clear() {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << "\033[2J\033[H" << std::flush;
}

inline void banner(const std::string& role, const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(out_mutex);
    const int width = std::min(68, columns() - 2);
    std::cout << CYAN << BOLD;
    border("╔", "═", "╗", width);
    boxed_line("TCP METİN + GÖRÜNTÜ MESAJLAŞMA SİSTEMİ", width);
    border("╠", "═", "╣", width);
    boxed_line("Rol      : " + role, width);
    boxed_line("Uç Nokta : " + endpoint, width);
    border("╚", "═", "╝", width);
    std::cout << RESET << DIM;
    for (const auto& line : wrap("Komutlar: !yardim  !durum  !temizle  !resim <dosya>  cikis", columns()))
        std::cout << line << '\n';
    std::cout << RESET << '\n' << std::flush;
}

inline void status(const std::string& text) {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << GREEN << BOLD << "[✓] " << RESET << text << '\n' << std::flush;
}
inline void info(const std::string& text) {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << CYAN << "[i] " << RESET << text << '\n' << std::flush;
}
inline void warn(const std::string& text) {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << YELLOW << "[!] " << RESET << text << '\n' << std::flush;
}
inline void error(const std::string& text) {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cerr << RED << BOLD << "[X] " << RESET << text << '\n' << std::flush;
}
inline void prompt() {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << MAGENTA << BOLD << "Sen > " << RESET << std::flush;
}

inline void message_box(const std::string& text) {
    std::lock_guard<std::mutex> lock(out_mutex);
    const int width = std::min(68, columns() - 2);
    std::cout << '\n' << BLUE;
    border("┌", "─", "┐", width);
    std::cout << "│ GELEN MESAJ\n" << RESET;
    for (const auto& line : wrap(text, static_cast<size_t>(width - 2)))
        std::cout << BLUE << "│ " << RESET << line << '\n';
    std::cout << BLUE;
    border("└", "─", "┘", width);
    std::cout << RESET << std::flush;
}

inline std::string shell_quote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    return out + "'";
}

inline void image_card(const std::string& path, uint32_t bytes, uint32_t checksum) {
    int preview_result = 0;
    {
        std::lock_guard<std::mutex> lock(out_mutex);
        std::cout << '\n' << GREEN << "[ GELEN GÖRSEL ]\n" << RESET
                  << "  Dosya : " << path << '\n'
                  << "  Boyut : " << bytes << " bayt\n"
                  << "  CRC32 : 0x" << std::hex << std::uppercase << checksum << std::dec
                  << GREEN << " DOĞRULANDI ✓\n" << RESET << std::flush;
        const int width = std::max(16, std::min(58, columns() - 2));
        // Keep options compatible with chafa 1.2.x (the WSL package version).
        const std::string command = "timeout 10s chafa --colors full --symbols block --duration 0 --size "
            + std::to_string(width) + "x20 -- " + shell_quote(path);
        preview_result = std::system(command.c_str());
    }
    if (preview_result != 0)
        warn("Görsel kaydedildi; terminal önizlemesi gösterilemedi. kur.sh ile chafa kurulumunu kontrol edin.");
}

inline void progress(const std::string& label, uint64_t done, uint64_t total) {
    if (total == 0) total = 1;
    const int width = 20;
    const int percent = static_cast<int>(std::min<uint64_t>(100, done * 100 / total));
    const int fill = percent * width / 100;
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << '\r' << CYAN << label << " [" << RESET;
    for (int i = 0; i < width; ++i) std::cout << (i < fill ? "█" : "░");
    std::cout << CYAN << "] " << std::setw(3) << percent << '%' << RESET << std::flush;
    if (done >= total) std::cout << '\n' << std::flush;
}

inline void help() {
    std::lock_guard<std::mutex> lock(out_mutex);
    std::cout << YELLOW << BOLD << "\nKOMUTLAR\n" << RESET
              << "  normal metin          Karşı tarafa metin gönderir\n"
              << "  !resim <dosya>        JPG/JPEG/PNG görsel gönderir\n"
              << "  dosya sürükle-bırak   Görseli komutsuz otomatik gönderir\n"
              << "  !durum                Bağlantı durumunu gösterir\n"
              << "  !temizle              Terminali temizler\n"
              << "  !yardim               Bu menüyü gösterir\n"
              << "  cikis                 Programı kapatır\n\n" << std::flush;
}
}
#endif
