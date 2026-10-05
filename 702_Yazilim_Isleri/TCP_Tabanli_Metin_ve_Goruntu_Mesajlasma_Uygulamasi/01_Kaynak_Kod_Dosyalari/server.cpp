#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include "chat_common.h"

namespace {
constexpr int PORT = 5000;

struct Client {
    uint16_t id;
    int socket;
    std::mutex send_mutex;
    Client(uint16_t client_id, int socket_fd) : id(client_id), socket(socket_fd) {}
};

std::mutex clients_mutex;
std::map<uint16_t, std::shared_ptr<Client>> clients;

bool send_locked(const std::shared_ptr<Client>& client, const Frame& frame) {
    std::lock_guard<std::mutex> lock(client->send_mutex);
    return send_frame(client->socket, frame);
}

void send_error(const std::shared_ptr<Client>& client, const std::string& message) {
    Frame frame{MSG_ERROR, 0, {}, bytes(message.substr(0, MAX_ERROR_SIZE))};
    send_locked(client, frame);
}

uint16_t reserve_id() {
    for (uint32_t id = 1; id <= 65535; ++id)
        if (clients.find(static_cast<uint16_t>(id)) == clients.end()) return static_cast<uint16_t>(id);
    return 0;
}

Frame list_response(uint16_t requester) {
    Frame response{MSG_LIST_RESPONSE, 0, {}, {}};
    std::lock_guard<std::mutex> lock(clients_mutex);
    response.data.push_back(static_cast<unsigned char>(clients.size() - 1));
    for (const auto& [id, unused] : clients) {
        (void)unused;
        if (id == requester) continue;
        const uint16_t network_id = htons(id);
        const auto* raw = reinterpret_cast<const unsigned char*>(&network_id);
        response.data.insert(response.data.end(), raw, raw + sizeof(network_id));
    }
    return response;
}

bool valid_client_frame(const Frame& frame, std::string& error) {
    if (frame.type == MSG_LIST_REQUEST) {
        if (frame.client_id || !frame.filename.empty() || !frame.data.empty()) {
            error = "Liste isteği alanları sıfır olmalıdır."; return false;
        }
        return true;
    }
    if (frame.type == MSG_TEXT) {
        if (!frame.filename.empty() || frame.data.empty() || frame.data.size() > MAX_TEXT_SIZE) {
            error = "Geçersiz metin çerçevesi."; return false;
        }
        if (!valid_utf8(frame.data)) { error = "Metin geçerli UTF-8 değil."; return false; }
        return true;
    }
    if (frame.type == MSG_IMAGE) {
        if (!safe_wire_filename(frame.filename) || frame.data.empty() ||
            frame.data.size() > MAX_IMAGE_SIZE || !valid_image_signature(frame.filename, frame.data)) {
            error = "Geçersiz görüntü, dosya adı veya JPG/PNG imzası."; return false;
        }
        return true;
    }
    error = "İstemciden gelen ileti türüne izin verilmiyor.";
    return false;
}

void client_worker(const std::shared_ptr<Client>& client) {
    uint16_t network_id = htons(client->id);
    Frame assignment{MSG_ID_ASSIGN, 0, {}, std::vector<unsigned char>(2)};
    std::memcpy(assignment.data.data(), &network_id, 2);
    if (!send_locked(client, assignment)) goto cleanup;
    ui::status("İstemci bağlandı; kimlik: " + std::to_string(client->id));

    for (;;) {
        Frame incoming;
        std::string error;
        if (!receive_frame(client->socket, incoming, error)) {
            if (!error.empty()) send_error(client, error);
            if (error == "CRC32 doğrulaması başarısız.") continue;
            break;
        }
        if (!valid_client_frame(incoming, error)) {
            send_error(client, error);
            continue;
        }
        if (incoming.type == MSG_LIST_REQUEST) {
            if (!send_locked(client, list_response(client->id))) break;
            continue;
        }
        if (incoming.client_id == 0 || incoming.client_id == client->id) {
            send_error(client, "Hedef kimliği geçersiz; kendinize ileti gönderemezsiniz.");
            continue;
        }
        std::shared_ptr<Client> target;
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            auto it = clients.find(incoming.client_id);
            if (it != clients.end()) target = it->second;
        }
        if (!target) {
            send_error(client, "Hedef istemci bağlı değil.");
            continue;
        }
        incoming.client_id = client->id;
        if (!send_locked(target, incoming))
            send_error(client, "İleti hedefe teslim edilemedi.");
    }

cleanup:
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        auto it = clients.find(client->id);
        if (it != clients.end() && it->second == client) clients.erase(it);
    }
    {
        std::lock_guard<std::mutex> lock(client->send_mutex);
        shutdown(client->socket, SHUT_RDWR);
        close(client->socket);
    }
    ui::warn("İstemci ayrıldı; kimlik: " + std::to_string(client->id));
}
}

int main() {
    ui::clear();
    ui::banner("SUNUCU", "0.0.0.0:5000 • en fazla 5 istemci");
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { ui::error("Socket oluşturulamadı."); return 1; }
    int enabled = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 ||
        listen(server_fd, MAX_CLIENTS) < 0) {
        ui::error(std::string("Sunucu başlatılamadı: ") + std::strerror(errno));
        close(server_fd); return 1;
    }
    ui::info("5000/TCP dinleniyor; protokol: 13 bayt başlık + CRC32.");
    for (;;) {
        int socket_fd = accept(server_fd, nullptr, nullptr);
        if (socket_fd < 0) { if (errno == EINTR) continue; ui::error("accept başarısız."); break; }
        set_socket_timeout(socket_fd);
        std::shared_ptr<Client> client;
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            if (clients.size() < MAX_CLIENTS) {
                uint16_t id = reserve_id();
                client = std::make_shared<Client>(id, socket_fd);
                clients[id] = client;
            }
        }
        if (!client) {
            Frame rejected{MSG_ERROR, 0, {}, bytes("Sunucu kapasitesi dolu (en fazla 5 istemci).")};
            send_frame(socket_fd, rejected); shutdown(socket_fd, SHUT_RDWR); close(socket_fd);
            continue;
        }
        std::thread(client_worker, client).detach();
    }
    close(server_fd);
    return 0;
}
