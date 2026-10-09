#ifndef FAILUREMANAGER_H
#define FAILUREMANAGER_H

#include <stack>
using namespace std;

class FailureManager
{
private:
    stack<int> failures;

public:
    void addFailure(int id);
    int undoFailure();
    void display();
};

#endif
