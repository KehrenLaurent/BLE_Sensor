//#include <string>
//#include <sstream>
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

// Temperature reading
#define temperaratureService BLEUUID((uint16_t)0x2A1C)

BLECharacteristic temperatureCharacteristic(
    BLEUUID((uint16_t)0x2A6E),
    BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY);

// Setting service
// #define uuidsettingsService BLEUUID((uint16_t)0x1801)
// #define uuidMeasurementIntervalCharacteristic BLEUUID((uint16_t)0x2A21)
#define uuidsettingsService "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define uuidMeasurementIntervalCharacteristic "beb5483e-36e1-4688-b7f5-ea07361b26a8"
BLECharacteristic *pMeasurementIntervalCharacteristic = NULL;

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  };
};

class SettingsCharateristicCallbacks : public BLECharacteristicCallbacks
{
  void OnWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
  };
};

float getTemperature()
{
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  Serial.print("Temperature : ");
  Serial.println(temperatureC);
  return temperatureC;
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

  // --------------Settings----------------
  BLEService *pSettings = pServer->createService(uuidsettingsService);
  BLECharacteristic *pMeasurementIntervalCharacteristic = pSettings->createCharacteristic(
      uuidMeasurementIntervalCharacteristic,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  pMeasurementIntervalCharacteristic->setCallbacks(new SettingsCharateristicCallbacks());
  pMeasurementIntervalCharacteristic->setValue("1000");

  pMeasurementIntervalCharacteristic->addDescriptor(new BLE2902());

  BLEDescriptor MeasurementIntervaleDescriptor(BLEUUID((uint16_t)0x2901));
  MeasurementIntervaleDescriptor.setValue("Set or read interval or measurement");
  pMeasurementIntervalCharacteristic->addDescriptor(&MeasurementIntervaleDescriptor);
  pSettings->start();

  // --------------Temperature----------------

  // Create the BLE Service
  BLEService *pTemprature = pServer->createService(temperaratureService);

  // Create a BLE Characteristic
  pTemprature->addCharacteristic(&temperatureCharacteristic);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  temperatureCharacteristic.addDescriptor(new BLE2902());

  BLEDescriptor TemperatureDescriptor(BLEUUID((uint16_t)0x2901));
  TemperatureDescriptor.setValue("Temperature -40 to 60°C");
  temperatureCharacteristic.addDescriptor(&TemperatureDescriptor);

  pServer->getAdvertising()->addServiceUUID(temperaratureService);

  // Start the service
  pTemprature->start();

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
  //int i;
  //std::istringstream(pMeasurementIntervalCharacteristic->getValue()) >> i;
  //Serial.print(i);
  delay(1000);
}
