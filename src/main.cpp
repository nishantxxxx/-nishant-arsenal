#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #define _CRT_SECURE_NO_WARNINGS
  #include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <conio.h>
  #pragma comment(lib, "Ws2_32.lib")
  using socket_t = SOCKET;
  inline void close_socket_graceful(socket_t s) {
      if (s != INVALID_SOCKET) {
          shutdown(s, SD_SEND);
          closesocket(s);
      }
  }
  #define CLOSE_SOCKET(s) close_socket_graceful(s)
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
  inline void close_socket_graceful(socket_t s) {
      if (s != INVALID_SOCKET) {
          shutdown(s, SHUT_WR);
          close(s);
      }
  }
  #define CLOSE_SOCKET(s) close_socket_graceful(s)
#endif

inline bool send_all(socket_t sock, const char* data, size_t total) {
    size_t sent = 0;
    while (sent < total) {
        int to_send = static_cast<int>((total - sent) > 65536 ? 65536 : (total - sent));
        int n = send(sock, data + sent, to_send, 0);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

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
#include <set>
#include <mutex>
#include <ctime>

// Global metrics & Telemetry
std::atomic<uint64_t> g_total_requests(0);
std::atomic<uint64_t> g_total_page_views(0);
std::set<std::string> g_unique_ips;
std::mutex g_telemetry_mutex;
auto g_start_time = std::chrono::steady_clock::now();

std::string get_host_time() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &now_c);
#else
    localtime_r(&now_c, &tm_buf);
#endif
    char time_str[32];
    std::strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm_buf);
    return std::string(time_str);
}

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

std::string get_blocked_content() {
    const char* paths[] = {
        "src/website/blocked.html",
        "src\\website\\blocked.html",
        "website/blocked.html",
        "website\\blocked.html",
        "blocked.html",
        "../src/website/blocked.html",
        "..\\src\\website\\blocked.html",
        "../../src/website/blocked.html",
        "..\\..\\src\\website\\blocked.html",
        "E:/Cpp/hehe/src/website/blocked.html",
        "E:\\Cpp\\hehe\\src\\website\\blocked.html"
    };

    for (const char* p : paths) {
        std::ifstream file(p, std::ios::binary);
        if (file.is_open()) {
            std::ostringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }
    }

    return "<!DOCTYPE html><html><head><title>ARXSHNAL ARCADE DEBUG DETECTED</title></head>"
           "<body style='background:#030408;color:#ff1e42;font-family:sans-serif;text-align:center;padding:50px;'>"
           "<h1>ARXSHNAL ARCADE DEBUG DETECTED</h1>"
           "<p>CLOSE DEVTOOLS AND DEBUGGER</p>"
           "</body></html>";
}

// Embedded Favicon Data
static const unsigned char G_FAVICON_DATA[1150] = {
    0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x10, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x68, 0x04, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0xee, 0xbb, 0x50, 0x10, 0xee, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0xee, 0xbb, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x50, 0x10, 0xee, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0xee, 0xbb, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x50, 0x10, 0xee, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0xee, 0xbb, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x50, 0x10, 0xee, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0xee, 0xbb, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x50, 0x10, 0xee, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0xee, 0xbb, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x50, 0x10, 0xee, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0xee, 0xbb, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x50, 0x10, 0xee, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0xee, 0xbb, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x50, 0x10, 0xee, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0xee, 0xbb, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x50, 0x10, 0xee, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0xee, 0xbb, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x50, 0x10, 0xee, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0xee, 0xbb, 0x30, 0x10, 0xff, 0xff, 0x30, 0x10, 0xff, 0xff, 0x50, 0x10, 0xee, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0xee, 0xbb, 0x50, 0x10, 0xee, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

std::string get_favicon_content() {
    const char* paths[] = {
        "src/website/favicon.ico",
        "src\\website\\favicon.ico",
        "website/favicon.ico",
        "website\\favicon.ico",
        "favicon.ico",
        "../src/website/favicon.ico",
        "..\\src\\website\\favicon.ico",
        "../../src/website/favicon.ico",
        "..\\..\\src\\website\\favicon.ico",
        "E:/Cpp/hehe/src/website/favicon.ico",
        "E:\\Cpp\\hehe\\src\\website\\favicon.ico"
    };
    for (const char* p : paths) {
        std::ifstream file(p, std::ios::binary);
        if (file.is_open()) {
            std::ostringstream ss;
            ss << file.rdbuf();
            std::string content = ss.str();
            if (!content.empty()) return content;
        }
    }
    return std::string(reinterpret_cast<const char*>(G_FAVICON_DATA), sizeof(G_FAVICON_DATA));
}

// Brutal Security & Defense Matrix Headers
const char* G_SECURITY_HEADERS = 
    "X-Content-Type-Options: nosniff\r\n"
    "X-Frame-Options: DENY\r\n"
    "Referrer-Policy: strict-origin-when-cross-origin\r\n"
    "X-XSS-Protection: 1; mode=block\r\n"
    "Permissions-Policy: camera=(), microphone=(), geolocation=(), payment=(), usb=()\r\n"
    "Content-Security-Policy: default-src 'self' blob:; "
    "script-src 'self' 'unsafe-inline' 'unsafe-eval' https://accounts.google.com https://apis.google.com; "
    "frame-src https://accounts.google.com; "
    "style-src 'self' 'unsafe-inline' https://fonts.googleapis.com; "
    "font-src 'self' https://fonts.gstatic.com; "
    "img-src 'self' data: https: blob:; "
    "media-src 'self' blob: data:; "
    "connect-src 'self' https://accounts.google.com https://www.googleapis.com https://oauth2.googleapis.com https://discord.com; "
    "frame-ancestors 'none'; object-src 'none'; base-uri 'self'; form-action 'self';\r\n"
    "X-Arsenal-Shield: ACTIVE-CLASSIFIED\r\n";

void handle_client(socket_t client_socket, sockaddr_in client_addr) {
    char ip_buf[INET_ADDRSTRLEN] = "127.0.0.1";
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip_buf, INET_ADDRSTRLEN);
    std::string client_ip = ip_buf;
    int client_port = ntohs(client_addr.sin_port);

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
    std::string method, raw_route, http_version;
    std::istringstream stream(request_str);
    stream >> method >> raw_route >> http_version;

    // Strip query string from route so routes like "/?auth_required=true" match "/"
    std::string route = raw_route;
    auto query_pos = route.find('?');
    if (query_pos != std::string::npos) {
        route = route.substr(0, query_pos);
    }

    // Brutal Anti-Scanner & Bot Defense
    std::string req_lower = request_str;
    for (char &c : req_lower) c = static_cast<char>(tolower(c));
    const char* malicious_scanners[] = {"sqlmap", "nikto", "masscan", "dirbuster", "gobuster", "acunetix", "wappalyzer", "havij", "nuclei", "zgrab"};
    for (const char* sc : malicious_scanners) {
        if (req_lower.find(sc) != std::string::npos) {
            std::cout << "[SECURITY BLOCKED] Intercepted malicious scanner tool: " << sc << "\n";
            std::string body = "{\"status\":\"denied\",\"violation\":\"MALICIOUS_SCANNER_DETECTED\",\"defense\":\"NIshxnt C++20 Core\"}";
            std::ostringstream oss;
            oss << "HTTP/1.1 403 Forbidden\r\n"
                << "Content-Type: application/json\r\n"
                << "Content-Length: " << body.length() << "\r\n"
                << "X-Arsenal-Shield: BLOCKED\r\n"
                << "Connection: close\r\n\r\n"
                << body;
            std::string res = oss.str();
            send(client_socket, res.c_str(), static_cast<int>(res.length()), 0);
            CLOSE_SOCKET(client_socket);
            return;
        }
    }


    std::cout << "[HTTP] " << method << " " << route << " -> Total Requests: " << g_total_requests << "\n";

    if (route == "/" || route == "/index.html" || route == "/catalog" || route == "/catalog.html" || route == "/about" || route == "/about.html") {
        g_total_page_views++;
        size_t unique_count = 0;
        {
            std::lock_guard<std::mutex> lock(g_telemetry_mutex);
            g_unique_ips.insert(client_ip);
            unique_count = g_unique_ips.size();
        }
        std::cout << "\033[90m[" << get_host_time() << "]\033[0m \033[96m[🚀 SITE OPEN]\033[0m "
                  << "IP: \033[97m" << client_ip << ":" << client_port << "\033[0m | "
                  << "Route: \033[93m" << route << "\033[0m | "
                  << "Total Page Views: \033[92m" << g_total_page_views << "\033[0m | "
                  << "Unique Visitors: \033[95m" << unique_count << "\033[0m\n";
    }

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

    if (route == "/favicon.ico") {
        std::string ico = get_favicon_content();
        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: image/x-icon\r\n"
            << "Content-Length: " << ico.size() << "\r\n"
            << "Cache-Control: public, max-age=604800, immutable\r\n"
            << "Connection: close\r\n"
            << "Access-Control-Allow-Origin: *\r\n\r\n";
        std::string header = oss.str();
        send(client_socket, header.c_str(), static_cast<int>(header.length()), 0);
        send(client_socket, ico.data(), static_cast<int>(ico.size()), 0);
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
        oss << "Access-Control-Allow-Origin: *\r\n" << G_SECURITY_HEADERS << "\r\n";
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
        oss << "Access-Control-Allow-Origin: *\r\n" << G_SECURITY_HEADERS << "\r\n";
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
        oss << "Access-Control-Allow-Origin: *\r\n" << G_SECURITY_HEADERS << "\r\n";
        oss << html;
        response = oss.str();
    }
    else if (route == "/blocked" || route == "/blocked.html") {
        std::string html = get_blocked_content();
        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\n";
        oss << "Content-Type: text/html; charset=UTF-8\r\n";
        oss << "Content-Length: " << html.length() << "\r\n";
        oss << "Connection: close\r\n";
        oss << "Access-Control-Allow-Origin: *\r\n" << G_SECURITY_HEADERS << "\r\n";
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
        else if (ext == ".pack") mime = "text/plain";
        else if (ext == ".dat") mime = "text/plain";
        else if (ext == ".html") mime = "text/html; charset=UTF-8";
        else if (ext == ".webm") mime = "audio/webm";
        else if (ext == ".m4a") mime = "audio/mp4";
        else if (ext == ".mp3") mime = "audio/mpeg";

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
            std::ifstream vf(vp, std::ios::binary | std::ios::ate);
            if (vf.is_open()) {
                std::streamsize file_size = vf.tellg();
                vf.seekg(0, std::ios::beg);

                size_t start_byte = 0;
                size_t end_byte = static_cast<size_t>(file_size > 0 ? (file_size - 1) : 0);
                bool is_range = false;

                size_t range_pos = request_str.find("Range: bytes=");
                if (range_pos != std::string::npos) {
                    size_t eq_pos = range_pos + 13;
                    size_t dash_pos = request_str.find('-', eq_pos);
                    if (dash_pos != std::string::npos) {
                        is_range = true;
                        std::string s_start = request_str.substr(eq_pos, dash_pos - eq_pos);
                        if (!s_start.empty()) {
                            try { start_byte = static_cast<size_t>(std::stoull(s_start)); } catch (...) {}
                        }
                        size_t rn_pos = request_str.find("\r\n", dash_pos);
                        if (rn_pos != std::string::npos && rn_pos > dash_pos + 1) {
                            std::string s_end = request_str.substr(dash_pos + 1, rn_pos - dash_pos - 1);
                            if (!s_end.empty()) {
                                try { end_byte = static_cast<size_t>(std::stoull(s_end)); } catch (...) {}
                            }
                        }
                        if (end_byte >= static_cast<size_t>(file_size)) {
                            end_byte = static_cast<size_t>(file_size > 0 ? (file_size - 1) : 0);
                        }
                    }
                }

                size_t content_len = (end_byte >= start_byte && file_size > 0) ? (end_byte - start_byte + 1) : 0;

                std::ostringstream oss;
                if (is_range) {
                    oss << "HTTP/1.1 206 Partial Content\r\n";
                    oss << "Content-Range: bytes " << start_byte << "-" << end_byte << "/" << file_size << "\r\n";
                } else {
                    oss << "HTTP/1.1 200 OK\r\n";
                }
                oss << "Content-Type: " << mime << "\r\n";
                oss << "Content-Length: " << content_len << "\r\n";
                oss << "Accept-Ranges: bytes\r\n";
                oss << "Connection: close\r\n";
                oss << "Access-Control-Allow-Origin: *\r\n" << G_SECURITY_HEADERS << "\r\n";

                std::string header = oss.str();
                send_all(client_socket, header.c_str(), header.length());

                vf.seekg(start_byte, std::ios::beg);
                char chunk[65536];
                size_t remaining = content_len;
                while (remaining > 0 && vf.good()) {
                    size_t to_read = (remaining < sizeof(chunk)) ? remaining : sizeof(chunk);
                    vf.read(chunk, to_read);
                    std::streamsize bytes_read = vf.gcount();
                    if (bytes_read <= 0) break;
                    if (!send_all(client_socket, chunk, static_cast<size_t>(bytes_read))) {
                        break;
                    }
                    remaining -= static_cast<size_t>(bytes_read);
                }

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
        oss << "Access-Control-Allow-Origin: *\r\n" << G_SECURITY_HEADERS << "\r\n";
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
    else if (route == "/api/telemetry") {
        std::string payload = "";
        auto body_pos = request_str.find("\r\n\r\n");
        if (body_pos != std::string::npos) {
            payload = request_str.substr(body_pos + 4);
        }

        auto get_field = [&](const std::string& key) -> std::string {
            std::string search = "\"" + key + "\":\"";
            auto p1 = payload.find(search);
            if (p1 != std::string::npos) {
                p1 += search.length();
                auto p2 = payload.find("\"", p1);
                if (p2 != std::string::npos) {
                    return payload.substr(p1, p2 - p1);
                }
            }
            std::string search_num = "\"" + key + "\":";
            auto np1 = payload.find(search_num);
            if (np1 != std::string::npos) {
                np1 += search_num.length();
                while (np1 < payload.length() && (payload[np1] == ' ' || payload[np1] == '\t')) np1++;
                auto np2 = payload.find_first_of(",}\r\n", np1);
                if (np2 != std::string::npos) {
                    return payload.substr(np1, np2 - np1);
                }
            }
            return "";
        };

        std::string ev_type = get_field("type");
        std::string vid = get_field("vid");
        std::string page = get_field("page");

        std::lock_guard<std::mutex> lock(g_telemetry_mutex);
        if (ev_type == "TAP") {
            std::string target = get_field("target");
            std::string x = get_field("x");
            std::string y = get_field("y");
            std::cout << "\033[90m[" << get_host_time() << "]\033[0m \033[93m[🎯 USER TAP]\033[0m "
                      << "IP: \033[97m" << client_ip << "\033[0m | "
                      << "ID: \033[96m" << (vid.empty() ? "GUEST" : vid) << "\033[0m | "
                      << "Target: \033[92m" << target << "\033[0m "
                      << "at (X:" << x << ", Y:" << y << ") | "
                      << "Page: " << page << "\n";
        }
        else if (ev_type == "BGM") {
            std::string state = get_field("state");
            std::string vol = get_field("vol");
            std::cout << "\033[90m[" << get_host_time() << "]\033[0m \033[95m[🎵 BGM SOUND]\033[0m "
                      << "IP: \033[97m" << client_ip << "\033[0m | "
                      << "State: \033[92m" << state << "\033[0m | "
                      << "Vol: \033[93m" << vol << "%\033[0m\n";
        }
        else if (ev_type == "PAGE_OPEN") {
            std::string ref = get_field("referrer");
            std::cout << "\033[90m[" << get_host_time() << "]\033[0m \033[96m[🌐 CLIENT SESSION]\033[0m "
                      << "IP: \033[97m" << client_ip << "\033[0m | "
                      << "ID: \033[96m" << vid << "\033[0m | "
                      << "Referrer: " << ref << " | Page: " << page << "\n";
        }
        else if (ev_type == "AUTH") {
            std::string provider = get_field("provider");
            std::string name = get_field("name");
            std::cout << "\033[90m[" << get_host_time() << "]\033[0m \033[91m[🛡️ AUTH CLEARANCE]\033[0m "
                      << "IP: \033[97m" << client_ip << "\033[0m | "
                      << "Provider: \033[93m" << provider << "\033[0m | "
                      << "User: \033[97m" << name << "\033[0m | "
                      << "Clearance: \033[92mALPHA-1\033[0m\n";
        }

        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: application/json\r\n"
            << "Content-Length: 15\r\n"
            << "Access-Control-Allow-Origin: *\r\n"
            << "Connection: close\r\n\r\n"
            << "{\"status\":\"ok\"}";
        std::string res = oss.str();
        send_all(client_socket, res.c_str(), res.length());
        CLOSE_SOCKET(client_socket);
        return;
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

        std::thread(handle_client, client_socket, client_addr).detach();
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
    SetConsoleTitleA("NIshxnt Arsenal - Live Telemetry & Server Engine");
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

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
