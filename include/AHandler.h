#ifndef __A_HANDLER_H__
#define __A_HANDLER_H__

#include "ABase.h"
#include "ALooper.h"

#include <cstdint>
#include <memory>

namespace diordna {

struct AMessage;

struct AHandler : public std::enable_shared_from_this<AHandler> {
    AHandler() : mId(0), mVerboseStats(false), mMessageCounter(0) {}

    ALooper::handler_id id() const { return mId; }

    std::shared_ptr<ALooper> looper() const { return mLooper.lock(); }

    std::weak_ptr<ALooper> getLooper() const { return mLooper; }

    std::weak_ptr<AHandler> getHandler() const {
        return std::const_pointer_cast<AHandler>(shared_from_this());
    }

protected:
    virtual void onMessageReceived(const std::shared_ptr<AMessage> &msg) = 0;

private:
    friend struct AMessage;       // deliverMessage()
    friend struct ALooperRoster;  // setID()

    ALooper::handler_id mId;
    std::weak_ptr<ALooper> mLooper;

    inline void setID(ALooper::handler_id id, const std::weak_ptr<ALooper> &looper) {
        mId = id;
        mLooper = looper;
    }

    bool mVerboseStats;
    uint32_t mMessageCounter;
    KeyedVector<uint32_t, uint32_t> mMessages;

    void deliverMessage(const std::shared_ptr<AMessage> &msg);

    DECLARE_NON_COPYASSIGNABLE(AHandler);
};

}  // namespace diordna

#endif  // __A_HANDLER_H__