#include <iostream>
#include <string>
using namespace std;

void placeOrder() {
    string instrument_name, label, type;
    int amount;

    cout << "Enter instrument name (e.g., BTC-PERPETUAL, ETH-PERPETUAL): ";
    cin >> instrument_name;
    cout << "Enter amount (e.g., 10, 50, 100): ";
    cin >> amount;
    cout << "Enter label (e.g., market001, limit002): ";
    cin >> label;
    cout << "Enter type (e.g., market, limit): ";
    cin >> type;

    cout << "Placed order for " << amount << " of " << instrument_name << " with label " << label << " and type " << type << "...\n" << endl << endl << endl << endl << endl;
}

void cancelOrder() {
    string order_id;

    cout << "Enter order ID to cancel (e.g., USDC-123456789): ";
    cin >> order_id;

    cout << "Canceled order with ID " << order_id << "...\n" << endl << endl << endl << endl << endl;
}

void modifyOrder() {
    string order_id;
    int amount;
    double price;

    cout << "Enter order ID to modify (e.g., USDC-123456789): ";
    cin >> order_id;
    cout << "Enter new amount (e.g., 20, 100): ";
    cin >> amount;
    cout << "Enter new price (e.g., 25000.50): ";
    cin >> price;

    cout << "Modified order " << order_id << " to new amount " << amount << " and price " << price << "...\n" << endl << endl << endl << endl << endl;
}

void getOrderbook() {
    string instrument_name;
    int depth;

    cout << "Enter instrument name (e.g., BTC-PERPETUAL, ETH-PERPETUAL): ";
    cin >> instrument_name;
    cout << "Enter depth (number of levels, e.g., 5, 10): ";
    cin >> depth;

    cout << "Fetched orderbook for " << instrument_name << " with depth " << depth << "...\n" << endl << endl << endl << endl << endl;
}

void viewCurrentPositions() {
    string currency, kind;

    cout << "Enter currency (e.g., BTC, ETH): ";
    cin >> currency;
    cout << "Enter kind (e.g., future, option): ";
    cin >> kind;

    cout << "Viewing positions for " << currency << " with kind " << kind << "...\n" << endl << endl << endl << endl << endl;
}

bool cliMenu() {
    int choice;

    cout << "Select an option:\n";
    cout << "1. Place order\n";
    cout << "2. Cancel order\n";
    cout << "3. Modify order\n";
    cout << "4. Get orderbook\n";
    cout << "5. View current positions\n";
    cout << "6. Exit\n";
    cout << "Enter your choice: ";
    cin >> choice;

    switch (choice) {
        case 1:
            placeOrder();
            break;
        case 2:
            cancelOrder();
            break;
        case 3:
            modifyOrder();
            break;
        case 4:
            getOrderbook();
            break;
        case 5:
            viewCurrentPositions();
            break;
        case 6:
            cout << "Exiting...\n";
            return false;
        default:
            cout << "Invalid choice, please try again.\n";
            break;
    }
    return true;
}

int main() {
    while (cliMenu()) {}
    return 0;
}
