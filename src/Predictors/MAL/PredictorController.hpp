#pragma once

#include "Predictors/SequenceBuffer/SequenceBuffer.hpp"
#include "Predictors/dense_bgl_model.h"
#include "PredictorController.hpp"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "Arduino.h"


#define MIN_INPUT_VALUE 47
#define MAX_INPUT_VALUE 400


enum class MALState{
    MAL_IDLE,
    MAL_READY,
    MAL_ERROR,
    MAL_NOT_READY,
};


class PredictorController{

    private:
    
    
    static MALState malState;
    static SequenceBuffer modelInputSequence;
    

    static const tflite::Model* model;
    static tflite::MicroInterpreter* interpreter;
    static TfLiteTensor* input;
    static TfLiteTensor* output;
    static tflite::MicroMutableOpResolver<10> resolver;    
    static tflite::MicroErrorReporter micro_error_reporter;
    
    static const uint tensor_arena_size  = 30*1024;

    static uint8_t tensor_arena[tensor_arena_size];


    float normalizeInput(float value);
    float normalizeOutput(float value);


    public:
    bool isReady();
    void init();
    void feed(float unnormalizedSensorReading);
    int predict();     //* I have no idea!
    


};