#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "protocol.h"

namespace fs = std::filesystem;
#define PORT 5000

// Görüntü gönderme fonksiyonu
bool send_image(int sock, const std::string& filepath) {
    if (!fs::exists(filepath)) {
        std::cerr << "[Hata] Dosya bulunamadi: " << filepath << std::endl;
        return false;
    }

    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[Hata] Dosya acilamadi!" << std::endl;
        return false;
    }

    std::streamsize file_size = file.tellg();
    if (file_size < 0 || file_size > MAX_IMAGE_SIZE) {
        std::cerr << "[Hata] Dosya boyutu 50 MB'dan buyuk olamaz!" << std::endl;
        return false;
    }
    file.seekg(0, std::ios::beg);

    std::string filename = fs::path(filepath).filename().string();
    std::string extension = fs::path(filename).extension().string();
    if (extension != ".jpg" && extension != ".JPG" &&
        extension != ".png" && extension != ".PNG") {
        std::cerr << "[Hata] Sadece JPG ve PNG dosyalari gonderilebilir!" << std::endl;
        return false;
    }
    if (filename.empty() || filename.size() > UINT16_MAX) {
        std::cerr << "[Hata] Dosya adi cok uzun!" << std::endl;
        return false;
    }
    uint16_t name_len = static_cast<uint16_t>(filename.size());

    // 7 baytlık başlık
    MessageHeader header;
    header.type = MSG_IMAGE;
    header.filename_len = htons(name_len);
    header.data_size = htonl(static_cast<uint32_t>(file_size));

    if (!send_exact(sock, &header, sizeof(header))) return false;
    if (!send_exact(sock, filename.data(), name_len)) return false;

    // Dosyayı parça parça gönder
    std::vector<char> buffer(4096);
    while (file_size > 0) {
        std::streamsize to_read = std::min(file_size, static_cast<std::streamsize>(buffer.size()));
        file.read(buffer.data(), to_read);
        if (!send_exact(sock, buffer.data(), to_read)) return false;
        file_size -= to_read;
    }

    std::cout << "[Bilgi] Gorsel gonderildi: " << filename << std::endl;
    return true;
}

// Arka planda sürekli gelen mesaj ve görselleri dinleyen fonksiyon
void receive_thread_func(int sock) {
    while (true) {
        MessageHeader header;
        if (!read_exact(sock, &header, sizeof(header))) {
            std::cout << "\n[Bilgi] Karsi taraf baglantiyi kapatti." << std::endl;
            break;
        }

        uint16_t filename_len = ntohs(header.filename_len);
        uint32_t data_size = ntohl(header.data_size);

        if (header.type != MSG_TEXT && header.type != MSG_IMAGE) {
            std::cerr << "\n[Hata] Gecersiz mesaj turu." << std::endl;
            break;
        }
        if ((header.type == MSG_TEXT && (filename_len != 0 || data_size > MAX_TEXT_SIZE)) ||
            (header.type == MSG_IMAGE && (filename_len == 0 || data_size > MAX_IMAGE_SIZE))) {
            std::cerr << "\n[Hata] Mesaj boyutu gecersiz." << std::endl;
            break;
        }

        if (header.type == MSG_TEXT) {
            std::vector<char> buffer(data_size + 1, 0);
            if (!read_exact(sock, buffer.data(), data_size)) break;
            std::cout << "\n[Gelen Mesaj]: " << buffer.data() << std::endl;
            std::cout << "Sen: " << std::flush;
        } 
        else if (header.type == MSG_IMAGE) {
            std::string filename(filename_len, '\0');
            if (!read_exact(sock, &filename[0], filename_len)) break;

            fs::create_directories("alinanlar");
            std::string save_path = "alinanlar/" + filename;

            if (fs::exists(save_path)) {
                std::cerr << "\n[Hata] Dosya zaten var, uzerine yazilmadi: " << save_path << std::endl;
                std::vector<char> ignored(data_size);
                if (!read_exact(sock, ignored.data(), data_size)) break;
                continue;
            }

            std::ofstream out_file(save_path, std::ios::binary | std::ios::out);
            if (!out_file.is_open()) {
                std::cerr << "\n[Hata] Alinan dosya olusturulamadi." << std::endl;
                break;
            }
            uint32_t remaining = data_size;
            std::vector<char> buffer(4096);
            bool success = true;

            while (remaining > 0) {
                uint32_t to_read = std::min(remaining, static_cast<uint32_t>(buffer.size()));
                if (!read_exact(sock, buffer.data(), to_read)) {
                    success = false;
                    break;
                }
                out_file.write(buffer.data(), to_read);
                remaining -= to_read;
            }

            if (success) {
                std::cout << "\n[Gelen Gorsel]: " << save_path << " dosyasina kaydedildi." << std::endl;
                
            } else {
                std::cerr << "\n[Hata] Gorsel eksik alindi!" << std::endl;
            }
            std::cout << "Sen: " << std::flush;
        }
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 1);

    std::cout << "Sunucu " << PORT << " portunda dinliyor..." << std::endl;

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

    std::cout << "-> Baglanti kuruldu! Metin yazabilir veya '!resim dosya.png' iletebilirsiniz." << std::endl;

    std::thread receiver(receive_thread_func, client_socket);

    std::string line;
    while (true) {
        std::cout << "Sen: " << std::flush;
        if (!std::getline(std::cin, line) || line == "cikis") break;
        if (line.empty()) continue;

        if (line.rfind("!resim ", 0) == 0) {
            std::string path = line.substr(7);
            send_image(client_socket, path);
        } else {
            MessageHeader header;
            header.type = MSG_TEXT;
            header.filename_len = htons(0);
            if (line.size() > MAX_TEXT_SIZE) {
                std::cerr << "[Hata] Metin 1 MB'dan buyuk olamaz!" << std::endl;
                continue;
            }
            header.data_size = htonl(static_cast<uint32_t>(line.size()));

            if (!send_exact(client_socket, &header, sizeof(header)) ||
                !send_exact(client_socket, line.data(), line.size())) break;
        }
    }

    close(client_socket);
    close(server_fd);
    if (receiver.joinable()) receiver.join();
    return 0;
}