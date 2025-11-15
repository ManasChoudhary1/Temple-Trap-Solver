#pragma GCC optimize("O3", "unroll-loops")
#include <memory_resource>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <vector>
#include <queue>
#include <unordered_map>
#include <memory>
#include <functional>
#include <cmath>
#include <cstring>
#define ll long long
#define vec(type) vector<type>
#define pb push_back
#define all(v) v.begin(), v.end()
#define fastio() ios::sync_with_stdio(false); cin.tie(nullptr);

using namespace std;
using namespace chrono;

struct GameState;

class Tile {
protected:
    vector<int> _top_open;
    vector<int> _ground_open;
    bool _hole;
    int _stairs;
    int _orient;

    static vector<int> calculate_openings(const vector<int> &openings, int orient) {
        vector<int> rotated_openings;
        rotated_openings.reserve(openings.size());
        for (auto opening : openings) {
            if (opening == -1) {
                rotated_openings.pb(-1);
            } else {
                int rotated_opening = (opening + orient) % 4;
                rotated_openings.pb(rotated_opening);
            }
        }
        return rotated_openings;
    }

public:
    Tile(const vector<int> & base_top_opens, const vector<int> & base_ground_opens, bool hole, int base_stairs, int orient) {
        _top_open = calculate_openings(base_top_opens, orient);
        _ground_open = calculate_openings(base_ground_opens, orient);
        _hole = hole;
        _stairs = calculate_openings({base_stairs}, orient)[0];
        _orient = orient;
    }
    virtual ~Tile() = default;

    const vector<int>& getTopOpenings() const { return _top_open; }
    const vector<int>& getGroundOpenings() const { return _ground_open; }
    bool hasHole() const { return _hole; }
    bool hasStairs() const { return (_stairs != -1); }
    // creating a unique id for each tile
    int getTileId() const {
        int id = 0;
        if(hasStairs()) id += 1000;
        if(_hole) id += 100;
        for(int o : _top_open) {
            if (o >= 0 && o < 4) id |= (1 << o);
        }
        for(int o : _ground_open) {
            if (o >= 0 && o < 4) id |= (1 << (o + 4));
        }
        return id;
    }

    virtual char getTileType() const = 0;
};

// TILE DEFINITIONS (A-H) as given in the pdf (not the pictures)
class TileA : public Tile {
public: TileA(int orient) : Tile({0,1},{},false,-1,orient){} char getTileType() const override { return 'A'; } };
class TileB : public Tile {
public: TileB(int orient) : Tile({0, 1},{},false,-1,orient){} char getTileType() const override { return 'B'; } };
class TileC : public Tile {
public: TileC(int orient) : Tile({0, 2},{},false,-1,orient){} char getTileType() const override { return 'C'; } };
class TileD : public Tile {
public: TileD(int orient) : Tile({3},{1},true,3,orient){} char getTileType() const override { return 'D'; } };
class TileE : public Tile {
public: TileE(int orient) : Tile({3},{1},true,3,orient){} char getTileType() const override { return 'E'; } };
class TileF : public Tile {
public: TileF(int orient) : Tile({},{0, 1},true,-1,orient){} char getTileType() const override { return 'F'; } };
class TileG : public Tile {
public: TileG(int orient) : Tile({},{1, 2},true,-1,orient){} char getTileType() const override { return 'G'; } };
class TileH : public Tile {
public: TileH(int orient) : Tile({},{2, 3},true,-1,orient){} char getTileType() const override { return 'H'; } };
class TileExit : public Tile {
public: TileExit() : Tile({1}, {}, true, -1, 0) {} char getTileType() const override { return 'X'; } };

using PawnPosition = pair<pair<int, int>, int>;

// Array-based board: 3x3 grid, nullptr means empty
using BoardConfig = array<array<Tile*, 3>, 3>;

// Compact board state via encoding
struct CompactState {
    uint64_t board_hash;  // Compact board representation
    int pawn_r, pawn_c, pawn_floor;
    int blank_r, blank_c;

    CompactState() = default;

    CompactState(const GameState& state);

    bool operator==(const CompactState& other) const {
        return board_hash == other.board_hash &&
               pawn_r == other.pawn_r && pawn_c == other.pawn_c && pawn_floor == other.pawn_floor &&
               blank_r == other.blank_r && blank_c == other.blank_c;
    }
};

struct CompactStateHash {
    size_t operator()(const CompactState& state) const {
        size_t h = state.board_hash;
        h ^= (state.pawn_r * 73856093) ^ (state.pawn_c * 19349663) ^ (state.pawn_floor * 83492791);
        h ^= (state.blank_r * 50331653) ^ (state.blank_c * 25165843);
        return h;
    }
};

struct GameState {
    BoardConfig board;
    PawnPosition pawn_pos;
    pair<int, int> blank_pos;
    int cost_so_far;
    int heuristic_cost;

    // Caching the compact representation
    mutable CompactState compact_cache;
    mutable bool compact_cached = false;

    const CompactState& getCompact() const {
        if (!compact_cached) {
            compact_cache = CompactState(*this);
            compact_cached = true;
        }
        return compact_cache;
    }

    bool operator==(const GameState& other) const {
        return getCompact() == other.getCompact();
    }
};

//  CompactState constructor
CompactState::CompactState(const GameState& state) {
    pawn_r = state.pawn_pos.first.first;
    pawn_c = state.pawn_pos.first.second;
    pawn_floor = state.pawn_pos.second;
    blank_r = state.blank_pos.first;
    blank_c = state.blank_pos.second;

    // hashing using position and tile info
    board_hash = 0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            if (state.board[r][c] != nullptr) {
                int tile_val = state.board[r][c]->getTileType() * 10000 + state.board[r][c]->getTileId();
                int pos_idx = r * 3 + c;
                board_hash ^= ((uint64_t)tile_val << (pos_idx * 6));
            }
        }
    }
}

struct GameStateHash {
    size_t operator()(const GameState& state) const {
        CompactStateHash h;
        return h(state.getCompact());
    }
};

struct CompareGameState {
    bool operator()(const GameState& a, const GameState& b) const {
        if (a.heuristic_cost != b.heuristic_cost)
            return a.heuristic_cost > b.heuristic_cost;
        // Tie-break by cost_so_far
        return a.cost_so_far < b.cost_so_far;
    }
};

/// Heuristic Function
int calculate_heuristic(const GameState& current_state) {
    // return 0; // uncomment to compare performance with and without heuristic
    const pair<int,int>& coords = current_state.pawn_pos.first;
    const pair<int,int>& blank_tile_pos = current_state.blank_pos;
    int heuristic  = 0;
    // standard manhattan
    heuristic += coords.first  + coords.second +1;

    // If blank tile is on 0,0 then extra cost of atleast one will occur
    heuristic += (blank_tile_pos.first==0 && blank_tile_pos.second==0) ;
    // We check if the tile on 0,0 has left top open or not, in case it is not open,
    // extra cost of atleast manhattan_distance of empty tile to 0,0 + 1 will occur;
    if (current_state.blank_pos!=make_pair(0,0) ){
        const vector<int> top_openings_00 = current_state.board[0][0]->getTopOpenings();
        bool exit_is_open = false;
        for (int val : top_openings_00) {
            if (val == 3) {
                exit_is_open = true;
                break;
            }
        }
        if (exit_is_open==false) {
            heuristic += blank_tile_pos.first + blank_tile_pos.second + 1;
        }
    }
    return 1*heuristic;
}

inline bool is_valid_board_pos(int r, int c) {
    return r >= 0 && r < 3 && c >= 0 && c < 3;
}

inline bool is_exit_pos(int r, int c) {
    return r == 0 && c == -1;
}

bool is_valid_and_connected(int r1, int c1, int r2, int c2, int floor, const BoardConfig& board) {
    // Check if coordinates are in valid range (including exit position)
    bool pos1_valid = is_valid_board_pos(r1, c1);
    bool pos2_valid = is_valid_board_pos(r2, c2) || is_exit_pos(r2, c2);

    if (!pos1_valid || !pos2_valid) return false;
    // Check if tiles exist at both positions
    Tile* tile1 = nullptr;
    Tile* tile2 = nullptr;

    if (is_valid_board_pos(r1, c1)) {
        tile1 = board[r1][c1];
    }

    if (is_valid_board_pos(r2, c2)) {
        tile2 = board[r2][c2];
    } else if (is_exit_pos(r2, c2)) {
        // tile2 will be set by caller for exit tile
        return false; // Exit tile handled separately
    }

    if (tile1 == nullptr || tile2 == nullptr) return false;

    const auto& openings1 = (floor == 0) ? tile1->getGroundOpenings() : tile1->getTopOpenings();
    const auto& openings2 = (floor == 0) ? tile2->getGroundOpenings() : tile2->getTopOpenings();

    if (r1 == r2) {
        if (c2 > c1) {
            // East: tile1 needs 1 (E), tile2 needs 3 (W)
            bool has1 = false, has2 = false;
            for (int o : openings1) if (o == 1) { has1 = true; break; }
            for (int o : openings2) if (o == 3) { has2 = true; break; }
            return has1 && has2;
        } else {
            // West: tile1 needs 3 (W), tile2 needs 1 (E)
            bool has1 = false, has2 = false;
            for (int o : openings1) if (o == 3) { has1 = true; break; }
            for (int o : openings2) if (o == 1) { has2 = true; break; }
            return has1 && has2;
        }
    } else if (c1 == c2) {
        if (r2 > r1) {
            // South: tile1 needs 2 (S), tile2 needs 0 (N)
            bool has1 = false, has2 = false;
            for (int o : openings1) if (o == 2) { has1 = true; break; }
            for (int o : openings2) if (o == 0) { has2 = true; break; }
            return has1 && has2;
        } else {
            // North: tile1 needs 0 (N), tile2 needs 2 (S)
            bool has1 = false, has2 = false;
            for ( int o : openings1) if (o == 0) { has1 = true; break; }
            for (int o : openings2) if (o == 2) { has2 = true; break; }
            return has1 && has2;
        }
    }
    return false;
}

/// check if tile is connected to exit
bool is_connected_to_exit(int r1, int c1, int floor, const BoardConfig& board, Tile* exit_tile) {
    if (!is_valid_board_pos(r1, c1)) return false;

    Tile* tile1 = board[r1][c1];
    if (tile1 == nullptr) return false;

    const auto& openings1 = (floor == 0) ? tile1->getGroundOpenings() : tile1->getTopOpenings();
    const auto& openings2 = (floor == 0) ? exit_tile->getGroundOpenings() : exit_tile->getTopOpenings();

    // Exit is at (0, -1), so we're moving West from (0, 0)
    // tile1 needs 3, tile2 needs 1
    bool has1 = false, has2 = false;
    for (int o : openings1) if (o == 3) { has1 = true; break; }
    for (int o : openings2) if (o == 1) { has2 = true; break; }
    return has1 && has2;
}
/// hashing the pawn position
struct PawnPositionHash {
    size_t operator()(const PawnPosition& pos) const {
        return (pos.first.first * 73856093) ^ (pos.first.second * 19349663) ^ (pos.second * 83492791);
    }
};

static TileExit global_exit_tile;
/// returns states after valid pawn moves
vector<GameState> find_pawn_moves(const GameState& current_state) {
    vector<GameState> possible_new_states;
    possible_new_states.reserve(20);

    const auto& board = current_state.board;
    const auto& start_pos = current_state.pawn_pos;

    Tile* exit_tile = &global_exit_tile;
    const pair<int,int> exit_coords = {0, -1};

    static PawnPosition bfs_queue[1000];
    static int bfs_cost[1000];
    int head = 0, tail = 0;

    unordered_map<PawnPosition, int, PawnPositionHash> min_walk_costs;
    min_walk_costs.reserve(50);

    bfs_queue[tail] = start_pos;
    bfs_cost[tail] = 0;
    tail++;
    min_walk_costs[start_pos] = 0;

    static const int dr[] = {-1, 0, 1, 0};
    static const int dc[] = {0, 1, 0, -1};

    while (head < tail) {
        PawnPosition current_pos = bfs_queue[head];
        int current_walk_cost = bfs_cost[head];
        head++;

        auto current_coords = current_pos.first;
        auto current_floor = current_pos.second;
        int cr = current_coords.first;
        int cc = current_coords.second;

        if (min_walk_costs[current_pos] < current_walk_cost) continue;

        for (int i = 0; i < 4; ++i) {
            int nr = cr + dr[i];
            int nc = cc + dc[i];

            if (is_valid_board_pos(nr, nc)) {
                PawnPosition neighbor_pos = {{nr, nc}, current_floor};
                int next_walk_cost = current_walk_cost + 1;

                if (is_valid_and_connected(cr, cc, nr, nc, current_floor, board)) {
                    auto it = min_walk_costs.find(neighbor_pos);
                    if (it == min_walk_costs.end() || next_walk_cost < it->second) {
                        min_walk_costs[neighbor_pos] = next_walk_cost;
                        if (tail < 1000) {
                            bfs_queue[tail] = neighbor_pos;
                            bfs_cost[tail] = next_walk_cost;
                            tail++;
                        }
                    }
                }
            }
        }

        // Explore Stairs
        if (is_valid_board_pos(cr, cc) && board[cr][cc] != nullptr && board[cr][cc]->hasStairs()) {
            PawnPosition other_floor_pos = {{cr, cc}, 1 - current_floor};
            int next_walk_cost = current_walk_cost;
            auto it = min_walk_costs.find(other_floor_pos);
            if (it == min_walk_costs.end() || next_walk_cost < it->second) {
                min_walk_costs[other_floor_pos] = next_walk_cost;
                if (tail < 1000) {
                    bfs_queue[tail] = other_floor_pos;
                    bfs_cost[tail] = next_walk_cost;
                    tail++;
                }
            }
        }

        // Special Exit Check
        if (cr == 0 && cc == 0 && current_floor == 1) {
            PawnPosition exit_pawn_pos = {exit_coords, 1};
            int next_walk_cost = current_walk_cost + 1;

            if (is_connected_to_exit(cr, cc, 1, board, exit_tile)) {
                auto it = min_walk_costs.find(exit_pawn_pos);
                if (it == min_walk_costs.end() || next_walk_cost < it->second) {
                    min_walk_costs[exit_pawn_pos] = next_walk_cost;
                }
            }
        }
    }

    for (const auto& [end_pos, walk_cost] : min_walk_costs) {
        auto end_coords = end_pos.first;
        auto end_floor = end_pos.second;
        int er = end_coords.first;
        int ec = end_coords.second;

        bool is_resting_spot = (walk_cost > 0 && end_floor == 0 && !is_exit_pos(er, ec) &&
                                is_valid_board_pos(er, ec) && board[er][ec] != nullptr && board[er][ec]->hasHole());
        bool is_exit_spot = is_exit_pos(er, ec);

        if (is_resting_spot || is_exit_spot) {
            GameState new_state = current_state;
            new_state.pawn_pos = end_pos;
            new_state.cost_so_far = current_state.cost_so_far + walk_cost;
            int h = calculate_heuristic(new_state);
            new_state.heuristic_cost = new_state.cost_so_far + h;
            new_state.compact_cached = false;
            possible_new_states.push_back(move(new_state));
        }
    }
    return possible_new_states;
}


/// returns states after valid tile slides
vector<GameState> find_tile_slides(const GameState& current_state) {
    vector<GameState> possible_new_states;
    possible_new_states.reserve(4);

    const auto& blank_pos = current_state.blank_pos;
    const auto& pawn_pos = current_state.pawn_pos;

    int br = blank_pos.first;
    int bc = blank_pos.second;

    static const int dr[] = {-1, 0, 1, 0};
    static const int dc[] = {0, 1, 0, -1};

    for (int i = 0; i < 4; ++i) {
        int nr = br + dr[i];
        int nc = bc + dc[i];

        if (!is_valid_board_pos(nr, nc)) continue;
        if (nr == pawn_pos.first.first && nc == pawn_pos.first.second) continue;

        GameState new_state = current_state;
        new_state.board[br][bc] = new_state.board[nr][nc];
        new_state.board[nr][nc] = nullptr;
        new_state.blank_pos = {nr, nc};
        new_state.cost_so_far = current_state.cost_so_far + 1;
        int h = calculate_heuristic(new_state);
        new_state.heuristic_cost = new_state.cost_so_far + h;
        new_state.compact_cached = false;
        possible_new_states.push_back(move(new_state));
    }
    return possible_new_states;
}

bool is_goal_state(const GameState& current_state) {
    return current_state.pawn_pos.first == make_pair(0, -1);
}

void print_board(const GameState& state) {
    if (state.pawn_pos.first == make_pair(0, -1)) {
        cout << "  Cost: " << state.cost_so_far << endl;
        cout << "  PAWN HAS EXITED!" << endl;
        return;
    }
    cout << "  Cost: " << state.cost_so_far << endl;
    for (int r = 0; r < 3; ++r) {
        cout << "  ";
        for (int c = 0; c < 3; ++c) {
            if (r == state.pawn_pos.first.first && c == state.pawn_pos.first.second) {
                char tile_type = '?';
                if (state.board[r][c] != nullptr) tile_type = state.board[r][c]->getTileType();
                cout << "[P" << state.pawn_pos.second << tile_type << "]";
            } else if (r == state.blank_pos.first && c == state.blank_pos.second) {
                cout << "[   ]";
            } else {
                if (state.board[r][c] != nullptr) {
                    char tile_type = state.board[r][c]->getTileType();
                    cout << "[ " << tile_type << " ]";
                } else {
                    cout << "[???]";
                }
            }
        }
        cout << endl;
    }
}

void print_path(GameState end_state, unordered_map<CompactState, GameState, CompactStateHash>& parent_map, const GameState& initial_state) {
    vector<GameState> path;
    GameState current = end_state;

    while (!(current == initial_state)) {
        path.push_back(current);
        auto it = parent_map.find(current.getCompact());
        if (it == parent_map.end()) {
            if (!(current == initial_state)) cerr << "Error: Could not reconstruct path fully." << endl;
            break;
        }
        current = it->second;
    }
    path.push_back(initial_state);
    reverse(all(path));

    cout << "--- SOLUTION PATH ---" << endl;
    cout << "Total Cost: " << end_state.cost_so_far << " (in " << path.size() - 1 << " total steps)" << endl;
    for (int i = 0; i < path.size(); ++i) {
        auto& state = path[i];
        cout << "\n--- Step " << i << " ---" << endl;
        print_board(state);
    }
}

void solve() {
    GameState initial_state;
    vector<unique_ptr<Tile>> tile_storage;
    tile_storage.reserve(10);

    // Initialize board
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            initial_state.board[r][c] = nullptr;
        }
    }

    string s;
    cin >> s;
    if (s.size() != 20) {
        cerr << "Invalid input length. Expected 20 characters." << endl;
        return;
    }

    auto make_tile = [&](char type, int orient) -> Tile* {
        switch (type) {
            case 'A': tile_storage.push_back(make_unique<TileA>(orient)); break;
            case 'B': tile_storage.push_back(make_unique<TileB>(orient)); break;
            case 'C': tile_storage.push_back(make_unique<TileC>(orient)); break;
            case 'D': tile_storage.push_back(make_unique<TileD>(orient)); break;
            case 'E': tile_storage.push_back(make_unique<TileE>(orient)); break;
            case 'F': tile_storage.push_back(make_unique<TileF>(orient)); break;
            case 'G': tile_storage.push_back(make_unique<TileG>(orient)); break;
            case 'H': tile_storage.push_back(make_unique<TileH>(orient)); break;
            default: return nullptr;
        }
        return tile_storage.back().get();
    };

    int idx = 0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            if (idx >= 18) break;
            char type = s[idx];
            char orient_ch = s[idx + 1];
            idx += 2;

            if (type == '-' && orient_ch == '-') {
                initial_state.blank_pos = {r, c};
                continue;
            }
            int orient = orient_ch - '0';
            if (orient < 0 || orient > 3) {
                cerr << "Invalid orientation for tile " << type << " at (" << r << "," << c << ")" << endl;
                return;
            }

            Tile* tile_ptr = make_tile(type, orient);
            if (tile_ptr) {
                initial_state.board[r][c] = tile_ptr;
            }
        }
    }

    int pawn_r = s[18] - '0';
    int pawn_c = s[19] - '0';
    initial_state.pawn_pos = {{pawn_r, pawn_c}, 0};
    initial_state.cost_so_far = 0;
    int h = calculate_heuristic(initial_state);
    initial_state.heuristic_cost = initial_state.cost_so_far + h;
    priority_queue<GameState, vector<GameState>, CompareGameState> pq;
    unordered_map<CompactState, int, CompactStateHash> min_cost;
    unordered_map<CompactState, GameState, CompactStateHash> parent_map;
    /// Most states I got while testing were nearly 6lakhs so reveserving 600000
    min_cost.reserve(600000);
    parent_map.reserve(600000);

    pq.push(initial_state);
    min_cost[initial_state.getCompact()] = 0;

    GameState solution_state;
    bool solution_found = false;

    int states_explored = 0;

    while (!pq.empty()) {
        GameState current_state = pq.top();
        pq.pop();

        states_explored++;

        auto compact = current_state.getCompact();
        if (min_cost.count(compact) && current_state.cost_so_far > min_cost[compact]) {
            continue;
        }

        if (is_goal_state(current_state)) {
            solution_state = current_state;
            solution_found = true;
            break;
        }

        vector<GameState> pawn_moves = find_pawn_moves(current_state);
        for (auto& next_state : pawn_moves) {
            auto next_compact = next_state.getCompact();
            auto it = min_cost.find(next_compact);
            if (it == min_cost.end() || next_state.cost_so_far < it->second) {
                min_cost[next_compact] = next_state.cost_so_far;
                parent_map[next_compact] = current_state;
                pq.push(move(next_state));
            }
        }

        vector<GameState> tile_slides = find_tile_slides(current_state);
        for (auto& next_state : tile_slides) {
            auto next_compact = next_state.getCompact();
            auto it = min_cost.find(next_compact);
            if (it == min_cost.end() || next_state.cost_so_far < it->second) {
                min_cost[next_compact] = next_state.cost_so_far;
                parent_map[next_compact] = current_state;
                pq.push(move(next_state));
            }
        }
    }

    if (solution_found) {
        print_path(solution_state, parent_map, initial_state);
        cerr << "States explored: " << states_explored << endl;
    } else {
        cout << "No solution found." << endl;
        cout << "\nInitial State:" << endl;
        print_board(initial_state);
    }
}

int main() {
    fastio();
    auto start = high_resolution_clock::now();
    freopen("input.txt", "r", stdin);
    freopen("output.txt", "w", stdout);
    freopen("error.txt", "w", stderr);
    int t=1;
    cin >> t; // comment to test one test case at a time
    while (t--) {
        solve();
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    cerr << "Time taken by the code: " << duration.count() << " microseconds" << endl;
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
    return 0;
}
