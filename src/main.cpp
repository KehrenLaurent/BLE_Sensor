/*


   Create a server for BLE sensor

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.
*/
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
// #include <iostream>
// #include <string>

// EETROM adress
#define EEPROM_SIZE 40
uint8_t deviceNameAddressStart = 0;
uint8_t deviceNameAddressEND = 20;
uint8_t intervalOfMeasurementAddress = 20;
uint8_t calibrationAAddress = 25;
uint8_t calibrationBAddress = 30;
uint8_t IsNotFirstRunAddress = 39;

// Base value
char deviceName[20];                // Device name of sensor
DeviceAddress sensorSerial;         // Serial number of sensor
unsigned int intervalOfMeasurement; // Interval of measurement
float calibration_a;                // Parameter of calibration a
float calibration_b;                // Parameter of calibration b

// Temperature sensor
OneWire oneWire(15); // 15 pin of data DS18B20
DallasTemperature sensors(&oneWire);
char temperature;
bool deviceConnected;

void writeCharToEEPROM(char *charToWrite, int addressStart, int addressEND)
{
  int j = 0;
  for (int i = addressStart; i < addressEND; i++)
  {
    EEPROM.write(i, charToWrite[j]);
    j++;
    EEPROM.commit();
  }
}

void readCharToEEPROM(char *charForStore, int addressStart, int addressEND)
{
  for (int i = addressStart; i < addressEND; i++)
  {
    charForStore[i] = char(EEPROM.read(i));
  }
}

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "13012F00-F8C3-4F4A-A8F4-15CD926DA146" // UART service UUID
#define CHARACTERISTIC_UUID_INTERVAL_MEASUREMENT "13012F01-F8C3-4F4A-A8F4-15CD926DA146"
#define CHARACTERISTIC_UUID_CALIBRATION_A "13012F02-F8C3-4F4A-A8F4-15CD926DA146"
#define CHARACTERISTIC_UUID_CALIBRATION_B "13012F08-F8C3-4F4A-A8F4-15CD926DA146"
#define CHARACTERISTIC_UUID_SENSOR_SERIAL "13012F06-F8C3-4F4A-A8F4-15CD926DA146"
#define CHARACTERISTIC_UUID_DEVICE_NAME "13012F07-F8C3-4F4A-A8F4-15CD926DA146"
#define CHARACTERISTIC_UUID_TEMPERATURE "1d17cffa-ddea-45d5-bb06-0c56b404e224"

BLECharacteristic *TemperatureCharacteristic;

// header function
float getTemperature();   // Read temperature of sensor
void notifyTemperature(); // Notify temperature

void initEEPROM()
{
  // setup EEPROM after the fist run
  if (int(EEPROM.read(IsNotFirstRunAddress)) == 1)
  {
    // read value in EEPROM

    // reade device name
    readCharToEEPROM(deviceName, deviceNameAddressStart, deviceNameAddressEND); // read device name

    // read intervalOfMeasurement
    intervalOfMeasurement = EEPROM.readUInt(intervalOfMeasurementAddress);

    // read calibration A
    calibration_a = EEPROM.readFloat(calibrationAAddress);

    // read calibration B
    calibration_b = EEPROM.readFloat(calibrationBAddress);
  }
  else
  {
    // set default value in EEPROM

    // for device name
    char deviceName[20] = "Sonde BLE";
    writeCharToEEPROM(deviceName, deviceNameAddressStart, deviceNameAddressEND);

    // write default Interval of measurement
    intervalOfMeasurement = 1000;
    EEPROM.writeUInt(intervalOfMeasurementAddress, intervalOfMeasurement);

    // write default calibration a
    calibration_a = 1.0;
    EEPROM.writeFloat(calibrationAAddress, calibration_a);

    // write default calibration b
    calibration_b = 0.0;
    EEPROM.writeFloat(calibrationBAddress, calibration_b);

    // Change value IsNotFirstRun => 1 for save is not the fisrt run
    EEPROM.write(IsNotFirstRunAddress, 1);

    // Save modification
    EEPROM.commit();
  }
}

void setValueDefaultOrEetrom()
{
  // read device name
  // String deviceNameTemp;
  // EEPROM.get(deviceNameAddress, deviceNameTemp);
  // Serial.print("Read device Name : ");
  // Serial.println(deviceNameTemp);
  // if (deviceNameTemp.length() > 0)
  // {
  //   deviceName = deviceNameTemp;
  // }

  // Interval of measurement
  unsigned int intervalOfMeasurementTemp;
  EEPROM.get(intervalOfMeasurementAddress, intervalOfMeasurementTemp);
  Serial.print("Read interval measurement : ");
  Serial.println(intervalOfMeasurementTemp);
  if (intervalOfMeasurementTemp > 30)
  {
    intervalOfMeasurement = intervalOfMeasurementTemp;
  }

  // Calibration a
  float calibration_a_temp;
  EEPROM.get(calibrationAAddress, calibration_a_temp);
  Serial.print("Read calibration a : ");
  Serial.println(calibration_a_temp);
  if (calibration_a_temp)
  {
    calibration_a = calibration_a_temp;
  }
}

// Convert float to char to be send by BLE
char *convertFloatToString(float value)
{
  char *valueChar = new char[8];
  dtostrf(value, 1, 2, valueChar);
  return valueChar;
}

// Convert int to char to be sens by BLE
char *convertIntergerToChar(int value)
{
  char *valueChar = new char[8];
  itoa(value, valueChar, 10);
  return valueChar;
}

// Get sensor adress
String GetDeviceAddressToString()
{

  uint8_t address[8];
  String dataString;

  if (oneWire.search(address))
  {
    oneWire.search(address);
    for (uint8_t i = 0; i < 8; i++)
    {
      dataString += String(address[i], HEX);
    }
  }
  return dataString;
}

// Define Server Callbacks
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

// Define Characteristique Callbacks
class DeviceNameCharacteristicCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0 && value.length() <= 20)
    {
      const char *deviceNameTemp = value.c_str();
      strcpy(deviceName, deviceNameTemp);

      // for (int i = 0; i < sizeof(deviceName); i++)
      // {
      //   if (i < sizeof(deviceNameTemp))
      //   {
      //     deviceName[i] = deviceNameTemp[i];
      //   }
      //   else
      //   {
      //     deviceName[i] = " ";
      //   }
      // }

      Serial.print("Device name sto : ");
      Serial.println(deviceName);

      // storage value in eetrom
      writeCharToEEPROM(deviceName, deviceNameAddressStart, deviceNameAddressEND);
    }
  }
};

class IntervalCharacteristicCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0)
    {
      String StringValue = value.c_str();
      Serial.println(StringValue);
      if (StringValue.toInt())
      {
        intervalOfMeasurement = StringValue.toInt();

        // storage value in eetrom
        EEPROM.writeUInt(intervalOfMeasurementAddress, intervalOfMeasurement);
        EEPROM.commit();
      }
    }
  }
};

class CalibrationACharacteristicCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0)
    {
      String StringValue = value.c_str();
      Serial.println(StringValue);
      calibration_a = StringValue.toFloat();
      Serial.println(calibration_a);

      // storage value in eetrom
      EEPROM.writeFloat(calibrationAAddress, calibration_a);
      EEPROM.commit();
    }
  }
};

class CalibrationBCharacteristicCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0)
    {
      String StringValue = value.c_str();
      calibration_b = StringValue.toFloat();
      Serial.println(calibration_b);

      // storage value in eetrom
      EEPROM.writeFloat(calibrationBAddress, calibration_b);
      EEPROM.commit();
    }
  }
};

void setup()
{
  Serial.begin(115200); // run serial communication
  sensors.begin();      // start ic2 service
  sensors.requestTemperatures();

  // Start eeprom
  EEPROM.begin(EEPROM_SIZE);

  // set value default or eetrom
  // setValueDefaultOrEetrom();
  initEEPROM();

  // read serial number of sensor
  if (sensors.getDeviceCount() > 0)
  {
    sensors.getAddress(sensorSerial, 0);
  }

  // Create the BLE Device
  BLEDevice::init(std::string(deviceName)); // create and give name off ble device

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create characteristic for device name
  BLECharacteristic *DeviceNameCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_DEVICE_NAME,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  DeviceNameCharacteristic->setCallbacks(new DeviceNameCharacteristicCallbacks());
  DeviceNameCharacteristic->setValue(std::string(deviceName));

  // Create characteristic for interval of measurement
  BLECharacteristic *IntervalOfMeasurementCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_INTERVAL_MEASUREMENT,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  IntervalOfMeasurementCharacteristic->setCallbacks(new IntervalCharacteristicCallbacks());
  IntervalOfMeasurementCharacteristic->setValue(convertIntergerToChar(intervalOfMeasurement));

  // Create characteristic for calibration a
  BLECharacteristic *CalibrationACharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_CALIBRATION_A,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  CalibrationACharacteristic->setCallbacks(new CalibrationACharacteristicCallbacks());
  CalibrationACharacteristic->setValue(convertFloatToString(calibration_a));

  // Create characteristic for calibration a
  BLECharacteristic *CalibrationBCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_CALIBRATION_B,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  CalibrationBCharacteristic->setCallbacks(new CalibrationBCharacteristicCallbacks());
  CalibrationBCharacteristic->setValue(convertFloatToString(calibration_b));

  // Create characteristic for sensor serial
  BLECharacteristic *sensorSerialCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_SENSOR_SERIAL,
      BLECharacteristic::PROPERTY_READ);

  // Get address of sensors
  String strSerial;
  while (strSerial == "")
  {
    delay(100);
    strSerial = GetDeviceAddressToString();
    Serial.println(strSerial);
  }

  sensorSerialCharacteristic->setValue(std::string(strSerial.c_str()));

  // Create characteristic for notif temperature
  TemperatureCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TEMPERATURE,
      BLECharacteristic::PROPERTY_NOTIFY);

  TemperatureCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop()
{
  if (deviceConnected == true)
  {
    Serial.println("Send temperature");
    notifyTemperature();
    Serial.println(intervalOfMeasurement);
    delay(intervalOfMeasurement);
  }
}

// define fonction

float getTemperature()
{
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0) * calibration_a + calibration_b;
}

void notifyTemperature()
{
  TemperatureCharacteristic->setValue(convertFloatToString(getTemperature()));
  TemperatureCharacteristic->notify();
}
