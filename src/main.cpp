#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #define _CRT_SECURE_NO_WARNINGS
  #include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <conio.h>
  #pragma comment(lib, "Ws2_32.lib")
  using socket_t = SOCKET;
  #define CLOSE_SOCKET(s) closesocket(s)
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <cstdlib>
  using socket_t = int;
  #define INVALID_SOCKET (-1)
  #define SOCKET_ERROR (-1)
  #define CLOSE_SOCKET(s) close(s)
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <iomanip>
#include <regex>

// Global metrics
std::atomic<uint64_t> g_total_requests(0);
auto g_start_time = std::chrono::steady_clock::now();

#ifdef _WIN32
PROCESS_INFORMATION g_tunnel_pi{};

std::string find_cloudflared() {
    const char* candidates[] = {
        "cloudflared.exe",
        "../../cloudflared.exe",
        "..\\..\\cloudflared.exe",
        "../cloudflared.exe",
        "..\\cloudflared.exe",
        "E:/Cpp/hehe/cloudflared.exe",
        "E:\\Cpp\\hehe\\cloudflared.exe"
    };
    for (const char* p : candidates) {
        if (GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES) {
            return std::string(p);
        }
    }
    return "";
}

void copy_to_clipboard(const std::string& text) {
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hGlob) {
        memcpy(GlobalLock(hGlob), text.c_str(), text.size() + 1);
        GlobalUnlock(hGlob);
        SetClipboardData(CF_TEXT, hGlob);
    }
    CloseClipboard();
}

void start_tunnel_thread(const std::string& cloudflared_path, int mode) {
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return;
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;

    std::string cmd = "\"" + cloudflared_path + "\" tunnel --url http://localhost:8080";
    std::vector<char> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back('\0');

    if (!CreateProcessA(nullptr, cmd_buf.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &g_tunnel_pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return;
    }
    CloseHandle(hWritePipe);

    char buffer[512];
    DWORD bytesRead;
    std::string accumulated;
    std::string public_url = "";
    auto t0 = std::chrono::steady_clock::now();

    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        accumulated += buffer;

        std::regex url_regex(R"(https://[a-zA-Z0-9\-]+\.trycloudflare\.com)");
        std::smatch match;
        if (std::regex_search(accumulated, match, url_regex)) {
            public_url = match.str();
            break;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - t0).count();
        if (elapsed > 25) break;
    }

    if (!public_url.empty()) {
        copy_to_clipboard(public_url);
        std::cout << "\n========================================================================\n";
        if (mode == 2) {
            std::cout << "         >>> OPTION 2: GLOBAL PUBLIC LINK READY (SABKE LIYE) <<<        \n";
        } else {
            std::cout << "         >>> OPTION 3: DUAL HOSTING ACTIVE (BOTH SATH ME) <<<          \n";
        }
        std::cout << "========================================================================\n";
        if (mode == 3) {
            std::cout << " [+] Localhost URL    : http://localhost:8080\n";
        }
        std::cout << " [+] LIVE PUBLIC LINK : " << public_url << "  [COPIED TO CLIPBOARD]\n";
        std::cout << "========================================================================\n";
        std::cout << " [i] Yeh link kisi ko bhi share karein (Phone, Jio/Airtel, Foreign)!\n";
        std::cout << " [i] Jab tak yeh console open hai, tab tak link 100% chalegi.\n";
        std::cout << " [i] Console close karte hi server aur public link turant band ho jayenge.\n";
        std::cout << "========================================================================\n\n";
    }

    CloseHandle(hReadPipe);
}

BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_CLOSE_EVENT || ctrlType == CTRL_BREAK_EVENT) {
        std::cout << "\n[!] Server stopped by user. Cleaning up...\n";
        if (g_tunnel_pi.hProcess) {
            TerminateProcess(g_tunnel_pi.hProcess, 0);
            CloseHandle(g_tunnel_pi.hProcess);
            CloseHandle(g_tunnel_pi.hThread);
        }
        WSACleanup();
        ExitProcess(0);
    }
    return FALSE;
}
#endif

// Read external HTML file if available, otherwise fallback
std::string get_webpage_content() {
    const char* paths[] = {
        "src/website/index.html",
        "src\\website\\index.html",
        "website/index.html",
        "website\\index.html",
        "index.html",
        "../src/website/index.html",
        "..\\src\\website\\index.html",
        "../../src/website/index.html",
        "..\\..\\src\\website\\index.html",
        "E:/Cpp/hehe/src/website/index.html",
        "E:\\Cpp\\hehe\\src\\website\\index.html"
    };

    for (const char* p : paths) {
        std::ifstream file(p, std::ios::binary);
        if (file.is_open()) {
            std::stringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }
    }

    return "<!DOCTYPE html><html><head><title>NIshxnt Arsenal</title></head>"
           "<body style='background:#0d0f12;color:#fff;font-family:sans-serif;text-align:center;padding:50px;'>"
           "<h1>NIshxnt ARSENAL</h1>"
           "<p>White & Grey Minimalist Tactical Engine</p>"
           "</body></html>";
}

std::string get_catalog_content() {
    const char* paths[] = {
        "src/website/catalog.html",
        "src\\website\\catalog.html",
        "website/catalog.html",
        "website\\catalog.html",
        "catalog.html",
        "../src/website/catalog.html",
        "..\\src\\website\\catalog.html",
        "../../src/website/catalog.html",
        "..\\..\\src\\website\\catalog.html",
        "E:/Cpp/hehe/src/website/catalog.html",
        "E:\\Cpp\\hehe\\src\\website\\catalog.html"
    };

    for (const char* p : paths) {
        std::ifstream file(p, std::ios::binary);
        if (file.is_open()) {
            std::stringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }
    }

    return "<!DOCTYPE html><html><head><title>NIshxnt Arsenal - Catalog</title></head>"
           "<body style='background:#050608;color:#fff;font-family:sans-serif;text-align:center;padding:50px;'>"
           "<h1>NIshxnt ARSENAL CATALOG</h1>"
           "<p>Classified Combat Systems</p>"
           "<a href='/' style='color:#ff1e42;'>Return to Base</a>"
           "</body></html>";
}

std::string get_about_content() {
    const char* paths[] = {
        "src/website/about.html",
        "src\\website\\about.html",
        "website/about.html",
        "website\\about.html",
        "about.html",
        "../src/website/about.html",
        "..\\src\\website\\about.html",
        "../../src/website/about.html",
        "..\\..\\src\\website\\about.html",
        "E:/Cpp/hehe/src/website/about.html",
        "E:\\Cpp\\hehe\\src\\website\\about.html"
    };

    for (const char* p : paths) {
        std::ifstream file(p, std::ios::binary);
        if (file.is_open()) {
            std::ostringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }
    }

    return "<!DOCTYPE html><html><head><title>About Developer - NIshxnt</title></head>"
           "<body style='background:#050608;color:#fff;font-family:sans-serif;text-align:center;padding:50px;'>"
           "<h1>Developer - NIshxnt</h1>"
           "<p>Lead Architect</p>"
           "<a href='/catalog' style='color:#ff1e42;'>Return to Catalog</a>"
           "</body></html>";
}

void handle_client(socket_t client_socket) {
    char buffer[4096];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received <= 0) {
        CLOSE_SOCKET(client_socket);
        return;
    }

    g_total_requests++;
    buffer[bytes_received] = '\0';

    // Parse HTTP Method & Route
    std::string request_str(buffer);
    std::string method, route, http_version;
    std::istringstream stream(request_str);
    stream >> method >> route >> http_version;

    std::cout << "[HTTP] " << method << " " << route << " -> Total Requests: " << g_total_requests << "\n";

    std::string response;

    if (method == "OPTIONS") {
        std::ostringstream oss;
        oss << "HTTP/1.1 204 No Content\r\n";
        oss << "Access-Control-Allow-Origin: *\r\n";
        oss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
        oss << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
        oss << "Connection: close\r\n\r\n";
        response = oss.str();
        send(client_socket, response.c_str(), static_cast<int>(response.length()), 0);
        CLOSE_SOCKET(client_socket);
        return;
    }

    if (route == "/" || route == "/index.html") {
        std::string html = get_webpage_content();
        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\n";
        oss << "Content-Type: text/html; charset=UTF-8\r\n";
        oss << "Content-Length: " << html.length() << "\r\n";
        oss << "Connection: close\r\n";
        oss << "Access-Control-Allow-Origin: *\r\n\r\n";
        oss << html;
        response = oss.str();
    }
    else if (route == "/catalog" || route == "/catalog.html") {
        std::string html = get_catalog_content();
        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\n";
        oss << "Content-Type: text/html; charset=UTF-8\r\n";
        oss << "Content-Length: " << html.length() << "\r\n";
        oss << "Connection: close\r\n";
        oss << "Access-Control-Allow-Origin: *\r\n\r\n";
        oss << html;
        response = oss.str();
    }
    else if (route == "/about" || route == "/about.html") {
        std::string html = get_about_content();
        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\n";
        oss << "Content-Type: text/html; charset=UTF-8\r\n";
        oss << "Content-Length: " << html.length() << "\r\n";
        oss << "Connection: close\r\n";
        oss << "Access-Control-Allow-Origin: *\r\n\r\n";
        oss << html;
        response = oss.str();
    }
    else if (route.find('.') != std::string::npos) {
        std::string filename = route;
        if (!filename.empty() && filename[0] == '/') {
            filename = filename.substr(1);
        }
        std::string ext = "";
        auto dot_pos = filename.rfind('.');
        if (dot_pos != std::string::npos) {
            ext = filename.substr(dot_pos);
        }
        std::string mime = "application/octet-stream";
        if (ext == ".mp4") mime = "video/mp4";
        else if (ext == ".png") mime = "image/png";
        else if (ext == ".jpg" || ext == ".jpeg") mime = "image/jpeg";
        else if (ext == ".webp") mime = "image/webp";
        else if (ext == ".svg") mime = "image/svg+xml";
        else if (ext == ".ico") mime = "image/x-icon";
        else if (ext == ".css") mime = "text/css";
        else if (ext == ".js") mime = "application/javascript";
        else if (ext == ".html") mime = "text/html; charset=UTF-8";

        std::vector<std::string> vpaths = {
            "src/website/" + filename,
            "src\\website\\" + filename,
            "website/" + filename,
            "website\\" + filename,
            filename,
            "../src/website/" + filename,
            "..\\src\\website\\" + filename,
            "../../src/website/" + filename,
            "..\\..\\src\\website\\" + filename,
            "E:/Cpp/hehe/src/website/" + filename,
            "E:\\Cpp\\hehe\\src\\website\\" + filename
        };
        for (const auto& vp : vpaths) {
            std::ifstream vf(vp, std::ios::binary);
            if (vf.is_open()) {
                std::ostringstream ss;
                ss << vf.rdbuf();
                std::string content = ss.str();
                std::ostringstream oss;
                oss << "HTTP/1.1 200 OK\r\n";
                oss << "Content-Type: " << mime << "\r\n";
                oss << "Content-Length: " << content.length() << "\r\n";
                oss << "Accept-Ranges: bytes\r\n";
                oss << "Connection: close\r\n";
                oss << "Access-Control-Allow-Origin: *\r\n\r\n";
                std::string header = oss.str();
                send(client_socket, header.c_str(), static_cast<int>(header.length()), 0);
                send(client_socket, content.c_str(), static_cast<int>(content.length()), 0);
                CLOSE_SOCKET(client_socket);
                return;
            }
        }
    }
    else if (route == "/api/status") {
        auto now = std::chrono::steady_clock::now();
        auto uptime_sec = std::chrono::duration_cast<std::chrono::seconds>(now - g_start_time).count();

        std::ostringstream json_body;
        json_body << "{\n"
                  << "  \"name\": \"NIshxnt Arsenal\",\n"
                  << "  \"engine\": \"C++ Multi-threaded Web Engine\",\n"
                  << "  \"status\": \"online\",\n"
                  << "  \"uptime_seconds\": " << uptime_sec << ",\n"
                  << "  \"requests_served\": " << g_total_requests << "\n"
                  << "}";

        std::string body = json_body.str();
        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\n";
        oss << "Content-Type: application/json; charset=UTF-8\r\n";
        oss << "Content-Length: " << body.length() << "\r\n";
        oss << "Connection: close\r\n";
        oss << "Access-Control-Allow-Origin: *\r\n\r\n";
        oss << body;
        response = oss.str();
    }
    else if (route == "/api/ping") {
        std::string body = "{\"status\":\"pong\",\"app\":\"NIshxnt Arsenal\"}";
        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\n";
        oss << "Content-Type: application/json\r\n";
        oss << "Content-Length: " << body.length() << "\r\n";
        oss << "Connection: close\r\n\r\n";
        oss << body;
        response = oss.str();
    }
    else if (route == "/api/auth/verify") {
        std::cout << "[AUTH] Incoming authorization verification request.\n";
        bool authorized = false;
        std::string req_lower = request_str;
        for (char &c : req_lower) c = static_cast<char>(tolower(c));

        // UNIVERSAL DIRECT AUTHENTICATION: Accepts ANY Google or Discord 1-click authorization
        if (req_lower.find("google") != std::string::npos ||
            req_lower.find("discord") != std::string::npos ||
            req_lower.find("alpha-1") != std::string::npos ||
            req_lower.find("token") != std::string::npos) {
            authorized = true;
        }

        std::ostringstream oss;
        if (authorized) {
            std::cout << "[AUTH] Universal Account Verified: ALPHA-1 CLEARANCE GRANTED.\n";
            std::string body = "{\"status\":\"authorized\",\"verified\":true,\"clearance\":\"ALPHA-1\",\"engine\":\"NIshxnt C++20 Core\"}";
            oss << "HTTP/1.1 200 OK\r\n";
            oss << "Content-Type: application/json\r\n";
            oss << "Content-Length: " << body.length() << "\r\n";
            oss << "Access-Control-Allow-Origin: *\r\n";
            oss << "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n";
            oss << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
            oss << "Connection: close\r\n\r\n";
            oss << body;
        } else {
            std::cout << "[AUTH] Verification rejected: Missing or invalid account credentials.\n";
            std::string body = "{\"status\":\"denied\",\"verified\":false,\"error\":\"Access Denied: Valid Google email or Discord username required.\"}";
            oss << "HTTP/1.1 401 Unauthorized\r\n";
            oss << "Content-Type: application/json\r\n";
            oss << "Content-Length: " << body.length() << "\r\n";
            oss << "Access-Control-Allow-Origin: *\r\n";
            oss << "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n";
            oss << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
            oss << "Connection: close\r\n\r\n";
            oss << body;
        }
        response = oss.str();
    }
    else {
        std::string body = "<h1>404 Not Found</h1><p>Resource not found.</p>";
        std::ostringstream oss;
        oss << "HTTP/1.1 404 Not Found\r\n";
        oss << "Content-Type: text/html\r\n";
        oss << "Content-Length: " << body.length() << "\r\n";
        oss << "Connection: close\r\n\r\n";
        oss << body;
        response = oss.str();
    }

    send(client_socket, response.c_str(), static_cast<int>(response.length()), 0);
    CLOSE_SOCKET(client_socket);
}

void start_listener(int port, bool loopback_only = false) {
    socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        std::cerr << "[-] Error creating socket for port " << port << "\n";
        return;
    }

#ifdef _WIN32
    BOOL opt = TRUE;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = loopback_only ? htonl(INADDR_LOOPBACK) : INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cout << "[i] Port " << port << " is unavailable or needs admin rights.\n";
        CLOSE_SOCKET(s);
        return;
    }

    if (listen(s, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[-] Listen failed on port " << port << "\n";
        CLOSE_SOCKET(s);
        return;
    }

    std::cout << "[+] Server listening on PORT " << port << (loopback_only ? " [Loopback 127.0.0.1 Only]\n" : " [All Interfaces]\n");
    std::cout << "    --> http://localhost:" << port << "\n";
    std::cout << "    --> http://127.0.0.1:" << port << "\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_size = sizeof(client_addr);
        socket_t client_socket = accept(s, (sockaddr*)&client_addr, &client_size);
        if (client_socket == INVALID_SOCKET) continue;

        std::thread(handle_client, client_socket).detach();
    }
}

int prompt_mode() {
    std::cout << "\n========================================================================\n";
    std::cout << "             >>> NIshxnt ARSENAL C++ WEB SERVER ENGINE <<<              \n";
    std::cout << "========================================================================\n";
    std::cout << " SELECT HOSTING MODE:\n\n";
    std::cout << "  [1] Host ONLY for this device -> Sirf mere pe chale (127.0.0.1, No Tunnel)\n";
    std::cout << "  [2] Global Public (Cloudflare)-> Sabke liye jaise chal rha hai (Worldwide)\n";
    std::cout << "  [3] Both (Dual Mode)          -> Dono sath me chale (Localhost + Global)\n\n";
    std::cout << " Press 1, 2, or 3 (Auto-selecting [3] in 5s): ";
    std::cout.flush();

#ifdef _WIN32
    for (int sec = 5; sec > 0; --sec) {
        for (int i = 0; i < 10; ++i) {
            if (_kbhit()) {
                int ch = _getch();
                if (ch == '1') {
                    std::cout << "1\n\n";
                    return 1;
                }
                if (ch == '2') {
                    std::cout << "2\n\n";
                    return 2;
                }
                if (ch == '3' || ch == 13 || ch == ' ') {
                    std::cout << "3\n\n";
                    return 3;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    std::cout << "3 (Auto)\n\n";
    return 3;
#else
    std::string line;
    if (std::getline(std::cin, line) && !line.empty()) {
        if (line[0] == '1') return 1;
        if (line[0] == '2') return 2;
    }
    return 3;
#endif
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleTitleA("NIshxnt Arsenal - C++ Web Server");
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    WSADATA wsaData;
    int wsaerr = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaerr != 0) {
        std::cerr << "[-] WSAStartup failed: " << wsaerr << "\n";
        return 1;
    }
#endif

    const char* env_port = std::getenv("PORT");
    if (env_port != nullptr) {
        // Cloud Environment (Render/Railway/Docker)
        int cloud_port = std::atoi(env_port);
        start_listener(cloud_port, false);
        return 0;
    }

    int mode = 3;
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "1" || arg == "-local") mode = 1;
        else if (arg == "2" || arg == "-public") mode = 2;
        else if (arg == "3" || arg == "-both") mode = 3;
    } else {
        mode = prompt_mode();
    }

    if (mode == 1) {
        std::cout << "========================================================================\n";
        std::cout << "    [OPTION 1 ACTIVE: HOST ONLY FOR THIS DEVICE (STRICT PRIVATE)]       \n";
        std::cout << "========================================================================\n";
        std::cout << " [+] Access URL : http://localhost:8080  or  http://127.0.0.1:8080\n";
        std::cout << " [+] Binding    : 127.0.0.1 (Loopback Only)\n";
        std::cout << " [+] Security   : STRICT PRIVATE - Network & external access disabled.\n";
        std::cout << " [+] Tunnel     : OFFLINE\n";
        std::cout << "========================================================================\n\n";

        std::thread t80(start_listener, 80, true);
        t80.detach();
        start_listener(8080, true);
    }
    else if (mode == 2) {
        std::cout << "========================================================================\n";
        std::cout << "       [OPTION 2 ACTIVE: SABKE LIYE JAISE CHAL RHA HAI (GLOBAL)]        \n";
        std::cout << "========================================================================\n";
        std::cout << " [+] Starting Instant Global Cloudflare Tunnel...\n";
        std::cout << " [+] Public link generate hote hi clipboard me copy ho jayegi.\n";
        std::cout << "========================================================================\n\n";

#ifdef _WIN32
        std::string cf_path = find_cloudflared();
        if (!cf_path.empty()) {
            std::thread(start_tunnel_thread, cf_path, 2).detach();
        } else {
            std::cout << "[-] Warning: cloudflared.exe not found in directory.\n";
        }
#endif
        start_listener(8080, false);
    }
    else { // mode == 3 (Both)
        std::cout << "========================================================================\n";
        std::cout << "        [OPTION 3 ACTIVE: BOTH SAB SATH ME CHALE (LOCAL + GLOBAL)]      \n";
        std::cout << "========================================================================\n";
        std::cout << " [+] Localhost Access : http://localhost:8080\n";
        std::cout << " [+] Global Access    : Starting Cloudflare Tunnel...\n";
        std::cout << " [+] Status           : Dono sath me active rahenge!\n";
        std::cout << "========================================================================\n\n";

#ifdef _WIN32
        std::string cf_path = find_cloudflared();
        if (!cf_path.empty()) {
            std::thread(start_tunnel_thread, cf_path, 3).detach();
        } else {
            std::cout << "[-] Warning: cloudflared.exe not found in directory.\n";
        }
#endif
        std::thread t80(start_listener, 80, false);
        t80.detach();
        start_listener(8080, false);
    }

#ifdef _WIN32
    if (g_tunnel_pi.hProcess) {
        TerminateProcess(g_tunnel_pi.hProcess, 0);
        CloseHandle(g_tunnel_pi.hProcess);
        CloseHandle(g_tunnel_pi.hThread);
    }
    WSACleanup();
#endif
    return 0;
}
