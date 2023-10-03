#ifndef __BLE_FRIESH_STREAM_H__
#define __BLE_FRIESH_STREAM_H__

#include <Arduino.h>
#include <BLEDevice.h>


#define FRIESH_SERVICE_UUID "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define FRIESH_RX_CHARACTERISTIC_UUID "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define FRIESH_TX_CHARACTERISTIC_UUID "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

#define FRIESH_MTU      20
#define BLE_FRIESH_STREAM_RX_BUFFER_LEN   64


class BLEFrieshStream : public Stream, BLECharacteristicCallbacks
{
public:
    BLEFrieshStream(BLEServer *pServer) ;
    virtual ~BLEFrieshStream() ;

    inline int available() { return _rxlen; };
    int read() override;
    int peek() override;
    void flush() override;

    size_t write(uint8_t c) override;

    size_t write(const uint8_t *buffer, size_t len) override;

    void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) override;

    void onStatus(BLECharacteristic *pCharacteristic, Status s, uint32_t code) override;

protected:
    BLEServer  *_pServer;
    BLEService *_pService;
    BLECharacteristic *_pRXChar;
    BLECharacteristic *_pTXChar;

    uint8_t _rxbuffer[BLE_FRIESH_STREAM_RX_BUFFER_LEN];
    uint16_t _rxhead;
    uint16_t _rxtail;
    uint16_t _rxlen;

    FreeRTOS::Semaphore _rxSemaphore   = FreeRTOS::Semaphore("RX");
    FreeRTOS::Semaphore _txSemaphore  = FreeRTOS::Semaphore("TX");

};





#endif //__BLE_FRIESH_STREAM_H__