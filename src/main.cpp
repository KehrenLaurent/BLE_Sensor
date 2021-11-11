// For BLE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
// For DallasTemperature
#include <OneWire.h>
#include <DallasTemperature.h>

// Init Variables For Sensor
int sensorPin = 15;
OneWire oneWire(sensorPin);
DallasTemperature sensors(&oneWire);

// BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// Declare Fonction
float getTemperature();


#define enviornmentService BLEUUID((uint16_t)0x181A)

BLECharacteristic temperatureCharacteristic(
    BLEUUID((uint16_t)0x2A6E),
    BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY);
//BLEDescriptor tempDescriptor(BLEUUID((uint16_t)0x2901));

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};

float getTemperature()
{
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

void setup()
{
  Serial.begin(115200);

  //Init DallasTemperature
  sensors.begin();

  // Create the BLE Device
  BLEDevice::init("MyESP32");

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pEnviornment = pServer->createService(enviornmentService);

  // Create a BLE Characteristic
  pEnviornment->addCharacteristic(&temperatureCharacteristic);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  temperatureCharacteristic.addDescriptor(new BLE2902());

  BLEDescriptor TemperatureDescriptor(BLEUUID((uint16_t)0x2901));
  TemperatureDescriptor.setValue("Temperature -40 to 60°C");
  temperatureCharacteristic.addDescriptor(&TemperatureDescriptor);

  pServer->getAdvertising()->addServiceUUID(enviornmentService);

  // Start the service
  pEnviornment->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop()
{
  float temperature = getTemperature();
  Serial.println(temperature);

  if (deviceConnected)
  {
    int16_t value;
    value = (temperature * 100);
    Serial.println(value);
    temperatureCharacteristic.setValue((uint8_t *)&value, 2);
    temperatureCharacteristic.notify();
  }
  delay(1000);
}
