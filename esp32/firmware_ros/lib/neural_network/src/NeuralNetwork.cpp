#include "NeuralNetwork.h"
#include "model.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#include "esp_heap_caps.h"
#include "esp_system.h"

// Keep this comfortably above observed arena usage to avoid allocation failures
// when model/operator versions change.
const size_t kDesiredArenaSize = 120 * 1024;
const size_t kMinArenaSize = 64 * 1024;
const size_t kArenaAlignment = 16;
const size_t kArenaAllocHeadroom = 1024;

NeuralNetwork::NeuralNetwork() :
    m_model(nullptr),
    m_interpreter(nullptr),
    input(nullptr),
    output(nullptr),
    m_tensor_arena_alloc(nullptr),
    m_tensor_arena(nullptr),
    m_tensor_arena_size(0),
    m_is_ready(false)
{
    size_t largest_spiram_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    size_t largest_internal_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    m_tensor_arena_size = kDesiredArenaSize;
    size_t arena_alloc_size = m_tensor_arena_size + kArenaAlignment - 1;

    if (largest_spiram_block >= arena_alloc_size)
    {
        m_tensor_arena_alloc = static_cast<uint8_t *>(
            heap_caps_malloc(arena_alloc_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }

    if (!m_tensor_arena_alloc)
    {
        size_t max_internal_arena = 0;
        if (largest_internal_block > (kArenaAlignment - 1 + kArenaAllocHeadroom))
        {
            max_internal_arena = largest_internal_block - (kArenaAlignment - 1) - kArenaAllocHeadroom;
        }

        if (max_internal_arena < m_tensor_arena_size)
        {
            m_tensor_arena_size = max_internal_arena;
        }

        if (m_tensor_arena_size >= kMinArenaSize)
        {
            arena_alloc_size = m_tensor_arena_size + kArenaAlignment - 1;
            m_tensor_arena_alloc = static_cast<uint8_t *>(
                heap_caps_malloc(arena_alloc_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        }
    }

    if (!m_tensor_arena_alloc)
    {
        TF_LITE_REPORT_ERROR(&m_error_reporter,
                             "Could not allocate arena (%u bytes). Free heap=%u, largest internal=%u, largest spiram=%u",
                             static_cast<unsigned int>(m_tensor_arena_size),
                             static_cast<unsigned int>(esp_get_free_heap_size()),
                             static_cast<unsigned int>(largest_internal_block),
                             static_cast<unsigned int>(largest_spiram_block));
        return;
    }

    uintptr_t aligned_address = (reinterpret_cast<uintptr_t>(m_tensor_arena_alloc) + (kArenaAlignment - 1U)) & ~static_cast<uintptr_t>(kArenaAlignment - 1U);
    m_tensor_arena = reinterpret_cast<uint8_t *>(aligned_address);

    TF_LITE_REPORT_ERROR(&m_error_reporter,
                         "Loading model (len=%u, arena=%u, free heap=%u, largest internal=%u)",
                         converted_model_tflite_len,
                         static_cast<unsigned int>(m_tensor_arena_size),
                         static_cast<unsigned int>(esp_get_free_heap_size()),
                         static_cast<unsigned int>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));

    m_model = tflite::GetModel(converted_model_tflite);
    if (m_model->version() != TFLITE_SCHEMA_VERSION)
    {
        TF_LITE_REPORT_ERROR(&m_error_reporter, "Model provided is schema version %d not equal to supported version %d.",
                             m_model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }
    // Build an interpreter to run the model with.
    m_interpreter = new tflite::MicroInterpreter(
        m_model, m_resolver, m_tensor_arena, m_tensor_arena_size, &m_error_reporter);

    // Allocate memory from the tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = m_interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        TF_LITE_REPORT_ERROR(&m_error_reporter,
                             "AllocateTensors() failed. Arena=%u, free heap=%u, largest internal=%u",
                             static_cast<unsigned int>(m_tensor_arena_size),
                             static_cast<unsigned int>(esp_get_free_heap_size()),
                             static_cast<unsigned int>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));
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
