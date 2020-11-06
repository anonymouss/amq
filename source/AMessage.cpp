#define TAG "AMessage"

#include <AHandler.h>
#include <ALooperRoster.h>
#include <AMessage.h>

#include <cassert>
#include <cstdint>
#include <string>

namespace diordna {

extern ALooperRoster gLooperRoster;

status_t AReplyToken::setReply(const std::shared_ptr<AMessage> &reply) {
    if (mReplied) {
        LOG("E : trying to post a duplicated reply");
        return ALREADY_EXISTS;
    }
    assert(mReply == nullptr);
    mReply = reply;
    mReplied = true;
    return OK;
}

AMessage::AMessage() : mWhat(0), mTarget(0), mNumItems(0) {}

AMessage::AMessage(uint32_t what, const std::shared_ptr<const AHandler> &handler)
    : mWhat(what), mNumItems(0) {
    setTarget(handler);
}

AMessage::~AMessage() { clear(); }

void AMessage::setWhat(uint32_t what) { mWhat = what; }

uint32_t AMessage::what() const { return mWhat; }

void AMessage::setTarget(const std::shared_ptr<const AHandler> &handler) {
    if (handler == nullptr) {
        LOG("W : configured a null handler");
        mTarget = 0;
    } else {
        mTarget = handler->id();
        mHandler = handler->getHandler();
        mLooper = handler->getLooper();
    }
}

void AMessage::clear() {
    for (auto i = 0; i < mNumItems; ++i) {
        auto *item = &mItems[i];
        delete[] item->mName;
        item->mName = nullptr;
        freeItemValue(item);
    }
    mNumItems = 0;
}

void AMessage::freeItemValue(Item *item) {
    switch (item->mType) {
        case kTypeString: {
            delete item->u.stringValue;
            break;
        }
        // TODO: case kTypeOthers: extendable
        default: break;
    }
    item->mType = kTypeNone;
}

std::size_t AMessage::findItemIndex(const char *name, std::size_t len) const {
    std::size_t i = 0;
    for (; i < mNumItems; ++i) {
        if (len != mItems[i].mNameLength) { continue; }
        if (!std::memcmp(mItems[i].mName, name, len)) { break; }
    }
    return i;
}

void AMessage::Item::setName(const char *name, std::size_t len) {
    mNameLength = len;
    mName = new char[len + 1];
    std::memcpy((void *)mName, name, len + 1);
}

AMessage::Item *AMessage::allocateItem(const char *name) {
    auto len = std::strlen(name);
    auto i = findItemIndex(name, len);
    Item *item = nullptr;

    if (i < mNumItems) {
        item = &mItems[i];
        freeItemValue(item);
    } else {
        assert(mNumItems < kMaxNumItems);
        i = mNumItems++;
        item = &mItems[i];
        item->mType = kTypeNone;
        item->setName(name, len);
    }

    return item;
}

const AMessage::Item *AMessage::findItem(const char *name, Type type) const {
    auto i = findItemIndex(name, std::strlen(name));
    if (i < mNumItems) {
        const auto *item = &mItems[i];
        return item->mType == type ? item : nullptr;
    }
    return nullptr;
}

bool AMessage::contains(const char *name) const {
    auto i = findItemIndex(name, std::strlen(name));
    return i < mNumItems;
}

#define BASIC_TYPE_SET_FIND(NAME, FIELD, TYPE)                       \
    void AMessage::set##NAME(const char *name, TYPE value) {         \
        auto *item = allocateItem(name);                             \
        item->mType = kType##NAME;                                   \
        item->u.FIELD = value;                                       \
    }                                                                \
                                                                     \
    bool AMessage::find##NAME(const char *name, TYPE *value) const { \
        const auto *item = findItem(name, kType##NAME);              \
        if (item) {                                                  \
            *value = item->u.FIELD;                                  \
            return true;                                             \
        }                                                            \
        return false;                                                \
    }

BASIC_TYPE_SET_FIND(Int32, int32Value, int32_t)
BASIC_TYPE_SET_FIND(Int64, int64Value, int64_t)
// BASIC_TYPE_SET_FIND(Size, sizeValue, std::size_t)
// BASIC_TYPE_SET_FIND(Float, floatValue, float)
// BASIC_TYPE_SET_FIND(Double, doubleValue, double)
// Extendable

#undef BASIC_TYPE_SET_FIND

void AMessage::setString(const char *name, const char *s) {
    auto *item = allocateItem(name);
    item->mType = kTypeString;
    item->u.stringValue = s;
}

void AMessage::setString(const char *name, const std::string &s) { setString(name, s.c_str()); }

bool AMessage::findString(const char *name, std::string *value) const {
    const auto *item = findItem(name, kTypeString);
    if (item != nullptr) {
        *value = *item->u.stringValue;
        return true;
    }
    return false;
}

void AMessage::setObject(const char *name, const std::shared_ptr<void> &obj) {
    auto *item = allocateItem(name);
    item->mType = kTypeObject;
    // item->u.arbitraryValue = obj.get();  // FIXME: will it be null?
    item->mObj = obj;
}

// FIXME: not sure if it's correct
bool AMessage::findObject(const char *name, std::shared_ptr<void> *obj) const {
    const auto *item = findItem(name, kTypeString);
    if (item != nullptr) {
        *obj = item->mObj;
        return true;
    }
    return false;
}

void AMessage::deliver() {
    std::shared_ptr<AHandler> handler = mHandler.lock();
    if (handler == nullptr) {
        LOG("W : failed to deliver message as target handler %d is gone", mTarget);
        return;
    }
    handler->deliverMessage(shared_from_this());
}

status_t AMessage::post(int64_t delayUs) {
    std::shared_ptr<ALooper> looper = mLooper.lock();
    if (looper == nullptr) {
        LOG("W : failed to post message as target looper for handler %d is gone", mTarget);
        return NAME_NOT_FOUND;
    }
    looper->post(shared_from_this(), delayUs);
    return OK;
}

status_t AMessage::postAndAwaitResponse(std::shared_ptr<AMessage> *response) {
    std::shared_ptr<ALooper> looper = mLooper.lock();
    if (looper == nullptr) {
        LOG("W : failed to post message as target looper for handler %d is gone", mTarget);
        return NAME_NOT_FOUND;
    }

    std::shared_ptr<AReplyToken> token = looper->createReplyToken();
    if (token == nullptr) {
        LOG("E : failed to create reply token");
        return NO_MEMORY;
    }
    setObject("reolyID", token);

    looper->post(shared_from_this(), 0);
    return looper->awaitResponse(token, response);
}

status_t AMessage::postReply(const std::shared_ptr<AReplyToken> &replyToken) {
    if (replyToken == nullptr) {
        LOG("W : failed to post reply to a null token");
        return NAME_NOT_FOUND;
    }

    std::shared_ptr<ALooper> looper = replyToken->getLooper();
    if (looper == nullptr) {
        LOG("W : failed to post reply as target looper is gone.");
        return NAME_NOT_FOUND;
    }

    return looper->postReply(replyToken, shared_from_this());
}

bool AMessage::senderAwaitsResponse(std::shared_ptr<AReplyToken> *replyToken) {
    std::shared_ptr<void> obj;
    if (!findObject("replyID", &obj)) { return false; }

    // FIXME: is it ok?
    *replyToken = std::static_pointer_cast<AReplyToken>(obj);
    setObject("replyID", nullptr);

    return *replyToken != nullptr;
}

std::shared_ptr<AMessage> AMessage::dup() const {
    std::shared_ptr<AMessage> msg = std::make_shared<AMessage>(mWhat, mHandler.lock());
    msg->mNumItems = mNumItems;

    for (auto i = 0; i < mNumItems; ++i) {
        const auto *from = &mItems[i];
        auto *to = &msg->mItems[i];

        to->setName(from->mName, from->mNameLength);
        to->mType = from->mType;
        to->u = from->u;
    }

    return msg;
}

std::string AMessage::debugString(int32_t indent) const {
    // TODO:
    return UNIMPLEMENTED_MESSAGE;
}

}  // namespace diordna