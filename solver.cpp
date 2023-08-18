#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <signal.h>
using namespace std;

#define MAXDIM 32
#define MAX_SCORE 12345678
#define MIN_SCORE -12345678
enum zero_level { ALL, TOP, NONE };

// Colors: Red, Green, Yellow, Blue, Purple, blacK
string arr_grid[7] = {
    "KGKGYPRB",
    "YBGKRKBK",
    "GRKGYRYB",
    "YBPBRBPK",
    "BRYKBPYP",
    "YKPRYKBG",
    "RBRGKPKY",
};
// string arr_grid[3] = {
//     "2132",
//     "4253",
//     "1324",
// };
bool START_BOTTOM_LEFT = true;          // Player 1 is in bottom left.
bool ENCODE_COLORS_AS_INTS = false;     // Grid above is encoded using ints intead of letters.
bool SHOW_ALL_OPTIONS = false;          // Show scores for all options.
zero_level INCLUDE_EMPTY_MOVES = NONE;  // Consider 0-moves at ALL levels, just the TOP level, or NONE.


enum color { X,R,G,Y,B,P,K,L };
string colorstr = "_RGYBPK";
string backgrounds[7] = { "", "41", "42", "43", "44", "45", "40" };
bool done = true;

color char_to_col(char c) {
    switch (c) {
    case 'R': return R;
    case 'G': return G;
    case 'Y': return Y;
    case 'B': return B;
    case 'P': return P;
    case 'K': return K;
    case 'r': return R;
    case 'g': return G;
    case 'y': return Y;
    case 'b': return B;
    case 'p': return P;
    case 'k': return K;
    default: return X;
    }
}

enum flag { EXACT, LOWER, UPPER, STALE };
typedef struct {
    int depth;
    flag flag;
    int score;
    color move;
} entry;

bool compare_colors(pair<color,unordered_set<int>> p1, pair<color,unordered_set<int>> p2) {
    return p1.second.size() > p2.second.size();
}

class Game {
    color grid[MAXDIM][MAXDIM];
    int w,h;
    int area;
    int has[2][MAXDIM*MAXDIM];
    public: int counts[2] = {1, 1};
    color currs[2];
    bool turn;
    int depth = 0;
    unordered_map<string,entry> map;

    public: Game(vector<string> grid, bool turn_) {
        turn = turn_;
        parse_grid(grid);
    }

    void parse_grid(vector<string> rows) {
        h = rows.size();
        w = rows[0].length();
        area = h*w;
        has[0][0] = (h-1) << 4;
        has[1][0] = w-1;
        for (int j = 0; j < h; j++) {
            string row = rows[j];
            assert (w == row.length());
            for (int i = 0; i < w; i++) {
                char c = row[i];
                grid[j][i] = ENCODE_COLORS_AS_INTS ? ((color) (c-'0')) : char_to_col(c);
            }
        }
        currs[0] = grid[h-1][0];
        currs[1] = grid[0][w-1];
    }

    unordered_map<color,unordered_set<int>> get_neighbors() {
        int turn_idx = turn ? 0 : 1;
        int ds[4] = {-1,16,1,-16};
        int *me_p = has[turn_idx];
        unordered_set<int> me_has(me_p, me_p+counts[turn_idx]);
        int *opp_p = has[1-turn_idx];
        unordered_set<int> opp_has(opp_p, opp_p+counts[1-turn_idx]);
        unordered_map<color,unordered_set<int>> neighs;
        for (int coord: me_has) {
            for (int d: ds) {
                int newcoord = coord+d;
                if (newcoord < 0) continue;
                if (me_has.find(newcoord) != me_has.end()) continue;
                if (opp_has.find(newcoord) != opp_has.end()) continue;
                int x = newcoord & 15;
                int y = newcoord >> 4;
                if (x >= w || y >= h) continue;
                color col = grid[y][x];
                if (col == currs[0] || col == currs[1]) continue;
                neighs[col].insert(newcoord);
            }
        }
        return neighs;
    }

    pair<int,color> do_move(color col, unordered_set<int> neighs) {
        int turn_idx = turn ? 0 : 1;
        int num = 0;
        int curr_count = counts[turn_idx];
        for (int coord: neighs) {
            has[turn_idx][curr_count+num++] = coord;
        }
        counts[turn_idx] += num;
        color prev_curr = currs[turn_idx];
        currs[turn_idx] = col;
        turn = !turn;
        depth++;
        return {num, prev_curr};
    }

    public: bool is_valid_move(color col) {
        return col > X && col < L && currs[0] != col && currs[1] != col;
    }

    public: pair<int,color> move(color col) {
        if (!is_valid_move(col)) {
            throw invalid_argument("color not allowed");
        }
        int turn_idx = turn ? 0 : 1;
        int ds[4] = {-1,16,1,-16};
        int *me_p = has[turn_idx];
        unordered_set<int> me_has(me_p, me_p+counts[turn_idx]);
        int *opp_p = has[1-turn_idx];
        unordered_set<int> opp_has(opp_p, opp_p+counts[1-turn_idx]);
        unordered_set<int> neighs;
        for (int coord: me_has) {
            for (int d: ds) {
                int newcoord = coord+d;
                if (newcoord < 0) continue;
                if (me_has.find(newcoord) != me_has.end()) continue;
                if (opp_has.find(newcoord) != opp_has.end()) continue;
                int x = newcoord & 15;
                int y = newcoord >> 4;
                if (x >= w || y >= h) continue;
                if (grid[y][x] != col) continue;
                neighs.insert(newcoord);
            }
        }
        return do_move(col, neighs);
    }
    
    public: void unmove(int num, color prev_curr) {
        turn = !turn;
        int turn_idx = turn ? 0 : 1;
        counts[turn_idx] -= num;
        currs[turn_idx] = prev_curr;
        depth--;
    }

    public: void print_grid() {
        for (int j = -1; j < h; j++) {
            if (ENCODE_COLORS_AS_INTS && j == -1 && turn) continue;
            for (int i = -1; i < w; i++) {
                if (ENCODE_COLORS_AS_INTS && i == -1) continue;
                if (j == -1 || i == -1) {
                    cout << (!ENCODE_COLORS_AS_INTS ? "\033[0m  " : !turn && i == w-1 ? " v" : "   ");
                    continue;
                }
                int coord = (j << 4) | i;
                int owner = 0;
                for (int p = 0; p < 2; p++) {
                    for (int k = 0; k < counts[p]; k++) {
                        if (has[p][k] == coord) {
                            owner = p+1;
                            break;
                        }
                    }
                    if (owner != 0) break;
                }
                color col = owner == 0 ? grid[j][i] : currs[owner-1];
                if (ENCODE_COLORS_AS_INTS) {
                    cout << col << owner;
                    cout << (i == w-1 ? ' ' : '|');
                } else {
                    string sq = turn && i == 0 && j == h-1 ? ": " : !turn && i == w-1 && j == 0 ? " :" : "  ";
                    cout << "\033[" << backgrounds[col] << 'm' << sq;
                }
            }
            cout << "\033[0m" << endl;
        }
        if (ENCODE_COLORS_AS_INTS && turn) cout << '^' << endl;
        cout << endl;
    }

    string get_state_string() {
        string state = turn ? "1|" : "2|";
        state += to_string(currs[0])+"|";
        state += to_string(currs[1])+"|";
        int count0 = counts[0];
        vector<int> has0(has[0],has[0]+count0);
        sort(has0.begin(),has0.end());
        for (int coord: has0) {
            state += to_string(coord)+",";
        }
        state += "|";
        int count1 = counts[1];
        vector<int> has1(has[1],has[1]+count1);
        sort(has1.begin(),has1.end());
        for (int coord: has1) {
            state += to_string(coord)+",";
        }
        return state;
    }

    public: bool check_over() {
        return counts[0]+counts[1] == area;
    }

    public: entry get_best_move(bool full=SHOW_ALL_OPTIONS, int alpha=MIN_SCORE, int beta=MAX_SCORE, unordered_set<string> past={}) {
        if (done) return {0, EXACT, 0, X};
        int alpha_orig = alpha;
        if (check_over()) {
            return {depth, EXACT, counts[turn ? 0 : 1]-counts[turn ? 1 : 0], X};
        }
        string state_string = get_state_string();
        if (INCLUDE_EMPTY_MOVES == ALL && past.find(state_string) != past.end()) {
            return {depth, STALE, counts[turn ? 0 : 1]-counts[turn ? 1 : 0], X};
        }
        if (!full) {
            auto it = map.find(state_string);
            if (it != map.end()) {
                entry entry = it->second;
                if (entry.depth <= depth) {
                    switch (entry.flag) {
                    case EXACT:
                        return entry;
                    case LOWER:
                        alpha = max(alpha, entry.score);
                        break;
                    case UPPER:
                        beta = min(beta, entry.score);
                        break;
                    case STALE:
                        break;
                    }
                    if (alpha >= beta) return entry;
                }
            }
        }
        int best_score = MIN_SCORE;
        color best_move = X;
        unordered_map<color,unordered_set<int>> poss = get_neighbors();
        if (poss.size() == 0 || INCLUDE_EMPTY_MOVES == ALL || (INCLUDE_EMPTY_MOVES == TOP && full)) {
            for (int c = R; c != L; c++) {
                if (c == currs[0] || c == currs[1]) continue;
                if (poss.find((color) c) != poss.end()) continue;
                unordered_set<int> empty;
                poss[(color) c] = empty;
            }
        }
        vector<pair<color,unordered_set<int>>> ordered(poss.begin(), poss.end());
        sort(ordered.begin(), ordered.end(), compare_colors);
        unordered_set<string> new_past(past);
        if (INCLUDE_EMPTY_MOVES == ALL) {
            new_past.insert(state_string);
        }
        for (pair<color,unordered_set<int>> p: ordered) {
            if (!full) alpha = max(alpha, best_score);
            color c = p.first;
            pair<int,color> num_prev_curr = do_move((color) c, p.second);
            int num = num_prev_curr.first;
            color prev_curr = num_prev_curr.second;
            entry entry = (INCLUDE_EMPTY_MOVES != ALL || num != 0)
                ? get_best_move(false, -beta, -alpha)
                : get_best_move(false, -beta, -alpha, new_past);
            unmove(num, prev_curr);
            int score = -entry.score;
            if (full && !done) {
                cout << endl;
                if (ENCODE_COLORS_AS_INTS) cout << c;
                else cout << "\033[" << backgrounds[c] << "m  \033[0m " << colorstr[c];
                cout << ": " << score << (entry.flag == EXACT ? ' ' : '?') << flush;
            }
            if (score > best_score) {
                best_score = score;
                best_move = c;
                if (!full && best_score >= beta) break;
            }
        }
        entry entry;
        entry.score = best_score;
        entry.move = best_move;
        entry.depth = depth;
        bool is_upper = best_score <= alpha_orig;
        bool is_lower = best_score >= beta;
        entry.flag = is_upper ? UPPER : is_lower ? LOWER : EXACT;
        if (!done) map[state_string] = entry;
        return entry;
    }
};

void my_handler(int s) {
    cout << endl;
    if (!done) {
        done = true;
        cout << "Calculation interrupted!" << endl;
        return;
    }
    exit(1);
}

int main(void) {
    signal (SIGINT, my_handler);
    vector<string> grid(arr_grid, arr_grid+sizeof(arr_grid)/sizeof(string));
    Game game(grid, START_BOTTOM_LEFT);
    vector<pair<int,color>> history;
    char c = 0;
    string moves_string = ENCODE_COLORS_AS_INTS ? "123456" : "RGYBPK";
    string enter_string = "Enter move (" + moves_string + "=move, Z=undo, C=recalc): ";
    while (true) {
        if (!game.check_over()) {
            if (c != 'c') game.print_grid();
            done = false;
            cout << "Calculating best move... (Ctrl-C to cancel) " << flush;
            entry entry = game.get_best_move();
            if (!done) {
                color col = entry.move;
                cout << endl << "best = ";
                if (ENCODE_COLORS_AS_INTS) cout << col;
                else cout << "\033[" << backgrounds[col] << "m  \033[0m " << colorstr[col];
                cout << ": " << entry.score << endl;
                done = true;
            }
            do {
                cout << enter_string;
                cin >> c;
            } while (c != 'c' && c != 'C' && 
                    ((c != 'z' && c != 'Z') || history.size() == 0) && 
                    !game.is_valid_move(ENCODE_COLORS_AS_INTS ? (color) (c-'0') : char_to_col(c)));
            if (c == 'c' || c == 'C') continue;
            if ((c == 'z' || c == 'Z') && history.size() > 0) {
                pair<int,color> p = history.back();
                history.pop_back();
                game.unmove(p.first, p.second);
            } else {
                color col = ENCODE_COLORS_AS_INTS ? (color) (c-'0') : char_to_col(c);
                history.push_back(game.move(col));
            }
        } else {
            game.print_grid();
            if (game.counts[0] == game.counts[1]) {
                cout << "It's a tie: " << game.counts[0] << " to " << game.counts[1] << '!' << endl;
            } else if (game.counts[0] > game.counts[1]) {
                cout << "Player " << (START_BOTTOM_LEFT ? 1 : 2) << " wins " << game.counts[0] << " to " << game.counts[1] << '!' << endl;
            } else {
                cout << "Player " << (START_BOTTOM_LEFT ? 2 : 1) << " wins " << game.counts[1] << " to " << game.counts[0] << '!' << endl;
            }
            do {
                cout << "Z to undo: ";
                cin >> c;
            } while ((c != 'z' && c != 'Z') || history.size() == 0);
            pair<int,color> p = history.back();
            history.pop_back();
            game.unmove(p.first, p.second);
        }
    }
    game.print_grid();
}
