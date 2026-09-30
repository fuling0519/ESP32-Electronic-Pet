#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "pet/PetData.h"
class BLECharacteristic;
class BLEServer;
namespace Ble {
class Link {
public:
    bool init();
    void update(const Pet::PetData& pet, uint32_t nowMs);
private:
    static constexpr uint16_t kMaxJson = 1024;
    static constexpr uint16_t kMaxFrame = 20;
    static constexpr uint8_t kHeader = 4, kChunk = 16, kQueueSize = 8;
    struct RxFrame { uint16_t length; uint8_t bytes[kMaxFrame]; };
    struct TxMessage { uint16_t length; bool status; char json[kMaxJson + 1]; };
    class ServerCallbacks;
    class CommandCallbacks;
    friend class ServerCallbacks;
    friend class CommandCallbacks;
    void connected();
    void disconnected();
    void onWrite(BLECharacteristic* characteristic);
    void receive(const RxFrame& frame, uint32_t nowMs);
    void processCommand();
    void result(uint32_t id, bool hasId, const char* code);
    bool queue(const char* json, bool status);
    bool makeStatus(const Pet::PetData& pet, char* out, size_t cap, uint16_t& len) const;
    void publishStatus(const Pet::PetData& pet, uint32_t nowMs);
    void sendFrame(uint32_t nowMs);
    void resetRx();

    QueueHandle_t rxQueue_ = nullptr;
    BLEServer* server_ = nullptr;
    BLECharacteristic* command_ = nullptr;
    BLECharacteristic* event_ = nullptr;
    volatile bool connected_ = false;
    bool observedConnected_ = false;
    bool rxActive_ = false;
    uint16_t rxMessageId_ = 0, rxLength_ = 0;
    uint8_t rxCount_ = 0, rxNext_ = 0;
    uint32_t rxStartedAt_ = 0;
    char rxJson_[kMaxJson + 1]{};
    TxMessage tx_[kQueueSize]{}, current_{};
    uint8_t txHead_ = 0, txTail_ = 0, txCount_ = 0;
    bool txActive_ = false, forceStatus_ = false;
    uint16_t txMessageId_ = 0;
    uint8_t txIndex_ = 0, txCountFrames_ = 0;
    uint32_t lastFrameAt_ = 0, lastStatusAt_ = 0;
    char lastStatus_[kMaxJson + 1]{};
};
}  // namespace Ble
