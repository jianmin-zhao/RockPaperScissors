#include "play.h"
#include "utilities.h"
#include "game.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <unistd.h>

// class Agent starts:

const std::string Agent::Roles[3] = { "Moderator", "Player1", "Player2" };
Agent* Agent::agents[3] = { nullptr, nullptr, nullptr };

Agent::Role Agent::roleByName(const std::string& roleName) {
    if (roleName == Agent::Roles[Agent::Role::Moderator]) {
        return Agent::Role::Moderator;
    } else if (roleName == Agent::Roles[Agent::Role::Player1]) {
        return Agent::Role::Moderator;
    } else if (roleName == Agent::Roles[Agent::Role::Player2]) {
        return Agent::Role::Moderator;
    } else {
        return Agent::Role::Undefined;
    }
}

Agent* Agent::getAgent(Role role) {
    if (role == Role::Undefined) {
        return nullptr;
    } else {
        return agents[role];
    }
}

void Agent::setAgent(Role role, Agent* agent) {
    if (role != Role::Undefined && role == agent->getRole()) {
        agents[role] = agent;
    } else {
        throw std::invalid_argument("Agent::setAgent receives invalid arguments.");
    }
}

bool Agent::sendMsg(const std::string& msg, bool block) {
    return msgQueue.send(msg, block);
}

bool Agent::startDuty(){
    if (dutyThread != nullptr) {
        return false;
    }
    dutyThread = new std::thread(&Agent::onDuty, this);
}

std::thread* Agent::endDuty() {
    if (dutyThread == nullptr) {
        msgQueue.flush();
        return nullptr;
    }
    std::string msg = Agent::Roles[getRole()] + "\nEND";
    msgQueue.send(msg);
    std::thread* ret = dutyThread;
    dutyThread = nullptr;
    return ret;
}

OneMessageQueue& Agent::getMsgQueue() {
    return msgQueue;
}

// class Agent ends.

// class Player starts:

Agent::Role Player::getRole() const {
    return role;
}

void Player::setRole(Role r) {
    if (r == Role::Player1 || r == Role::Player2) {
        role = r;
    } else {
        throw std::invalid_argument("Player::setRole() receives invalid argument.");
    }
}

void Player::setLogger(std::vector<std::string>* logger) {
    this->logger = logger;
}

void Player::onDuty() {
    while (true) {
        // may block
        std::string msg = getMsgQueue().fetch();

        std::vector<std::string> terms = utilities::splitStringByNewline(msg);
        if (terms.size() < 2) {
            // invalid format. Must be: sender\n<msg header>
            continue;
        }
        
        if (terms[0] == Agent::Roles[getRole()] && terms[1] == "END") {
            // break out the forever loop
            break;
        }

        Role agentRole = Agent::roleByName(terms[0]);
        Agent* agent = Agent::getAgent(agentRole);
        if (agent == nullptr) {
            // disregard if not valid sender
            continue;
        }

        if (agentRole == Role::Moderator) {
            if (terms[1] == "show-hand") {
                // terms[2] is expected to be json of the game history
                if (terms.size() < 3) {
                    continue;
                }
                std::vector<GameRecord> ghist = game::fromJson(terms[2]);
                game::Play play = findPlayBasedOnHistory(ghist);
                std::string response = Agent::Roles[role] + "\nresponse\n" + game::Plays[play];
                if (logger != nullptr) {
                    logger->push_back(game::Plays[play]);
                }
                agent->sendMsg(response);
            }
        }
    }
}
    
game::Play Player::findPlayBasedOnHistory(const std::vector<GameRecord>& ghist) const {
    if (getRole() == Agent::Role::Player1) {
        return (game::Play) (rand() % 3);
    } else {
        if (ghist.size() == 0) {
            return (game::Play) (rand() % 3);
        } else {
            return (game::Play) ((ghist[ghist.size() - 1].Inputs.Player1 + 2) % 3);
        }
    }
}

// class Player ends.

// class Moderator starts:

Agent::Role Moderator::getRole() const {
    return Agent::Role::Moderator;
}

namespace
{
    struct GameState {
        int gameCount = 0;
        Agent* player1 = nullptr;
        Agent* player2 = nullptr;
        int pendingPlayer;
        GameRecord gameRecord;

        bool nextGame(const std::vector<GameRecord>& ghist) {
            if (gameCount == 0) {
                pendingPlayer = 0;
                return false;
            }
            --gameCount;
            gameRecord.reset();
            player1->sendMsg(Agent::Roles[Agent::Role::Moderator] + "\nshow-hand\n" + game::toJson(ghist));
            pendingPlayer = 1;
            return true;
        }

        void nextPlayer(const std::vector<GameRecord>& ghist) {
            player2->sendMsg(Agent::Roles[Agent::Role::Moderator] + "\nshow-hand\n" + game::toJson(ghist));
            pendingPlayer = 2;
        }
    };
} // anonymous namespace

void Moderator::onDuty() {
    GameState gstate;
    while (true) {
        // may block
        std::string msg = getMsgQueue().fetch();

        std::vector<std::string> terms = utilities::splitStringByNewline(msg);
        if (terms.size() < 2) {
            // invalid format. Must be: sender\n<msg header>
            continue;
        }
        
        if (terms[0] == Agent::Roles[getRole()]) {
            if (terms[1] == "END") {
                // break out the forever-loop
                break;
            } else if (terms[1] == "batch-games") {
                if (gstate.gameCount > 0) {
                    // We have to finish the previous batch; disregard it!
                    std::cout << "games are in progress." << std::endl;
                    continue;
                }
                if (terms.size() < 3) {
                    // protocol error; disregard
                    continue;
                }
                // terms[2] must be a number
                std::istringstream iss(terms[2]);
                int count = 0;
                iss >> count;
                if (count <= 0) {
                    std::cout << "Batch count must be greater than 0" << std::endl;
                    continue;
                }
                gstate.gameCount = count;
                gstate.player1 = Agent::getAgent(Agent::Role::Player1);
                if (gstate.player1 == nullptr) {
                    std::cout << "Player1 not present." << std::endl;
                    gstate.gameCount = 0;
                    continue;
                }
                gstate.player2 = Agent::getAgent(Agent::Role::Player2);
                if (gstate.player2 == nullptr) {
                    std::cout << "Player2 not present." << std::endl;
                    gstate.gameCount = 0;
                    continue;
                }
                gstate.nextGame(gameHistory);
            }
        } else if (terms[0] == Agent::Roles[Agent::Role::Player1]) {
            if (gstate.pendingPlayer == 1) {
                if (terms[1] == "response") {
                    if (terms.size() < 3) {
                        std::cout << "protocol error: no response in the message" << std::endl;
                        continue;
                    }
                    game::Play play = game::play(terms[2]);
                    if (play == game::Play::Invalid) {
                        gstate.gameRecord.Round = gameHistory.size() + 1;
                        gstate.gameRecord.Winner = Agent::Roles[Agent::Role::Player2];
                        gstate.gameRecord.Inputs.Player1 = game::Play::Invalid;
                        // this is made up. Player2 enjoys advantage of not having to show his/her hand.
                        gstate.gameRecord.Inputs.Player2 = game::Play::Rock;
                        gameHistory.emplace_back(gstate.gameRecord);
                        gstate.nextGame(gameHistory);
                    } else {
                        gstate.gameRecord.Inputs.Player1 = play;
                        gstate.nextPlayer(gameHistory);
                    }
                }
            }
        } else if (terms[0] == Agent::Roles[Agent::Role::Player2]) {
            if (gstate.pendingPlayer == 2) {
                if (terms[1] == "response") {
                    if (terms.size() < 3) {
                        std::cout << "protocol error: no response in the message" << std::endl;
                        continue;
                    }
                    game::Play play = game::play(terms[2]);
                    if (play == game::Play::Invalid) {
                        gstate.gameRecord.Inputs.Player2 = game::Play::Invalid;
                        gstate.gameRecord.Winner = Agent::Roles[Agent::Role::Player1];
                    } else {
                        gstate.gameRecord.Inputs.Player2 = play;
                        if (gstate.gameRecord.Inputs.Player1 == gstate.gameRecord.Inputs.Player2) {
                            gstate.gameRecord.Winner = "null";
                        } else if (gstate.gameRecord.Inputs.Player1 == (gstate.gameRecord.Inputs.Player2 + 1) % 3) {
                            gstate.gameRecord.Winner = Agent::Roles[Agent::Role::Player2];
                        } else {
                            gstate.gameRecord.Winner = Agent::Roles[Agent::Role::Player1];
                        }
                    }
                    gstate.gameRecord.Round = gameHistory.size() + 1;
                    gameHistory.emplace_back(gstate.gameRecord);
                    if (!gstate.nextGame(gameHistory)) {
                        std::unique_lock<std::mutex> lock(gameDoneMutex);
                        gameStatus = 2;
                        gameDoneCv.notify_one();
                    }
                }
            }
        } else {
            std::cout << "received msg " + terms[1] + " from sender " + terms[0] << std::endl;
        }
    }
}

bool Moderator::startGameSetAsync(int count) {
    if (gameStatus != 0) {
        return false;
    }
    if (count < 1) {
        return false;
    }
    std::ostringstream oss;
    oss << Agent::Roles[getRole()] + "\nbatch-games\n" << count;
    getMsgQueue().send(oss.str());
    gameStatus = 1;
    gameHistory.clear();
    return true;
}
    
std::vector<GameRecord> Moderator::getAsyncResult() {
    std::unique_lock<std::mutex> lock(gameDoneMutex);
    while (gameStatus != 2) {
        gameDoneCv.wait(lock);
    }
    std::vector<GameRecord> ret = std::move(gameHistory);
    gameHistory.clear();
    gameStatus = 0;
    return ret;
}

namespace play
{
    void main(int gameSetCount) {

        if (gameSetCount < 1) {
            std::cout << "At least to play one Rock-Paper-Scissors" << std::endl;
            return;
        }

        // Create the game participants.
        Player* player1 = new Player("jack");
        Player* player2 = new Player("susan");
        player1->setRole(Agent::Role::Player1);
        player2->setRole(Agent::Role::Player2);
        Moderator*  moderator = new Moderator("Bruce");

        // set up the roles.
        Agent::setAgent(Agent::Role::Player1, player1);
        Agent::setAgent(Agent::Role::Player2, player2);
        Agent::setAgent(Agent::Moderator, moderator);

        // Put them on duty, in a loop of listening to the messages.
        player1->startDuty();
        player2->startDuty();
        moderator->startDuty();

        if (moderator->startGameSetAsync(gameSetCount)) {
            std::vector<GameRecord> result = moderator->getAsyncResult();
            game::printSummary(result);
            std::string resultFile = "result.json";
            std::ofstream output(resultFile);
            output << game::printJson(result) << std::endl;
            output.close();
            char* path = get_current_dir_name();
            std::string spath(path);
            spath += "/";
            spath += resultFile;
            std::cout << "The results of all " << gameSetCount << " plays are logged in JSON file, " << spath << std::endl;
        } else {
            std::cout << "Sorry, the program encountered fatal errors." << std::endl;
        }

        // We may continue do a new game set by exchanging the roles.

        // Terminate the threads of each participants.
        std::thread* t1 = player1->endDuty();
        std::thread* t2 = player2->endDuty();
        std::thread* t3 = moderator->endDuty();
        t1->join();
        t2->join();
        t3->join();

        // free resources.
        delete player1;
        delete player2;
        delete moderator;
        player1 = nullptr;
        player2 = nullptr;
        moderator = nullptr;
    }

    namespace test
    {
        void assert(bool assertion, const std::string& msg) {
            if (!assertion) {
                throw std::logic_error(msg);
            }
        }

        void test() {
            std::cout << "--- Start testing the game set of 100 plays --- " << std::endl;

            Player* player1 = new Player("jack");
            Player* player2 = new Player("susan");
            player1->setRole(Agent::Role::Player1);
            player2->setRole(Agent::Role::Player2);
            Moderator*  moderator = new Moderator("Bruce");
            
            Agent::setAgent(Agent::Role::Player1, player1);
            Agent::setAgent(Agent::Role::Player2, player2);
            Agent::setAgent(Agent::Moderator, moderator);

            std::vector<std::string> player1log;
            std::vector<std::string> player2log;
            player1->setLogger(&player1log);
            player2->setLogger(&player2log);
            
            player1->startDuty();
            player2->startDuty();
            moderator->startDuty();

            assert(
                moderator->startGameSetAsync(100),
                "test fail: fails to start");

            std::vector<GameRecord> result = moderator->getAsyncResult();
            assert(result.size() == 100, "The result has missed entries.");
            std::string resultJson = game::printJson(result);

            assert(player1log.size() == 100, "Player1 show-hand count error");
            assert(player2log.size() == 100, "Player2 show-hand count error");
            // Verify winner by inspecting the Json, as opposed to the c++ vector from which the Json derives.
            // Limitation: only using the result as Json string in memory, 
            // and we only verify the "Winner" lines.
            int count = 0;
            std::vector<char> buf(200);
            std::istringstream iss(resultJson);
            const std::string p1 = "\"Player1\"";
            const std::string p2 = "\"Player2\"";
            while (iss.getline(buf.data(), buf.size())) {
                std::string line(buf.data());
                size_t pos = line.find("Winner");
                if (pos == std::string::npos) {
                    continue;
                }

                pos = line.find_first_of(':', pos + 7);
                assert(pos != std::string::npos, "output Json syntax error");
                size_t pos1 = line.find_first_not_of(' ', pos + 1);
                assert(pos1 != std::string::npos, "output Json syntax error");
                size_t pos2 = line.find_last_of(',');
                assert(pos2 != std::string::npos, "output Json syntax error");
                pos2 = line.find_last_not_of(' ', pos2 - 1);
                assert(pos2 != std::string::npos && pos1 < pos2, "output Json syntax error");

                std::string winner = line.substr(pos1, pos2 - pos1 + 1);
                assert(winner == p1 || winner == p2 || winner == "null", "output Json syntax error");
                game::Play play1 = game::play(player1log[count]);
                game::Play play2 = game::play(player2log[count]);
                if (play1 == play2) {
                    assert(winner == "null", "game logic error");
                } else if (play1 == game::Play::Invalid) {
                    assert(winner == p2, "game logic error");
                } else if (play2 == game::Play::Invalid) {
                    assert(winner == p1, "game logic error");
                } else if (play2 == (play1 + 1) % 3) {
                    assert(winner == p1, "game logic error");
                } else {
                    assert(winner == p2, "game logic error");
                }
                ++count;
                assert(count <= 100, "extra entry is found in the result Json.");
            }
            assert(count == 100, "the result Json missing entries");

            std::thread* t1 = player1->endDuty();
            std::thread* t2 = player2->endDuty();
            std::thread* t3 = moderator->endDuty();
            t1->join();
            t2->join();
            t3->join();

            std::cout << "The test is completed successfully!" << std::endl;
        }
    }
}