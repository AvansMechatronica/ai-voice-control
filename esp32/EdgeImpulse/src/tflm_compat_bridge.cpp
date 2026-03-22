#include <cstdarg>
#include <cstdio>

#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

void Log(const char* format, va_list args) {
    vprintf(format, args);
    printf("\n");
}

namespace tflite {
namespace micro {

TfLiteRegistration RegisterOp(
    void* (*init)(TfLiteContext* context, const char* buffer, size_t length),
    TfLiteStatus (*prepare)(TfLiteContext* context, TfLiteNode* node),
    TfLiteStatus (*invoke)(TfLiteContext* context, TfLiteNode* node),
    void (*free)(TfLiteContext* context, void* buffer)) {
    return {/*init=*/init,
            /*free=*/free,
            /*prepare=*/prepare,
            /*invoke=*/invoke,
            /*profiling_string=*/nullptr,
            /*builtin_code=*/0,
            /*custom_name=*/nullptr,
            /*version=*/0,
            /*registration_external=*/nullptr};
}

}  // namespace micro
}  // namespace tflite
