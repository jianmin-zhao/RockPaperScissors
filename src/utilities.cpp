#include "utilities.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <sstream>
#include <cassert>

bool OneMessageQueue::send(const std::string& msg, bool block) {
    if (msg.empty()) {
        // empty string is not a valid message
        return false;
    }
    std::unique_lock<std::mutex> lock(mutex);
    if (!block) {
        bool ret = this->msg.empty();
        if (ret) {
            this->msg = msg;
            cv.notify_all();
        }
        return ret;
    }
    while (!this->msg.empty()) {
        cv.wait(lock);
    }
    this->msg = msg;
    cv.notify_all();
    return true;
}

std::string OneMessageQueue::fetch(bool block) {
    std::unique_lock<std::mutex> lock(mutex);
    if (!block) {
        std::string ret;
        if (!this->msg.empty()) {
            ret = std::move(this->msg);
            this->msg.clear();
            cv.notify_all();
        }
        return ret;
    }
    while (this->msg.empty()) {
        cv.wait(lock);
    }
    std::string ret = std::move(this->msg);
    this->msg.clear();
    cv.notify_all();
    return ret;
}

void OneMessageQueue::flush() {
    std::lock_guard<std::mutex> lg(mutex);
    this->msg.clear();
}

namespace utilities
{
    std::vector<std::string> splitStringByNewline(const std::string& s) {
        std::istringstream is(s);
        std::vector<char> buf(s.length()+1);
        std::vector<std::string> terms;
        while (is.getline(buf.data(), buf.size())) {
            terms.push_back(buf.data());
            std::fill(buf.begin(), buf.end(), ' ');
        }
        return terms;
    }

    namespace test
    {
        OneMessageQueue* queue = nullptr;
        int count = 10;
        int counter = 0;

        void sender(int th) {
            std::cout << "sender " << th << " thread id " << std::this_thread::get_id() << std::endl;
            for (int i = 0; i < count; ++i) {
                std::ostringstream oss;
                oss << i;
                queue->send(oss.str());
            }
            queue->send("end");
        }

        void fetcher() {
            std::cout << "fetch thread id " << std::this_thread::get_id() << std::endl;
            int endCount = 0;
            while (true) {
                std::string msg = queue->fetch();
                if (msg == "end") {
                    if (++endCount == 2) {
                        break;
                    }
                } else {
                    ++counter;
                }
            }
        }

        void MsgQueueTest() {

            std::cout << "main thread " << std::this_thread::get_id() << std::endl;

            queue = new OneMessageQueue();
            count = 10000;
            counter = 0;
            std::thread sender_thread(sender, 1);
            std::thread sender_thread2(sender, 2);
            std::thread fetcher_thread(fetcher);

            sender_thread.join();
            sender_thread2.join();
            fetcher_thread.join();

            std::cout << "final counter = " << counter << std::endl;
            assert(counter == count * 2);
        }
    }
}
