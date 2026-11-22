# GridRescue — fixed runnable version

GridRescue is an educational C++17 console simulation of a small power grid. It demonstrates OOP, graph adjacency lists, BFS, DFS, Dijkstra's shortest path algorithm, a stack for failure history, and a priority queue for consumer prioritization.

## Web UI (new)
A browser dashboard now sits on top of the same C++ logic. `WebMain.cpp` is a tiny built-in HTTP server (no libraries) that serves `web/` and a JSON API (`GridJson.cpp`) backed by `GridManager`, so BFS, DFS, Dijkstra, the failure stack and the priority-queue redistribution all run in C++.

```bash
./run.sh            # Linux/macOS: builds GridRescueWeb and opens http://localhost:8080
./run.sh 9000       # custom port
./run.sh console    # original console version
```
Windows: double-click `run.bat` (`run.bat console` for the console version).

### Building the grid yourself (map editor)
The map is now an editor. Pick a tool from the toolbar above the map (keyboard shortcut in brackets):

| Tool | What a click does |
|---|---|
| Select (V) | Click a node or line to open it in the **Inspector**; drag a node to move it. `Delete` removes the selection. |
| Power station (P) | Places a power station on an empty spot (snaps to a 20-unit grid). |
| Substation (S) | Places a substation on an empty spot. |
| Connect (C) | Click one node, then another, to add a transmission line. Length is taken from the map distance (10 map units = 1 km) unless you type a value in "New line length". |
| Fail (F) | Fails a node; clicking the most recent failure restores it (stack order). |
| Delete (D) | Deletes the clicked node (and its lines) or line. |

`Esc` cancels a half-drawn line, then returns to Select. The Inspector lets you rename a node, change its capacity/output, see its lines, fail/restore/delete it, or use it as the From/To node for DFS and Dijkstra. Selecting a line lets you change its length or delete it.

"Start from" loads a preset (the original 4-substation diamond, or N substations) or a **blank canvas**. Consumers can be added and removed in the Consumers card.

### How multiple power stations are handled
- Power stations and substations share one ID space; IDs are graph vertex indices. A deleted node's ID is reused by the next node you add.
- **Affected areas** uses a multi-source BFS (`bfsMulti` in `Algorithms.cpp`) from every working power station.
- Power stations can fail too; a failed station supplies nothing and its lines go out of service.
- **Redistribution** splits the powered grid into *islands* (connected components containing at least one working station). Each substation can only hand out power while its own island's generation budget lasts, so a station never feeds a part of the grid it is not wired to. Stations not wired to any substation are reported as idle.

### Simplifications to keep in mind
- Consumers are not placed on the map: any consumer can draw from any powered substation (as in the original design). Placing consumers as nodes would make the model more realistic.
- Line capacity / power-flow physics are not modelled; distance is only a path weight for Dijkstra.
- The grid lives in the server's memory, so it resets when you restart `GridRescueWeb`.

### Web API (all mutations are POST)
`state`, `affected`, `connection?from&to`, `path?from&to`, `search?name` (GET);
`addnode?type=station|substation&x&y[&name][&cap]`, `move?id&x&y`, `updatenode?id[&name][&cap]`, `removenode?id`,
`addline?from&to&dist`, `updateline?from&to&dist`, `removeline?from&to`, `fail?id`, `undo`, `restore`,
`reset?n` (load preset), `clear` (blank canvas), `redistribute`, `addconsumer?name&req&pri`, `removeconsumer?id`.

The server only listens on 127.0.0.1.

## Requirements
- A C++ compiler such as GCC (g++) or MinGW-w64.
- No third-party libraries are required for this console version.

## Run on Windows (VS Code terminal / MinGW)
Open this folder in VS Code, open the terminal in this folder, then run:

```powershell
g++ -std=c++17 Main.cpp GridManager.cpp Graph.cpp Algorithms.cpp FailureManager.cpp PowerManager.cpp PowerStation.cpp Substation.cpp TransmissionLine.cpp Consumer.cpp -o GridRescue.exe
.\GridRescue.exe
```

You can also double-click `run.bat` if `g++` is on your PATH.

## Run on Linux / macOS
```bash
chmod +x run.sh
./run.sh
```

Or compile manually:
```bash
g++ -std=c++17 Main.cpp GridManager.cpp Graph.cpp Algorithms.cpp FailureManager.cpp PowerManager.cpp PowerStation.cpp Substation.cpp TransmissionLine.cpp Consumer.cpp -o GridRescue
./GridRescue
```

## Menu
1. Display power grid
2. Display power stations and substations
3. Display consumers
4. Create a failure (any node ID: power station or substation)
5. Undo the latest failure
6. Find affected areas (BFS)
7. Check connection (DFS)
8. Find the shortest available path (Dijkstra)
9. Redistribute power by consumer priority
10. Search for a component by name (e.g. `PS1`, `SS1`)
11. Show failure history
0. Exit

## Fixes made
- Standardized source and header filenames so their capitalization matches the `#include` statements on case-sensitive systems.
- Corrected the misspelled consumer source filename.
- Added build/run instructions and simple launch scripts.
- Added bounds checks to graph algorithms to avoid invalid node IDs causing out-of-range access.

## Note
This is a learning simulation only, not software for controlling real electrical infrastructure. The Dear ImGui GUI mentioned in the original README is not included; the web UI replaces it.
