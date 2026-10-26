#include "ble/PetEvents.h"
#include <ArduinoJson.h>
#include <stdio.h>
#include "ble/Messages.h"

namespace Ble {
namespace {
bool playerAction(PetEventType t) {
    return t==PetEventType::Fed || t==PetEventType::Cleaned || t==PetEventType::Treated ||
        t==PetEventType::Slept || t==PetEventType::Woke || t==PetEventType::Departed ||
        t==PetEventType::Adopted || t==PetEventType::MemorialAdded || t==PetEventType::MemorialDeleted;
}
bool contains(const uint64_t* ids, uint8_t count, uint64_t id) {
    for (uint8_t i=0;i<count;++i) if (ids[i]==id) return true;
    return false;
}
}

const char* petEventName(PetEventType t) {
    switch (t) {
        case PetEventType::Fed: return "fed"; case PetEventType::Cleaned: return "cleaned";
        case PetEventType::Treated: return "treated"; case PetEventType::Slept: return "slept";
        case PetEventType::Woke: return "woke"; case PetEventType::Hatched: return "hatched";
        case PetEventType::Grew: return "grew"; case PetEventType::LevelUp: return "level_up";
        case PetEventType::GotSick: return "got_sick"; case PetEventType::Died: return "died";
        case PetEventType::Departed: return "departed"; case PetEventType::Adopted: return "adopted";
        case PetEventType::MemorialAdded: return "memorial_added";
        case PetEventType::MemorialDeleted: return "memorial_deleted";
    }
    return "unknown";
}

bool makePetEventJson(const PetEvent& e, char* out, size_t cap, uint16_t& len) {
    StaticJsonDocument<JSON_OBJECT_SIZE(8) + 64> d;
    char id[21], memorial[21];
    snprintf(id,sizeof(id),"%llu",static_cast<unsigned long long>(e.petId));
    d["v"]=1; d["type"]="pet_event"; d["pet_id"]=id; d["event"]=petEventName(e.type);
    if (e.type==PetEventType::LevelUp) d["level"]=e.level;
    if (e.type==PetEventType::MemorialAdded || e.type==PetEventType::MemorialDeleted) {
        snprintf(memorial,sizeof(memorial),"%llu",static_cast<unsigned long long>(e.memorialPetId));
        d["memorial_pet_id"]=memorial;
    }
    if (playerAction(e.type)) d["source"]=e.fromWeb ? "web" : "device";
    if (d.overflowed()) return false;
    const size_t written=serializeJson(d,out,cap);
    if (!written || written>=cap || written>kMaxMessageBytes) return false;
    len=static_cast<uint16_t>(written); return true;
}

void PetEventDetector::reset(const Pet::PetData& pet, const Storage::Memorials& memorials) {
    ready_=true; petId_=pet.petId(); stage_=pet.lifeStage(); level_=pet.level();
    sick_=pet.isSick(); dead_=pet.isDead(); departed_=pet.isDeparted();
    memorialCount_=memorials.count();
    for (uint8_t i=0;i<memorialCount_;++i) memorialIds_[i]=memorials.at(i)->petId;
}

uint8_t PetEventDetector::detect(const Pet::PetData& pet, const Storage::Memorials& memorials,
                                 PetEvent* out, uint8_t cap) {
    if (!ready_) { reset(pet,memorials); return 0; }
    uint8_t n=0;
    const auto emit=[&](PetEventType type, uint64_t memorialId) {
        if (n>=cap) return;
        PetEvent& e=out[n++]; e=PetEvent{}; e.type=type; e.petId=pet.petId();
        e.level=pet.level(); e.memorialPetId=memorialId;
    };
    if (pet.petId()!=petId_) emit(PetEventType::Adopted,0);  // A new pet: no carried-over diffs.
    else {
        const Pet::LifeStage s=pet.lifeStage();
        if (stage_==Pet::LifeStage::Egg && s!=Pet::LifeStage::Egg) emit(PetEventType::Hatched,0);
        if (stage_!=Pet::LifeStage::Adult && s==Pet::LifeStage::Adult) emit(PetEventType::Grew,0);
        if (pet.level()>level_) emit(PetEventType::LevelUp,0);
        if (!sick_ && pet.isSick()) emit(PetEventType::GotSick,0);
        if (!dead_ && pet.isDead()) emit(PetEventType::Died,0);
        if (!departed_ && pet.isDeparted()) emit(PetEventType::Departed,0);
    }
    uint64_t now[Storage::kMemorialCapacity];
    const uint8_t count=memorials.count();
    for (uint8_t i=0;i<count;++i) now[i]=memorials.at(i)->petId;
    for (uint8_t i=0;i<memorialCount_;++i)
        if (!contains(now,count,memorialIds_[i])) emit(PetEventType::MemorialDeleted,memorialIds_[i]);
    for (uint8_t i=0;i<count;++i)
        if (!contains(memorialIds_,memorialCount_,now[i])) emit(PetEventType::MemorialAdded,now[i]);
    reset(pet,memorials);
    return n;
}

}  // namespace Ble
