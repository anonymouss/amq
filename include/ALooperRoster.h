#ifndef __A_LOOPER_ROSTER_H__
#define __A_LOOPER_ROSTER_H__

#include "ABase.h"
#include "ALooper.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace diordna {

struct ALooperRoster {
    ALooperRoster();

    ALooper::handler_id registerHandler(const std::shared_ptr<ALooper> &looper,
                                        const std::shared_ptr<AHandler> &handler);
    void unregisterHandler(ALooper::handler_id handlerId);
    void unregisterStaleHandlers();

    void dump(int fd, const std::vector<std::string> &args);

private:
    struct HandlerInfo {
        std::weak_ptr<ALooper> mLooper;
        std::weak_ptr<AHandler> mHandler;
    };

    std::mutex mLock;
    KeyedVector<ALooper::handler_id, HandlerInfo> mHandlers;
    ALooper::handler_id mNextHandlerId;

    DECLARE_NON_COPYASSIGNABLE(ALooperRoster);
};

}  // namespace diordna

#endif  // __A_LOOPER_ROSTER_H__