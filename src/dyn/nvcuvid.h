#ifndef DYN_NVCUVID_H_
#define DYN_NVCUVID_H_

#include "dyn.h"

// defs
#include <nvcuvid.h>

namespace dyn {

#if defined(WIN32)
static const char NVCUVID_SO[] = "nvcuvid.dll";
#else
static const char NVCUVID_SO[] = "libnvcuvid.so.1";
#endif
DYN_REGISTER(NVCUVID_SO, cuvidCreateDecoder);
DYN_REGISTER(NVCUVID_SO, cuvidReconfigureDecoder);
DYN_REGISTER(NVCUVID_SO, cuvidDestroyDecoder);
DYN_REGISTER(NVCUVID_SO, cuvidDecodePicture);
DYN_REGISTER(NVCUVID_SO, cuvidGetDecodeStatus);
DYN_REGISTER(NVCUVID_SO, cuvidGetDecoderCaps);
DYN_REGISTER(NVCUVID_SO, cuvidCreateVideoParser);
DYN_REGISTER(NVCUVID_SO, cuvidDestroyVideoParser);
DYN_REGISTER(NVCUVID_SO, cuvidParseVideoData);
DYN_REGISTER(NVCUVID_SO, cuvidMapVideoFrame);
DYN_REGISTER(NVCUVID_SO, cuvidUnmapVideoFrame);
DYN_REGISTER(NVCUVID_SO, cuvidCtxLockCreate);
DYN_REGISTER(NVCUVID_SO, cuvidCtxLockDestroy);

}  // namespace dyn

#endif  // DYN_NVCUVID_H_