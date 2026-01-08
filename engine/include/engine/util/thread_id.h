#ifndef ENGINE_UTIL_THREAD_ID_H_
#define ENGINE_UTIL_THREAD_ID_H_

namespace engine::util {

enum class ThreadId { kMain, kLogic, kNetwork, kAudio, kDebug };

}  // namespace engine::util

#endif  // ENGINE_UTIL_THREAD_ID_H_
