#define TAG "AHandler"

#include <AHandler.h>
#include <AMessage.h>

namespace diordna {

void AHandler::deliverMessage(const std::shared_ptr<AMessage> &msg) {
    onMessageReceived(msg);
    ++mMessageCounter;
    if (mVerboseStats) {
        auto what = msg->what();
        mMessages[what]++;
    }
}

}  // namespace diordna