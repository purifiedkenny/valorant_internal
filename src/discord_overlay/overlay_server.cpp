#include <include/discord_overlay/overlay_server.hpp>
#include <include/utils/logger.hpp>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

namespace maxi::discord_overlay {

    static std::mutex        g_mtx;
    static std::string       g_json = R"({"w":0,"h":0,"actors":[]})";
    static std::atomic<bool> g_run{ false };
    static SOCKET            g_listen = INVALID_SOCKET;
    static std::thread       g_thr;
    static SetCallback       g_set_cb = nullptr;
    static std::atomic<bool> g_html_active{ false };

    void set_config_callback(SetCallback cb) { g_set_cb = cb; }

    bool overlay_html_active() { return g_html_active.load(); }

    void set_frame_json(std::string json) {
        std::lock_guard<std::mutex> lk(g_mtx);
        g_json = std::move(json);
    }


    static void handle_set(const std::string& req) {
        auto q = req.find('?');
        if (q == std::string::npos) return;
        std::string query = req.substr(q + 1);
        auto sp = query.find(' ');
        if (sp != std::string::npos) query = query.substr(0, sp);
        std::string key, val;
        for (auto& kv : { std::make_pair(std::string("k="), &key), std::make_pair(std::string("v="), &val) }) {
            size_t p = query.find(kv.first);
            if (p == std::string::npos) continue;
            p += kv.first.size();
            size_t e = query.find('&', p);
            *kv.second = query.substr(p, e == std::string::npos ? std::string::npos : e - p);
        }
        if (!key.empty() && g_set_cb) g_set_cb(key, val == "1" || val == "true");
    }

    static void serve_client(SOCKET c) {
        char buf[2048];
        const int n = recv(c, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { closesocket(c); return; }
        buf[n] = '\0';
        const std::string req(buf);

        if (req.rfind("GET /set", 0) == 0) {
            handle_set(req);
            const std::string ok = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: 2\r\nConnection: close\r\n\r\nok";
            send(c, ok.c_str(), (int)ok.size(), 0);
            closesocket(c);
            return;
        }

        std::string body;
        {
            std::lock_guard<std::mutex> lk(g_mtx);
            body = g_json;
        }
        const std::string resp =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Cache-Control: no-store\r\n"
            "Connection: close\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
        send(c, resp.c_str(), (int)resp.size(), 0);
        closesocket(c);
    }

    static void loop() {
        while (g_run.load()) {
            sockaddr_in cli{};
            int cl = sizeof(cli);
            SOCKET c = accept(g_listen, reinterpret_cast<sockaddr*>(&cli), &cl);
            if (c == INVALID_SOCKET) {
                if (!g_run.load()) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            serve_client(c);
        }
    }

    void start_overlay_server(int port) {
        if (g_run.load()) return;
        WSADATA wsa{};
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { LOG_ERROR("overlay http: WSAStartup failed"); return; }

        g_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (g_listen == INVALID_SOCKET) { LOG_ERROR("overlay http: socket failed"); return; }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(port));
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (bind(g_listen, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            LOG_ERROR(std::format("overlay http: bind failed {}", WSAGetLastError()));
            closesocket(g_listen); g_listen = INVALID_SOCKET; return;
        }
        if (listen(g_listen, 4) == SOCKET_ERROR) {
            LOG_ERROR("overlay http: listen failed");
            closesocket(g_listen); g_listen = INVALID_SOCKET; return;
        }

        g_run.store(true);
        g_thr = std::thread(loop);
        LOG_INFO(std::format("Overlay HTTP server listening on 127.0.0.1:{}", port));
    }

    void stop_overlay_server() {
        g_run.store(false);
        if (g_listen != INVALID_SOCKET) { closesocket(g_listen); g_listen = INVALID_SOCKET; }
        if (g_thr.joinable()) g_thr.join();
        WSACleanup();
    }

    bool install_discord_overlay_html() {

        wchar_t exe[MAX_PATH]{};
        if (!GetModuleFileNameW(nullptr, exe, MAX_PATH)) return false;
        std::filesystem::path html = std::filesystem::path(exe).parent_path() / "assets" / "maxi_overlay.html";
        if (!std::filesystem::exists(html)) {
            LOG_ERROR(std::format("overlay: HTML not found: {}", html.string()));
            return false;
        }
        const std::string html_url = "file:///" + html.generic_string();


        wchar_t* local = nullptr; size_t n = 0;
        _wdupenv_s(&local, &n, L"LOCALAPPDATA");
        if (!local) return false;
        std::filesystem::path base = std::filesystem::path(local) / "Discord";
        free(local);

        std::filesystem::path hostjs;
        std::error_code ec;
        for (auto& app : std::filesystem::directory_iterator(base, std::filesystem::directory_options::skip_permission_denied, ec)) {
            if (!app.is_directory()) continue;
            if (app.path().filename().string().rfind("app-", 0) != 0) continue;
            auto moddir = app.path() / "modules";
            if (!std::filesystem::exists(moddir)) continue;
            for (auto& ov : std::filesystem::directory_iterator(moddir, std::filesystem::directory_options::skip_permission_denied, ec)) {
                if (!ov.is_directory()) continue;
                if (ov.path().filename().string().rfind("discord_overlay2", 0) != 0) continue;
                std::filesystem::path cand = ov.path() / "discord_overlay2" / "host.js";
                if (std::filesystem::exists(cand)) { hostjs = cand; break; }
            }
            if (!hostjs.empty()) break;
        }
        if (hostjs.empty()) { LOG_ERROR("overlay: Discord host.js not found"); return false; }


        std::ifstream in(hostjs, std::ios::binary);
        if (!in) { LOG_ERROR("overlay: could not read host.js"); return false; }
        std::string src((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();

        if (src.find("MAXI_OVERLAY_URL") != std::string::npos) {
            LOG_INFO("overlay: host.js already patched");
            g_html_active.store(true);
            return true;
        }

        const std::string anchor = "const renderer = renderers[pid];";
        size_t pos = src.find(anchor);
        if (pos == std::string::npos) { LOG_ERROR("overlay: host.js anchor not found"); return false; }
        pos += anchor.size();
        const std::string inject =
            "\n    /* MAXI_OVERLAY_URL */ renderer.overlayURL = \"" + html_url + "\";";
        src.insert(pos, inject);


        std::filesystem::copy_file(hostjs, hostjs.string() + ".maxibak",
            std::filesystem::copy_options::overwrite_existing, ec);
        std::ofstream out(hostjs, std::ios::binary | std::ios::trunc);
        if (!out) { LOG_ERROR("overlay: could not write host.js"); return false; }
        out.write(src.data(), static_cast<std::streamsize>(src.size()));
        out.close();
        LOG_INFO(std::format("overlay: host.js patched -> {}", html_url));
        g_html_active.store(true);
        return true;
    }

}
