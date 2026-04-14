#include "BLEController.hpp"
#include "BLE/EventQueue/BLEEventQueue.hpp"

void BLEController::startAdvertising(){
  
  pAdvertising->setAdvertisingCompleteCallback([this](NimBLEAdvertising* pAdv){
    
    this->_event_queue->createEvent(BLE::Event::BT_CONNECTION_TIMEOUT);
  });

  NimBLEAddress bondedAddr = NimBLEDevice::getBondedAddress(0);
  if(bondedAddr){
    pAdvertising->start(20000, &bondedAddr); 

  }
  else{
    pAdvertising->start(20000); 

  }
  
}

void BLEController::stopAdvertising()
{
  pAdvertising->stop();
}

void BLEController::createSerivceAndCharacteristic(const char *serviceId, const char* characteristicId)  ///Figure this out
{
  NimBLEService *pService = this->pServer->getServiceByUUID(serviceId);



  NimBLECharacteristic *pCharacteristic = pService->createCharacteristic(characteristicId, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);  ///This could be a Value (Glucose, etc..)
  
  
  
  

  Serial.printf("Service UUID:        %s\n", pService->getUUID().toString().c_str());
  Serial.printf("Characteristic UUID: %s\n", pCharacteristic->getUUID().toString().c_str());

}


/*@brief  This inits the NimBLE BLE Stack */
void BLEController::init(BLEEventQueue& eventQueue){

  this->_event_queue = &eventQueue;

  Serial.println("Init BLE HAL");
  NimBLEDevice::init("GlucoraAI_PUMP");
    

  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  this->pServer = NimBLEDevice::createServer();
  this->pServer->setCallbacks(new BLEServerCallbacksHandler(eventQueue));

  
  
  pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName("GlucoraAI_PUMP"); 
   


  
}

void BLEController::createService(const char *serviceId)
{
  NimBLEService* pService = this->pServer->createService(serviceId);
    
    
}

void BLEController::startService(const char *serviceId)
{
  NimBLEService* pService = this->pServer->getServiceByUUID(serviceId);
  pService->start();
  pAdvertising->addServiceUUID(serviceId);
}
