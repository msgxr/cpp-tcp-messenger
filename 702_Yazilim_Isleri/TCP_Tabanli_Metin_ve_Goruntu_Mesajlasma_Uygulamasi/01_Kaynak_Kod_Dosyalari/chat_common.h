#ifndef CHAT_COMMON_H
#define CHAT_COMMON_H

#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/time.h>
#include "protocol.h"
#include "ui.h"

namespace fs = std::filesystem;

struct Frame {
    uint8_t type = 0;
    uint16_t client_id = 0;
    std::string filename;
    std::vector<unsigned char> data;
};

inline void set_socket_timeout(int sock, int seconds = 30) {
    timeval timeout{seconds, 0};
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}

inline void set_receive_timeout(int sock, int seconds) {
    timeval timeout{seconds, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

inline bool valid_utf8(const std::vector<unsigned char>& data) {
    for (size_t i = 0; i < data.size();) {
        unsigned char c = data[i];
        size_t n = c < 0x80 ? 1 : (c & 0xe0) == 0xc0 ? 2 :
                   (c & 0xf0) == 0xe0 ? 3 : (c & 0xf8) == 0xf0 ? 4 : 0;
        if (!n || i + n > data.size() || (n == 2 && c < 0xc2) || (n == 4 && c > 0xf4)) return false;
        for (size_t j = 1; j < n; ++j) if ((data[i + j] & 0xc0) != 0x80) return false;
        if ((n == 3 && c == 0xe0 && data[i + 1] < 0xa0) ||
            (n == 3 && c == 0xed && data[i + 1] >= 0xa0) ||
            (n == 4 && c == 0xf0 && data[i + 1] < 0x90) ||
            (n == 4 && c == 0xf4 && data[i + 1] >= 0x90)) return false;
        i += n;
    }
    return true;
}

inline uint32_t frame_crc(const std::string& filename, const std::vector<unsigned char>& data) {
    uint32_t crc = crc32_begin();
    crc = crc32_update(crc, reinterpret_cast<const unsigned char*>(filename.data()), filename.size());
    crc = crc32_update(crc, data.data(), data.size());
    return crc32_end(crc);
}

inline bool send_frame(int sock, const Frame& frame) {
    MessageHeader header{};
    header.type = frame.type;
    header.client_id = htons(frame.client_id);
    header.filename_len = htons(static_cast<uint16_t>(frame.filename.size()));
    header.data_size = htonl(static_cast<uint32_t>(frame.data.size()));
    header.checksum = htonl(frame.filename.empty() && frame.data.empty() ? 0u : frame_crc(frame.filename, frame.data));
    return send_exact(sock, &header, sizeof(header)) &&
           (frame.filename.empty() || send_exact(sock, frame.filename.data(), frame.filename.size())) &&
           (frame.data.empty() || send_exact(sock, frame.data.data(), frame.data.size()));
}

inline bool receive_frame(int sock, Frame& frame, std::string& error) {
    MessageHeader header{};
    if (!read_exact(sock, &header, 1)) return false;
    set_receive_timeout(sock, 30);
    if (!read_exact(sock, reinterpret_cast<unsigned char*>(&header) + 1, sizeof(header) - 1)) {
        set_receive_timeout(sock, 0);
        error = "Başlık tamamlanmadan bağlantı kesildi veya zaman aşımı oluştu.";
        return false;
    }
    frame.type = header.type;
    frame.client_id = ntohs(header.client_id);
    const uint16_t name_size = ntohs(header.filename_len);
    const uint32_t data_size = ntohl(header.data_size);
    const uint32_t expected_crc = ntohl(header.checksum);
    if (name_size > MAX_FILENAME_SIZE || data_size > MAX_IMAGE_SIZE) {
        set_receive_timeout(sock, 0);
        error = "Başlıkta izin verilen boyut sınırı aşıldı.";
        return false;
    }
    frame.filename.assign(name_size, '\0');
    frame.data.assign(data_size, 0);
    if ((name_size && !read_exact(sock, frame.filename.data(), name_size)) ||
        (data_size && !read_exact(sock, frame.data.data(), data_size))) {
        set_receive_timeout(sock, 0);
        error = "İleti tamamlanmadan bağlantı kesildi veya zaman aşımı oluştu.";
        return false;
    }
    set_receive_timeout(sock, 0);
    const uint32_t actual = name_size || data_size ? frame_crc(frame.filename, frame.data) : 0u;
    if (actual != expected_crc) { error = "CRC32 doğrulaması başarısız."; return false; }
    return true;
}

inline bool valid_image_signature(const std::string& name, const std::vector<unsigned char>& data) {
    std::string ext = fs::path(name).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    const bool jpeg = data.size() >= 3 && data[0] == 0xff && data[1] == 0xd8 && data[2] == 0xff;
    const unsigned char png[]{0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
    const bool is_png = data.size() >= 8 && std::equal(std::begin(png), std::end(png), data.begin());
    return ((ext == ".jpg" || ext == ".jpeg") && jpeg) || (ext == ".png" && is_png);
}

inline bool safe_wire_filename(const std::string& name) {
    return !name.empty() && name.size() <= MAX_FILENAME_SIZE &&
           name == fs::path(name).filename().string() && name != "." && name != ".." &&
           name.find('/') == std::string::npos && name.find('\\') == std::string::npos;
}

inline std::string sanitized_filename(const std::string& name) {
    std::string result;
    for (unsigned char c : fs::path(name).filename().string())
        result += std::isalnum(c) || c == '.' || c == '_' || c == '-' ? static_cast<char>(c) : '_';
    if (result.empty()) result = "gorsel.bin";
    if (result.size() > 180) result.resize(180);
    return result;
}

inline fs::path unique_path(const fs::path& directory, const std::string& filename) {
    fs::path candidate = directory / filename;
    for (unsigned int i = 1; fs::exists(candidate); ++i)
        candidate = directory / (fs::path(filename).stem().string() + "_" + std::to_string(i) + fs::path(filename).extension().string());
    return candidate;
}

inline bool save_received_image(const Frame& frame, fs::path& saved, std::string& error) {
    const fs::path directory = "702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/04_Uygulama_Ciktilari/Alinan_Dosyalar";
    std::error_code ec;
    fs::create_directories(directory, ec);
    if (ec) { error = "Alınan dosyalar dizini oluşturulamadı."; return false; }
    const std::string filename = sanitized_filename(frame.filename);
    int reservation = -1;
    for (unsigned int attempt = 0; attempt < 10000; ++attempt) {
        saved = unique_path(directory, filename);
        reservation = ::open(saved.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (reservation >= 0 || errno != EEXIST) break;
    }
    if (reservation < 0) { error = "Benzersiz görüntü adı ayrılamadı."; return false; }
    close(reservation);
    fs::path temporary = saved;
    temporary += ".part." + std::to_string(getpid());
    std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    if (!out) { fs::remove(saved, ec); error = "Geçici görüntü dosyası oluşturulamadı."; return false; }
    out.write(reinterpret_cast<const char*>(frame.data.data()), static_cast<std::streamsize>(frame.data.size()));
    out.close();
    if (!out) { fs::remove(temporary, ec); fs::remove(saved, ec); error = "Görüntü diske yazılamadı."; return false; }
    fs::rename(temporary, saved, ec);
    if (ec) { fs::remove(temporary, ec); fs::remove(saved, ec); error = "Görüntü kaydı tamamlanamadı."; return false; }
    return true;
}

inline std::vector<unsigned char> bytes(const std::string& text) { return {text.begin(), text.end()}; }

#endif
