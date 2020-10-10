
#include "utilities.h"
#include "game.h"
#include "play.h"

#include <sstream>

int main(int argc, char* argv[]) {
    // utilities::test::MsgQueueTest();
    // game::test::test();

    if (argc > 1) {
        std::string test("test");
        if (test == argv[1]) {
            play::test::test();
            return 0;
        }
    }
    
    int gameSetCount = 100;
    if (argc > 1) {
        std::istringstream iss(argv[1]);
        int n;
        iss >> n;
        if (n > 0) {
            gameSetCount = n;
        }
    }
    play::main(gameSetCount);
    return 0;
}
