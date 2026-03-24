#include "NeuralNetwork.h"
#include "model.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

// approximate working size of our model
const int kArenaSize = 50000;

NeuralNetwork::NeuralNetwork() :
    m_model(nullptr),
    m_interpreter(nullptr),
    input(nullptr),
    output(nullptr),
    m_tensor_arena_alloc(nullptr),
    m_tensor_arena(nullptr),
    m_is_ready(false)
{
    m_tensor_arena_alloc = static_cast<uint8_t *>(malloc(kArenaSize + 15));
    if (!m_tensor_arena_alloc)
    {
        TF_LITE_REPORT_ERROR(&m_error_reporter, "Could not allocate arena");
        return;
    }
    uintptr_t aligned_address = (reinterpret_cast<uintptr_t>(m_tensor_arena_alloc) + 15U) & ~static_cast<uintptr_t>(0x0f);
    m_tensor_arena = reinterpret_cast<uint8_t *>(aligned_address);

    TF_LITE_REPORT_ERROR(&m_error_reporter, "Loading model");

    m_model = tflite::GetModel(converted_model_tflite);
    if (m_model->version() != TFLITE_SCHEMA_VERSION)
    {
        TF_LITE_REPORT_ERROR(&m_error_reporter, "Model provided is schema version %d not equal to supported version %d.",
                             m_model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }
    // This pulls in the operators implementations we need
    m_resolver.AddConv2D();
    m_resolver.AddMaxPool2D();
    m_resolver.AddFullyConnected();
    m_resolver.AddMul();
    m_resolver.AddAdd();
    m_resolver.AddLogistic();
    m_resolver.AddReshape();
    m_resolver.AddQuantize();
    m_resolver.AddDequantize();
    m_resolver.AddSoftmax();

    // Build an interpreter to run the model with.
    m_interpreter = new tflite::MicroInterpreter(
        m_model, m_resolver, m_tensor_arena, kArenaSize, &m_error_reporter);

    // Allocate memory from the tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = m_interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        TF_LITE_REPORT_ERROR(&m_error_reporter, "AllocateTensors() failed");
        return;
    }

    size_t used_bytes = m_interpreter->arena_used_bytes();
    TF_LITE_REPORT_ERROR(&m_error_reporter, "Used bytes %d\n", used_bytes);

    TF_LITE_REPORT_ERROR(&m_error_reporter, "Output Size %d\n", m_interpreter->outputs_size());
    // Obtain pointers to the model's input and output tensors.
    input = m_interpreter->input(0);
    output = m_interpreter->output(0);
    if (!input || !output)
    {
        TF_LITE_REPORT_ERROR(&m_error_reporter, "Model tensors are not available");
        return;
    }

    m_is_ready = true;
}

NeuralNetwork::~NeuralNetwork()
{
    delete m_interpreter;
    free(m_tensor_arena_alloc);
}

bool NeuralNetwork::isReady() const
{
    return m_is_ready;
}

float *NeuralNetwork::getInputBuffer()
{
    if (!input)
    {
        return nullptr;
    }

    return input->data.f;
}

float *NeuralNetwork::getOutputBuffer()
{
    if (!output)
    {
        return nullptr;
    }

    return output->data.f;
}

NNResult NeuralNetwork::predict()
{
    if (!m_is_ready)
    {
        return {
            .score = 0,
            .index = -1};
    }

    m_interpreter->Invoke();
    // work out the "best output"
    float best_score = 0;
    int best_index = -1;
    for (int i = 0; i < 5; i++)
    {
        float score = output->data.f[i];
        if (score > best_score)
        {
            best_score = score;
            best_index = i;
        }
    }
    return {
        .score = best_score,
        .index = best_index};
}
