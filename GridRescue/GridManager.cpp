#include <iostream>
#include <algorithm>
#include <cmath>
#include <cctype>

#include "GridManager.h"
#include "Algorithms.h"

using namespace std;

static string lowerCase(string s)
{
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)tolower(c); });
    return s;
}

// Names are used as search keys and console input, so keep them to one word.
static bool validName(const string &n)
{
    if(n.empty() || n.size() > 20) return false;
    for(char c : n)
        if(!(isalnum((unsigned char)c) || c == '-' || c == '_')) return false;
    return true;
}

static const char *NAME_RULE = "Names must be 1-20 characters: letters, digits, '-' or '_' (no spaces).";

GridManager::GridManager()
    : graph(0)
{
}

// =====================================================================
// Helpers
// =====================================================================

int GridManager::stationIndex(int id) const
{
    for(size_t i = 0; i < stations.size(); i++)
        if(stations[i].id == id) return (int)i;
    return -1;
}

int GridManager::substationIndex(int id) const
{
    for(size_t i = 0; i < substations.size(); i++)
        if(substations[i].id == id) return (int)i;
    return -1;
}

int GridManager::lineIndex(int a, int b) const
{
    for(size_t i = 0; i < lines.size(); i++)
        if((lines[i].from == a && lines[i].to == b) || (lines[i].from == b && lines[i].to == a))
            return (int)i;
    return -1;
}

bool GridManager::isNode(int id) const
{
    return stationIndex(id) != -1 || substationIndex(id) != -1;
}

bool GridManager::isFailed(int id) const
{
    int s = stationIndex(id);
    if(s != -1) return stations[s].failed;
    int b = substationIndex(id);
    if(b != -1) return substations[b].failed;
    return true; // unknown IDs behave like failed nodes
}

void GridManager::setFailed(int id, bool value)
{
    int s = stationIndex(id);
    if(s != -1) stations[s].failed = value;
    int b = substationIndex(id);
    if(b != -1)
    {
        substations[b].failed = value;
        if(value) substations[b].currentLoad = 0;
    }
}

string GridManager::nodeName(int id) const
{
    int s = stationIndex(id);
    if(s != -1) return stations[s].name;
    int b = substationIndex(id);
    if(b != -1) return substations[b].name;
    return "Node " + to_string(id);
}

int GridManager::graphSize() const
{
    int m = -1;
    for(const auto &s : stations) m = max(m, s.id);
    for(const auto &s : substations) m = max(m, s.id);
    return m + 1;
}

int GridManager::freeId() const
{
    int id = 0;
    while(isNode(id)) id++;
    return id;
}

bool GridManager::nameTaken(const string &name, int exceptId) const
{
    string l = lowerCase(name);
    for(const auto &s : stations)
        if(s.id != exceptId && lowerCase(s.name) == l) return true;
    for(const auto &s : substations)
        if(s.id != exceptId && lowerCase(s.name) == l) return true;
    return false;
}

string GridManager::autoName(const string &prefix) const
{
    for(int k = 1;; k++)
    {
        string n = prefix + to_string(k);
        if(!nameTaken(n, -1)) return n;
    }
}

void GridManager::rebuild(bool resetLoads)
{
    graph = Graph(graphSize());

    for(const auto &s : stations) graph.addNode(s.id, s.name);
    for(const auto &s : substations) graph.addNode(s.id, s.name);

    // A line carries power only if neither end has failed.
    for(auto &l : lines)
    {
        l.active = !isFailed(l.from) && !isFailed(l.to);
        if(l.active) graph.addEdge(l.from, l.to, l.distance);
    }

    components.clear();
    for(const auto &s : stations) components[s.name] = s.id;
    for(const auto &s : substations) components[s.name] = s.id;

    // Any change to the topology makes the previous allocation stale.
    if(resetLoads)
        for(auto &s : substations) s.currentLoad = 0;
}

vector<bool> GridManager::failedFlags()
{
    vector<bool> failed(graph.size(), true);
    for(const auto &s : stations) failed[s.id] = s.failed;
    for(const auto &s : substations) failed[s.id] = s.failed;
    return failed;
}

vector<int> GridManager::sourceIds()
{
    vector<int> src;
    for(const auto &s : stations)
        if(!s.failed) src.push_back(s.id);
    return src;
}

vector<int> GridManager::reachable()
{
    return bfsMulti(graph, sourceIds(), failedFlags());
}

// Substation vector index -> can it currently receive power from any station?
vector<bool> GridManager::usableSubstations()
{
    vector<bool> usable(substations.size(), false);
    for(int node : reachable())
    {
        int b = substationIndex(node);
        if(b != -1) usable[b] = true;
    }
    return usable;
}

void GridManager::islands(vector<int> &group, vector<int> &budgets, vector<vector<int>> &islandStations)
{
    vector<bool> failed = failedFlags();
    vector<int> label(graph.size(), -1);

    budgets.clear();
    islandStations.clear();

    for(const auto &st : stations)
    {
        if(st.failed || label[st.id] != -1) continue;

        int k = (int)budgets.size();
        for(int node : bfs(graph, st.id, failed)) label[node] = k;

        int budget = 0;
        vector<int> members;
        for(size_t i = 0; i < stations.size(); i++)
        {
            if(!stations[i].failed && label[stations[i].id] == k)
            {
                budget += stations[i].capacity;
                members.push_back((int)i);
            }
        }
        budgets.push_back(budget);
        islandStations.push_back(members);
    }

    // An island with no substations cannot deliver anything, so drop it
    // (its stations are reported as idle by the API instead).
    vector<int> subCount(budgets.size(), 0);
    for(const auto &ss : substations)
        if(label[ss.id] >= 0) subCount[label[ss.id]]++;

    vector<int> remap(budgets.size(), -1);
    vector<int> keptBudgets;
    vector<vector<int>> keptStations;
    for(size_t k = 0; k < budgets.size(); k++)
    {
        if(subCount[k] == 0) continue;
        remap[k] = (int)keptBudgets.size();
        keptBudgets.push_back(budgets[k]);
        keptStations.push_back(islandStations[k]);
    }
    budgets = keptBudgets;
    islandStations = keptStations;

    group.assign(substations.size(), -1);
    for(size_t i = 0; i < substations.size(); i++)
    {
        int l = label[substations[i].id];
        group[i] = l >= 0 ? remap[l] : -1;
    }
}

// =====================================================================
// Preset grids
// =====================================================================

static void presetPosition(int n, int i, int &x, int &y)
{
    if(n == 4)
    {
        static const int P[4][2] = {{330, 280}, {580, 140}, {830, 280}, {580, 420}};
        x = P[i - 1][0]; y = P[i - 1][1];
        return;
    }
    if(n == 2)
    {
        static const int P[2][2] = {{420, 280}, {760, 280}};
        x = P[i - 1][0]; y = P[i - 1][1];
        return;
    }
    if(n == 3)
    {
        static const int P[3][2] = {{400, 280}, {700, 150}, {700, 410}};
        x = P[i - 1][0]; y = P[i - 1][1];
        return;
    }

    // Ellipse layout for any N, starting next to PS1 on the left.
    const double PI = 3.14159265358979323846;
    double cx = 580, cy = 280;
    double rx = n > 10 ? 340 : 300;
    double ry = n > 10 ? 220 : 190;
    double theta = PI - (2 * PI * (i - 1)) / n;
    x = (int)lround(cx + rx * cos(theta));
    y = (int)lround(cy + ry * sin(theta));
}

void GridManager::createGrid(int numSubstations)
{
    if (numSubstations < 2) numSubstations = 2;
    if (numSubstations > 50) numSubstations = 50;

    stations.clear();
    substations.clear();
    consumers.clear();
    lines.clear();
    components.clear();
    failureManager = FailureManager();

    // Scale PS1 capacity so it can comfortably supply larger grids
    int psCapacity = max(1000, numSubstations * 250);
    stations.push_back(PowerStation(0, "PS1", psCapacity, 90, 280));

    // Dynamic substations (SS1 to SSN, 500 kW capacity each)
    for (int i = 1; i <= numSubstations; i++)
    {
        int x, y;
        presetPosition(numSubstations, i, x, y);
        substations.push_back(Substation(i, "SS" + to_string(i), 500, x, y));
    }

    // Dynamic consumers
    struct ConsumerTemplate {
        string name;
        int demand;
        int priority;
    };
    static const ConsumerTemplate templates[] = {
        {"Hospital", 200, 1},
        {"EmergencyCenter", 150, 2},
        {"School", 100, 3},
        {"ResidentialArea", 250, 4},
        {"IndustrialPark", 200, 3},
        {"WaterTreatment", 180, 2},
        {"DataCenter", 220, 2},
        {"TelecomTower", 80, 1},
        {"CommercialHub", 160, 4},
        {"MetroStation", 190, 2},
        {"PoliceStation", 120, 1},
        {"FireStation", 140, 1},
        {"ResearchLab", 210, 3},
        {"CityHall", 110, 2},
        {"Airport", 300, 1},
        {"HarborDock", 240, 3},
        {"ShoppingMall", 170, 4},
        {"UniversityCampus", 190, 3},
        {"SportsComplex", 130, 4},
        {"SuburbanDistrict", 200, 4}
    };
    int numConsumers = min(numSubstations, 20);
    if (numSubstations == 4) numConsumers = 4;
    for (int i = 0; i < numConsumers; i++)
    {
        consumers.push_back(
            Consumer(i + 1, templates[i].name, templates[i].demand, templates[i].priority)
        );
    }
    nextConsumerId = numConsumers + 1;

    // Transmission lines topology
    if (numSubstations == 4)
    {
        // Preserve exact existing 4-substation diamond topology
        lines.push_back(TransmissionLine(0, 1, 10));
        lines.push_back(TransmissionLine(1, 2, 15));
        lines.push_back(TransmissionLine(2, 3, 20));
        lines.push_back(TransmissionLine(1, 4, 25));
        lines.push_back(TransmissionLine(4, 3, 10));
    }
    else if (numSubstations == 2)
    {
        lines.push_back(TransmissionLine(0, 1, 10));
        lines.push_back(TransmissionLine(1, 2, 15));
    }
    else if (numSubstations == 3)
    {
        lines.push_back(TransmissionLine(0, 1, 10));
        lines.push_back(TransmissionLine(1, 2, 15));
        lines.push_back(TransmissionLine(2, 3, 20));
        lines.push_back(TransmissionLine(3, 1, 25));
    }
    else
    {
        // 1. Primary feed line from PS1 to SS1
        lines.push_back(TransmissionLine(0, 1, 10));

        // 2. Ring connection through all substations (1->2, 2->3, ..., N->1)
        for (int i = 1; i < numSubstations; i++)
        {
            lines.push_back(TransmissionLine(i, i + 1, 12 + ((i * 7) % 15)));
        }
        lines.push_back(TransmissionLine(numSubstations, 1, 20));

        // 3. Redundant cross-links (chords) for mesh resilience
        for (int i = 1; i <= numSubstations / 2; i++)
        {
            int target = i + (numSubstations / 2);
            if (target <= numSubstations && target != i + 1 && !(i == 1 && target == numSubstations))
            {
                lines.push_back(TransmissionLine(i, target, 20 + ((i * 5) % 15)));
            }
        }
    }

    rebuild(true);
}

void GridManager::clearGrid()
{
    stations.clear();
    substations.clear();
    lines.clear();
    failureManager = FailureManager();
    rebuild(true);
}

// =====================================================================
// Console operations (also reused by the JSON API)
// =====================================================================

void GridManager::displayGrid()
{
    if(getTotalNodes() == 0)
    {
        cout << "The grid is empty.\n";
        return;
    }
    graph.display();
}

void GridManager::createFailure(int id)
{
    if (!isNode(id))
    {
        cout << "Invalid node ID.\n";
        return;
    }

    if (isFailed(id))
    {
        cout << nodeName(id) << " has already failed.\n";
        return;
    }

    setFailed(id, true);
    failureManager.addFailure(id);
    rebuild(true);

    cout << nodeName(id) << " has failed.\n";
}

void GridManager::undoFailure()
{
    int id = failureManager.undoFailure();

    if (id == -1)
    {
        cout << "No failure to undo.\n";
        return;
    }

    setFailed(id, false);
    rebuild(true);

    cout << nodeName(id) << " failure has been undone.\n";
}

void GridManager::restoreAll()
{
    int count = 0;
    int id;
    while ((id = failureManager.undoFailure()) != -1)
    {
        setFailed(id, false);
        count++;
    }
    rebuild(true);

    if (count == 0) cout << "Nothing to restore - no failures.\n";
    else cout << "Restored " << count << " failed node" << (count == 1 ? "" : "s") << ".\n";
}

void GridManager::affectedArea()
{
    vector<int> connected = reachable();

    cout << "\n------- AFFECTED AREAS -------\n";

    if (sourceIds().empty())
    {
        cout << "No working power station on the grid.\n";
    }

    for (const auto &st : stations)
    {
        cout << st.name << " : " << (st.failed ? "Failed (power station)" : "Power station") << "\n";
    }

    for (const auto &ss : substations)
    {
        bool found = find(connected.begin(), connected.end(), ss.id) != connected.end();
        cout << ss.name << " : " << (found ? "Connected" : "Affected") << "\n";
    }

    cout << "------------------------------\n";
}

void GridManager::checkConnection(int start, int end)
{
    if (!isNode(start) || !isNode(end))
    {
        cout << "Invalid node ID.\n";
        return;
    }

    vector<int> result = dfs(graph, start, failedFlags());
    bool found = find(result.begin(), result.end(), end) != result.end();

    cout << "\n" << (found ? "Connection exists.\n" : "No connection exists.\n");
}

void GridManager::alternatePath(int start, int end)
{
    if (!isNode(start) || !isNode(end))
    {
        cout << "Invalid node ID.\n";
        return;
    }

    vector<int> path = dijkstra(graph, start, end, failedFlags());

    cout << "\n------- BEST PATH -------\n";

    if (path.empty())
    {
        cout << "No path available.\n";
        return;
    }

    for (size_t i = 0; i < path.size(); i++)
    {
        cout << graph.getName(path[i]);
        if (i != path.size() - 1)
        {
            cout << " -> ";
        }
    }

    cout << "\n-------------------------\n";
}

void GridManager::redistributePower()
{
    vector<int> group, budgets;
    vector<vector<int>> members;
    islands(group, budgets, members);
    powerManager.redistribute(substations, consumers, group, budgets);
}

void GridManager::searchComponent(string name)
{
    if (components.find(name) == components.end())
    {
        cout << "Component not found.\n";
        return;
    }

    cout << "Component found.\n";
    cout << "Name: " << name << endl;
    cout << "ID: " << components[name] << endl;
}

void GridManager::showSubstations()
{
    cout << "\n------- POWER STATIONS -------\n";
    for (const auto &s : stations)
    {
        cout << "ID: " << s.id << " | " << s.name << " | Capacity: " << s.capacity
             << " kW | " << (s.failed ? "FAILED" : "ACTIVE") << endl;
    }

    cout << "\n------- SUBSTATIONS -------\n";

    for (const auto &s : substations)
    {
        cout << "ID: " << s.id
             << " | " << s.name
             << " | Load: " << s.currentLoad << "/" << s.capacity << " kW | "
             << (s.failed ? "FAILED" : "ACTIVE") << endl;
    }

    cout << "---------------------------\n";
}

void GridManager::showConsumers()
{
    cout << "\n------- CONSUMERS -------\n";

    for (size_t i = 0; i < consumers.size(); i++)
    {
        cout << consumers[i].id
             << " | " << consumers[i].name
             << " | " << consumers[i].requiredPower << " kW"
             << " | Priority: " << consumers[i].priority
             << endl;
    }

    cout << "-------------------------\n";
}

void GridManager::showFailures()
{
    failureManager.display();
}

bool GridManager::addTransmissionLine(int from, int to, int distance)
{
    if (!isNode(from) || !isNode(to) || from == to || distance <= 0)
    {
        return false;
    }
    if (lineIndex(from, to) != -1)
    {
        return false;
    }
    lines.push_back(TransmissionLine(from, to, distance));
    rebuild(true);
    return true;
}

// =====================================================================
// Interactive editing
// =====================================================================

string GridManager::addNode(bool isStation, string name, int capacity, int x, int y, int &newId)
{
    newId = -1;
    if (getTotalNodes() >= MAX_NODES)
        return "The map is full (" + to_string(MAX_NODES) + " nodes max). Delete a node first.";

    if (x < 0 || x > MAP_W || y < 0 || y > MAP_H)
        return "Position is outside the map.";

    if (name.empty()) name = autoName(isStation ? "PS" : "SS");
    else if (!validName(name)) return NAME_RULE;
    if (nameTaken(name, -1)) return "The name " + name + " is already used.";

    int maxCap = isStation ? 100000 : 10000;
    if (capacity == -1) capacity = isStation ? 1000 : 500;
    if (capacity < 1 || capacity > maxCap)
        return "Capacity must be between 1 and " + to_string(maxCap) + " kW.";

    newId = freeId();
    if (isStation) stations.push_back(PowerStation(newId, name, capacity, x, y));
    else substations.push_back(Substation(newId, name, capacity, x, y));

    rebuild(true);
    return "";
}

string GridManager::moveNode(int id, int x, int y)
{
    if (!isNode(id)) return "Unknown node.";
    if (x < 0 || x > MAP_W || y < 0 || y > MAP_H) return "Position is outside the map.";

    int s = stationIndex(id);
    if (s != -1) { stations[s].x = x; stations[s].y = y; }
    int b = substationIndex(id);
    if (b != -1) { substations[b].x = x; substations[b].y = y; }
    return ""; // position does not affect the graph
}

string GridManager::updateNode(int id, const string &name, int capacity)
{
    if (!isNode(id)) return "Unknown node.";

    int s = stationIndex(id);
    int b = substationIndex(id);

    if (!name.empty())
    {
        if (!validName(name)) return NAME_RULE;
        if (nameTaken(name, id)) return "The name " + name + " is already used.";
    }
    if (capacity != -1)
    {
        int maxCap = s != -1 ? 100000 : 10000;
        if (capacity < 1 || capacity > maxCap)
            return "Capacity must be between 1 and " + to_string(maxCap) + " kW.";
    }

    if (s != -1)
    {
        if (!name.empty()) stations[s].name = name;
        if (capacity != -1) stations[s].capacity = stations[s].availablePower = capacity;
    }
    else
    {
        if (!name.empty()) substations[b].name = name;
        if (capacity != -1) substations[b].capacity = capacity;
    }

    rebuild(capacity != -1);
    return "";
}

string GridManager::removeNode(int id)
{
    int s = stationIndex(id);
    int b = substationIndex(id);
    if (s == -1 && b == -1) return "Unknown node.";

    if (s != -1) stations.erase(stations.begin() + s);
    else substations.erase(substations.begin() + b);

    lines.erase(remove_if(lines.begin(), lines.end(),
                          [id](const TransmissionLine &l) { return l.from == id || l.to == id; }),
                lines.end());

    failureManager.remove(id);
    rebuild(true);
    return "";
}

string GridManager::updateLine(int from, int to, int distance)
{
    int i = lineIndex(from, to);
    if (i == -1) return "There is no line between these nodes.";
    if (distance < 1 || distance > 1000) return "Distance must be between 1 and 1000 km.";
    lines[i].distance = distance;
    rebuild(false);
    return "";
}

string GridManager::removeLine(int from, int to)
{
    int i = lineIndex(from, to);
    if (i == -1) return "There is no line between these nodes.";
    lines.erase(lines.begin() + i);
    rebuild(true);
    return "";
}

string GridManager::addConsumer(const string &name, int required, int priority, int &newId)
{
    newId = -1;
    if ((int)consumers.size() >= MAX_CONSUMERS)
        return "Consumer limit reached (" + to_string(MAX_CONSUMERS) + ").";
    if (!validName(name)) return NAME_RULE;
    for (const auto &c : consumers)
        if (lowerCase(c.name) == lowerCase(name)) return "A consumer called " + name + " already exists.";
    if (required < 1 || required > 100000) return "Demand must be between 1 and 100000 kW.";
    if (priority < 1 || priority > 9) return "Priority must be between 1 (most important) and 9.";

    newId = nextConsumerId++;
    consumers.push_back(Consumer(newId, name, required, priority));
    return "";
}

string GridManager::removeConsumer(int id)
{
    for (size_t i = 0; i < consumers.size(); i++)
    {
        if (consumers[i].id == id)
        {
            consumers.erase(consumers.begin() + i);
            return "";
        }
    }
    return "Unknown consumer.";
}
