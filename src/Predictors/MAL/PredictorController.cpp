
#include "PredictorController.hpp"

const tflite::Model* PredictorController::model= nullptr;
tflite::MicroInterpreter* PredictorController::interpreter = nullptr;
TfLiteTensor* PredictorController::input= nullptr;
TfLiteTensor* PredictorController::output= nullptr;

tflite::MicroErrorReporter PredictorController::micro_error_reporter;
tflite::MicroMutableOpResolver<10> PredictorController::resolver;
uint8_t PredictorController::tensor_arena[PredictorController::tensor_arena_size];    

MALState PredictorController::malState = MALState::MAL_NOT_READY;
SequenceBuffer PredictorController::modelInputSequence;


void PredictorController::init()
{
    model = tflite::GetModel(dense_model_data);
    
    resolver.AddFullyConnected();
    resolver.AddRelu();
    resolver.AddReshape();
    resolver.AddShape();
    resolver.AddStridedSlice();
    resolver.AddPack();
    resolver.AddUnidirectionalSequenceLSTM();
    
    
    tflite::ErrorReporter* error_reporter = &micro_error_reporter;
    static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, tensor_arena_size, error_reporter);
    interpreter = &static_interpreter;

    if(interpreter->AllocateTensors() != kTfLiteOk){
        Serial.println("Failed to allocate tensors!");
        malState = MALState::MAL_ERROR;

    }

    
    input = interpreter->input(0);
    output= interpreter->output(0);
    malState = MALState::MAL_IDLE;

}


void PredictorController::feed(float unnormalizedSensorReading){
    modelInputSequence.push(unnormalizedSensorReading);
    if(modelInputSequence.isReady()){
        malState = MALState::MAL_READY;
    }
}

int PredictorController::predict() {

    if(!modelInputSequence.isReady()){
        return -1;
    }

    for (int i = 0; i < 24; i++) {
      input->data.f[i] = normalizeInput(modelInputSequence[i]);
    }

    TfLiteStatus invoke_status = interpreter->Invoke();

    float scaled_output = output->data.f[0];

    if (isnan(scaled_output) || isinf(scaled_output)) {
      return -1;
    }

    if (scaled_output < 0.0f) scaled_output = 0.0f;
    if (scaled_output > 1.0f) scaled_output = 1.0f;

    float bgl_predicted = normalizeOutput(scaled_output);
    return (int)roundf(bgl_predicted);
}


float PredictorController::normalizeInput(float value){
    
    return (value - MIN_INPUT_VALUE) / (MAX_INPUT_VALUE - MIN_INPUT_VALUE);
    
}

float PredictorController::normalizeOutput(float value){
    
    return value * (MAX_INPUT_VALUE - MIN_INPUT_VALUE) + MIN_INPUT_VALUE;
    
}

bool PredictorController::isReady(){

    return malState == MALState::MAL_READY;

}