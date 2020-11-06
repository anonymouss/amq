#define TAG "B"

#include "B.h"
#include "A.h"

#include <AMessage.h>

namespace diordna {

void B::onMessageReceived(const std::shared_ptr<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatMessage_1:
            LOG("handling message 1");
            onMessage_1();
            break;
        case kWhatMessage_2:
            LOG("handling message 2");
            onMessage_2();
            break;
        default:
            LOG("ungrecognized message : %d", msg->what());;
            break;
    }
}

void B::handleMessage_1() {
    std::shared_ptr<AMessage> msg = std::make_shared<AMessage>(kWhatMessage_1, shared_from_this());
    msg->post();
}

void B::handleMessage_2() {
    std::shared_ptr<AMessage> msg = std::make_shared<AMessage>(kWhatMessage_2, shared_from_this());
    msg->post();
}

void B::onMessage_1() {
    LOG("message 1 <success>");
}

void B::onMessage_2() {
    LOG("message 2 <success>");
}

} // namespace diordna