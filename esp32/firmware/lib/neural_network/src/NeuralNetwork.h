#ifndef __NeuralNetwork__
#define __NeuralNetwork__

#include <stdint.h>
#include <stddef.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

namespace tflite
{
    class Model;
    class MicroInterpreter;
} // namespace tflite

struct TfLiteTensor;

typedef struct
{
    float score;
    int index;
} NNResult;

class NeuralNetwork
{
private:
    tflite::AllOpsResolver m_resolver;
    tflite::MicroErrorReporter m_error_reporter;
    const tflite::Model *m_model;
    tflite::MicroInterpreter *m_interpreter;
    TfLiteTensor *input;
    TfLiteTensor *output;
    uint8_t *m_tensor_arena_alloc;
    uint8_t *m_tensor_arena;
    size_t m_tensor_arena_size;
    bool m_is_ready;

public:
    NeuralNetwork();
    ~NeuralNetwork();
    bool isReady() const;
    float *getInputBuffer();
    float *getOutputBuffer();
    NNResult predict();
};

#endif