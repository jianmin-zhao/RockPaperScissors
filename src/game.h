#ifndef GAME_H
#define GAME_H

#include <string>
#include <vector>
#include <iostream>

class GameRecord;

namespace game
{
    enum Play {
        Rock = 0,
        Scissors,
        Paper,
        Invalid
    };
    extern std::string Plays[4];
    Play play(const std::string& playName);

    // Invariant: play(Plays[play]) == play, for any play of enum Play

    std::string toJson(const std::vector<GameRecord>& gHistory);
    std::string printJson(const std::vector<GameRecord>& gHistory);
    std::vector<GameRecord> fromJson(const std::string& json);

    void printSummary(const std::vector<GameRecord>& ghist, std::ostream& outs = std::cout);
}

class GameRecord {
public:
    GameRecord() :
        Round(0),
        Winner("null"),
        Inputs{game::Play::Invalid, game::Play::Invalid}
    {}
    GameRecord(const GameRecord& src) :
        Round(src.Round),
        Winner(src.Winner),
        Inputs{src.Inputs.Player1, src.Inputs.Player2}
    {}

    bool operator==(const GameRecord& rhs) const ;

    int Round;
    std::string Winner;
    struct {
        game::Play Player1;
        game::Play Player2;
    } Inputs;

    void reset();

    std::string toJson() const;
    std::string printJson(const std::string& indent) const;
    static GameRecord* fromJson(const std::string& str);
};

namespace game
{
    namespace test
    {
        void test();
    }
}

#endif
