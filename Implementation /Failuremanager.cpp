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
