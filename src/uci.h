#pragma once

#include <string>
#include "board.h"
#include "search.h"

class UCI {
public:
    UCI();
    void loop();

private:
    Board board;
    Searcher searcher;

    void cmd_uci();
    void cmd_isready();
    void cmd_ucinewgame();
    void cmd_position(const std::string &args);
    void cmd_go(const std::string &args);
    void cmd_setoption(const std::string &args);

    bool parse_move_str(const std::string &mstr, Move &outMove);
};

