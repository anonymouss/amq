#ifndef __A_H__

#include <memory>

namespace diordna {

class B;
class ALooper;

class A {
public:
    A();
    void sendMessage(int id);

private:
    std::shared_ptr<ALooper> mLooper;
    const std::shared_ptr<B> mB;
};

} // namespace diordna

#endif // __B_H__