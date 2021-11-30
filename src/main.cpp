/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <iostream>
#include <string>

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
char *devive_name = "ESP32 UART Test";

// Temperature sensor
#define ONE_WIRE_BUS 15
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
char temperature;
DeviceAddress sensorAddress;

//std::string rxValue; // Could also make this a global var to access it in loop()

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Define commands
const String INTERVALE = "intervale";
const String COR_A = "cor_a";
const String COR_B = "cor_b";
const String HL = "HL";
const String LL = "LL";
const String SENSOR_SERIAL = "sensor_serial";
const String DEVICE_NAME = "device_name";
const String TEMP = "TEMP";

// header function
float getTemperature(); // Read temperature of sensor
void sendTemperature();
void sendDeviceName();

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

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0)
    {
      Serial.println("*********");
      Serial.print("Received Value: ");

      for (int i = 0; i < rxValue.length(); i++)
      {
        Serial.print(rxValue[i]);
      }

      Serial.println();

      // Permet de determiner la commande et la valeur associée
      std::string command = "";
      std::string command_value = "";

      if (rxValue.find("=") != -1)
      {
        command = rxValue.substr(0, rxValue.find("="));
        command_value = rxValue.substr(rxValue.find("=") + 1, rxValue.length());
      }
      else
      {
        command = rxValue;
      }

      // Do stuff based on the command received from the app
      if (INTERVALE.equals(command.c_str()))
      {
        Serial.print("Intervale Commande");
      }
      else if (COR_A.equals(command.c_str()))
      {
        Serial.print("Correction a");
      }
      else if (COR_B.equals(command.c_str()))
      {
        Serial.print("Correction b");
      }
      else if (HL.equals(command.c_str()))
      {
        Serial.print("Read high limit");
      }
      else if (LL.equals(command.c_str()))
      {
        Serial.print("Read low limit");
      }
      else if (SENSOR_SERIAL.equals(command.c_str()))
      {
        Serial.print("Read Serial Number");
      }
      else if (DEVICE_NAME.equals(command.c_str()))
      {
        Serial.print("Read Device Name");
        sendDeviceName();
      }
      else if (TEMP.equals(command.c_str()))
      {
        Serial.print("Read Temperature");
        sendTemperature();
      }
      Serial.println();
      Serial.println("*********");
    }
  }
};

void setup()
{
  Serial.begin(115200);

  sensors.begin();

  // Create the BLE Device
  BLEDevice::init("devive_name"); // Give it a name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX,
      BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop()
{
  if (deviceConnected)
  {
    // Fabricate some arbitrary junk for now...

    // temperature = getTemperature();

    // sendTemperature();

    // Let's convert the value to a char array:
    // char txString[8];                     // make sure this is big enuffz
    // dtostrf(temperature, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer

    //    pCharacteristic->setValue(&txValue, 1); // To send the integer value
    //    pCharacteristic->setValue("Hello!"); // Sending a test message
    // pCharacteristic->setValue(txString);

    // pCharacteristic->notify(); // Send the value to the app!
    // Serial.print("*** Sent Value: ");
    // Serial.print(temperature);
    // Serial.println(" ***");

    // You can add the rxValue checks down here instead
    // if you set "rxValue" as a global var at the top!
    // Note you will have to delete "std::string" declaration
    // of "rxValue" in the callback function.
    //    if (rxValue.find("A") != -1) {
    //      Serial.println("Turning ON!");
    //      digitalWrite(LED, HIGH);
    //    }
    //    else if (rxValue.find("B") != -1) {
    //      Serial.println("Turning OFF!");
    //      digitalWrite(LED, LOW);
    //    }
  }
  delay(10000);
}

// define fonction

float getTemperature()
{
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

void sendFormatedData(char data[8])
{
  pCharacteristic->setValue(data);
  pCharacteristic->notify();
}

void sendTemperature()
{
  float temperature = getTemperature();
  char command[30] = "TEMP=";
  char txString[8];
  dtostrf(temperature, 1, 2, txString);
  strcat(command, txString);

  pCharacteristic->setValue(command);
  pCharacteristic->notify();
}

void sendDeviceName()
{
  char command[30] = "device_name=";
  strcat(command, devive_name);

  pCharacteristic->setValue(command);
  pCharacteristic->notify();
}