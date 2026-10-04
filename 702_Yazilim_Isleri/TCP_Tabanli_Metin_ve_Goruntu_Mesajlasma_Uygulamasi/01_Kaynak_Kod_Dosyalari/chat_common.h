#ifndef CHAT_COMMON_H
#define CHAT_COMMON_H

#include <ctime>
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>
#include "protocol.h"
#include "ui.h"

namespace fs = std::filesystem;

inline std::string lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

inline bool valid_image_extension(const std::string& filename) {
    std::string ext = lower_copy(fs::path(filename).extension().string());
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png";
}

inline std::string safe_filename(const std::string& raw) {
    std::string name = fs::path(raw).filename().string();
    std::string out;
    out.reserve(name.size());
    for (unsigned char c : name) {
        if (std::isalnum(c) || c == '.' || c == '_' || c == '-') out.push_back(static_cast<char>(c));
        else out.push_back('_');
    }
    if (out.empty()) out = "gorsel.bin";
    if (out.size() > 180) out.resize(180);
    return out;
}

inline fs::path unique_save_path(const fs::path& dir, const std::string& filename) {
    fs::path candidate = dir / filename;
    if (!fs::exists(candidate)) return candidate;
    fs::path stem = candidate.stem();
    fs::path ext = candidate.extension();
    for (int i = 1; i < 10000; ++i) {
        fs::path p = dir / (stem.string() + "_" + std::to_string(i) + ext.string());
        if (!fs::exists(p)) return p;
    }
    return dir / ("alinan_" + std::to_string(::time(nullptr)) + ext.string());
}

inline bool file_crc32(const std::string& filepath, uint32_t& result) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return false;
    std::vector<unsigned char> buffer(8192);
    uint32_t crc = crc32_begin();
    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        std::streamsize n = file.gcount();
        if (n > 0) crc = crc32_update(crc, buffer.data(), static_cast<size_t>(n));
    }
    if (file.bad()) return false;
    result = crc32_end(crc);
    return true;
}

inline bool send_text_message(int sock, const std::string& text) {
    if (text.size() > MAX_TEXT_SIZE) {
        ui::error("Metin 1 MB'dan büyük olamaz.");
        return false;
    }
    MessageHeader header{};
    header.type = MSG_TEXT;
    header.filename_len = htons(0);
    header.data_size = htonl(static_cast<uint32_t>(text.size()));
    header.checksum = htonl(crc32_bytes(text.data(), text.size()));
    return send_exact(sock, &header, sizeof(header)) &&
           (text.empty() || send_exact(sock, text.data(), text.size()));
}

inline bool send_image_message(int sock, const std::string& filepath) {
    if (!fs::exists(filepath) || !fs::is_regular_file(filepath)) {
        ui::error("Dosya bulunamadı: " + filepath);
        return false;
    }
    if (!valid_image_extension(filepath)) {
        ui::error("Yalnızca JPG/JPEG/PNG gönderilebilir.");
        return false;
    }

    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        ui::error("Dosya açılamadı.");
        return false;
    }
    std::streamsize sz = file.tellg();
    if (sz <= 0 || static_cast<uint64_t>(sz) > MAX_IMAGE_SIZE) {
        ui::error("Görsel boş olamaz ve 50 MB'dan büyük olamaz.");
        return false;
    }
    uint32_t size = static_cast<uint32_t>(sz);
    std::string filename = fs::path(filepath).filename().string();
    if (filename.empty() || filename.size() > 255) {
        ui::error("Dosya adı geçersiz veya çok uzun.");
        return false;
    }

    uint32_t checksum = 0;
    if (!file_crc32(filepath, checksum)) {
        ui::error("CRC32 hesaplanamadı.");
        return false;
    }

    MessageHeader header{};
    header.type = MSG_IMAGE;
    header.filename_len = htons(static_cast<uint16_t>(filename.size()));
    header.data_size = htonl(size);
    header.checksum = htonl(checksum);

    if (!send_exact(sock, &header, sizeof(header)) ||
        !send_exact(sock, filename.data(), filename.size())) {
        ui::error("Görsel üst bilgisi gönderilemedi.");
        return false;
    }

    file.clear();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(8192);
    uint32_t sent = 0;
    int last_percent = -5;
    while (sent < size) {
        uint32_t want = std::min<uint32_t>(static_cast<uint32_t>(buffer.size()), size - sent);
        file.read(buffer.data(), want);
        std::streamsize n = file.gcount();
        if (n <= 0 || !send_exact(sock, buffer.data(), static_cast<size_t>(n))) {
            ui::error("Görsel aktarımı yarıda kesildi.");
            return false;
        }
        sent += static_cast<uint32_t>(n);
        int percent = size ? static_cast<int>((static_cast<uint64_t>(sent) * 100) / size) : 100;
        if (percent >= last_percent + 5 || sent == size) {
            ui::progress("Gönderiliyor", sent, size);
            last_percent = percent;
        }
    }
    ui::status("Görsel gönderildi: " + filename + " | CRC32 doğrulama değeri hazır.");
    return true;
}

inline bool receive_one(int sock) {
    MessageHeader header{};
    if (!read_exact(sock, &header, sizeof(header))) return false;

    uint16_t filename_len = ntohs(header.filename_len);
    uint32_t data_size = ntohl(header.data_size);
    uint32_t expected_crc = ntohl(header.checksum);

    if (header.type != MSG_TEXT && header.type != MSG_IMAGE) {
        ui::error("Geçersiz mesaj türü alındı.");
        return false;
    }

    if (header.type == MSG_TEXT) {
        if (filename_len != 0 || data_size > MAX_TEXT_SIZE) {
            ui::error("Geçersiz metin paketi.");
            return false;
        }
        std::vector<char> data(data_size);
        if (data_size && !read_exact(sock, data.data(), data_size)) return false;
        uint32_t actual = crc32_bytes(data.data(), data.size());
        if (actual != expected_crc) {
            ui::error("Metin CRC32 kontrolü başarısız; veri gösterilmedi.");
            return true;
        }
        ui::message_box(std::string(data.begin(), data.end()));
        ui::status("Metin bütünlüğü CRC32 ile doğrulandı.");
        ui::prompt();
        return true;
    }

    if (filename_len == 0 || filename_len > 255 || data_size == 0 || data_size > MAX_IMAGE_SIZE) {
        ui::error("Geçersiz görsel paketi.");
        return false;
    }

    std::string raw_name(filename_len, '\0');
    if (!read_exact(sock, raw_name.data(), filename_len)) return false;
    std::string filename = safe_filename(raw_name);
    if (!valid_image_extension(filename)) {
        ui::error("Alınan dosyanın uzantısı kabul edilmiyor.");
        return false;
    }

    fs::create_directories("702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/04_Uygulama_Ciktilari/Alinan_Dosyalar");
    const fs::path output_dir = "702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/04_Uygulama_Ciktilari/Alinan_Dosyalar";
    fs::path save_path;
    int reserved = -1;
    for (int attempt = 0; attempt < 10000; ++attempt) {
        save_path = unique_save_path(output_dir, filename);
        reserved = ::open(save_path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (reserved >= 0 || errno != EEXIST) break;
    }
    if (reserved < 0) {
        ui::error("Alınan dosya için benzersiz kayıt yolu oluşturulamadı.");
        return false;
    }
    ::close(reserved);
    std::ofstream out(save_path, std::ios::binary);
    if (!out) {
        fs::remove(save_path);
        ui::error("Alınan dosya oluşturulamadı.");
        return false;
    }

    std::vector<unsigned char> buffer(8192);
    uint32_t remaining = data_size;
    uint32_t received = 0;
    uint32_t crc = crc32_begin();
    int last_percent = -5;
    while (remaining > 0) {
        uint32_t n = std::min<uint32_t>(remaining, static_cast<uint32_t>(buffer.size()));
        if (!read_exact(sock, buffer.data(), n)) {
            out.close();
            fs::remove(save_path);
            ui::error("Görsel aktarımı yarıda kesildi.");
            return false;
        }
        out.write(reinterpret_cast<char*>(buffer.data()), n);
        if (!out) {
            out.close();
            fs::remove(save_path);
            ui::error("Alınan görsel diske yazılamadı.");
            return false;
        }
        crc = crc32_update(crc, buffer.data(), n);
        remaining -= n;
        received += n;
        int percent = data_size ? static_cast<int>((static_cast<uint64_t>(received) * 100) / data_size) : 100;
        if (percent >= last_percent + 5 || received == data_size) {
            ui::progress("Alınıyor    ", received, data_size);
            last_percent = percent;
        }
    }
    out.close();
    if (!out) {
        fs::remove(save_path);
        ui::error("Alınan görsel dosyası tamamlanamadı.");
        return false;
    }
    uint32_t actual_crc = crc32_end(crc);
    if (actual_crc != expected_crc) {
        fs::remove(save_path);
        ui::error("Görsel CRC32 kontrolü başarısız; bozuk dosya silindi.");
        ui::prompt();
        return true;
    }

    ui::image_card(save_path.string(), data_size, actual_crc);
    ui::prompt();
    return true;
}

inline void receive_loop(int sock) {
    while (receive_one(sock)) {}
    ui::warn("Karşı taraf bağlantıyı kapattı veya bağlantı koptu.");
}

#endif
