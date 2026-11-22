#include <iostream>
#include "GridManager.h"

using namespace std;

int main()
{
    GridManager grid;

    grid.createGrid(4);

    int choice;

    do
    {
        int n = grid.getNumSubstations();
        cout << "\n";
        cout << "====================================\n";
        cout << "   GRID RESCUE (" << n << " Substations)\n";
        cout << "====================================\n";

        cout << "1. Display Power Grid\n";
        cout << "2. Display Stations & Substations\n";
        cout << "3. Display Consumers\n";
        cout << "4. Create Failure\n";
        cout << "5. Undo Failure\n";
        cout << "6. Find Affected Area\n";
        cout << "7. Check Connection\n";
        cout << "8. Find Alternate Path \n";
        cout << "9. Redistribute Power\n";
        cout << "10. Search Component\n";
        cout << "11. Show Failure History\n";
        cout << "12. Configure Number of Substations (2-20)\n";
        cout << "0. Exit\n";

        cout << "====================================\n";

        cout << "Enter choice: ";
        cin >> choice;

        switch(choice)
        {
            case 1:
                grid.displayGrid();
                break;

            case 2:
                grid.showSubstations();
                break;

            case 3:
                grid.showConsumers();
                break;

            case 4:
            {
                int id;
                cout << "Enter node ID to fail (power station or substation): ";
                cin >> id;

                grid.createFailure(id);
                break;
            }

            case 5:
                grid.undoFailure();
                break;

            case 6:
                grid.affectedArea();
                break;

            case 7:
            {
                int start;
                int end;

                cout << "Enter starting node ID: ";
                cin >> start;

                cout << "Enter ending node ID: ";
                cin >> end;

                grid.checkConnection(start, end);
                break;
            }

            case 8:
            {
                int start;
                int end;

                cout << "Enter starting node ID: ";
                cin >> start;

                cout << "Enter ending node ID: ";
                cin >> end;

                grid.alternatePath(start, end);
                break;
            }

            case 9:
                grid.redistributePower();
                break;

            case 10:
            {
                string name;

                cout << "Enter component name: ";
                cin >> name;

                grid.searchComponent(name);
                break;
            }

            case 11:
                grid.showFailures();
                break;

            case 12:
            {
                int num;
                cout << "Enter number of substations (2-20): ";
                cin >> num;
                if (num < 2 || num > 20)
                {
                    cout << "Invalid number! Must be between 2 and 20.\n";
                }
                else
                {
                    grid.createGrid(num);
                    cout << "Grid re-initialized with " << num << " substations.\n";
                }
                break;
            }

            case 0:
                cout << "Exiting GridRescue...\n";
                break;

            default:
                cout << "Invalid choice.\n";
        }

    } while(choice != 0);

    return 0;
}
