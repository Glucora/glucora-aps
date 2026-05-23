#include "BLEController.hpp"
#include "BLE/EventQueue/BLEEventQueue.hpp"
#include <cstring>

static BLEController* g_glucoseClientOwner = nullptr;

class GlucoseClientCallbacks : public NimBLEClientCallbacks {
  public:
    explicit GlucoseClientCallbacks(BLEController* owner) : _owner(owner) {}

    void onConnect(NimBLEClient* pClient) override {
      Serial.printf("Connected to peer %s\n", pClient->getPeerAddress().toString().c_str());
      pClient->updateConnParams(12, 24, 0, 200);
    }

    void onDisconnect(NimBLEClient* pClient, int reason) override {
      Serial.printf("Peer disconnected: %s, reason: %d\n", pClient->getPeerAddress().toString().c_str(), reason);
      if (_owner != nullptr) {
        _owner->handleGlucoseClientDisconnect(reason);
      }
    }

    void onConnectFail(NimBLEClient* pClient, int reason) override {
      Serial.printf("Failed to connect to %s, reason: %d\n", pClient->getPeerAddress().toString().c_str(), reason);
      if (_owner != nullptr) {
        _owner->handleGlucoseClientDisconnect(reason);
      }
    }

  private:
    BLEController* _owner;
};

static GlucoseClientCallbacks g_clientCallbacks(nullptr);

static float decodeLittleEndianFloat(const uint8_t* data)
{
  float value = 0.0f;
  std::memcpy(&value, data, sizeof(float));
  return value;
}

void BLEController::glucoseNotifyCallback(NimBLERemoteCharacteristic* pCharacteristic,
                                          uint8_t*                    pData,
                                          size_t                      length,
                                          bool                        isNotify)
{
  if (g_glucoseClientOwner == nullptr || g_glucoseClientOwner->_sampleHandler == nullptr) {
    return;
  }

  if (length < sizeof(float)) {
    Serial.printf("Glucose payload too small: %u bytes\n", static_cast<unsigned>(length));
    return;
  }

  const float reading = decodeLittleEndianFloat(pData);
  Serial.printf("%s from %s / %s -> %0.2f\n",
                isNotify ? "Notification" : "Indication",
                pCharacteristic->getRemoteService()->getUUID().toString().c_str(),
                pCharacteristic->getUUID().toString().c_str(),
                reading);

  g_glucoseClientOwner->_sampleHandler(reading);
}

void BLEController::startAdvertising(){
  
  pAdvertising->setAdvertisingCompleteCallback([this](NimBLEAdvertising* pAdv){
    
    this->_event_queue->createEvent(BLE::Event::BT_CONNECTION_TIMEOUT);
  });

  NimBLEAddress bondedAddr = NimBLEDevice::getBondedAddress(0);
  if(bondedAddr){
    pAdvertising->start(0, &bondedAddr); 

  }
  else{
    pAdvertising->start(0); 

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

void BLEController::initGlucoseClient(const char* peerAddress,
                                      const char* serviceUuid,
                                      const char* characteristicUuid,
                                      GlucoseSampleHandler sampleHandler)
{
  g_glucoseClientOwner = this;
  _peerAddress = peerAddress;
  _serviceUuid = serviceUuid;
  _characteristicUuid = characteristicUuid;
  _sampleHandler = sampleHandler;

  if (pClient == nullptr) {
    pClient = NimBLEDevice::createClient();
    if (pClient == nullptr) {
      Serial.println("Failed to create BLE client");
      return;
    }

    g_clientCallbacks = GlucoseClientCallbacks(this);
    pClient->setClientCallbacks(&g_clientCallbacks, false);
    pClient->setConnectTimeout(5000);
    pClient->setConnectionParams(12, 24, 0, 200, 16, 16);
  }

  maintainGlucoseClientConnection();
}

bool BLEController::isGlucoseClientConnected() const
{
  return pClient != nullptr && pClient->isConnected() && _glucoseClientConnected;
}

void BLEController::handleGlucoseClientDisconnect(int reason)
{
  Serial.printf("Scheduling glucose reconnect after disconnect, reason: %d\n", reason);
  _glucoseClientConnected = false;
  pRemoteCharacteristic = nullptr;
  _lastReconnectAttempt = millis();
}

bool BLEController::connectGlucoseClient()
{
  if (pClient == nullptr || _peerAddress == nullptr || _serviceUuid == nullptr || _characteristicUuid == nullptr) {
    return false;
  }

  if (pClient->isConnected()) {
    return subscribeToGlucoseCharacteristic();
  }

  NimBLEUUID serviceUuid(_serviceUuid);
  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setActiveScan(true);
  pScan->setMaxResults(20);

  Serial.printf("Scanning for glucose peer %s advertising service %s\n", _peerAddress, _serviceUuid);
  NimBLEScanResults results = pScan->getResults(5000, false);

  Serial.printf("Scan complete. Found %d devices:\n", results.getCount());
  for (int i = 0; i < results.getCount(); ++i) {
    const NimBLEAdvertisedDevice* device = results.getDevice(i);
    if (device != nullptr) {
      Serial.printf("  [%d] MAC: %s | Name: %s | Services: %s | Connectable: %d\n",
                    i,
                    device->getAddress().toString().c_str(),
                    device->getName().c_str(),
                    device->haveServiceUUID() ? "yes" : "no",
                    device->isConnectable() ? 1 : 0);
      if (device->haveServiceUUID()) {
        Serial.printf("       Service UUID: %s\n", device->getServiceUUID().toString().c_str());
      }
    }
  }

  const NimBLEAdvertisedDevice* matchedDevice = nullptr;
  for (int i = 0; i < results.getCount(); ++i) {
    const NimBLEAdvertisedDevice* device = results.getDevice(i);
    if (device == nullptr) {
      continue;
    }

    const bool addressMatches = device->getAddress().toString() == std::string(_peerAddress);
    const bool serviceMatches = device->haveServiceUUID() && device->isAdvertisingService(serviceUuid);
    
    if (addressMatches) {
      Serial.printf("Matched by address: %s\n", device->getAddress().toString().c_str());
      matchedDevice = device;
      break;
    }
    if (serviceMatches) {
      Serial.printf("Matched by service UUID: %s @ %s\n", serviceUuid.toString().c_str(), device->getAddress().toString().c_str());
      matchedDevice = device;
      break;
    }
  }

  if (matchedDevice == nullptr) {
    Serial.printf("Failed to find glucose peer %s during scan\n", _peerAddress);
    return false;
  }

  Serial.printf("Found glucose peer: %s\n", matchedDevice->toString().c_str());

  if (!pClient->connect(matchedDevice, true, false, true)) {
    Serial.printf("Failed to connect to glucose peer %s\n", _peerAddress);
    return false;
  }

  if (!pClient->secureConnection()) {
    Serial.println("Failed to secure glucose connection");
    pClient->disconnect();
    return false;
  }

  if (!pClient->discoverAttributes()) {
    Serial.println("Failed to discover glucose peer attributes");
    pClient->disconnect();
    return false;
  }

  return subscribeToGlucoseCharacteristic();
}

bool BLEController::subscribeToGlucoseCharacteristic()
{
  if (pClient == nullptr || !pClient->isConnected()) {
    return false;
  }

  NimBLERemoteService* pService = pClient->getService(_serviceUuid);
  if (pService == nullptr) {
    Serial.printf("Failed to find glucose service %s\n", _serviceUuid);
    return false;
  }

  pRemoteCharacteristic = pService->getCharacteristic(_characteristicUuid);
  if (pRemoteCharacteristic == nullptr) {
    Serial.printf("Failed to find glucose characteristic %s\n", _characteristicUuid);
    return false;
  }

  if (pRemoteCharacteristic->canNotify()) {
    if (!pRemoteCharacteristic->subscribe(true, glucoseNotifyCallback)) {
      Serial.println("Failed to subscribe for glucose notifications");
      return false;
    }
  } else if (pRemoteCharacteristic->canIndicate()) {
    if (!pRemoteCharacteristic->subscribe(false, glucoseNotifyCallback)) {
      Serial.println("Failed to subscribe for glucose indications");
      return false;
    }
  } else {
    Serial.println("Glucose characteristic does not support notify/indicate");
    return false;
  }

  _glucoseClientConnected = true;
  Serial.printf("Glucose subscription active on %s / %s\n", _serviceUuid, _characteristicUuid);
  return true;
}

void BLEController::maintainGlucoseClientConnection()
{
  if (_sampleHandler == nullptr || pClient == nullptr) {
    return;
  }

  // Avoid central-role scanning/reconnect while a phone is connected to this peripheral.
  if (pServer != nullptr && pServer->getConnectedCount() > 0) {
    return;
  }

  if (pClient->isConnected() && _glucoseClientConnected) {
    return;
  }

  const unsigned long now = millis();
  if (now - _lastReconnectAttempt < _reconnectDelayMs) {
    return;
  }

  _lastReconnectAttempt = now;
  if (!connectGlucoseClient()) {
    _glucoseClientConnected = false;
    pRemoteCharacteristic = nullptr;
  }
}
