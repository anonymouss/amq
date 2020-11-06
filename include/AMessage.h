#ifndef __A_MESSAGE_H__
#define __A_MESSAGE_H__

#include "ABase.h"
#include "ALooper.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace diordna {

struct AHandler;

struct AReplyToken {
    explicit AReplyToken(const std::shared_ptr<ALooper> &looper)
        : mLooper(looper), mReplied(false) {}

private:
    friend struct AMessage;
    friend struct ALooper;
    std::weak_ptr<ALooper> mLooper;
    std::shared_ptr<AMessage> mReply;
    bool mReplied;

    std::shared_ptr<ALooper> getLooper() const { return mLooper.lock(); }

    bool retrieveReply(std::shared_ptr<AMessage> *reply) {
        if (mReplied) {
            *reply = mReply;
            mReply = nullptr;
        }
        return mReplied;
    }

    status_t setReply(const std::shared_ptr<AMessage> &reply);
};

struct AMessage : public std::enable_shared_from_this<AMessage> {
    AMessage();
    AMessage(uint32_t what, const std::shared_ptr<const AHandler> &handler);

    void setWhat(uint32_t what);
    uint32_t what() const;

    void setTarget(const std::shared_ptr<const AHandler> &handler);
    void clear();

    void setInt32(const char *name, int32_t value);
    void setInt64(const char *name, int64_t value);
    void setString(const char *name, const std::string &s);
    void setString(const char *name, const char *s);
    // void setMessage(const char *name, const std::shared_ptr<AMessage> &msg);
    void setObject(const char *name, const std::shared_ptr<void> &obj);  // XXX: tricky?
    // XXX: extendable

    bool contains(const char *name) const;

    bool findInt32(const char *name, int32_t *value) const;
    bool findInt64(const char *name, int64_t *value) const;
    bool findString(const char *name, std::string *value) const;
    // bool findMessage(const char *name, std::shared_ptr<AMessage> *msg) const;
    bool findObject(const char *name, std::shared_ptr<void> *obj) const;  // XXX
    // XXX: extendable

    status_t post(int64_t delayUs = 0);

    // block call. post message and wait for response or error
    status_t postAndAwaitResponse(std::shared_ptr<AMessage> *response);

    //  If this returns true, the sender of this message is synchronously awaiting a response and
    //  the reply token is consumed from the message and stored into replyID. The reply token must
    //  be used to send the response using "postReply" below.
    bool senderAwaitsResponse(std::shared_ptr<AReplyToken> *replyID);

    // Posts the message as a response to a reply token.  A reply token can only be used once.
    // Returns OK if the response could be posted; otherwise, an error
    status_t postReply(const std::shared_ptr<AReplyToken> &replyID);

    // perform deep-copy of "this"
    std::shared_ptr<AMessage> dup() const;

    // add all items from "other" into "this"
    void extend(const std::shared_ptr<AMessage> &other);

    // std::shared_ptr<AMessage> changesFrom(const std::shared_ptr<const AMessage> &other,
    //                                       bool deep = false) const;

    std::string debugString(int32_t indent = 0) const;

    enum Type {
        kTypeNone,
        kTypeInt32,
        kTypeInt64,
        kTypeSize,
        kTypeFloat,
        kTypeDouble,
        // kTypePointer,
        kTypeString,
        kTypeObject,
        // kTypeMessage,
        // XXX: extendable
    };

    // XXX: extendable for more types. or more complicate structure. eg. Buffer/Data

    virtual ~AMessage();

private:
    friend struct ALooper;  // deliver
    uint32_t mWhat;

    // debug only
    ALooper::handler_id mTarget;

    std::weak_ptr<AHandler> mHandler;
    std::weak_ptr<ALooper> mLooper;

    struct Item {
        union {
            int32_t int32Value;
            int64_t int64Value;
            std::size_t sizeValue;
            float floatValue;
            double doubleValue;
            void  *arbitraryValue;
            const char *stringValue;
            // XXX: extendable
        } u;
        std::shared_ptr<void> mObj = nullptr; // FIXME: workaround for set/findObject
        const char *mName;
        std::size_t mNameLength;
        Type mType;
        void setName(const char *name, std::size_t len);
    };

    enum { kMaxNumItems = 64 };
    Item mItems[kMaxNumItems];
    std::size_t mNumItems;

    Item *allocateItem(const char *name);
    void freeItemValue(Item *item);
    const Item *findItem(const char *name, Type type) const;
    std::size_t findItemIndex(const char *name, std::size_t len) const;

    void deliver();

    DECLARE_NON_COPYASSIGNABLE(AMessage);
};

}  // namespace diordna

#endif  // __A_MESSAGE_H__