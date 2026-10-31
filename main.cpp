#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <iomanip>
#include <ctime>
#include <limits>
#include <stdexcept>
#include <algorithm>

using namespace std;

const string ACCOUNT_FILE = "accounts.txt";
const string TRANSACTION_FILE = "transactions.txt";

class BankException : public runtime_error {
public:
    explicit BankException(const string& message) : runtime_error(message) {}
};

class Account {
private:
    int accountNumber;
    string holderName;
    double balance;

protected:
    void setBalance(double amount) { balance = amount; }

public:
    Account(int number, const string& name, double openingBalance)
        : accountNumber(number), holderName(name), balance(openingBalance) {}

    virtual ~Account() = default;

    int getAccountNumber() const { return accountNumber; }
    string getHolderName() const { return holderName; }
    double getBalance() const { return balance; }

    virtual string getAccountType() const = 0;
    virtual double minimumBalance() const = 0;

    void deposit(double amount) {
        if (amount <= 0) throw BankException("Deposit amount must be greater than zero.");
        setBalance(getBalance() + amount);
    }

    virtual void withdraw(double amount) {
        if (amount <= 0) throw BankException("Withdrawal amount must be greater than zero.");
        if (getBalance() - amount < minimumBalance())
            throw BankException("Insufficient balance or minimum-balance rule would be violated.");
        setBalance(getBalance() - amount);
    }

    void debitForTransfer(double amount) { withdraw(amount); }
    void creditForTransfer(double amount) { deposit(amount); }
};

class SavingsAccount : public Account {
public:
    SavingsAccount(int n, const string& name, double balance)
        : Account(n, name, balance) {}
    string getAccountType() const override { return "SAVINGS"; }
    double minimumBalance() const override { return 500.0; }
};

class CurrentAccount : public Account {
public:
    CurrentAccount(int n, const string& name, double balance)
        : Account(n, name, balance) {}
    string getAccountType() const override { return "CURRENT"; }
    double minimumBalance() const override { return 0.0; }
};

class Bank {
private:
    vector<unique_ptr<Account>> accounts;
    int nextAccountNumber = 1001;

    Account* findAccount(int number) {
        for (auto& account : accounts)
            if (account->getAccountNumber() == number) return account.get();
        return nullptr;
    }

    static string currentTime() {
        time_t now = time(nullptr);
        tm* local = localtime(&now);
        stringstream ss;
        ss << put_time(local, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    void recordTransaction(int accountNo, const string& type,
                           double amount, const string& note) {
        ofstream file(TRANSACTION_FILE, ios::app);
        if (!file) throw BankException("Could not open transaction file.");
        file << accountNo << '|' << currentTime() << '|' << type << '|'
             << fixed << setprecision(2) << amount << '|' << note << '\n';
    }

    void saveAccounts() const {
        ofstream file(ACCOUNT_FILE);
        if (!file) throw BankException("Could not save account data.");
        for (const auto& a : accounts) {
            file << a->getAccountNumber() << '|'
                 << a->getHolderName() << '|'
                 << a->getAccountType() << '|'
                 << fixed << setprecision(2) << a->getBalance() << '\n';
        }
    }

    void loadAccounts() {
        ifstream file(ACCOUNT_FILE);
        if (!file) return; // First run: file will be created when an account is opened.

        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string numberText, name, type, balanceText;
            if (!getline(ss, numberText, '|') ||
                !getline(ss, name, '|') ||
                !getline(ss, type, '|') ||
                !getline(ss, balanceText)) continue;

            try {
                int number = stoi(numberText);
                double balance = stod(balanceText);
                if (type == "SAVINGS")
                    accounts.push_back(make_unique<SavingsAccount>(number, name, balance));
                else if (type == "CURRENT")
                    accounts.push_back(make_unique<CurrentAccount>(number, name, balance));
                nextAccountNumber = max(nextAccountNumber, number + 1);
            } catch (...) {
                // Ignore malformed records and continue loading valid accounts.
            }
        }
    }

public:
    Bank() { loadAccounts(); }

    void createAccount() {
        string name;
        int typeChoice;
        double openingBalance;

        cout << "Account holder name (do not use | character): ";
        getline(cin >> ws, name);
        if (name.empty() || name.find('|') != string::npos)
            throw BankException("Invalid account holder name.");

        cout << "1. Savings (minimum balance Rs. 500)\n"
             << "2. Current (minimum balance Rs. 0)\nChoose account type: ";
        if (!(cin >> typeChoice) || (typeChoice != 1 && typeChoice != 2))
            throw BankException("Invalid account type.");

        cout << "Opening balance: Rs. ";
        if (!(cin >> openingBalance) || openingBalance < 0)
            throw BankException("Opening balance must be a valid non-negative number.");

        if (typeChoice == 1 && openingBalance < 500)
            throw BankException("Savings account requires at least Rs. 500 to open.");

        int number = nextAccountNumber++;
        if (typeChoice == 1)
            accounts.push_back(make_unique<SavingsAccount>(number, name, openingBalance));
        else
            accounts.push_back(make_unique<CurrentAccount>(number, name, openingBalance));

        saveAccounts();
        cout << "\nAccount created successfully. Your account number is: " << number << "\n";
    }

    void showAccount() {
        int number;
        cout << "Enter account number: ";
        if (!(cin >> number)) throw BankException("Account number must be numeric.");
        Account* a = findAccount(number);
        if (!a) throw BankException("Account not found.");

        cout << "\nAccount Number : " << a->getAccountNumber()
             << "\nHolder Name    : " << a->getHolderName()
             << "\nAccount Type   : " << a->getAccountType()
             << "\nBalance        : Rs. " << fixed << setprecision(2) << a->getBalance()
             << "\n";
    }

    void depositMoney() {
        int number;
        double amount;
        cout << "Enter account number: ";
        if (!(cin >> number)) throw BankException("Account number must be numeric.");
        Account* a = findAccount(number);
        if (!a) throw BankException("Account not found.");
        cout << "Enter deposit amount: Rs. ";
        if (!(cin >> amount)) throw BankException("Amount must be numeric.");

        a->deposit(amount);
        saveAccounts();
        recordTransaction(number, "DEPOSIT", amount, "Cash deposit");
        cout << "Deposit successful. New balance: Rs. " << a->getBalance() << "\n";
    }

    void withdrawMoney() {
        int number;
        double amount;
        cout << "Enter account number: ";
        if (!(cin >> number)) throw BankException("Account number must be numeric.");
        Account* a = findAccount(number);
        if (!a) throw BankException("Account not found.");
        cout << "Enter withdrawal amount: Rs. ";
        if (!(cin >> amount)) throw BankException("Amount must be numeric.");

        a->withdraw(amount);
        saveAccounts();
        recordTransaction(number, "WITHDRAWAL", amount, "ATM cash withdrawal");
        cout << "Withdrawal successful. New balance: Rs. " << a->getBalance() << "\n";
    }

    void transferMoney() {
        int fromNumber, toNumber;
        double amount;
        cout << "Sender account number: ";
        if (!(cin >> fromNumber)) throw BankException("Account number must be numeric.");
        cout << "Receiver account number: ";
        if (!(cin >> toNumber)) throw BankException("Account number must be numeric.");
        if (fromNumber == toNumber) throw BankException("Sender and receiver must be different.");

        Account* sender = findAccount(fromNumber);
        Account* receiver = findAccount(toNumber);
        if (!sender || !receiver) throw BankException("Sender or receiver account not found.");

        cout << "Transfer amount: Rs. ";
        if (!(cin >> amount)) throw BankException("Amount must be numeric.");
        if (amount <= 0) throw BankException("Transfer amount must be greater than zero.");

        // Validate before changing either account, so a rejected transfer changes no balance.
        if (sender->getBalance() - amount < sender->minimumBalance())
            throw BankException("Insufficient balance for this transfer.");

        sender->debitForTransfer(amount);
        receiver->creditForTransfer(amount);
        saveAccounts();
        recordTransaction(fromNumber, "TRANSFER_OUT", amount,
                          "To account " + to_string(toNumber));
        recordTransaction(toNumber, "TRANSFER_IN", amount,
                          "From account " + to_string(fromNumber));
        cout << "Transfer successful.\n";
    }

    void miniStatement() {
        int number;
        cout << "Enter account number: ";
        if (!(cin >> number)) throw BankException("Account number must be numeric.");
        if (!findAccount(number)) throw BankException("Account not found.");

        ifstream file(TRANSACTION_FILE);
        if (!file) {
            cout << "No transactions available yet.\n";
            return;
        }

        vector<string> rows;
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string accountText;
            if (getline(ss, accountText, '|') && accountText == to_string(number))
                rows.push_back(line);
        }

        cout << "\n----- MINI STATEMENT (latest 5) -----\n";
        if (rows.empty()) cout << "No transactions found.\n";
        else {
            size_t start = rows.size() > 5 ? rows.size() - 5 : 0;
            for (size_t i = start; i < rows.size(); ++i) {
                stringstream ss(rows[i]);
                string accountText, date, type, amount, note;
                getline(ss, accountText, '|');
                getline(ss, date, '|');
                getline(ss, type, '|');
                getline(ss, amount, '|');
                getline(ss, note);
                cout << date << " | " << left << setw(13) << type
                     << " | Rs. " << right << setw(9) << amount
                     << " | " << note << '\n';
            }
        }
        cout << "Current balance: Rs. " << fixed << setprecision(2)
             << findAccount(number)->getBalance() << "\n";
    }
};

int main() {
    Bank bank;
    int choice;

    do {
        cout << "\n========== BANKING & ATM SIMULATOR ==========\n"
             << "1. Create Account\n"
             << "2. View Account\n"
             << "3. Deposit\n"
             << "4. Withdraw\n"
             << "5. Transfer\n"
             << "6. Mini Statement\n"
             << "0. Exit\n"
             << "Enter choice: ";

        if (!(cin >> choice)) {
            cout << "\nInvalid input. Please enter a menu number.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        try {
            switch (choice) {
                case 1: bank.createAccount(); break;
                case 2: bank.showAccount(); break;
                case 3: bank.depositMoney(); break;
                case 4: bank.withdrawMoney(); break;
                case 5: bank.transferMoney(); break;
                case 6: bank.miniStatement(); break;
                case 0: cout << "Thank you for using the simulator.\n"; break;
                default: cout << "Invalid menu choice. Select 0 to 6.\n";
            }
        } catch (const BankException& e) {
            cout << "Operation failed: " << e.what() << "\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        } catch (const exception& e) {
            cout << "Unexpected error: " << e.what() << "\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    } while (choice != 0);

    return 0;
}