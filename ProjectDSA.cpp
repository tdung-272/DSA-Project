#include <iostream>
#include <queue>
#include <ctime>
#include <string>
#include <iomanip>
#include <thread>
#include <chrono>
#include <set>
#include <vector>

using namespace std;

struct Event {
    string name;
    string description;
    time_t startTime;
    time_t endTime;
    int priority;

    Event() = default;
    Event(string n, string d, time_t s, time_t e, int p)
        : name(n), description(d), startTime(s), endTime(e), priority(p) {}

    void print() const {
        cout << "Event: " << name << "\nDescription: " << description
             << "\nStart: " << ctime(&startTime)
             << "End: " << ctime(&endTime)
             << "Priority: " << priority << "\n-------------------------\n";
    }
};

struct AVLNode {
    Event event;
    AVLNode* left;
    AVLNode* right;
    int height;

    AVLNode(Event e) : event(e), left(nullptr), right(nullptr), height(1) {}
};

int getHeight(AVLNode* node) {
    return node ? node->height : 0;
}

int getBalance(AVLNode* node) {
    return node ? getHeight(node->left) - getHeight(node->right) : 0;
}

AVLNode* rightRotate(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* T2 = x->right;

    x->right = y;
    y->left = T2;
    y->height = 1 + max(getHeight(y->left), getHeight(y->right));
    x->height = 1 + max(getHeight(x->left), getHeight(x->right));

    return x;
}

AVLNode* leftRotate(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;

    y->left = x;
    x->right = T2;
    x->height = 1 + max(getHeight(x->left), getHeight(x->right));
    y->height = 1 + max(getHeight(y->left), getHeight(y->right));

    return y;
}

AVLNode* insert(AVLNode* node, Event e) {
    if (!node) return new AVLNode(e);

    if (e.startTime < node->event.startTime)
        node->left = insert(node->left, e);
    else if (e.startTime > node->event.startTime)
        node->right = insert(node->right, e);
    else return node;

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));
    int balance = getBalance(node);

    if (balance > 1 && e.startTime < node->left->event.startTime)
        return rightRotate(node);
    if (balance < -1 && e.startTime > node->right->event.startTime)
        return leftRotate(node);
    if (balance > 1 && e.startTime > node->left->event.startTime) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    if (balance < -1 && e.startTime < node->right->event.startTime) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

AVLNode* minValueNode(AVLNode* node) {
    AVLNode* current = node;
    while (current->left) current = current->left;
    return current;
}

AVLNode* deleteNode(AVLNode* root, time_t startTime) {
    if (!root) return root;

    if (startTime < root->event.startTime)
        root->left = deleteNode(root->left, startTime);
    else if (startTime > root->event.startTime)
        root->right = deleteNode(root->right, startTime);
    else {
        if (!root->left || !root->right) {
            AVLNode* temp = root->left ? root->left : root->right;
            delete root;
            return temp;
        }
        AVLNode* temp = minValueNode(root->right);
        root->event = temp->event;
        root->right = deleteNode(root->right, temp->event.startTime);
    }

    root->height = 1 + max(getHeight(root->left), getHeight(root->right));
    int balance = getBalance(root);

    if (balance > 1 && getBalance(root->left) >= 0)
        return rightRotate(root);
    if (balance > 1 && getBalance(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }
    if (balance < -1 && getBalance(root->right) <= 0)
        return leftRotate(root);
    if (balance < -1 && getBalance(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }

    return root;
}

void inOrder(AVLNode* root) {
    if (!root) return;
    inOrder(root->left);
    root->event.print();
    inOrder(root->right);
}

void findConflicts(AVLNode* root, Event e, vector<Event>& conflicts) {
    if (!root) return;
    if (!(e.endTime <= root->event.startTime || e.startTime >= root->event.endTime)) {
        conflicts.push_back(root->event);
    }
    if (e.startTime < root->event.startTime)
        findConflicts(root->left, e, conflicts);
    else
        findConflicts(root->right, e, conflicts);
}

AVLNode* findEvent(AVLNode* root, time_t startTime) {
    if (!root) return nullptr;
    if (startTime == root->event.startTime) return root;
    if (startTime < root->event.startTime)
        return findEvent(root->left, startTime);
    return findEvent(root->right, startTime);
}

struct Compare {
    bool operator()(const Event& a, const Event& b) {
        return a.priority < b.priority;
    }
};

priority_queue<Event, vector<Event>, Compare> reminderQueue;

time_t inputTime(const string& label) {
    int d, m, y, h, min;
    cout << "Enter " << label << " time (dd mm yyyy hh mm): ";
    cin >> d >> m >> y >> h >> min;
    tm t = {};
    t.tm_mday = d;
    t.tm_mon = m - 1;
    t.tm_year = y - 1900;
    t.tm_hour = h;
    t.tm_min = min;
    return mktime(&t);
}

void notifyEvents() {
    time_t now = time(0);
    priority_queue<Event, vector<Event>, Compare> temp;
    bool hasReminder = false;

    while (!reminderQueue.empty()) {
        Event e = reminderQueue.top(); reminderQueue.pop();

        double diff = difftime(e.startTime, now);
        if (diff <= 300 && diff > 0) {
            cout << "\nReminder: Event '" << e.name << "' is starting at " << ctime(&e.startTime);
            hasReminder = true;
        }

        temp.push(e);
    }
    swap(reminderQueue, temp);

    if (!hasReminder) {
        cout << "No upcoming events.\n";
    }
}

AVLNode* editEvent(AVLNode* root) {
    time_t t = inputTime("start time of event to edit");
    AVLNode* node = findEvent(root, t);
    if (!node) {
        cout << "Event not found.\n";
        return root;
    }
    cout << "Editing event:\n";
    node->event.print();

    priority_queue<Event, vector<Event>, Compare> tempQueue;
    while (!reminderQueue.empty()) {
        Event e = reminderQueue.top(); reminderQueue.pop();
        if (e.startTime != node->event.startTime)
            tempQueue.push(e);
    }
    swap(reminderQueue, tempQueue);

    root = deleteNode(root, t);

    string name, desc;
    int prio;
    cin.ignore();
    cout << "New event name: "; getline(cin, name);
    cout << "New description: "; getline(cin, desc);
    time_t start = inputTime("new start");
    time_t end = inputTime("new end");
    cout << "New priority (1-10): "; cin >> prio;

    Event newEvent(name, desc, start, end, prio);
    root = insert(root, newEvent);
    reminderQueue.push(newEvent);
    cout << "Event updated.\n";
    return root;
}

int main() {
    AVLNode* root = nullptr;

    while (true) {
        cout << "\n===== STUDENT ACTIVITY SCHEDULE MANAGER =====\n";
        cout << "1. Add event\n2. Show schedule\n3. Check reminders\n4. Delete event\n5. Edit event\n6. Exit\nChoose: ";
        int choice; cin >> choice;

        if (choice == 1) {
            string name, desc;
            int prio;
            cin.ignore();
            cout << "Event name: "; getline(cin, name);
            cout << "Description: "; getline(cin, desc);
            time_t start = inputTime("start");

            if (start < time(0)) {
                cout << "The event time is in the past.\n";
                continue;
            }

            time_t end = inputTime("end");
            cout << "Priority (1-10): "; cin >> prio;

            Event newEvent(name, desc, start, end, prio);
            vector<Event> conflicts;
            findConflicts(root, newEvent, conflicts);

            if (!conflicts.empty()) {
                cout << "Conflict with existing events:\n";
                for (auto& e : conflicts) e.print();

                bool canReplace = true;
                for (auto& e : conflicts) {
                    if (e.priority >= newEvent.priority)
                        canReplace = false;
                }

                if (canReplace) {
                    for (auto& e : conflicts) {
                        root = deleteNode(root, e.startTime);
                        priority_queue<Event, vector<Event>, Compare> tempQueue;
                        while (!reminderQueue.empty()) {
                            Event ev = reminderQueue.top(); reminderQueue.pop();
                            if (ev.startTime != e.startTime)
                                tempQueue.push(ev);
                        }
                        swap(reminderQueue, tempQueue);
                    }

                    root = insert(root, newEvent);
                    reminderQueue.push(newEvent);
                    cout << "Conflicting events removed. New event added.\n";
                } else {
                    cout << "Event not added (lower priority).\n";
                }
            } else {
                root = insert(root, newEvent);
                reminderQueue.push(newEvent);
                cout << "Event added.\n";
            }

        } else if (choice == 2) {
            cout << "\nEvent Schedule:\n";
            if (!root) {
                cout << "No events scheduled.\n";
            } else {
                inOrder(root);
            }

        } else if (choice == 3) {
            notifyEvents();

        } else if (choice == 4) {
            time_t t = inputTime("start time of event to delete");
            AVLNode* target = findEvent(root, t);
            if (!target) {
                cout << "Event not found.\n";
            } else {
                priority_queue<Event, vector<Event>, Compare> tempQueue;
                while (!reminderQueue.empty()) {
                    Event e = reminderQueue.top(); reminderQueue.pop();
                    if (e.startTime != t)
                        tempQueue.push(e);
                }
                swap(reminderQueue, tempQueue);
                root = deleteNode(root, t);
                cout << "Event deleted.\n";
            }

        } else if (choice == 5) {
            root = editEvent(root);

        } else if (choice == 6) {
            break;
        }

        this_thread::sleep_for(chrono::seconds(1));
    }

    return 0;
}
