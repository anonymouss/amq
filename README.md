ported from Android native message queue, decouple it from Android system libs and replace them with pure C++ std libs.

core:

- AHandler - the handler to process messages
- ALooper - the thread loop which contains a message queue, it fetches meesage and deliver it to handler to process
- AMessage - the message itself

Android source locates at: https://android.googlesource.com/platform/frameworks/av/+/refs/heads/master/media/libstagefright/foundation/

build & run:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```
