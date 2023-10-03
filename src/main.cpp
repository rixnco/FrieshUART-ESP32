#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <Adafruit_NeoPixel.h>

#include "BLEFrieshStream.h"


class BLEFrieshConnection : public BLEServerCallbacks
{
public:
    BLEFrieshConnection();

    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) override;
    void onDisconnect(BLEServer *pServer) override;

    bool isConnected();
protected:
    bool _connected;
};


BLEServer               *pFrieshServer;
BLEFrieshConnection     frieshConnection;
bool                    g_frieshClientConnected = false;
Stream                  *pFrieshStream;


#if defined(ARDUINO_LOLIN_C3_PICO)
#define LED_RGB   LED_BUILTIN
Adafruit_NeoPixel pixels(1, LED_RGB, NEO_RGB + NEO_KHZ800);
static bool __cdc_serial= false;
#endif

static bool     __last_blink_state = false;
static uint32_t __last_blink_time = 0;

#if defined(ARDUINO_LOLIN_C3_SUPERMINI) || defined(ARDUINO_LOLIN_C3_MINI)
#define SET_LED(state) do { \
    digitalWrite(LED_BUILTIN, state); \
  } while(0)
#elif defined(ARDUINO_LOLIN_C3_PICO)
#define SET_LED(state) do { \
    pixels.setPixelColor(0, state?pixels.Color(0, __cdc_serial?50:0, 50):pixels.Color(0, __cdc_serial?50:0, 0)); \
    pixels.show(); \
  } while(0)
#endif





void setup() {
#if defined(ARDUINO_LOLIN_C3_SUPERMINI) || defined(ARDUINO_LOLIN_C3_MINI)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(0);
#elif defined(ARDUINO_LOLIN_C3_PICO)
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setPixelColor(0, pixels.Color(50, 0, 0));
  pixels.show(); 
#endif

  Serial0.begin(115200);



  Serial.begin(115200);
  delay(3000);
  if(Serial)
  {
    __cdc_serial = true;
    SET_LED(__last_blink_state);
  }

  Serial.println("Starting...");
  // Initializes the BLE Stack
  Serial.print("Init BLE stack...");
  BLEDevice::init("FrieshUART");
  Serial.println("...OK");

  // Initializes the Friesh Stream server
  Serial.print("Create server...");
  pFrieshServer = BLEDevice::createServer();
  Serial.println("...OK");
  Serial.print("set callbacks...");
  pFrieshServer->setCallbacks(&frieshConnection);
  Serial.println("...OK");
  Serial.print("Create FrieshStream...");
  pFrieshStream= new BLEFrieshStream(pFrieshServer);
  Serial.println("...OK");
  g_frieshClientConnected = false;
  delay(500);

  Serial.print("Advertising the friesh Server");
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(FRIESH_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Started...OK");

  __last_blink_state = true;
  SET_LED(__last_blink_state);
}


void loop() {
  uint32_t now = millis();

  if(__cdc_serial != Serial)
  {
    __cdc_serial = Serial;
    SET_LED(__last_blink_state);
  }
  
  // disconnecting
  if (g_frieshClientConnected && !frieshConnection.isConnected())
  {
      Serial.println("start advertising");
      g_frieshClientConnected = false;
      pFrieshServer->startAdvertising(); // restart advertising
      __last_blink_time = now;
      __last_blink_state = true;
      SET_LED(__last_blink_state);
      return;
  }
  // connecting
  if (!g_frieshClientConnected && frieshConnection.isConnected())
  {
      __last_blink_state = true;
      SET_LED(__last_blink_state);
      Serial.println("Stop advertising");
      // do stuff here on connecting
      BLEDevice::getAdvertising()->stop();
      g_frieshClientConnected = true;
      return;
  }
  // connected
  if (g_frieshClientConnected && frieshConnection.isConnected())
  {
    int mtu = pFrieshServer->getPeerMTU(pFrieshServer->getConnId()) - 3;
    uint8_t buffer[mtu];
    int16_t idx = -1;

    // Process Serial0 input
    while(Serial0.available())
    {
      buffer[++idx] = Serial0.read();
      if(idx+1>=mtu)
      {
        pFrieshStream->write(buffer, idx+1);
        idx =-1;
      }
    }
    if(idx+1>0)
    {
        pFrieshStream->write(buffer, idx+1);
        idx =-1;
    }

    // Process Serial input
    while(Serial.available())
    {
      buffer[++idx] = Serial.read();
      if(idx+1>=mtu)
      {
        pFrieshStream->write(buffer, idx+1);
        idx =-1;
      }
    }
    if(idx+1>0)
    {
        pFrieshStream->write(buffer, idx+1);
        idx =-1;
    }


    // Process FrieshUART input
    while(pFrieshStream->available())
    {
      int c = pFrieshStream->read();
      Serial0.write(c);
      Serial.write(c);
    }
    return;
  }
  if(now-__last_blink_time>250)
  {
    __last_blink_time = now;
    __last_blink_state = !__last_blink_state;
    SET_LED(__last_blink_state);
  }

}

BLEFrieshConnection::BLEFrieshConnection() : _connected(false)
{
}

void BLEFrieshConnection::onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param)
{
    uint64_t addr =  (*(uint64_t*)param->connect.remote_bda) & 0x0000FFFFFFFFFFFF;
    _connected = true;
    Serial.printf("FrieshClient connected: %012X\n", addr);
};

void BLEFrieshConnection::onDisconnect(BLEServer *pServer)
{
    _connected = false;
    Serial.println("FrieshClient disconnected...");
}

bool BLEFrieshConnection::isConnected() 
{
    return _connected;
}