#include "chat_common.h"

#include <arpa/inet.h>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <set>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {
constexpr uint16_t TEST_PORT = 55001;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

int connect_client() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(TEST_PORT);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    if (connect(sock, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        close(sock);
        return -1;
    }
    timeval timeout{2, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    return sock;
}

Frame receive_checked(int sock) {
    Frame frame;
    std::string error;
    require(receive_frame(sock, frame, error), "Çerçeve alınamadı: " + error);
    return frame;
}

uint16_t assigned_id(int sock) {
    Frame frame = receive_checked(sock);
    require(frame.type == MSG_ID_ASSIGN && frame.client_id == 0 &&
            frame.filename.empty() && frame.data.size() == 2,
            "Kimlik atama çerçevesi geçersiz.");
    uint16_t network_id = 0;
    std::memcpy(&network_id, frame.data.data(), sizeof(network_id));
    return ntohs(network_id);
}

void drain_list_updates(int sock) {
    for (;;) {
        pollfd descriptor{sock, POLLIN, 0};
        if (poll(&descriptor, 1, 100) <= 0) return;
        Frame frame = receive_checked(sock);
        require(frame.type == MSG_LIST_RESPONSE,
                "Kimlik atamasından sonra beklenmeyen çerçeve alındı.");
    }
}

struct ServerProcess {
    pid_t pid = -1;
    explicit ServerProcess(const char* executable) {
        pid = fork();
        require(pid >= 0, "Sunucu alt süreci oluşturulamadı.");
        if (pid == 0) {
            int null_fd = open("/dev/null", O_WRONLY);
            if (null_fd >= 0) {
                dup2(null_fd, STDOUT_FILENO);
                dup2(null_fd, STDERR_FILENO);
                close(null_fd);
            }
            const std::string port = std::to_string(TEST_PORT);
            execl(executable, executable, port.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }
    }
    ~ServerProcess() {
        if (pid > 0) {
            kill(pid, SIGTERM);
            waitpid(pid, nullptr, 0);
        }
    }
};

struct Clients {
    std::vector<int> sockets;
    ~Clients() { for (int sock : sockets) close(sock); }
};
}

int main(int argc, char* argv[]) {
    try {
        require(argc == 2, "Kullanım: integration_test <sunucu_yolu>");
        ServerProcess server(argv[1]);
        Clients clients;

        for (int attempt = 0; attempt < 60 && clients.sockets.empty(); ++attempt) {
            int sock = connect_client();
            if (sock >= 0) clients.sockets.push_back(sock);
            else std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        require(!clients.sockets.empty(), "Test sunucusu başlamadı.");
        while (clients.sockets.size() < 5) {
            int sock = connect_client();
            require(sock >= 0, "Beş istemci bağlantısı kurulamadı.");
            clients.sockets.push_back(sock);
        }

        std::vector<uint16_t> ids;
        for (int sock : clients.sockets) ids.push_back(assigned_id(sock));
        require(std::set<uint16_t>(ids.begin(), ids.end()).size() == 5,
                "İstemci kimlikleri benzersiz değil.");
        for (int sock : clients.sockets) drain_list_updates(sock);

        require(send_frame(clients.sockets[0], Frame{MSG_LIST_REQUEST, 0, {}, {}}),
                "Liste isteği gönderilemedi.");
        Frame list = receive_checked(clients.sockets[0]);
        require(list.type == MSG_LIST_RESPONSE && list.client_id == 0 &&
                list.data.size() == 9 && list.data[0] == 4,
                "Aktif istemci listesi geçersiz.");
        std::set<uint16_t> listed;
        for (size_t offset = 1; offset < list.data.size(); offset += 2) {
            uint16_t network_id = 0;
            std::memcpy(&network_id, list.data.data() + offset, 2);
            listed.insert(ntohs(network_id));
        }
        require(listed == std::set<uint16_t>(ids.begin() + 1, ids.end()),
                "Liste yanıtında yanlış istemciler var.");

        const std::string message = "Türkçe özel mesaj";
        require(send_frame(clients.sockets[0], Frame{MSG_TEXT, ids[1], {}, bytes(message)}),
                "Özel metin gönderilemedi.");
        Frame text = receive_checked(clients.sockets[1]);
        require(text.type == MSG_TEXT && text.client_id == ids[0] &&
                std::string(text.data.begin(), text.data.end()) == message,
                "Özel metin doğru hedefe ulaşmadı.");

        const std::string group_message = "İki alıcıya ortak mesaj";
        for (size_t index : {size_t{1}, size_t{2}})
            require(send_frame(clients.sockets[0], Frame{MSG_TEXT, ids[index], {}, bytes(group_message)}),
                    "Çoklu alıcı mesajı gönderilemedi.");
        for (size_t index : {size_t{1}, size_t{2}}) {
            Frame group = receive_checked(clients.sockets[index]);
            require(group.type == MSG_TEXT && group.client_id == ids[0] &&
                    std::string(group.data.begin(), group.data.end()) == group_message,
                    "Çoklu alıcı mesajı doğru istemcilere ulaşmadı.");
        }

        const std::string broadcast_message = "Tüm istemcilere mesaj";
        for (size_t index = 1; index < clients.sockets.size(); ++index)
            require(send_frame(clients.sockets[0], Frame{MSG_TEXT, ids[index], {}, bytes(broadcast_message)}),
                    "Toplu mesaj gönderilemedi.");
        for (size_t index = 1; index < clients.sockets.size(); ++index) {
            Frame broadcast = receive_checked(clients.sockets[index]);
            require(broadcast.type == MSG_TEXT && broadcast.client_id == ids[0] &&
                    std::string(broadcast.data.begin(), broadcast.data.end()) == broadcast_message,
                    "Toplu mesaj bütün istemcilere ulaşmadı.");
        }

        const std::vector<unsigned char> png{0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a,
                                              't', 'e', 's', 't'};
        require(send_frame(clients.sockets[1], Frame{MSG_IMAGE, ids[2], "test.png", png}),
                "Özel görüntü gönderilemedi.");
        Frame image = receive_checked(clients.sockets[2]);
        require(image.type == MSG_IMAGE && image.client_id == ids[1] &&
                image.filename == "test.png" && image.data == png,
                "Özel görüntü doğru hedefe ulaşmadı.");

        require(send_frame(clients.sockets[0], Frame{MSG_TEXT, ids[0], {}, bytes("self")}),
                "Kendi kimliğine gönderim testi başlatılamadı.");
        require(receive_checked(clients.sockets[0]).type == MSG_ERROR,
                "Kendi kimliğine gönderim reddedilmedi.");

        int sixth = connect_client();
        require(sixth >= 0, "Altıncı TCP bağlantısı kurulamadı.");
        require(receive_checked(sixth).type == MSG_ERROR, "Altıncı istemci kapasite hatası almadı.");
        close(sixth);

        shutdown(clients.sockets.back(), SHUT_RDWR);
        close(clients.sockets.back());
        clients.sockets.pop_back();
        int replacement = -1;
        for (int attempt = 0; attempt < 40; ++attempt) {
            int candidate = connect_client();
            if (candidate >= 0) {
                Frame response = receive_checked(candidate);
                if (response.type == MSG_ID_ASSIGN) { replacement = candidate; break; }
                close(candidate);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        require(replacement >= 0, "Boşalan kapasiteye yeni istemci kabul edilmedi.");
        clients.sockets.push_back(replacement);

        std::cout << "çoklu istemci C++ entegrasyon testi başarılı\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "entegrasyon testi başarısız: " << exception.what() << '\n';
        return 1;
    }
}
