#pragma once
#include <Arduino.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "pet/PetData.h"
#include "ble/CareCommand.h"
#include "ble/GameCommand.h"
#include "ble/LifecycleCommand.h"
#include "ble/MemorialCommand.h"
#include "ble/Messages.h"
#include "ble/OpCache.h"
#include "ble/PetEvents.h"
#include "ble/SoundCommand.h"
#include "ble/TxQueue.h"
class NimBLECharacteristic;
class NimBLEServer;
namespace Ble {
// One parsed command for the application: a Web play command or a new care/lifecycle/memorial op.
struct Request {
    enum class Kind : uint8_t { None, Game, Care, Lifecycle, Memorial, Sound };
    Kind kind = Kind::None;
    GameRequest game{};
    CareRequest care{};
    LifecycleRequest lifecycle{};
    MemorialRequest memorial{};
    SoundRequest sound{};
};
class Link {
public:
    bool init();
    void stop();
    void update(const Pet::PetData& pet, uint32_t nowMs);
    bool isConnected() const { return connected_; }

    // One parsed command waits here until the application takes it. Game commands are
    // answered with reply() plus, on success, the matching event; a care or lifecycle op with
    // reply() when rejected, or accept...() and later finish...() exactly once.
    bool takeRequest(Request& request);
    void reply(uint32_t commandId, const char* code);
    void sendGameSession(uint32_t commandId, uint32_t session, const char* game, uint32_t minMs);
    void sendGameResult(uint32_t commandId, uint32_t session, const Games::WebGameResult& result);
    void sendSessionEnd(uint32_t session, const char* reason);
    // command_result ok and the op recorded as pending (replays answer ok until finished).
    void acceptCare(const CareRequest& request);
    // Records the outcome for replays and sends action_result with the latest command id.
    void finishCare(const CareRequest& request, const CareOutcome& outcome);
    void acceptLifecycle(const LifecycleRequest& request);
    void finishLifecycle(const LifecycleRequest& request, const LifecycleOutcome& outcome);
    void acceptMemorial(const MemorialRequest& request);
    void finishMemorial(const MemorialRequest& request, const MemorialOutcome& outcome);
    void acceptSound(const SoundRequest& request);
    void finishSound(const SoundRequest& request, const SoundOutcome& outcome);
    // The saved sound mode for the status "sound" field (the Volume screen's unsaved audition is not it).
    void setSoundMuted(bool muted) { statusSources_.sound = muted ? "muted" : "on"; }
    // idle, menu, sleeping, ended, device_game, web_game or busy; in every status snapshot.
    void setActivity(const char* activity);
    // Pet sync status fields (sad reasons, memorial count/hash); both must outlive the link.
    void attachStatusSources(const Pet::SadState& sad, const Storage::Memorials& memorials);
    // Sent only while connected and subscribed, and dropped when the queue is nearly full.
    void sendPetEvent(const PetEvent& event);
private:
    static constexpr uint16_t kMaxJson = 1024;
    static constexpr uint16_t kMaxFrame = 20;
    static constexpr uint8_t kHeader = 4, kChunk = 16, kQueueSize = 8;
    struct RxFrame { uint16_t length; uint8_t bytes[kMaxFrame]; };
    class ServerCallbacks;
    class CommandCallbacks;
    class EventCallbacks;
    friend class ServerCallbacks;
    friend class CommandCallbacks;
    friend class EventCallbacks;
    void connected();
    void disconnected();
    void onWrite(NimBLECharacteristic* characteristic);
    void receive(const Pet::PetData& pet, const RxFrame& frame, uint32_t nowMs);
    void processCommand(const Pet::PetData& pet);
    void result(uint32_t id, bool hasId, const char* code);
    void processCare(JsonVariantConst message, uint32_t id);
    void processLifecycle(JsonVariantConst message, uint32_t id, LifecycleAction action);
    void processGetMemorials(const Pet::PetData& pet, JsonVariantConst message, uint32_t id);
    void processDeleteMemorial(JsonVariantConst message, uint32_t id);
    void processSetSound(JsonVariantConst message, uint32_t id);
    // Op cache lookup shared by the write commands: true when this command was answered here.
    bool answeredFromCache(uint64_t op, OpKind kind, uint8_t action, uint64_t target, uint32_t id);
    void sendActionResult(uint32_t id, const OpCache::Entry& entry);
    bool queue(const char* json, TxKind kind);
    bool makeStatus(const Pet::PetData& pet, char* out, size_t cap, uint16_t& len) const;
    void publishStatus(const Pet::PetData& pet, uint32_t nowMs);
    void sendFrame(uint32_t nowMs);
    void resetRx();

    char deviceId_[19]{};  // esp32- followed by the factory MAC in uppercase hex.
    QueueHandle_t rxQueue_ = nullptr;
    NimBLEServer* server_ = nullptr;
    NimBLECharacteristic* command_ = nullptr;
    NimBLECharacteristic* event_ = nullptr;
    std::atomic<bool> connected_{false}, subscribed_{false};
    bool observedConnected_ = false;
    bool rxActive_ = false;
    uint16_t rxMessageId_ = 0, rxLength_ = 0;
    uint8_t rxCount_ = 0, rxNext_ = 0;
    uint32_t rxStartedAt_ = 0;
    char rxJson_[kMaxJson + 1]{};
    TxQueue tx_;
    TxQueue::Message current_{};
    bool txActive_ = false, forceStatus_ = false;
    uint16_t txMessageId_ = 0;
    uint8_t txIndex_ = 0, txCountFrames_ = 0;
    uint32_t lastFrameAt_ = 0, lastStatusAt_ = 0;
    uint32_t lastHeapAt_ = 0;
    char lastStatus_[kMaxJson + 1]{};
    Request request_{};
    OpCache ops_;  // Survives reconnects: a client retries the same op after reconnecting.
    const char* activity_ = "busy";
    StatusSources statusSources_{};
};
}  // namespace Ble
