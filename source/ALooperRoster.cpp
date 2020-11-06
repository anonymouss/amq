#define TAG "ALooperRoster"

#include <AHandler.h>
#include <ALooperRoster.h>
#include <AMessage.h>

#include <string>
#include <vector>
#include <cassert>

namespace diordna {

static bool verboseStats = false;

ALooperRoster::ALooperRoster() : mNextHandlerId(1) {}

ALooper::handler_id ALooperRoster::registerHandler(const std::shared_ptr<ALooper> &looper,
                                                   const std::shared_ptr<AHandler> &handler) {
    std::lock_guard<std::mutex> _lock(mLock);

    if (handler->id() != 0) {
        assert(!"A handler must only be registered once.");
        return INVALID_OPERATION;
    }

    HandlerInfo info{looper, handler};
    auto handlerId = mNextHandlerId++;
    mHandlers[handlerId] = info;

    handler->setID(handlerId, looper);
    return handlerId;
}

void ALooperRoster::unregisterHandler(ALooper::handler_id handlerId) {
    std::lock_guard<std::mutex> _lock(mLock);

    auto it = mHandlers.find(handlerId);
    if (it == mHandlers.end()) { return; }

    const HandlerInfo &info = mHandlers[handlerId];
    std::shared_ptr<AHandler> handler = info.mHandler.lock();

    if (handler != nullptr) { handler->setID(0, std::weak_ptr<ALooper>()); }

    mHandlers.erase(it);
}

void ALooperRoster::unregisterStaleHandlers() {
    std::vector<std::shared_ptr<ALooper>> activeLoopers;
    {
        std::lock_guard<std::mutex> _lock(mLock);

        for (auto it = mHandlers.begin(); it != mHandlers.end(); ++it) {
            const auto &info = it->second;
            std::shared_ptr<ALooper> looper = info.mLooper.lock();
            if (looper == nullptr) {
                LOG("Unregistering stale handler %d", it->first);
                it = mHandlers.erase(it);
                std::advance(it, 1);
            } else {
                // At this point 'looper' might be the only sp<> keeping the object alive. To
                // prevent it from going out of scope and having ~ALooper call this method again
                // recursively and then deadlocking because of the Autolock above, add it to a
                // Vector which will go out of scope after the lock has been released.
                activeLoopers.push_back(looper);
            }
        }
    }
}

void ALooperRoster::dump(int fd, const std::vector<std::string> &args) {
    // TODO: pass
}

}  // namespace diordna