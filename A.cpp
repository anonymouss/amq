#define TAG "A"

#include "A.h"
#include "B.h"

#include <ALooper.h>

namespace diordna {

A::A() : mLooper(std::make_shared<ALooper>()), mB(std::make_shared<B>()) {
    mLooper->setName("A Looper");
    mLooper->start(false /* running on calling thread */);
    mLooper->registerHandler(mB);
}

void A::sendMessage(int id) {
    LOG("sending message %d", (id % 2));;
    if (id % 2) {
        mB->handleMessage_1();
    } else {
        mB->handleMessage_2();
    }
}

} // namespace diordna