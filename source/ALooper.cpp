#define TAG "ALooper"

#include <AHandler.h>
#include <ALooper.h>
#include <ALooperRoster.h>
#include <AMessage.h>

#include <cassert>
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

using namespace std::chrono_literals;

namespace diordna {

ALooperRoster gLooperRoster;

struct ALooper::LooperThread {
    explicit LooperThread(ALooper *looper) : mLooper(looper), mStopped(false) {
        mThreadId = std::this_thread::get_id();
    }

    void run() {
        mThread = std::thread([this]() { this->threadLoop(this->mExitSignal.get_future()); });
        mThread.detach();
        mStopped = false;
    }

    void stop() {
        if (mStopped) { return; }
        mExitSignal.set_value();
        mStopped = true;
    }

    virtual ~LooperThread() {
        if (!mStopped) { stop(); }
    }

    bool isCurrentThread() const { return std::this_thread::get_id() == mThreadId; }

private:
    ALooper *mLooper;
    std::thread::id mThreadId;
    std::thread mThread;
    std::promise<void> mExitSignal;
    bool mStopped;

    void threadLoop(std::future<void> exitListner) {
        LOG("start %s", __func__);
        do {
            if (!mLooper->loop()) {
                LOG("some errors happen, exiting thread loop...");
                break;
            }
        } while (exitListner.wait_for(std::chrono::nanoseconds(1)) == std::future_status::timeout);
        LOG("exit %s", __func__);
    }
};

// static
int64_t ALooper::GetNowUs() {
    // FIXME: ???
    // auto now = std::chrono::system_clock::now().time_since_epoch();
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    auto nowUs = std::chrono::duration_cast<std::chrono::microseconds>(now);
    return nowUs.count();
}

ALooper::ALooper() : mRunningLocally(false) { gLooperRoster.unregisterStaleHandlers(); }

ALooper::~ALooper() { stop(); }

void ALooper::setName(const char *name) { mName = std::string{name}; }

ALooper::handler_id ALooper::registerHandler(const std::shared_ptr<AHandler> &handler) {
    return gLooperRoster.registerHandler(shared_from_this(), handler);
}

void ALooper::unregisterHandler(ALooper::handler_id handlerId) {
    gLooperRoster.unregisterHandler(handlerId);
}

status_t ALooper::start(bool runOnCallingThread) {
    if (runOnCallingThread) {
        {
            std::lock_guard<std::mutex> _lock(mLock);
            if (mThread != nullptr || mRunningLocally) { return INVALID_OPERATION; }
            mRunningLocally = true;
        }

        do {
        } while (loop());

        return OK;
    }

    std::lock_guard<std::mutex> _lock(mLock);
    if (mThread != nullptr || mRunningLocally) { return INVALID_OPERATION; }

    mThread = std::make_shared<LooperThread>(this);
    mThread->run();
    return OK;
}

status_t ALooper::stop() {
    std::shared_ptr<LooperThread> _thread;
    bool runningLocally;

    {
        std::lock_guard<std::mutex> _lock(mLock);
        _thread = mThread;
        runningLocally = mRunningLocally;
        mThread = nullptr;
        mRunningLocally = false;
    }

    if (_thread == nullptr && !runningLocally) { return INVALID_OPERATION; }

    if (_thread != nullptr) { _thread->stop(); }

    mQueueChangedCondition.notify_one();
    {
        std::lock_guard<std::mutex> _lock(mRepliesLock);
        mRepliesCondition.notify_all();
    }

    // XXX: check if _thread is nullptr? and stop twice?
    if (!runningLocally && !_thread->isCurrentThread()) { _thread->stop(); }

    return OK;
}

void ALooper::post(const std::shared_ptr<AMessage> &msg, int64_t delayUs) {
    std::lock_guard<std::mutex> _lock(mLock);

    int64_t whenUs;
    if (delayUs > 0) {
        auto nowUs = GetNowUs();
        whenUs = (delayUs > INT64_MAX - nowUs ? INT64_MAX : nowUs + delayUs);
    } else {
        whenUs = GetNowUs();
    }

    auto it = mEventQueue.begin();
    while (it != mEventQueue.end() && (*it).mWhenUs <= whenUs) { ++it; }

    Event event{whenUs, msg};
    if (it == mEventQueue.begin()) { mQueueChangedCondition.notify_one(); }

    mEventQueue.insert(it, event);
}

bool ALooper::loop() {
    Event event;
    {
        std::unique_lock<std::mutex> _lock(mLock);
        if (mThread == nullptr && !mRunningLocally) {
            LOG("expect to run in an independent thread but remote thread is null");
            return false;
        }

        if (mEventQueue.empty()) {
            mQueueChangedCondition.wait(_lock);
            return true;
        }
        auto whenUs = mEventQueue.begin()->mWhenUs;
        auto nowUs = GetNowUs();

        if (whenUs > nowUs) {
            auto delayUs = whenUs - nowUs;
            if (delayUs > INT64_MAX / 1000) { delayUs = INT64_MAX / 1000; }
            mQueueChangedCondition.wait_for(_lock, delayUs * 1us);
            return true;
        }

        event = *mEventQueue.begin();
        mEventQueue.erase(mEventQueue.begin());
    }

    event.mMessage->deliver();
    return true;
}

// to be called by AMessage::postAndAwaitResponse only
std::shared_ptr<AReplyToken> ALooper::createReplyToken() {
    return std::make_shared<AReplyToken>(shared_from_this());
}

// to be called by AMessage::postAndAwaitResponse only
status_t ALooper::awaitResponse(const std::shared_ptr<AReplyToken> &replyToken,
                                std::shared_ptr<AMessage> *response) {
    // return status in case we want to handle an interrupted wait
    std::unique_lock<std::mutex> _lock(mRepliesLock);
    assert(replyToken != nullptr);
    while (!replyToken->retrieveReply(response)) {
        {
            std::lock_guard<std::mutex> _lock_l(mLock);
            if (mThread == nullptr) { return NAME_NOT_FOUND; }
        }
        mRepliesCondition.wait(_lock);
    }
    return OK;
}

status_t ALooper::postReply(const std::shared_ptr<AReplyToken> &replyToken,
                            const std::shared_ptr<AMessage> &reply) {
    std::lock_guard<std::mutex> _lock(mRepliesLock);
    auto err = replyToken->setReply(reply);
    if (err == OK) { mRepliesCondition.notify_all(); }
    return err;
}

}  // namespace diordna