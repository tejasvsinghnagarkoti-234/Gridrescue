#include <iostream>
#include "FailureManager.h"

using namespace std;

void FailureManager::addFailure(int id)
{
    failures.push(id);
}

int FailureManager::undoFailure()
{
    if(failures.empty())
    {
        return -1;
    }

    int id = failures.top();

    failures.pop();

    return id;
}

void FailureManager::display()
{
    if(failures.empty())
    {
        cout << "No failure history.\n";
        return;
    }

    stack<int> temp = failures;

    cout << "Failure history: ";

    while(!temp.empty())
    {
        cout << temp.top() << " ";
        temp.pop();
    }

    cout << endl;
}

vector<int> FailureManager::history() const
{
    vector<int> out;
    stack<int> temp = failures;

    while(!temp.empty())
    {
        out.insert(out.begin(), temp.top());
        temp.pop();
    }

    return out;
}


void FailureManager::remove(int id)
{
    stack<int> kept;
    while(!failures.empty())
    {
        if(failures.top() != id) kept.push(failures.top());
        failures.pop();
    }
    while(!kept.empty())
    {
        failures.push(kept.top());
        kept.pop();
    }
}

bool FailureManager::empty() const
{
    return failures.empty();
}
