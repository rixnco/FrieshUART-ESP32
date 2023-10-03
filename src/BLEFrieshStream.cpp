#include "BLEFrieshStream.h"

#include <BLE2902.h>

BLEFrieshStream::BLEFrieshStream(BLEServer *pServer) : 
    _pServer(pServer), 
    _pService(nullptr), 
    _pRXChar(nullptr), 
    _pTXChar(nullptr),
    _rxhead(0), 
    _rxtail(0), 
    _rxlen(0)
{
  // Create the BLE Stream Service
  _pService = pServer->createService(FRIESH_SERVICE_UUID);

  // Create the BLE Stream RX Characteristic
  _pRXChar = _pService->createCharacteristic(
      FRIESH_RX_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_WRITE_NR);

  // Create the BLE Stream TX Characteristic
  _pTXChar = _pService->createCharacteristic(
      FRIESH_TX_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_NOTIFY);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  _pTXChar->addDescriptor(new BLE2902());

  _txSemaphore.give();

  _pRXChar->setCallbacks(this);
  _pTXChar->setCallbacks(this);

  // Start the service
  _pService->start();
}

BLEFrieshStream::~BLEFrieshStream() {
  // TODO clean resources !!!
}

int BLEFrieshStream::read()
{
  int c = -1;
  _rxSemaphore.take();
  if (_rxlen)
  {
    c = _rxbuffer[_rxtail];
    _rxlen -= 1;
    _rxtail = (_rxtail + 1) % BLE_FRIESH_STREAM_RX_BUFFER_LEN;
  }
  _rxSemaphore.give();
  return c;
}

int BLEFrieshStream::peek()
{
  int c = -1;
  _rxSemaphore.take();
  if (_rxlen)
  {
    c = _rxbuffer[_rxtail];
  }
  _rxSemaphore.give();
  return c;
}

void BLEFrieshStream::flush() 
{ 
    _rxhead=_rxtail=_rxlen=0; 
}

size_t BLEFrieshStream::write(uint8_t c)
{
  return write(&c, 1);
}

size_t BLEFrieshStream::write(const uint8_t *buffer, size_t len)
{
  int offset = 0;
//  int mtu = FRIESH_MTU;
  int mtu = _pServer->getPeerMTU(_pServer->getConnId()) - 3;
  while (len > 0)
  {
    // Serial.println("waiting sem");
    _txSemaphore.take();
    int l = len > mtu ? mtu : len;
    _pTXChar->setValue((uint8_t *)&buffer[offset], l);
    _pTXChar->notify(true);
    offset += l;
    len -= l;
  }
  return len;
}

void BLEFrieshStream::onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
{

  uint16_t len = param->write.len;
  uint8_t *pvalue = param->write.value;
  //    Serial.println((char*)pvalue);
  _rxSemaphore.take();
  for (int t = 0; t < len; ++t)
  {
    _rxbuffer[_rxhead] = pvalue[t];
    _rxhead = (_rxhead + 1) % BLE_FRIESH_STREAM_RX_BUFFER_LEN;
    if (_rxlen >= BLE_FRIESH_STREAM_RX_BUFFER_LEN)
    {
      // overflow !!!
      _rxtail = (_rxtail + 1) % BLE_FRIESH_STREAM_RX_BUFFER_LEN;
    }
    else
    {
      _rxlen += 1;
    }
  }
  _rxSemaphore.give();
}

void BLEFrieshStream::onStatus(BLECharacteristic *pCharacteristic, Status s, uint32_t code)
{
  _txSemaphore.give();
}
