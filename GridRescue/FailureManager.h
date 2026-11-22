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
    vector<int> history() const; 
    void remove(int id);         
    bool empty() const;
};

#endif
