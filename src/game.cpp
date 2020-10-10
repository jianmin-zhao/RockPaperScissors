#include "game.h"
#include "play.h"
#include <iostream>
#include <sstream>
#include <cassert>

// Produces a JSON string with no extra while space, to make deserialization, fromJson(), easier.
std::string GameRecord::toJson() const {
    std::ostringstream oss;
    oss << "{" << "\"Round\":" << Round << ",";
    if (Winner == "null") {
        oss << "\"Winner\":null,";
    } else {
        oss << "\"Winner\":\"" << Winner << "\",";
    }
    oss << "\"Inputs\":{\"Player1\":\"" << game::Plays[Inputs.Player1] << "\",\"Player2\":\""
    << game::Plays[Inputs.Player2] << "\"}}";
    return oss.str();
}

// Pretty formated Json to be printed. Cannot be passed to GameRecord::fromJson().
std::string GameRecord::printJson(const std::string& indent) const {
    std::ostringstream oss;
    oss << indent + "{\n";
    oss << indent + "\t\"Round\": " << Round << ",\n";
    if (Winner == "null") {
        oss << indent + "\t\"Winner\": null,\n";
    } else {
        oss << indent + "\t\"Winner\": \"" << Winner << "\",\n";
    }
    oss << indent + "\t\"Inputs\": {\n";
    oss << indent + "\t\t\"Player1\": \"" << game::Plays[Inputs.Player1] << "\",\n";
    oss << indent + "\t\t\"Player2\": \"" << game::Plays[Inputs.Player2] << "\"\n";
    oss << indent + "\t}\n";
    oss << indent + "}";
    return oss.str();
}

bool GameRecord::operator==(const GameRecord& rhs) const {
    if (Round == rhs.Round && Winner == rhs.Winner &&
        Inputs.Player1 == rhs.Inputs.Player1 && Inputs.Player2 == rhs.Inputs.Player2) {
        return true;
    } else {
        return false;
    }
}

void GameRecord::reset() {
    Round = 0;
    Winner = "unknown";
    Inputs.Player1 = game::Play::Invalid;
    Inputs.Player2 = game::Play::Invalid;
}

// This is not a general JSON parser; valid json input may not be parsed properly.
// Consider this as deserialization method with toJson being the serialization method.
GameRecord* GameRecord::fromJson(const std::string& str) {
    size_t len = str.length();
    if (len < 2 || str[0] != '{' || str[len-1] != '}') {
        return nullptr;
    }

    std::string cont = str.substr(1, len - 2);
    size_t pos = cont.find_first_of(',');
    if (pos == std::string::npos) {
        return nullptr;
    }
    std::string terms[3];
    // Three terms: Round, Winner, and Inputs. Only Inputs is a compound, containing '{'.
    // We assumed the value of "Winner" may not contain braces.
    terms[0] = cont.substr(0, pos);
    if (terms[0].find_first_of('{') != std::string::npos) {
        size_t pos2 = cont.find_last_of(',');
        if (pos2 == std::string::npos) {
            return nullptr;
        }
        terms[0] = cont.substr(pos2 + 1, cont.length() - pos2 - 1);
        cont = cont.substr(0, pos2);
    } else {
        cont = cont.substr(pos + 1, cont.length() - pos - 1);
    }

    pos = cont.find_first_of(',');
    if (pos == std::string::npos) {
        return nullptr;
    }
    terms[1] = cont.substr(0, pos);
    if (terms[1].find_first_of('{') != std::string::npos) {
        size_t pos2 = cont.find_last_of(',');
        if (pos2 == std::string::npos) {
            return nullptr;
        }
        terms[1] = cont.substr(pos2 + 1, cont.length() - pos2 - 1);
        cont = cont.substr(0, pos2);
    } else {
        cont = cont.substr(pos + 1, cont.length() - pos - 1);
    }
    terms[2] = cont;

    GameRecord* gr = new GameRecord();
    bool succ = true;
    for (int i = 0; i < 3; ++i) {
        std::string& term = terms[i];
        size_t pos = term.find_first_of(':', 0);
        if (pos == std::string::npos) {
            succ = false;
            break;
        }
        std::string key = term.substr(0, pos);
        if (key.length() < 2 || key[0] != '"' || key[key.length()-1] != '"') {
            succ = false;
            break;
        }
        key = key.substr(1, key.length() - 2);
        std::string val = term.substr(pos + 1, term.length() - pos - 1);
        if (key == "Round") {
            if (val.length() == 0) {
                succ = false;
                break;
            }
            std::istringstream iss(val);
            int num;
            iss >> num;
            // checking num
            gr->Round = num;
        } else if (key == "Winner") {
            if (val == "null") {
                gr->Winner = "null";
            } else {
                if (val.length() < 3 || val[0] != '"' || val[val.length()-1] != '"') {
                    succ = false;
                    break;
                }
                gr->Winner = val.substr(1, val.length() - 2);
            }
        } else if (key == "Inputs") {
            if (val.length() < 2 || val[0] != '{' || val[val.length()-1] != '}') {
                succ = false;
                break;
            }
            std::string val_in = val.substr(1, val.length() - 2);
            size_t pos = val_in.find_first_of(',');
            if (pos == std::string::npos) {
                succ = false;
                break;
            }
            std::string ps[2];
            ps[0] = val_in.substr(0, pos);
            ps[1] = val_in.substr(pos + 1, val_in.length() - pos - 1);
            int p_i;
            for (p_i = 0; p_i < 2; ++p_i) {
                std::string& p = ps[p_i];
                size_t pos2 = p.find_first_of(':');
                if (pos2 == std::string::npos) {
                    break;
                }
                std::string player = p.substr(0, pos2);
                std::string play = p.substr(pos2 + 1, p.length() - pos2 - 1);
                if (player.length() < 3 || player[0] != '"' || player[player.length()-1] != '"') {
                    break;
                }
                if (play.length() < 3 || play[0] != '"' || play[play.length()-1] != '"') {
                    break;
                }
                player = player.substr(1, player.length() - 2);
                play = play.substr(1, play.length() - 2);
                game::Play ePlay = game::play(play);
                if (ePlay == game::Play::Invalid) {
                    break;
                }
                if (player == "Player1") {
                    gr->Inputs.Player1 = ePlay;
                } else if (player == "Player2") {
                    gr->Inputs.Player2 = ePlay;
                } else {
                    break;
                }
            }
            if (p_i < 2) {
                succ = false;
                break;
            }
        } else {
            succ = false;
            break;
        }
    }
    if (succ) {
        return gr;
    } else {
        delete gr;
        return nullptr;
    }
}

namespace game
{
    std::string Plays[4] = { "Rock", "Scissors", "Paper", "Invalid" };

    Play play(const std::string& playName)
    {
        if (playName == Plays[Play::Rock]) {
            return Play::Rock;
        } else if (playName == Plays[Play::Scissors]) {
            return Play::Scissors;
        } else if (playName == Plays[Play::Paper]) {
            return Play::Paper;
        } else {
            return Play::Invalid;
        }
    }

    std::string toJson(const std::vector<GameRecord>& gHistory) {
        std::ostringstream oss;
        oss << "[";
        bool first = true;
        for (auto iter = gHistory.begin(); iter != gHistory.end(); ++iter) {
            if (first) {
                first = false;
            } else {
                oss << ",";
            }
            oss << iter->toJson();
        }
        oss << "]";
        return oss.str();
    }

    std::string printJson(const std::vector<GameRecord>& gHistory) {
        std::ostringstream oss;
        oss << "[\n";
        bool first = true;
        for (auto iter = gHistory.begin(); iter != gHistory.end(); ++iter) {
            if (first) {
                first = false;
            } else {
                oss << ",\n";
            }
            oss << iter->printJson("\t");
        }
        oss << "\n]";
        return oss.str();
    }

    std::vector<GameRecord> fromJson(const std::string& json) {
        std::vector<GameRecord> ret;

        if (json.length() < 4 || json[0] != '[' || json[json.length()-1] != ']') {
            // not a json of array. At least [{}]
            return ret;
        }
        std::string str = json.substr(1, json.length() - 2);
        size_t pos1 = 0;
        if (str[pos1] != '{') {
            return ret;
        }

        size_t pos2;
        bool good = true;
        while (true) {
            // assertion: str[pos1] == '{'
            int count = 1;
            pos2 = pos1;
            std::string xx = str.substr(pos1, str.length() - pos1);
            for (int i = pos1 + 1; i < str.length(); ++i) {
                switch (str[i]) {
                    case '{':
                        ++count;
                        break;
                    case '}':
                        if (count == 1) {
                            pos2 = i;
                        } else {
                            --count;
                        }
                        break;
                    default:
                        break;
                }
                if (pos2 > pos1) {
                    break; // of for-loop
                }
            }
            if (pos2 > pos1) {
                std::string rec = str.substr(pos1, pos2 - pos1 + 1);
                GameRecord* gr = GameRecord::fromJson(rec);
                if (gr == nullptr) {
                    good = false;
                    break;
                } else {
                    ret.emplace_back(*gr);
                    delete gr;
                }
                if (pos2 < str.length() - 3) {
                    pos1 = pos2 + 1;
                    if (str[pos1] != ',') {
                        good = false;
                        break;
                    }
                    ++pos1;
                    if (str[pos1] != '{') {
                        good = false;
                        break;
                    }
                } else {
                    break;
                }
            } else {
                good = false;
                break;
            }
        }
        if (good) {
            return ret;
        } else {
            return std::vector<GameRecord>();
        }
    }

    void printSummary(const std::vector<GameRecord>& ghist, std::ostream& outs) {
        outs << "-------------------------\n";
        outs << "Number of plays: " << ghist.size() << "\n";
        int a = 0;
        int b = 0;
        int tie = 0;
        for (auto iter = ghist.begin(); iter != ghist.end(); ++iter) {
            if (iter->Winner == Agent::Roles[Agent::Role::Player1]) {
                ++a;
            } else if (iter->Winner == Agent::Roles[Agent::Role::Player2]) {
                ++b;
            } else {
                ++tie;
            }
        }
        outs << "Player 1, " << Agent::getAgent(Agent::Role::Player1)->getName() << ", won " << a << " plays.\n";
        outs << "Player 2, " << Agent::getAgent(Agent::Role::Player2)->getName() << ", won " << b << " plays.\n";
        outs << "Tied " << tie << " plays." << std::endl;
        outs << "+++++++++++++++++++++++++\n";
    }

    namespace test
    {
        void test() {
            std::cout << Plays[Play::Rock] << " - " << Plays[Play::Scissors]
            << " - " << Plays[Play::Paper] << std::endl;

            GameRecord gr;
            gr.Round = 1;
            gr.Winner = "Player2";
            gr.Inputs.Player1 = Play::Rock;
            gr.Inputs.Player2 = Play::Paper;

            std::string json = gr.toJson();
            std::cout << json << std::endl;
            GameRecord* g = GameRecord::fromJson(json);
            std::cout << g->toJson() << std::endl;
            assert(gr == *g);
            delete g;

            std::cout << gr.printJson("\t") << std::endl;

            std::vector<GameRecord> grs;
            grs.emplace_back(gr);
            GameRecord gr2;
            gr2.Round = 2;
            gr2.Winner = "null";
            gr2.Inputs.Player1 = Play::Scissors;
            gr2.Inputs.Player2 = Play::Scissors;
            grs.emplace_back(gr2);
            std::string json2 = game::toJson(grs);
            std::cout << json2 << std::endl;
            std::vector<GameRecord> grs2 = game::fromJson(json2);
            std::cout << game::toJson(grs2) << std::endl;
            
            std::cout << game::printJson(grs) << std::endl;
        }
    }
}