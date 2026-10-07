#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <thread>
#include <chrono>

using namespace std;

string RESET = "\033[0m";
string BLACK = "\033[40m";
string RED = "\033[41m";
string GREEN = "\033[42m";
string BLUE = "\033[44m";
string FG_BLK = "\033[30m";
string FG_WHT = "\033[37m";
string BOLD = "\033[1m";

struct ships {
    int player;
    int ready;
    int board[10][10];
    int target[10][10];
    int shipId[10][10];
};

ships me, opp;

int myId = 0;
int oppId = 0;

void waitMs(int ms) {
    this_thread::sleep_for(chrono::milliseconds(ms));
}

void clearScreen() {
    cout << "\033[2J\033[H";
}

int colToNum(char c) {
    if (c >= 'A' && c <= 'J') return c - 'A';
    if (c >= 'a' && c <= 'j') return c - 'a';
    return -1;
}

void initShips(ships& s, int p) {
    s.player = p;
    s.ready = 0;
    for (int r = 0; r < 10; r++) {
        for (int c = 0; c < 10; c++) {
            s.board[r][c] = 0;
            s.target[r][c] = 0;
            s.shipId[r][c] = 0;
        }
    }
}

// ---------------- drawing ----------------

string getShipChar(ships& s, int r, int c) {
    if (s.board[r][c] != 1 && s.board[r][c] != 2) return "   ";

    int myShip = s.shipId[r][c];

    bool up = (r > 0) && (s.shipId[r - 1][c] == myShip);
    bool down = (r < 9) && (s.shipId[r + 1][c] == myShip);
    bool left = (c > 0) && (s.shipId[r][c - 1] == myShip);
    bool right = (c < 9) && (s.shipId[r][c + 1] == myShip);

    if (left || right) {
        if (left && right)  return "=|=";
        if (!left && right) return "<= ";
        if (left && !right) return " =>";
        return "<=>";
    }
    if (up || down) {
        if (up && down)  return " | ";
        if (!up && down) return " ^ ";
        if (up && !down) return " v ";
        return "<=>";
    }
    return "<=>";
}

void drawTopLine() {
    cout << "   ";
    for (int c = 0; c < 10; c++) cout << "+---";
    cout << "+\n";
}

void drawColHeader() {
    cout << "   ";
    for (int c = 0; c < 10; c++) cout << "| " << char('A' + c) << " ";
    cout << "|\n";
}

void drawPlacementBoard() {
    cout << "\n  " << BOLD << "PLACEMENT BOARD" << RESET << "  (your ships)\n\n";
    drawTopLine();
    drawColHeader();
    drawTopLine();

    for (int r = 0; r < 10; r++) {
        if (r + 1 < 10) cout << " " << (r + 1) << " ";
        else            cout << (r + 1) << " ";

        for (int c = 0; c < 10; c++) {
            cout << "|";
            string g = getShipChar(me, r, c);
            if (me.board[r][c] == 1) {
                cout << GREEN << FG_BLK << g << RESET;
            }
            else if (me.board[r][c] == 2) {
                cout << RED << FG_BLK << g << RESET;
            }
            else if (me.board[r][c] == 3) {
                cout << BLUE << FG_BLK << " ~ " << RESET;
            }
            else {
                cout << BLACK << FG_WHT << " . " << RESET;
            }
        }
        cout << "|\n";
        drawTopLine();
    }
}

void drawTargetBoard() {
    cout << "\n  " << BOLD << "TARGET BOARD" << RESET << "  (your shots)\n\n";
    drawTopLine();
    drawColHeader();
    drawTopLine();

    for (int r = 0; r < 10; r++) {
        if (r + 1 < 10) cout << " " << (r + 1) << " ";
        else            cout << (r + 1) << " ";

        for (int c = 0; c < 10; c++) {
            cout << "|";
            if (me.target[r][c] == 2) {
                cout << GREEN << FG_BLK << " X " << RESET;
            }
            else if (me.target[r][c] == 1) {
                cout << RED << FG_BLK << " o " << RESET;
            }
            else {
                cout << BLUE << FG_BLK << " ~ " << RESET;
            }
        }
        cout << "|\n";
        drawTopLine();
    }
}

// ---------------- file helpers ----------------

string shipFileName(int player) {
    if (player == 1) return "ship_p1.txt";
    else             return "ship_p2.txt";
}

void writeMyShips() {
    ofstream f(shipFileName(myId).c_str(), ios::binary | ios::trunc);
    f.write((char*)&me, sizeof(ships));
    f.close();
}

void writeOppShips() {
    ofstream f(shipFileName(oppId).c_str(), ios::binary | ios::trunc);
    f.write((char*)&opp, sizeof(ships));
    f.close();
}

void readMyShips() {
    ifstream f(shipFileName(myId).c_str(), ios::binary);
    if (!f) return;
    f.read((char*)&me, sizeof(ships));
    f.close();
}

void readOppShips() {
    ifstream f(shipFileName(oppId).c_str(), ios::binary);
    if (!f) return;
    f.read((char*)&opp, sizeof(ships));
    f.close();
}

int readTurn() {
    ifstream f("B.txt", ios::binary);
    if (!f) return 0;
    int turn = 0, seq = 0;
    f.read((char*)&turn, sizeof(int));
    f.read((char*)&seq, sizeof(int));
    return turn;
}

int readSeq() {
    ifstream f("B.txt", ios::binary);
    if (!f) return 0;
    int turn = 0, seq = 0;
    f.read((char*)&turn, sizeof(int));
    f.read((char*)&seq, sizeof(int));
    return seq;
}

void writeTurn(int turn, int seq) {
    ofstream f("B.txt", ios::binary | ios::trunc);
    f.write((char*)&turn, sizeof(int));
    f.write((char*)&seq, sizeof(int));
}

// ---------------- placement ----------------

int shipSizes[5] = { 2, 3, 3, 4, 5 };

bool fitsOnBoard(int r, int c, int size, bool horiz) {
    if (horiz) {
        if (c + size > 10) return false;
        for (int i = 0; i < size; i++)
            if (me.board[r][c + i] != 0) return false;
    }
    else {
        if (r + size > 10) return false;
        for (int i = 0; i < size; i++)
            if (me.board[r + i][c] != 0) return false;
    }
    return true;
}

void putShip(int r, int c, int size, bool horiz, int shipNum) {
    for (int i = 0; i < size; i++) {
        if (horiz) {
            me.board[r][c + i] = 1;
            me.shipId[r][c + i] = shipNum;
        }
        else {
            me.board[r + i][c] = 1;
            me.shipId[r + i][c] = shipNum;
        }
    }
}

void placeShips() {
    for (int s = 0; s < 5; s++) {
        int size = shipSizes[s];
        bool done = false;

        while (!done) {
            clearScreen();
            cout << "=== PLACE YOUR SHIPS ===  (" << (s + 1) << "/5)\n\n";
            drawPlacementBoard();
            cout << "\nPlacing ship of size " << size
                << "  (H)orizontal or (V)ertical\n";
            cout << "Enter: <col><row> <H|V>   e.g.  A5 H\n> ";

            string line;
            getline(cin, line);
            if (line == "") continue;

            int start = 0;
            while (start < (int)line.size() && line[start] == ' ') start++;
            int end = (int)line.size() - 1;
            while (end >= 0 && (line[end] == ' ' || line[end] == '\t')) end--;
            if (end < start) continue;
            line = line.substr(start, end - start + 1);

            if (line.size() < 2) {
                cout << "Need at least a column letter and row number.\n";
                waitMs(900);
                continue;
            }

            int col = colToNum(line[0]);
            if (col < 0) {
                cout << "Column must be A-J.\n";
                waitMs(900);
                continue;
            }

            int i = 1;
            while (i < (int)line.size() && line[i] >= '0' && line[i] <= '9') i++;
            if (i == 1) {
                cout << "Need a row number after the column letter.\n";
                waitMs(900);
                continue;
            }
            int row = atoi(line.substr(1, i - 1).c_str()) - 1;
            if (row < 0 || row > 9) {
                cout << "Row must be 1-10.\n";
                waitMs(900);
                continue;
            }

            bool horiz = false, gotOri = false;
            for (int k = i; k < (int)line.size(); k++) {
                char ch = line[k];
                if (ch == ' ' || ch == '\t') continue;
                if (ch == 'H' || ch == 'h') { horiz = true;  gotOri = true; break; }
                if (ch == 'V' || ch == 'v') { horiz = false; gotOri = true; break; }
                break;
            }
            if (!gotOri) {
                cout << "Need H or V for orientation.  Example: A5 H\n";
                waitMs(900);
                continue;
            }

            if (!fitsOnBoard(row, col, size, horiz)) {
                cout << "Illegal placement - overlaps or off-board.\n";
                waitMs(1000);
                continue;
            }
            putShip(row, col, size, horiz, s + 1);
            done = true;
        }
    }
    clearScreen();
    cout << "All ships placed.\n";
    drawPlacementBoard();
    waitMs(1500);
}

// ---------------- win check ----------------

bool allSunk(int grid[10][10]) {
    for (int r = 0; r < 10; r++)
        for (int c = 0; c < 10; c++)
            if (grid[r][c] == 1) return false;
    return true;
}

// ---------------- turn sync ----------------

void waitForMyTurn(int lastSeq) {
    while (true) {
        ifstream f("B.txt", ios::binary);
        if (f) {
            int turn = 0, seq = 0;
            f.read((char*)&turn, sizeof(int));
            f.read((char*)&seq, sizeof(int));
            f.close();

            if (turn == -1) return;
            if (turn == myId && seq != lastSeq) return;
        }
        waitMs(300);
    }
}

// ---------------- shooting ----------------

void doShoot(int row, int col) {
    if (opp.board[row][col] == 1) {
        opp.board[row][col] = 2;
        me.target[row][col] = 2;
        cout << "Hit!\n";
    }
    else if (opp.board[row][col] == 0) {
        opp.board[row][col] = 3;
        me.target[row][col] = 1;
        cout << "Miss.\n";
    }
    else {
        cout << "Already shot there.\n";
    }
    waitMs(900);
}

// ---------------- menu ----------------

char showMenu() {
    clearScreen();
    cout << "=== BATTLESHIP ===  Player " << me.player << "\n\n";
    cout << "  [P] Show placement board\n";
    cout << "  [T] Show target board\n";
    cout << "  [S] Shoot\n";
    cout << "  [Q] Quit\n\n> ";

    string s;
    cin >> s;
    if (s == "") return '?';
    char c = s[0];
    if (c >= 'a' && c <= 'z') c -= 32;
    return c;
}

// ---------------- main ----------------

int main() {
    ifstream check("A.dat", ios::binary);
    char flag = 0;
    if (check) check.read(&flag, 1);
    check.close();

    if (flag == '1') {
        myId = 2;
    }
    else {
        myId = 1;
        ofstream out("A.dat", ios::binary | ios::trunc);
        out.put('1');
        out.close();
    }
    oppId = (myId == 1) ? 2 : 1;

    cout << "You are Player " << myId << ".\n";
    waitMs(700);

    initShips(me, myId);
    initShips(opp, oppId);

    if (myId == 1) {
        ifstream bcheck("B.txt", ios::binary | ios::ate);
        bool needInit = true;
        if (bcheck) {
            if (bcheck.tellg() >= (streamsize)(2 * sizeof(int))) needInit = false;
        }
        bcheck.close();
        if (needInit) writeTurn(1, 1);

        me.ready = 0;
        writeMyShips();

        ships empty;
        initShips(empty, 2);
        ofstream f("ship_p2.txt", ios::binary | ios::trunc);
        f.write((char*)&empty, sizeof(ships));
        f.close();
    }
    else {
        int waited = 0;
        while (waited < 60000) {
            ifstream t("B.txt", ios::binary);
            ifstream s("ship_p1.txt", ios::binary);
            bool ok = (t.is_open() && s.is_open());
            t.close(); s.close();
            if (ok) break;
            waitMs(300);
            waited += 300;
        }
        readOppShips();
    }

    cin.ignore(10000, '\n');
    placeShips();

    me.ready = 1;
    writeMyShips();

    bool gameOver = false;
    int  lastSeq = 0;

    while (!gameOver) {
        char choice = showMenu();

        if (choice == 'P') {
            // Refresh own board first so any incoming hits are visible.
            readMyShips();
            clearScreen();
            drawPlacementBoard();
            cout << "\nPress enter to continue...";
            cin.ignore(10000, '\n');
            cin.get();
            continue;
        }
        if (choice == 'T') {
            readMyShips();
            clearScreen();
            drawTargetBoard();
            cout << "\nPress enter to continue...";
            cin.ignore(10000, '\n');
            cin.get();
            continue;
        }
        if (choice == 'Q') {
            int seq = readSeq() + 1;
            writeTurn(-1, seq);
            cout << "Quit.\n";
            return 0;
        }
        if (choice != 'S') continue;

        // Wait until opponent has placed ships.
        while (true) {
            readOppShips();
            if (opp.ready == 1) break;
            cout << "\rWaiting for opponent to place ships...   ";
            cout.flush();
            waitMs(400);
        }
        cout << "\r                                          \r";

        cout << "\nWaiting for opponent to take their turn...\n";
        waitForMyTurn(lastSeq);
        lastSeq = readSeq();

        // Read BOTH files fresh: own board (may have new hits from opponent)
        // and opponent board (to shoot at).
        readMyShips();
        readOppShips();

        clearScreen();
        drawTargetBoard();
        cout << "\nEnter target: <col><row>  e.g.  D7\n> ";

        string coord;
        cin >> coord;
        if (coord.size() < 2) {
            cout << "Bad coord.\n";
            waitMs(800);
            continue;
        }
        int col = colToNum(coord[0]);
        int row = atoi(coord.substr(1).c_str()) - 1;
        if (col < 0 || row < 0 || row > 9) {
            cout << "Bad coord.\n";
            waitMs(800);
            continue;
        }
        if (me.target[row][col] != 0) {
            cout << "You already shot there.\n";
            waitMs(800);
            continue;
        }

        // Record the shot on BOTH sides.
        doShoot(row, col);

        // Write the OPPONENT's file first (contains the new hit/miss on
        // their board). Then our own file (contains our updated target).
        writeOppShips();
        writeMyShips();

        int currentSeq = readSeq();
        writeTurn(oppId, currentSeq + 1);

        if (allSunk(opp.board)) {
            cout << "\n*** YOU WIN! ***\n";
            int cs = readSeq();
            writeTurn(-1, cs + 1);
            gameOver = true;
        }
    }

    return 0;
}