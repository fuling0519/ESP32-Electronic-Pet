#include "ble/BleLink.h"
#include "hardware/DeviceIdentity.h"
#include <ArduinoJson.h>
#include <BLE2902.h>
#include <BLECharacteristic.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEService.h>
#include <stdio.h>
#include <string.h>

namespace Ble {
namespace {
constexpr char kService[] = "7d2a0001-8b7c-4f3a-9c2d-1e5f6a7b8c90";
constexpr char kCommand[] = "7d2a0002-8b7c-4f3a-9c2d-1e5f6a7b8c90";
constexpr char kEvent[] = "7d2a0003-8b7c-4f3a-9c2d-1e5f6a7b8c90";
constexpr char kName[] = "ESP32-PET";
#ifndef PET_FIRMWARE_VERSION
#define PET_FIRMWARE_VERSION "development"
#endif
const char* stage(Pet::LifeStage v) {
    switch (v) { case Pet::LifeStage::Egg:return "egg"; case Pet::LifeStage::Baby:return "baby"; case Pet::LifeStage::Adult:return "adult"; }
    return "unknown";
}
const char* health(Pet::HealthState v) {
    switch (v) { case Pet::HealthState::Healthy:return "healthy"; case Pet::HealthState::Sick:return "sick"; case Pet::HealthState::Dead:return "dead"; }
    return "unknown";
}
const char* sleep(Pet::SleepMode v) {
    switch (v) { case Pet::SleepMode::Awake:return "awake"; case Pet::SleepMode::Normal:return "normal"; case Pet::SleepMode::Deep:return "unknown"; }
    return "unknown";
}
}

class Link::ServerCallbacks final : public BLEServerCallbacks {
public:
    explicit ServerCallbacks(Link& link): link_(link) {}
    void onConnect(BLEServer*) override { link_.connected(); }
    void onDisconnect(BLEServer*) override { link_.disconnected(); }
private: Link& link_;
};
class Link::CommandCallbacks final : public BLECharacteristicCallbacks {
public:
    explicit CommandCallbacks(Link& link): link_(link) {}
    void onWrite(BLECharacteristic* c) override { link_.onWrite(c); }
private: Link& link_;
};

bool Link::init() {
    if (!Hardware::deviceId()[0]) return false;
    rxQueue_ = xQueueCreate(kQueueSize, sizeof(RxFrame));
    if (!rxQueue_) return false;
    BLEDevice::init(kName);
    server_ = BLEDevice::createServer();
    if (!server_) return false;
    server_->setCallbacks(new ServerCallbacks(*this));
    BLEService* service = server_->createService(kService);
    if (!service) return false;
    command_ = service->createCharacteristic(kCommand, BLECharacteristic::PROPERTY_WRITE);
    event_ = service->createCharacteristic(kEvent, BLECharacteristic::PROPERTY_NOTIFY);
    if (!command_ || !event_) return false;
    command_->setCallbacks(new CommandCallbacks(*this));
    event_->addDescriptor(new BLE2902());
    service->start();
    BLEDevice::getAdvertising()->addServiceUUID(kService);
    BLEDevice::getAdvertising()->setScanResponse(true);
    BLEDevice::startAdvertising();
    return true;
}
void Link::connected() { connected_ = true; }
void Link::disconnected() { connected_ = false; }

void Link::onWrite(BLECharacteristic* c) {
    if (!rxQueue_ || !c) return;
    size_t len = c->getLength();
    const uint8_t* data = c->getData();
    if (!data || len < kHeader || len > kMaxFrame) return;
    RxFrame f{};
    f.length = static_cast<uint16_t>(len);
    memcpy(f.bytes, data, len);
    xQueueSend(rxQueue_, &f, 0);
}
void Link::resetRx() {
    rxActive_ = false; rxMessageId_ = rxLength_ = 0;
    rxCount_ = rxNext_ = 0; rxStartedAt_ = 0;
}
bool Link::queue(const char* json, bool statusMessage) {
    if (!json || !connected_ || txCount_ >= kQueueSize) return false;
    size_t len = strlen(json);
    if (!len || len > kMaxJson) return false;
    TxMessage& slot = tx_[txTail_];
    slot.length = static_cast<uint16_t>(len);
    slot.status = statusMessage;
    memcpy(slot.json, json, len + 1);
    txTail_ = (txTail_ + 1) % kQueueSize;
    ++txCount_;
    return true;
}
void Link::result(uint32_t id, bool hasId, const char* code) {
    StaticJsonDocument<192> d;
    d["v"]=1; d["type"]="command_result";
    if (hasId) d["id"]=id; else d["id"]=nullptr;
    d["ok"] = strcmp(code,"ok")==0; d["code"]=code;
    char json[kMaxJson+1];
    size_t len=serializeJson(d,json,sizeof(json));
    if (len && len<=kMaxJson) queue(json,false);
}
bool Link::makeStatus(const Pet::PetData& pet, char* out, size_t cap, uint16_t& len) const {
    StaticJsonDocument<512> d;
    d["v"]=1; d["type"]="status";
    d["device_id"]=Hardware::deviceId();
    JsonObject p=d.createNestedObject("pet");
    char id[21], age[21];
    snprintf(id,sizeof(id),"%llu",static_cast<unsigned long long>(pet.petId()));
    snprintf(age,sizeof(age),"%llu",static_cast<unsigned long long>(pet.ageSeconds()));
    p["id"]=id; p["name"]=pet.name(); p["life_stage"]=stage(pet.lifeStage());
    p["satiety"]=pet.satiety(); p["mood"]=pet.mood(); p["cleanliness"]=pet.cleanliness();
    p["age_seconds"]=age; p["health"]=health(pet.healthState());
    p["is_dead"]=pet.isDead(); p["sleep"]=sleep(pet.sleepMode());
    size_t written=serializeJson(d,out,cap);
    if (!written || written>=cap || written>kMaxJson) return false;
    len=static_cast<uint16_t>(written); return true;
}
void Link::publishStatus(const Pet::PetData& pet, uint32_t nowMs) {
    if (!forceStatus_ && nowMs-lastStatusAt_<500) return;
    lastStatusAt_=nowMs; forceStatus_=false;
    char json[kMaxJson+1]; uint16_t len=0;
    if (!makeStatus(pet,json,sizeof(json),len) || strcmp(json,lastStatus_)==0) return;
    if (queue(json,true)) memcpy(lastStatus_,json,len+1);
}

void Link::receive(const RxFrame& f, uint32_t nowMs) {
    if (f.length<kHeader) { resetRx(); return; }
    uint16_t id=f.bytes[0] | (static_cast<uint16_t>(f.bytes[1])<<8);
    uint8_t index=f.bytes[2], count=f.bytes[3];
    uint16_t dataLen=f.length-kHeader;
    if (!count || index>=count || !dataLen || dataLen>kChunk) { resetRx(); return; }
    if (rxActive_ && nowMs-rxStartedAt_>3000) resetRx();
    if (!rxActive_) {
        if (index) return;
        rxActive_=true; rxMessageId_=id; rxCount_=count; rxNext_=0;
        rxLength_=0; rxStartedAt_=nowMs;
    }
    if (id!=rxMessageId_ || count!=rxCount_ || index!=rxNext_ ||
        rxLength_+dataLen>kMaxJson) { resetRx(); return; }
    memcpy(rxJson_+rxLength_,f.bytes+kHeader,dataLen);
    rxLength_+=dataLen; ++rxNext_;
    if (rxNext_==rxCount_) {
        rxJson_[rxLength_]='\0';
        processCommand();
        resetRx();
    }
}
void Link::processCommand() {
    StaticJsonDocument<128> filter;
    filter["v"]=true; filter["id"]=true; filter["cmd"]=true;
    StaticJsonDocument<192> d;
    auto err=deserializeJson(d,rxJson_,rxLength_,DeserializationOption::Filter(filter));
    if (err || !d.is<JsonObject>()) { result(0,false,"malformed_message"); return; }
    JsonVariant v=d["v"], rid=d["id"], cmdv=d["cmd"];
    const bool hasId=rid.is<uint32_t>();
    uint32_t id=hasId ? rid.as<uint32_t>() : 0;
    if (!v.is<int>() || !hasId || !cmdv.is<const char*>()) {
        result(id,hasId,"missing_field"); return;
    }
    if (!id) { result(id,true,"invalid_value"); return; }
    if (v.as<int>()!=1) { result(id,true,"unsupported_version"); return; }
    const char* cmd=cmdv.as<const char*>();
    bool isStatus=strcmp(cmd,"get_status")==0;
    bool isInfo=strcmp(cmd,"get_device_info")==0;
    if (!isStatus && !isInfo) { result(id,true,"unknown_command"); return; }
    result(id,true,"ok");
    if (isStatus) { forceStatus_=true; return; }
    StaticJsonDocument<384> info;
    info["v"]=1; info["type"]="device_info"; info["id"]=id; info["name"]=kName;
    info["device_id"]=Hardware::deviceId();
    info["protocol_version"]=1; info["firmware_version"]=PET_FIRMWARE_VERSION;
    info["max_message_bytes"]=kMaxJson;
    char json[kMaxJson+1]; size_t len=serializeJson(info,json,sizeof(json));
    if (len && len<=kMaxJson) queue(json,false);
}
void Link::sendFrame(uint32_t nowMs) {
    if (!connected_ || !event_ || nowMs-lastFrameAt_<20) return;
    if (!txActive_) {
        if (!txCount_) return;
        current_=tx_[txHead_]; txHead_=(txHead_+1)%kQueueSize; --txCount_;
        txActive_=true; txIndex_=0; ++txMessageId_; if (!txMessageId_) ++txMessageId_;
        uint16_t count=(current_.length+kChunk-1)/kChunk;
        if (!count || count>255) { txActive_=false; return; }
        txCountFrames_=static_cast<uint8_t>(count);
    }
    uint16_t offset=txIndex_*kChunk;
    uint16_t remain=current_.length-offset;
    uint8_t payload=remain>kChunk?kChunk:static_cast<uint8_t>(remain);
    uint8_t frame[kHeader+kChunk];
    frame[0]=txMessageId_&0xff; frame[1]=txMessageId_>>8;
    frame[2]=txIndex_; frame[3]=txCountFrames_;
    memcpy(frame+kHeader,current_.json+offset,payload);
    event_->setValue(frame,kHeader+payload); event_->notify();
    lastFrameAt_=nowMs;
    if (++txIndex_>=txCountFrames_) txActive_=false;
}
void Link::update(const Pet::PetData& pet, uint32_t nowMs) {
    bool connected=connected_;
    if (connected!=observedConnected_) {
        observedConnected_=connected; resetRx();
        txHead_=txTail_=txCount_=0; txActive_=false; lastStatus_[0]='\0';
        if (connected) forceStatus_=true;
        else BLEDevice::startAdvertising();
    }
    if (!connected) {
        RxFrame discard{};
        while (rxQueue_ && xQueueReceive(rxQueue_,&discard,0)==pdTRUE) {}
        return;
    }
    RxFrame frame{};
    if (rxQueue_ && xQueueReceive(rxQueue_,&frame,0)==pdTRUE) receive(frame,nowMs);
    publishStatus(pet,nowMs);
    sendFrame(nowMs);
}
}  // namespace Ble
