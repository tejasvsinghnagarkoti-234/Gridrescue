// GridRescue web server: serves the ./web frontend and a small JSON API
// backed by the real C++ GridManager. Listens on 127.0.0.1 only.
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <map>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET sock_t;
  #define CLOSESOCK closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  typedef int sock_t;
  #define CLOSESOCK close
  #define INVALID_SOCKET (-1)
#endif

#include "GridManager.h"

using namespace std;

static unique_ptr<GridManager> grid;

static void resetGrid(int numSubstations = 4)
{
    grid.reset(new GridManager());
    grid->createGrid(numSubstations);
}

static string urlDecode(const string &s)
{
    string o;
    for(size_t i = 0; i < s.size(); i++)
    {
        if(s[i] == '%' && i + 2 < s.size())
        {
            o += (char)strtol(s.substr(i + 1, 2).c_str(), nullptr, 16);
            i += 2;
        }
        else if(s[i] == '+') o += ' ';
        else o += s[i];
    }
    return o;
}

static map<string, string> parseQuery(const string &qs)
{
    map<string, string> m;
    size_t pos = 0;
    while(pos < qs.size())
    {
        size_t amp = qs.find('&', pos);
        if(amp == string::npos) amp = qs.size();
        string kv = qs.substr(pos, amp - pos);
        size_t eq = kv.find('=');
        if(eq != string::npos)
            m[urlDecode(kv.substr(0, eq))] = urlDecode(kv.substr(eq + 1));
        pos = amp + 1;
    }
    return m;
}

static int toInt(const map<string, string> &m, const string &k, int def = -1)
{
    auto it = m.find(k);
    if(it == m.end() || it->second.empty()) return def;
    char *end;
    long v = strtol(it->second.c_str(), &end, 10);
    return *end ? def : (int)v;
}

static string mimeFor(const string &p)
{
    auto ends = [&](const char *e) {
        size_t n = strlen(e);
        return p.size() >= n && p.compare(p.size() - n, n, e) == 0;
    };
    if(ends(".html")) return "text/html; charset=utf-8";
    if(ends(".css"))  return "text/css; charset=utf-8";
    if(ends(".js"))   return "application/javascript; charset=utf-8";
    if(ends(".svg"))  return "image/svg+xml";
    if(ends(".json")) return "application/json";
    return "application/octet-stream";
}

static string webRoot = "web";

static string response(int code, const string &type, const string &body)
{
    const char *text = code == 200 ? "OK" : code == 404 ? "Not Found" :
                       code == 405 ? "Method Not Allowed" : "Bad Request";
    ostringstream o;
    o << "HTTP/1.1 " << code << " " << text << "\r\n"
      << "Content-Type: " << type << "\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Cache-Control: no-store\r\n"
      << "Connection: close\r\n\r\n" << body;
    return o.str();
}

static string json(const string &result)
{
    return "{\"result\":" + result + ",\"state\":" + grid->apiState() + "}";
}

static string handle(const string &method, const string &target)
{
    string path = target, qs;
    size_t qm = target.find('?');
    if(qm != string::npos) { path = target.substr(0, qm); qs = target.substr(qm + 1); }
    auto p = parseQuery(qs);

    if(path.rfind("/api/", 0) == 0)
    {
        const string J = "application/json";
        bool post = method == "POST";
        string r;
        auto str = [&](const char *k) { return p.count(k) ? p[k] : string(); };

        // ---- read-only queries
        if(path == "/api/state")            r = "{\"ok\":true}";
        else if(path == "/api/affected")    r = grid->apiAffected();
        else if(path == "/api/connection")  r = grid->apiConnection(toInt(p, "from"), toInt(p, "to"));
        else if(path == "/api/path")        r = grid->apiPath(toInt(p, "from"), toInt(p, "to"));
        else if(path == "/api/search")      r = grid->apiSearch(str("name"));
        else
        {
            // ---- everything else changes the grid and must be a POST
            static const char *mutating[] = {
                "/api/fail", "/api/undo", "/api/restore", "/api/reset", "/api/clear",
                "/api/redistribute", "/api/addline", "/api/updateline", "/api/removeline",
                "/api/addnode", "/api/move", "/api/updatenode", "/api/removenode",
                "/api/addconsumer", "/api/removeconsumer"
            };
            bool known = false;
            for(const char *m : mutating) if(path == m) known = true;
            if(!known) return response(404, J, "{\"error\":\"unknown endpoint\"}");
            if(!post)  return response(405, J, "{\"error\":\"use POST\"}");

            if(path == "/api/fail")                r = grid->apiFail(toInt(p, "id"));
            else if(path == "/api/undo")           r = grid->apiUndo();
            else if(path == "/api/restore")        r = grid->apiRestore();
            else if(path == "/api/clear")          r = grid->apiClear();
            else if(path == "/api/redistribute")   r = grid->apiRedistribute();
            else if(path == "/api/addline")        r = grid->apiAddLine(toInt(p, "from"), toInt(p, "to"), toInt(p, "dist", 15));
            else if(path == "/api/updateline")     r = grid->apiUpdateLine(toInt(p, "from"), toInt(p, "to"), toInt(p, "dist"));
            else if(path == "/api/removeline")     r = grid->apiRemoveLine(toInt(p, "from"), toInt(p, "to"));
            else if(path == "/api/addnode")        r = grid->apiAddNode(str("type"), str("name"), toInt(p, "cap"), toInt(p, "x"), toInt(p, "y"));
            else if(path == "/api/move")           r = grid->apiMoveNode(toInt(p, "id"), toInt(p, "x"), toInt(p, "y"));
            else if(path == "/api/updatenode")     r = grid->apiUpdateNode(toInt(p, "id"), str("name"), toInt(p, "cap"));
            else if(path == "/api/removenode")     r = grid->apiRemoveNode(toInt(p, "id"));
            else if(path == "/api/addconsumer")    r = grid->apiAddConsumer(str("name"), toInt(p, "req"), toInt(p, "pri"));
            else if(path == "/api/removeconsumer") r = grid->apiRemoveConsumer(toInt(p, "id"));
            else // /api/reset : load a preset grid
            {
                int n = toInt(p, "n", -1);
                if(n == -1) n = 4;
                if(n < 2 || n > 20)
                {
                    return response(200, J, json("{\"ok\":false,\"message\":\"Number of substations must be between 2 and 20.\"}"));
                }
                resetGrid(n);
                r = "{\"ok\":true,\"message\":\"Loaded the preset grid with " + to_string(n) + " substations.\"}";
            }
        }

        return response(200, J, json(r));
    }

    // Static files
    if(method != "GET") return response(405, "text/plain", "Method not allowed");
    if(path == "/") path = "/index.html";
    if(path.find("..") != string::npos || path.find('\\') != string::npos)
        return response(400, "text/plain", "Bad path");

    ifstream f(webRoot + path, ios::binary);
    if(!f) return response(404, "text/plain", "Not found");
    stringstream ss;
    ss << f.rdbuf();
    return response(200, mimeFor(path), ss.str());
}

static bool sendAll(sock_t s, const string &d)
{
    size_t sent = 0;
    while(sent < d.size())
    {
        int n = (int)send(s, d.data() + sent, (int)(d.size() - sent), 0);
        if(n <= 0) return false;
        sent += n;
    }
    return true;
}

int main(int argc, char **argv)
{
    int port = argc > 1 ? atoi(argv[1]) : 8080;

    // Find the web folder whether run from the project folder or a subfolder.
    for(const char *c : {"web", "../web", "GridRescue/web"})
    {
        ifstream t(string(c) + "/index.html");
        if(t) { webRoot = c; break; }
    }

#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    resetGrid();

    sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((unsigned short)port);

    if(bind(srv, (sockaddr *)&addr, sizeof(addr)) < 0 || listen(srv, 16) < 0)
    {
        cerr << "Could not listen on port " << port
             << " (is it in use? try: GridRescueWeb <port>)\n";
        return 1;
    }

    cout << "GridRescue web UI running at http://localhost:" << port << "\n"
         << "Serving frontend from: " << webRoot << "\n"
         << "Press Ctrl+C to stop.\n" << flush;

    while(true)
    {
        sock_t c = accept(srv, nullptr, nullptr);
        if(c == INVALID_SOCKET) continue;

        string req;
        char buf[4096];
        size_t headerEnd = string::npos;
        size_t need = 0;

        while(true)
        {
            int n = (int)recv(c, buf, sizeof(buf), 0);
            if(n <= 0) break;
            req.append(buf, n);
            if(headerEnd == string::npos)
            {
                headerEnd = req.find("\r\n\r\n");
                if(headerEnd != string::npos)
                {
                    size_t cl = req.find("Content-Length:");
                    if(cl == string::npos) cl = req.find("content-length:");
                    if(cl != string::npos && cl < headerEnd)
                        need = (size_t)atol(req.c_str() + cl + 15);
                    need += headerEnd + 4;
                }
            }
            if(headerEnd != string::npos && req.size() >= need) break;
            if(req.size() > (1 << 20)) break;
        }

        istringstream line(req.substr(0, req.find("\r\n")));
        string method, target;
        line >> method >> target;

        string out = target.empty() ? response(400, "text/plain", "Bad request")
                                    : handle(method, target);
        sendAll(c, out);
        CLOSESOCK(c);
    }
}
