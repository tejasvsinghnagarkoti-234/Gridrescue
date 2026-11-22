
#include <iostream>
#include <sstream>
#include <algorithm>

#include "GridManager.h"
#include "Algorithms.h"

using namespace std;

static string esc(const string &s)
{
    string o;
    for(char c : s)
    {
        if(c == '"' || c == '\\') { o += '\\'; o += c; }
        else if(c == '\n') o += "\\n";
        else if((unsigned char)c < 0x20) o += ' ';
        else o += c;
    }
    return o;
}

static string q(const string &s) { return "\"" + esc(s) + "\""; }

static string intList(const vector<int> &v)
{
    string o = "[";
    for(size_t i = 0; i < v.size(); i++)
    {
        if(i) o += ",";
        o += to_string(v[i]);
    }
    return o + "]";
}

static string trimmed(string s)
{
    while(!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
    return s;
}


static string result(bool ok, const string &msg, const string &extra = "")
{
    return string("{\"ok\":") + (ok ? "true" : "false") + ",\"message\":" + q(msg) +
           (extra.empty() ? "" : "," + extra) + "}";
}
struct CoutCapture
{
    ostringstream ss;
    streambuf *old;
    CoutCapture() { old = cout.rdbuf(ss.rdbuf()); }
    ~CoutCapture() { cout.rdbuf(old); }
    string text() { return ss.str(); }
};

string GridManager::apiState()
{
    vector<bool> usable = usableSubstations();
    ostringstream o;

    o << "{\"map\":{\"w\":" << MAP_W << ",\"h\":" << MAP_H << ",\"maxNodes\":" << MAX_NODES << "},";

    o << "\"stations\":[";
    for(size_t i = 0; i < stations.size(); i++)
    {
        if(i) o << ",";
        o << "{\"id\":" << stations[i].id
          << ",\"name\":" << q(stations[i].name)
          << ",\"capacity\":" << stations[i].capacity
          << ",\"failed\":" << (stations[i].failed ? "true" : "false")
          << ",\"x\":" << stations[i].x << ",\"y\":" << stations[i].y << "}";
    }
    o << "],\"substations\":[";
    for(size_t i = 0; i < substations.size(); i++)
    {
        if(i) o << ",";
        o << "{\"id\":" << substations[i].id
          << ",\"name\":" << q(substations[i].name)
          << ",\"capacity\":" << substations[i].capacity
          << ",\"load\":" << substations[i].currentLoad
          << ",\"failed\":" << (substations[i].failed ? "true" : "false")
          << ",\"connected\":" << (usable[i] ? "true" : "false")
          << ",\"x\":" << substations[i].x << ",\"y\":" << substations[i].y << "}";
    }
    o << "],\"lines\":[";
    for(size_t i = 0; i < lines.size(); i++)
    {
        if(i) o << ",";
        o << "{\"from\":" << lines[i].from << ",\"to\":" << lines[i].to
          << ",\"distance\":" << lines[i].distance
          << ",\"active\":" << (lines[i].active ? "true" : "false") << "}";
    }
    o << "],\"consumers\":[";
    for(size_t i = 0; i < consumers.size(); i++)
    {
        if(i) o << ",";
        o << "{\"id\":" << consumers[i].id
          << ",\"name\":" << q(consumers[i].name)
          << ",\"required\":" << consumers[i].requiredPower
          << ",\"priority\":" << consumers[i].priority << "}";
    }
    o << "],\"history\":" << intList(failureManager.history()) << "}";
    return o.str();
}

string GridManager::apiFail(int id)
{
    bool ok = isNode(id) && !isFailed(id);
    string msg;
    {
        CoutCapture cap;
        createFailure(id);
        msg = cap.text();
    }
    return result(ok, trimmed(msg));
}

string GridManager::apiUndo()
{
    bool ok = !failureManager.empty();
    string msg;
    {
        CoutCapture cap;
        undoFailure();
        msg = cap.text();
    }
    return result(ok, trimmed(msg));
}

string GridManager::apiRestore()
{
    bool ok = !failureManager.empty();
    string msg;
    {
        CoutCapture cap;
        restoreAll();
        msg = cap.text();
    }
    return result(ok, trimmed(msg));
}

string GridManager::apiAffected()
{
    vector<int> sources = sourceIds();
    vector<int> connected = bfsMulti(graph, sources, failedFlags());
    vector<int> affected;
    for(const auto &s : substations)
        if(find(connected.begin(), connected.end(), s.id) == connected.end())
            affected.push_back(s.id);
    return "{\"ok\":true,\"algorithm\":\"BFS\",\"sources\":" + intList(sources) +
           ",\"order\":" + intList(connected) + ",\"affected\":" + intList(affected) + "}";
}

string GridManager::apiConnection(int start, int end)
{
    if(!isNode(start) || !isNode(end))
        return result(false, "Pick two nodes that exist on the map.");

    vector<int> order = dfs(graph, start, failedFlags());
    bool found = find(order.begin(), order.end(), end) != order.end();
    return string("{\"ok\":true,\"algorithm\":\"DFS\",\"connected\":") +
           (found ? "true" : "false") + ",\"order\":" + intList(order) + "}";
}

string GridManager::apiPath(int start, int end)
{
    if(!isNode(start) || !isNode(end))
        return result(false, "Pick two nodes that exist on the map.");

    vector<int> path = dijkstra(graph, start, end, failedFlags());
    int total = 0;
    for(size_t i = 0; i + 1 < path.size(); i++)
    {
        for(auto &e : graph.getNeighbours(path[i]))
            if(e.first == path[i + 1]) { total += e.second; break; }
    }
    return string("{\"ok\":true,\"algorithm\":\"Dijkstra\",\"found\":") +
           (path.empty() ? "false" : "true") + ",\"path\":" + intList(path) +
           ",\"distance\":" + to_string(total) + "}";
}

string GridManager::apiRedistribute()
{
    vector<int> group, budgets;
    vector<vector<int>> members;
    islands(group, budgets, members);

    RedistributionResult r = powerManager.allocate(substations, consumers, group, budgets);

    ostringstream o;
    o << "{\"ok\":true,\"supplied\":" << r.totalSupplied
      << ",\"budget\":" << r.supplyBudget << ",\"allocations\":[";
    for(size_t i = 0; i < r.allocations.size(); i++)
    {
        if(i) o << ",";
        o << "{\"consumer\":" << q(consumers[r.allocations[i].consumerIndex].name)
          << ",\"consumerId\":" << consumers[r.allocations[i].consumerIndex].id
          << ",\"substation\":" << q(substations[r.allocations[i].substationIndex].name)
          << ",\"substationId\":" << substations[r.allocations[i].substationIndex].id
          << ",\"power\":" << r.allocations[i].power << "}";
    }
    o << "],\"consumers\":[";
    for(size_t i = 0; i < consumers.size(); i++)
    {
        if(i) o << ",";
        o << "{\"id\":" << consumers[i].id << ",\"name\":" << q(consumers[i].name)
          << ",\"required\":" << consumers[i].requiredPower
          << ",\"delivered\":" << r.delivered[i] << "}";
    }
    o << "],\"islands\":[";
    for(size_t k = 0; k < budgets.size(); k++)
    {
        if(k) o << ",";
        vector<int> ids;
        for(int si : members[k]) ids.push_back(stations[si].id);
        vector<int> subs;
        for(size_t i = 0; i < substations.size(); i++)
            if(group[i] == (int)k) subs.push_back(substations[i].id);
        o << "{\"stations\":" << intList(ids) << ",\"substations\":" << intList(subs)
          << ",\"budget\":" << r.groupBudget[k] << ",\"supplied\":" << r.groupSupplied[k] << "}";
    }
    o << "],\"idle\":[";
    
    bool first = true;
    for(size_t i = 0; i < stations.size(); i++)
    {
        if(stations[i].failed) continue;
        bool used = false;
        for(const auto &m : members)
            if(find(m.begin(), m.end(), (int)i) != m.end()) used = true;
        if(used) continue;
        if(!first) o << ",";
        o << stations[i].id;
        first = false;
    }
    o << "]}";
    return o.str();
}

string GridManager::apiSearch(const string &name)
{
    auto it = components.find(name);
    if(it == components.end())
    {
        
        string low = name;
        transform(low.begin(), low.end(), low.begin(), ::tolower);
        for(auto &kv : components)
        {
            string k = kv.first;
            transform(k.begin(), k.end(), k.begin(), ::tolower);
            if(k == low) { it = components.find(kv.first); break; }
        }
    }
    if(it == components.end())
        return "{\"ok\":true,\"found\":false}";

    return "{\"ok\":true,\"found\":true,\"name\":" + q(it->first) +
           ",\"id\":" + to_string(it->second) + "}";
}

string GridManager::apiAddLine(int from, int to, int distance)
{
    if(!isNode(from) || !isNode(to))
        return result(false, "Both ends of a line must be nodes on the map.");
    if(from == to)
        return result(false, "A line needs two different nodes.");
    if(distance < 1 || distance > 1000)
        return result(false, "Distance must be between 1 and 1000 km.");
    if(lineIndex(from, to) != -1)
        return result(false, "A line already connects " + nodeName(from) + " and " + nodeName(to) + ".");

    addTransmissionLine(from, to, distance);
    return result(true, "Connected " + nodeName(from) + " \xE2\x86\x94 " + nodeName(to) +
                        " (" + to_string(distance) + " km).");
}

string GridManager::apiAddNode(const string &type, const string &name, int capacity, int x, int y)
{
    if(type != "station" && type != "substation")
        return result(false, "Node type must be 'station' or 'substation'.");

    bool isStation = type == "station";
    int id;
    string err = addNode(isStation, name, capacity, x, y, id);
    if(!err.empty()) return result(false, err);

    return result(true, string("Added ") + (isStation ? "power station " : "substation ") +
                        nodeName(id) + " (node " + to_string(id) + ").",
                  "\"id\":" + to_string(id));
}

string GridManager::apiMoveNode(int id, int x, int y)
{
    string err = moveNode(id, x, y);
    return err.empty() ? result(true, "") : result(false, err);
}

string GridManager::apiUpdateNode(int id, const string &name, int capacity)
{
    string before = nodeName(id);
    string err = updateNode(id, name, capacity);
    if(!err.empty()) return result(false, err);
    return result(true, "Updated " + before + (before != nodeName(id) ? " (now " + nodeName(id) + ")" : "") + ".");
}

string GridManager::apiRemoveNode(int id)
{
    string n = nodeName(id);
    string err = removeNode(id);
    if(!err.empty()) return result(false, err);
    return result(true, "Deleted " + n + " and its lines.");
}

string GridManager::apiUpdateLine(int from, int to, int distance)
{
    string err = updateLine(from, to, distance);
    if(!err.empty()) return result(false, err);
    return result(true, "Line " + nodeName(from) + " \xE2\x86\x94 " + nodeName(to) +
                        " is now " + to_string(distance) + " km.");
}

string GridManager::apiRemoveLine(int from, int to)
{
    string a = nodeName(from), b = nodeName(to);
    string err = removeLine(from, to);
    if(!err.empty()) return result(false, err);
    return result(true, "Deleted line " + a + " \xE2\x86\x94 " + b + ".");
}

string GridManager::apiClear()
{
    clearGrid();
    return result(true, "Map cleared. Consumers were kept.");
}

string GridManager::apiAddConsumer(const string &name, int required, int priority)
{
    int id;
    string err = addConsumer(name, required, priority, id);
    if(!err.empty()) return result(false, err);
    return result(true, "Added consumer " + name + " (" + to_string(required) +
                        " kW, priority " + to_string(priority) + ").", "\"id\":" + to_string(id));
}

string GridManager::apiRemoveConsumer(int id)
{
    string err = removeConsumer(id);
    if(!err.empty()) return result(false, err);
    return result(true, "Removed consumer.");
}
