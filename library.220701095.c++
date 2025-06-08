#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <ctime>

using namespace std;

enum UserType { REGULAR, LIBRARIAN };

class ExceptionHandler {
public:
    static void handle(const string& msg) {
        cout << "Error: " << msg << "\n";
    }
};

class Logger {
public:
    static void log(const string& msg) {
        cout << msg << "\n";
    }
};

class Book {
protected:
    int id;
    string title;
    string author;
    string category;
    string publishDate;
    int pages;
    bool isAvailable;
public:
    Book(int i, const string& t, const string& a, const string& c, const string& pd, int p) :
        id(i), title(t), author(a), category(c), publishDate(pd), pages(p), isAvailable(true) {}
    virtual ~Book() {}
    int getId() const { return id; }
    bool available() const { return isAvailable; }
    void setAvailable(bool av) { isAvailable = av; }
    virtual void print() const {
        cout << "ID: " << id << " | Title: " << title << " | Author: " << author << " | Category: " << category
             << " | Publish Date: " << publishDate << " | Pages: " << pages << " | Status: "
             << (isAvailable ? "Available" : "Borrowed") << "\n";
    }
    virtual double getFineRate() const { return 1.0; }
    virtual bool canBeBorrowed() const { return true; }
};

class TextBook : public Book {
    string level;
    string field;
public:
    TextBook(int i, const string& t, const string& a, const string& c, const string& pd, int p, const string& l, const string& f) :
        Book(i, t, a, c, pd, p), level(l), field(f) {}
    void print() const override {
        Book::print();
        cout << "   Level: " << level << " | Field: " << field << "\n";
    }
    double getFineRate() const override { return 0.5; }
};

class Magazine : public Book {
    int issueNumber;
public:
    Magazine(int i, const string& t, const string& a, const string& c, const string& pd, int p, int issue) :
        Book(i, t, a, c, pd, p), issueNumber(issue) {}
    void print() const override {
        Book::print();
        cout << "   Issue Number: " << issueNumber << "\n";
    }
    double getFineRate() const override { return 0.7; }
};

class ReferenceBook : public Book {
public:
    ReferenceBook(int i, const string& t, const string& a, const string& c, const string& pd, int p) :
        Book(i, t, a, c, pd, p) {}
    void print() const override {
        Book::print();
        cout << "   (Reference Book - cannot be borrowed)\n";
    }
    bool canBeBorrowed() const override { return false; }
    double getFineRate() const override { return 0.0; }
};

class User {
protected:
    int userId;
    string username;
    string password;
    UserType type;
    int maxBorrow;
    int borrowDays;
    int borrowedCount;
    double fines;
public:
    User(int id, const string& uname, const string& pwd, UserType t) :
        userId(id), username(uname), password(pwd), type(t), borrowedCount(0), fines(0.0) {
        if (type == REGULAR) {
            maxBorrow = 5;
            borrowDays = 14;
        } else {
            maxBorrow = 1000;
            borrowDays = 365;
        }
    }
    virtual ~User() {}
    int getUserId() const { return userId; }
    string getUsername() const { return username; }
    bool authenticate(const string& pwd) const { return pwd == password; }
    bool canBorrow() const { return borrowedCount < maxBorrow && fines < 100.0; }
    void borrowBook() { borrowedCount++; }
    void returnBook() { if (borrowedCount > 0) borrowedCount--; }
    int getBorrowDays() const { return borrowDays; }
    void addFine(double amount) { fines += amount; }
    double getFines() const { return fines; }
    void payFine(double amount) { fines -= amount; if (fines < 0) fines = 0; }
    virtual void print() const {
        cout << "UserID: " << userId << " | Username: " << username << " | Type: " << (type == REGULAR ? "Regular" : "Librarian")
             << " | Borrowed Books: " << borrowedCount << " | Fines: " << fines << "\n";
    }
    UserType getUserType() const { return type; }
};

class Librarian : public User {
public:
    Librarian(int id, const string& uname, const string& pwd) : User(id, uname, pwd, LIBRARIAN) {}
};

struct LoanRecord {
    int bookId;
    int userId;
    time_t borrowDate;
    time_t returnDate;
};

class LoanManagement {
    unordered_map<int, LoanRecord> activeLoans;
public:
    bool loanBook(int bookId, User& user, Book& book) {
        if (!book.canBeBorrowed()) {
            ExceptionHandler::handle("Book cannot be borrowed (reference book).");
            return false;
        }
        if (!book.available()) {
            ExceptionHandler::handle("Book is not available.");
            return false;
        }
        if (!user.canBorrow()) {
            ExceptionHandler::handle("User cannot borrow more books or has high fines.");
            return false;
        }
        time_t now = time(nullptr);
        LoanRecord record{ bookId, user.getUserId(), now, 0 };
        activeLoans[bookId] = record;
        book.setAvailable(false);
        user.borrowBook();
        Logger::log("Book borrowed successfully.");
        return true;
    }

    bool returnBook(int bookId, User& user, Book& book) {
        if (activeLoans.find(bookId) == activeLoans.end()) {
            ExceptionHandler::handle("No active loan for this book.");
            return false;
        }
        LoanRecord& record = activeLoans[bookId];
        if (record.userId != user.getUserId()) {
            ExceptionHandler::handle("This user did not borrow this book.");
            return false;
        }
        time_t now = time(nullptr);
        record.returnDate = now;
        double fine = calculateFine(book, record.borrowDate, now);
        if (fine > 0) {
            user.addFine(fine);
            Logger::log("Late return fine: " + to_string(fine));
        }
        book.setAvailable(true);
        user.returnBook();
        activeLoans.erase(bookId);
        Logger::log("Book returned successfully.");
        return true;
    }

    double calculateFine(Book& book, time_t borrowDate, time_t returnDate) {
        double daysLate = difftime(returnDate, borrowDate) / (60 * 60 * 24) - 14;
        if (daysLate <= 0) return 0.0;
        double rate = book.getFineRate();
        return daysLate * rate;
    }
};

class ReservationSystem {
    unordered_map<int, queue<int>> reservations;
    const int maxQueueSize = 10;
public:
    bool reserveBook(int bookId, int userId) {
        auto& q = reservations[bookId];
        if ((int)q.size() >= maxQueueSize) {
            ExceptionHandler::handle("Reservation queue is full.");
            return false;
        }
        for (int uid : q) {
            if (uid == userId) {
                ExceptionHandler::handle("User has already reserved this book.");
                return false;
            }
        }
        q.push(userId);
        Logger::log("Book reserved successfully.");
        return true;
    }
    void notifyNextUser(int bookId) {
        if (reservations.find(bookId) == reservations.end() || reservations[bookId].empty()) return;
        int nextUser = reservations[bookId].front();
        cout << "Notification: User " << nextUser << " can now borrow book " << bookId << "\n";
        reservations[bookId].pop();
    }
    void cancelReservation(int bookId, int userId) {
        if (reservations.find(bookId) == reservations.end()) return;
        queue<int> newQueue;
        while (!reservations[bookId].empty()) {
            int uid = reservations[bookId].front();
            reservations[bookId].pop();
            if (uid != userId) newQueue.push(uid);
        }
        reservations[bookId] = newQueue;
        Logger::log("Reservation cancelled if existed.");
    }
};

class LibraryManagementSystem {
    unordered_map<int, User*> users;
    unordered_map<int, Book*> books;
    LoanManagement loanManagement;
    ReservationSystem reservationSystem;
public:
    ~LibraryManagementSystem() {
        for (auto& p : users) delete p.second;
        for (auto& p : books) delete p.second;
    }

    void addUser(User* user) {
        if (users.find(user->getUserId()) != users.end()) {
            ExceptionHandler::handle("User ID already exists.");
            delete user;
            return;
        }
        users[user->getUserId()] = user;
        Logger::log("User added.");
    }

    void addBook(Book* book) {
        if (books.find(book->getId()) != books.end()) {
            ExceptionHandler::handle("Book ID already exists.");
            delete book;
            return;
        }
        books[book->getId()] = book;
        Logger::log("Book added.");
    }

    User* authenticateUser(int userId, const string& password) {
        if (users.find(userId) == users.end()) return nullptr;
        if (users[userId]->authenticate(password)) return users[userId];
        return nullptr;
    }

    void borrowBook(int userId, int bookId) {
        if (users.find(userId) == users.end()) {
            ExceptionHandler::handle("User not found.");
            return;
        }
        if (books.find(bookId) == books.end()) {
            ExceptionHandler::handle("Book not found.");
            return;
        }
        if (loanManagement.loanBook(bookId, *users[userId], *books[bookId])) {
            Logger::log("Book borrowed successfully.");
        }
    }

    void returnBook(int userId, int bookId) {
        if (users.find(userId) == users.end()) {
            ExceptionHandler::handle("User not found.");
            return;
        }
        if (books.find(bookId) == books.end()) {
            ExceptionHandler::handle("Book not found.");
            return;
        }
        if (loanManagement.returnBook(bookId, *users[userId], *books[bookId])) {
            reservationSystem.notifyNextUser(bookId);
        }
    }

    void reserveBook(int userId, int bookId) {
        if (users.find(userId) == users.end()) {
            ExceptionHandler::handle("User not found.");
            return;
        }
        if (books.find(bookId) == books.end()) {
            ExceptionHandler::handle("Book not found.");
            return;
        }
        reservationSystem.reserveBook(bookId, userId);
    }

    void showBooks() {
        for (auto& [id, book] : books) {
            book->print();
        }
    }

    void showUsers() {
        for (auto& [id, user] : users) {
            user->print();
        }
    }
};

void enterUsers(LibraryManagementSystem& system) {
    int n;
    cout << "Enter number of users to add: ";
    cin >> n;
    for (int i = 0; i < n; i++) {
        int id;
        string uname, pwd;
        int t;
        cout << "User #" << i+1 << " ID: ";
        cin >> id;
        cout << "Username: ";
        cin >> uname;
        cout << "Password: ";
        cin >> pwd;
        cout << "User Type (0=Regular, 1=Librarian): ";
        cin >> t;
        if (t == 0)
            system.addUser(new User(id, uname, pwd, REGULAR));
        else
            system.addUser(new Librarian(id, uname, pwd));
    }
}

void enterBooks(LibraryManagementSystem& system) {
    int n;
    cout << "Enter number of books to add: ";
    cin >> n;
    for (int i = 0; i < n; i++) {
        int id, pages, typeInt;
        string title, author, category, publishDate;
        cout << "Book #" << i+1 << " ID: ";
        cin >> id;
        cout << "Title: ";
        cin.ignore();
        getline(cin, title);
        cout << "Author: ";
        getline(cin, author);
        cout << "Category: ";
        getline(cin, category);
        cout << "Publish Date: ";
        getline(cin, publishDate);
        cout << "Number of pages: ";
        cin >> pages;
        cout << "Book Type (0=TextBook,1=Magazine,2=ReferenceBook): ";
        cin >> typeInt;
        if (typeInt == 0) {
            string level, field;
            cout << "Level: ";
            cin.ignore();
            getline(cin, level);
            cout << "Field: ";
            getline(cin, field);
            system.addBook(new TextBook(id, title, author, category, publishDate, pages, level, field));
        } else if (typeInt == 1) {
            int issue;
            cout << "Issue Number: ";
            cin >> issue;
            system.addBook(new Magazine(id, title, author, category, publishDate, pages, issue));
        } else {
            system.addBook(new ReferenceBook(id, title, author, category, publishDate, pages));
        }
    }
}

int main() {
    LibraryManagementSystem system;
    enterUsers(system);
    enterBooks(system);

    int userId;
    string password;
    cout << "Login\nUser ID: ";
    cin >> userId;
    cout << "Password: ";
    cin >> password;

    User* loggedInUser = system.authenticateUser(userId, password);
    if (!loggedInUser) {
        cout << "Authentication failed.\n";
        return 0;
    }
    cout << "Welcome, " << loggedInUser->getUsername() << "!\n";

    bool exit = false;
    while (!exit) {
        cout << "\nMenu:\n1. Show all books\n2. Borrow book\n3. Return book\n4. Reserve book\n5. Show all users\n0. Exit\nChoice: ";
        int choice; cin >> choice;
        switch (choice) {
            case 1:
                system.showBooks();
                break;
            case 2: {
                int bid;
                cout << "Enter Book ID to borrow: ";
                cin >> bid;
                system.borrowBook(loggedInUser->getUserId(), bid);
                break;
            }
            case 3: {
                int bid;
                cout << "Enter Book ID to return: ";
                cin >> bid;
                system.returnBook(loggedInUser->getUserId(), bid);
                break;
            }
            case 4: {
                int bid;
                cout << "Enter Book ID to reserve: ";
                cin >> bid;
                system.reserveBook(loggedInUser->getUserId(), bid);
                break;
            }
            case 5:
                system.showUsers();
                break;
            case 0:
                exit = true;
                break;
            default:
                cout << "Invalid choice.\n";
        }
    }
    cout << "Goodbye!\n";
    return 0;
}
