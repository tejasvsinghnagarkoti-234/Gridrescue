#ifndef FAILUREMANAGER_H
#define FAILUREMANAGER_H

#include <stack>
#include <vector>
using namespace std;

class FailureManager
{
private:
    stack<int> failures;

public:
    void addFailure(int id);
    int undoFailure();
    void display();
    vector<int> history() const; // oldest -> newest
    void remove(int id);         // drop a node that was deleted from the grid
    bool empty() const;
};

#endif
