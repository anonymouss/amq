#ifndef __B_H__

#include <AHandler.h>

#include <memory>

namespace diordna {

class AMessage;
class A;

class B : public AHandler {
public:
    void handleMessage_1();
    void handleMessage_2();

    //std::shared_ptr<B> This() { return this->shared_from_this(); }

protected:
    virtual void onMessageReceived(const std::shared_ptr<AMessage> &msg);

    enum {
        kWhatMessage_1,
        kWhatMessage_2,
    };

    std::weak_ptr<A> mA;

private:
    void onMessage_1();
    void onMessage_2();
};

} // namespace diordna

#endif // __B_H__