/* Copyright 2026
   Local backport of SHAPE kernel registration for vendored TFLM snapshot. */

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/op_macros.h"
#include "tensorflow/lite/micro/kernels/kernel_util.h"

namespace tflite {
namespace ops {
namespace micro {
namespace shape {

constexpr int kInputTensor = 0;
constexpr int kOutputTensor = 0;

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, node->inputs->size, 1);
  TF_LITE_ENSURE_EQ(context, node->outputs->size, 1);

  const TfLiteTensor* input = context->GetTensor(context, node->inputs->data[kInputTensor]);
  TfLiteTensor* output = context->GetTensor(context, node->outputs->data[kOutputTensor]);
  TF_LITE_ENSURE(context, input != nullptr);
  TF_LITE_ENSURE(context, output != nullptr);
  TF_LITE_ENSURE(context, input->dims != nullptr);
  TF_LITE_ENSURE(context, output->dims != nullptr);

  TF_LITE_ENSURE_EQ(context, output->dims->size, 1);
  TF_LITE_ENSURE_EQ(context, output->dims->data[0], input->dims->size);
  TF_LITE_ENSURE(context, output->type == kTfLiteInt32 || output->type == kTfLiteInt64);

  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* input = tflite::micro::GetEvalInput(context, node, kInputTensor);
  TfLiteEvalTensor* output = tflite::micro::GetEvalOutput(context, node, kOutputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  TF_LITE_ENSURE(context, output != nullptr);
  TF_LITE_ENSURE(context, input->dims != nullptr);

  const int rank = input->dims->size;
  if (output->type == kTfLiteInt64) {
    for (int i = 0; i < rank; ++i) {
      output->data.i64[i] = input->dims->data[i];
    }
  } else {
    for (int i = 0; i < rank; ++i) {
      output->data.i32[i] = input->dims->data[i];
    }
  }

  return kTfLiteOk;
}

}  // namespace shape

TfLiteRegistration Register_SHAPE() {
  return {/*init=*/nullptr,
          /*free=*/nullptr,
          /*prepare=*/shape::Prepare,
          /*invoke=*/shape::Eval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace micro
}  // namespace ops
}  // namespace tflite
