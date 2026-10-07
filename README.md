# GridRescue ⚡

## About the Project

GridRescue is a C++ project that simulates a small power grid.

The idea is to see what happens when a connection fails — which users are affected, whether an alternate path is available, and how limited power can be given to important users first.

The project is mainly made to apply the **DSA and OOP concepts** we are learning in C++ to a practical problem.

> This is an educational simulation and not a real power-grid control system.

## What does GridRescue do?

The power grid is represented using **nodes and connections**. Nodes can represent a power source, hospital, residential area, emergency service, or other consumers.

When a connection fails, the project checks the affected nodes and looks for an alternate path if one is available. Important consumers can also be given higher priority when power is limited.
---
## DSA Concepts Used

* **Graph & Adjacency List** – to represent the grid
* **BFS & DFS** – to check network connectivity
* **Dijkstra's Algorithm** – to find alternate paths
* **Priority Queue / Min-Heap** – to handle consumer priorities
* **Vector & Unordered Map** – to store and manage data

---

## OOP Concepts Used

* Classes and Objects
* Constructors
* Encapsulation
* Abstraction
* Functions and Data Members

---

## How It Works

```text
Create the grid
      ↓
Add nodes and connections
      ↓
Simulate a failure
      ↓
Find affected nodes
      ↓
Look for an alternate path
      ↓
Prioritize important consumers
      ↓
Redistribute available power
```

---

## Technologies

* C++
* C++17
* DSA & OOP
* Dear ImGui
* Git & GitHub

---

## Project Status

**Under Development**

We are currently building the basic grid structure and adding the algorithms and simulation features step by step.

---

## Future Improvements

* Larger grid networks
* More failure scenarios
* Different priority levels
* Saving and loading grid configurations

---

## Learning

Through GridRescue, we are learning how **graphs, BFS, DFS, Dijkstra, priority queues and OOP** can be combined to solve a problem inspired by a real-world situation.
