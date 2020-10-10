#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <mutex>
#include <vector>
#include <condition_variable>

// This queue only holds one message. Before the message is fetched, senders will be blocked and wait if "block" is true
// Empty string is not a valid message. fetch can return empty string only when block is false and there is no message 
// in thw queue.
class OneMessageQueue {
public:
    bool send(const std::string& msg, bool block=true);
    std::string fetch(bool block=true);
    void flush();

private:
    std::mutex mutex;
    std::condition_variable cv;
    std::string msg;
};

namespace utilities 
{
    std::vector<std::string> splitStringByNewline(const std::string& s);
    
    namespace test
    {
        void MsgQueueTest();
    }
}

#endif // #ifndef UTILITIES_H

