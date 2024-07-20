#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <vector>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <fstream>
#include <sstream>
#include <map>
#include <cstdlib>

// Для загрузки переменных окружения из .env
std::map<std::string, std::string> load_env(const std::string& filename) {
    std::map<std::string, std::string> env_map;
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            env_map[key] = value;
        }
    }
    return env_map;
}

class Server {
public:
    static const int BUFFER_SIZE = 1024; // Объявление BUFFER_SIZE как статической константы

    Server(int port, const std::string& file_name, const std::string& php_path, const std::string& static_files_path)
        : port(port), default_file(file_name), php_path(php_path), static_files_path(static_files_path) {
        // Создание сокета
        server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock == -1) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        // Привязка сокета к порту
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("bind failed");
            close(server_sock);
            exit(EXIT_FAILURE);
        }

        // Прослушивание входящих соединений
        if (listen(server_sock, 10) < 0) {
            perror("listen failed");
            close(server_sock);
            exit(EXIT_FAILURE);
        }

        std::cout << "Server listening on port " << port << std::endl;
    }

    ~Server() {
        close(server_sock);
    }

    void start() {
        while (true) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int *client_sock = new int;

            *client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
            if (*client_sock < 0) {
                perror("accept failed");
                delete client_sock;
                continue;
            }

            // Передаем указатель на сервер вместе с клиентским сокетом
            ClientArgs* args = new ClientArgs{*client_sock, default_file, php_path, static_files_path};
            // Создание потока для обработки клиента
            pthread_t thread;
            if (pthread_create(&thread, nullptr, &Server::handle_client, args) != 0) {
                perror("pthread_create failed");
                delete client_sock;
                delete args;
            } else {
                pthread_detach(thread);
            }
        }
    }

private:
    int server_sock;
    struct sockaddr_in server_addr;
    int port;
    std::string default_file;
    std::string php_path;
    std::string static_files_path;

    struct ClientArgs {
        int client_sock;
        std::string default_file;
        std::string php_path;
        std::string static_files_path;
    };

    static void *handle_client(void *arg) {
        ClientArgs* args = static_cast<ClientArgs*>(arg);
        int client_sock = args->client_sock;
        std::string default_file = args->default_file;
        std::string php_path = args->php_path;
        std::string static_files_path = args->static_files_path;
        delete args;

        // Получение информации о клиенте
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        getpeername(client_sock, (struct sockaddr *)&client_addr, &addr_len);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::cout << "Connection from " << client_ip << ":" << ntohs(client_addr.sin_port) << std::endl;

        // Чтение запроса клиента
        char buffer[BUFFER_SIZE]; // Используем BUFFER_SIZE
        ssize_t bytes_read = read(client_sock, buffer, BUFFER_SIZE - 1);
        if (bytes_read < 0) {
            perror("read failed");
            close(client_sock);
            return nullptr;
        }
        buffer[bytes_read] = '\0'; // Нуль-терминируем строку

        // Извлечение имени файла из URL
        std::string request(buffer);
        std::string requested_file;
        size_t pos1 = request.find("GET /");
        if (pos1 != std::string::npos) {
            size_t pos2 = request.find(" ", pos1 + 5);
            if (pos2 != std::string::npos) {
                requested_file = request.substr(pos1 + 5, pos2 - (pos1 + 5));
            }
        }
        if (requested_file.empty() || requested_file == "/") {
            requested_file = default_file; // Файл по умолчанию
        }

        // Определение пути к файлу
        std::string full_path = static_files_path + "/" + requested_file;

        // Определение типа содержимого
        std::string content_type = get_content_type(requested_file);

        // Обработка PHP файла
        if (requested_file.substr(requested_file.find_last_of('.') + 1) == "php") {
            // Запуск внешнего процесса и получение вывода
            std::string command = php_path + " " + full_path;
            std::string output = execute_command(command.c_str());

            // Формирование HTTP-ответа
            std::string response = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Content-Length: " + std::to_string(output.size()) + "\r\n"
                                   "\r\n" +
                                   output;

            write(client_sock, response.c_str(), response.size());
        } else {
            // Чтение статического файла
            std::ifstream file(full_path, std::ios::binary);
            if (!file) {
                std::string not_found_response = "HTTP/1.1 404 Not Found\r\n"
                                                 "Content-Type: text/html\r\n"
                                                 "Content-Length: 0\r\n"
                                                 "\r\n";
                write(client_sock, not_found_response.c_str(), not_found_response.size());
            } else {
                std::ostringstream oss;
                oss << file.rdbuf();
                std::string file_content = oss.str();

                // Формирование HTTP-ответа
                std::string response = "HTTP/1.1 200 OK\r\n"
                                       "Content-Type: " + content_type + "\r\n"
                                       "Content-Length: " + std::to_string(file_content.size()) + "\r\n"
                                       "\r\n" +
                                       file_content;

                write(client_sock, response.c_str(), response.size());
            }
        }
        close(client_sock);

        return nullptr;
    }

    static std::string execute_command(const char* cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    static std::string get_content_type(const std::string& file_name) {
        std::string extension = file_name.substr(file_name.find_last_of('.') + 1);
        if (extension == "html" || extension == "htm") return "text/html";
        if (extension == "js") return "application/javascript";
        if (extension == "css") return "text/css";
        if (extension == "png") return "image/png";
        if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
        if (extension == "gif") return "image/gif";
        return "application/octet-stream"; // По умолчанию
    }
};

int main() {
    // Загрузка конфигурации из .env файла
    auto env = load_env(".env");

    // Получение значений из переменных окружения
    int port = std::stoi(env["PORT"]);
    std::string default_file = env["FILE_NAME"];
    std::string php_path = env["PHP_PATH"];
    std::string static_files_path = env["STATIC_FILES_PATH"];

    // Создание экземпляра сервера
    Server server(port, default_file, php_path, static_files_path);

    // Запуск сервера
    server.start();

    return 0;
}

