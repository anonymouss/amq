#define TAG "main"

#include "A.h"

#include <ABase.h>

#include <thread>
#include <chrono>

using namespace std::literals::chrono_literals;

int main() {
    diordna::A a;

    LOG(" -- START -- ");

    a.sendMessage(1);
    a.sendMessage(2);

    std::this_thread::sleep_for(2s);
    LOG(" --  END  -- ");
}