#ifndef PLAY_H
#define PLAY_H

#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include "utilities.h"
#include "game.h"

class Agent {
public:
    enum Role {
        Moderator = 0,
        Player1,
        Player2,
        Undefined
    };
    static const std::string Roles[3];
    static Role   roleByName(const std::string& roleName);

    // Invariant: roleByName(Roles[role]) == role, for any role of enum Role except for Role::Undefined
    
    static Agent* getAgent(Role role);
    static void   setAgent(Role role, Agent* agent);

    Agent(const std::string& name_) :
        msgQueue(),
        dutyThread(nullptr),
        name(name_)
    {}

    bool sendMsg(const std::string& msg, bool block =true);

    // startDuty() & endDuty() must be called from the "main" thread
    bool startDuty();
    std::thread* endDuty();

    virtual Role getRole() const = 0;
    const std::string& getName() const { return name; }

protected:
    OneMessageQueue& getMsgQueue(); 
    virtual void onDuty() = 0;

private:
    OneMessageQueue msgQueue;
    std::thread* dutyThread;
    std::string name;
    static Agent* agents[3]; // Corresponding to 3 Roles.
};

// Player

class Player : public Agent {
public:
    Player(const std::string& name) : 
        Agent(name),
        role(Role::Undefined),
        logger(nullptr)
    {}

    Role getRole() const;
    // r must be either Role::Player1 or Role::Player2
    void setRole(Role r);

    void setLogger(std::vector<std::string>* logger);

private:
    void onDuty();
    game::Play findPlayBasedOnHistory(const std::vector<GameRecord>& ghist) const;
    Role role;
    std::vector<std::string>* logger;
};

class Moderator : public Agent {
public:
    Moderator(const std::string& name) :
        Agent(name),
        gameHistory(),
        gameStatus(0),
        gameDoneMutex(),
        gameDoneCv()
    {}

    Agent::Role getRole() const;

    bool startGameSetAsync(int count);
    std::vector<GameRecord> getAsyncResult();

private:
    void onDuty();

    std::vector<GameRecord> gameHistory;
    int gameStatus; // 0: ready, 1: game in-progress, 2: game completed, waiting for the client to get the result.
    std::mutex gameDoneMutex;
    std::condition_variable gameDoneCv;
};

namespace play
{
    void main(int gameSetCount);

    namespace test
    {
        void test();
    }
}

#endif
