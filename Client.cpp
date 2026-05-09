#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <Windows.h>
#include <vector>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// ПРАВИЛЬНЫЙ ВЫВОД (UTF-8 -> Консоль)
void sys_print(std::string s) {
    if (s.empty()) return;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    if (wlen <= 0) return;
    std::wstring ws(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], wlen);
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), ws.c_str(), (DWORD)wcslen(ws.c_str()), NULL, NULL);
}

// ПРАВИЛЬНЫЙ ВВОД (Консоль -> UTF-8)
std::string sys_input() {
    wchar_t wbuf[1024];
    DWORD read;
    if (!ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), wbuf, 1024, &read, NULL)) return "";

    std::wstring ws(wbuf, read);
    // Убираем символы переноса строки
    ws.erase(std::remove(ws.begin(), ws.end(), L'\r'), ws.end());
    ws.erase(std::remove(ws.begin(), ws.end(), L'\n'), ws.end());

    if (ws.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, NULL, 0, NULL, NULL);
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], len, NULL, NULL);
    return s.c_str();
}

int main() {
    // Включаем UTF-8 и Цвета
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    try {
        net::io_context ioc;
        tcp::resolver resolver{ ioc };
        auto ws = std::make_shared<websocket::stream<beast::tcp_stream>>(ioc);

        // Укажи IP своего сервера!
        auto const results = resolver.resolve("192.168.1.11", "8080");
        beast::get_lowest_layer(*ws).connect(results);
        ws->handshake("192.168.1.11", "/");

        sys_print((const char*)u8"Ваш никнейм: ");
        std::string nickname = sys_input();
        ws->write(net::buffer(nickname.empty() ? "User" : nickname));

        std::thread([ws]() {
            try {
                beast::flat_buffer buffer;
                while (true) {
                    buffer.clear();
                    ws->read(buffer);
                    std::string msg = beast::buffers_to_string(buffer.data());
                    // Выводим сообщение и возвращаем приглашение >>
                    sys_print("\r" + msg + "\n\033[31m>> \033[0m");
                }
            }
            catch (...) {}
            }).detach();

        while (true) {
            sys_print("\033[31m>> \033[0m");
            std::string line = sys_input();
            if (line == "exit") break;
            if (line.empty()) continue;

            beast::error_code ec;
            ws->write(net::buffer(line), ec);
            if (ec) break;
        }
    }
    catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        system("pause");
    }
    return 0;
}
