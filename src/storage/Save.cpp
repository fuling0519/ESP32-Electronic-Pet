#include "storage/Save.h"

#include <Arduino.h>
#include <stddef.h>
#include <string.h>
#include <nvs.h>

#include "storage/SaveFormat.h"

namespace Storage {
namespace {

#if defined(PET_WYVERN_TEST_MODE)
#if defined(PET_TREATMENT_TEST_MODE)
#if defined(PET_TREATMENT_TEST_BABY)
constexpr char kNamespace[] = "pet-w-treat-b";
#else
constexpr char kNamespace[] = "pet-w-treat-a";
#endif
#elif defined(PET_SAD_TEST_BABY)
constexpr char kNamespace[] = "pet-w-sad-b";
#else
constexpr char kNamespace[] = "pet-w-sad-a";
#endif
#elif defined(PET_SAD_TEST_MODE)
#if defined(PET_SAD_TEST_BABY)
constexpr char kNamespace[] = "pet-sad-b";
#else
constexpr char kNamespace[] = "pet-sad-a";
#endif
#elif defined(PET_TREATMENT_TEST_MODE)
#if defined(PET_TREATMENT_TEST_BABY)
constexpr char kNamespace[] = "pet-treat-b";
#else
constexpr char kNamespace[] = "pet-treat-a";
#endif
#elif defined(PET_DEEP_SLEEP_TEST_MODE)
constexpr char kNamespace[] = "pet-deep";
#elif defined(PET_DEATH_TEST_MODE)
constexpr char kNamespace[] = "pet-test";
#elif defined(PET_MEMORIAL_TEST_MODE)
constexpr char kNamespace[] = "pet-mem";
#elif defined(PET_SLEEP_TEST_MODE)
constexpr char kNamespace[] = "pet-sleep";
#elif defined(PET_GROWTH_TEST_MODE)
constexpr char kNamespace[] = "pet-grow";
#else
constexpr char kNamespace[] = "pet-save";
#endif
constexpr char kSlotAKey[] = "slot_a";
constexpr char kSlotBKey[] = "slot_b";
constexpr char kMemorialSlotAKey[] = "mem_a";
constexpr char kMemorialSlotBKey[] = "mem_b";
constexpr size_t kHeaderSize = 12;
constexpr size_t kCrcOffset = kPetSaveRecordSize - sizeof(uint32_t);
static_assert(kHeaderSize + kPetSavePayloadSize + sizeof(uint32_t) ==
              kPetSaveRecordSize,
              "Save record constants do not describe the same byte layout");
constexpr uint32_t kMemorialSaveMagic = 0x4D454D31UL;  // "MEM1"
constexpr uint16_t kMemorialSaveVersion = 2;
constexpr size_t kMemorialEntrySize = sizeof(uint64_t) +
    (Pet::kPetNameMaxLength + 1) + 3 * sizeof(uint8_t) + sizeof(uint64_t);
constexpr size_t kMemorialPayloadSize = 1 +
    Storage::kMemorialCapacity * kMemorialEntrySize;
constexpr size_t kMemorialRecordSize = kHeaderSize + kMemorialPayloadSize +
    sizeof(uint32_t);
static_assert(kMemorialRecordSize == kMemorialSaveRecordSize, "Memorial workspace size mismatch");
constexpr size_t kMemorialCrcOffset = kMemorialRecordSize - sizeof(uint32_t);

class ByteWriter {
public:
    ByteWriter(uint8_t* bytes, size_t capacity) : bytes_(bytes), capacity_(capacity) {}

    bool putU8(uint8_t value) { return putBytes(&value, sizeof(value)); }
    bool putU16(uint16_t value) {
        return putU8(static_cast<uint8_t>(value)) &&
               putU8(static_cast<uint8_t>(value >> 8));
    }
    bool putU32(uint32_t value) {
        for (uint8_t shift = 0; shift < 32; shift += 8) {
            if (!putU8(static_cast<uint8_t>(value >> shift))) return false;
        }
        return true;
    }
    bool putU64(uint64_t value) {
        for (uint8_t shift = 0; shift < 64; shift += 8) {
            if (!putU8(static_cast<uint8_t>(value >> shift))) return false;
        }
        return true;
    }
    bool putBytes(const void* source, size_t length) {
        if (position_ + length > capacity_) return false;
        memcpy(bytes_ + position_, source, length);
        position_ += length;
        return true;
    }
    size_t position() const { return position_; }

private:
    uint8_t* bytes_;
    size_t capacity_;
    size_t position_ = 0;
};

class ByteReader {
public:
    ByteReader(const uint8_t* bytes, size_t length) : bytes_(bytes), length_(length) {}

    bool getU8(uint8_t& value) { return getBytes(&value, sizeof(value)); }
    bool getU16(uint16_t& value) {
        uint8_t low = 0;
        uint8_t high = 0;
        if (!getU8(low) || !getU8(high)) return false;
        value = static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8);
        return true;
    }
    bool getU32(uint32_t& value) {
        value = 0;
        for (uint8_t shift = 0; shift < 32; shift += 8) {
            uint8_t byte = 0;
            if (!getU8(byte)) return false;
            value |= static_cast<uint32_t>(byte) << shift;
        }
        return true;
    }
    bool getU64(uint64_t& value) {
        value = 0;
        for (uint8_t shift = 0; shift < 64; shift += 8) {
            uint8_t byte = 0;
            if (!getU8(byte)) return false;
            value |= static_cast<uint64_t>(byte) << shift;
        }
        return true;
    }
    bool getBytes(void* destination, size_t length) {
        if (position_ + length > length_) return false;
        memcpy(destination, bytes_ + position_, length);
        position_ += length;
        return true;
    }
    size_t position() const { return position_; }

private:
    const uint8_t* bytes_;
    size_t length_;
    size_t position_ = 0;
};

uint32_t crc32(const uint8_t* bytes, size_t length) {
    uint32_t crc = 0xFFFFFFFFUL;
    for (size_t i = 0; i < length; ++i) {
        crc ^= bytes[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320UL & (0U - (crc & 1U)));
        }
    }
    return ~crc;
}

bool writeTimestamp(ByteWriter& writer, const Pet::SavedTimestamp& timestamp) {
    return writer.putU64(timestamp.unixSeconds) && writer.putU8(timestamp.valid ? 1 : 0);
}

bool readTimestamp(ByteReader& reader, Pet::SavedTimestamp& timestamp) {
    uint8_t valid = 0;
    if (!reader.getU64(timestamp.unixSeconds) || !reader.getU8(valid) || valid > 1) return false;
    timestamp.valid = valid == 1;
    return true;
}

bool writeMemorial(ByteWriter& writer, const MemorialRecord& memorial) {
    return writer.putU64(memorial.petId) &&
           writer.putBytes(memorial.name, sizeof(memorial.name)) &&
           writer.putU8(static_cast<uint8_t>(memorial.speciesId)) &&
           writer.putU64(memorial.ageSeconds) &&
           writer.putU8(static_cast<uint8_t>(memorial.kind)) && writer.putU8(memorial.stage);
}

bool readMemorial(ByteReader& reader, MemorialRecord& memorial, uint16_t version) {
    uint8_t species = 0;
    if (!reader.getU64(memorial.petId) ||
        !reader.getBytes(memorial.name, sizeof(memorial.name)) ||
        !reader.getU8(species) || !reader.getU64(memorial.ageSeconds)) {
        return false;
    }
    memorial.speciesId = static_cast<Pet::SpeciesId>(species);
    if (version == 2) {
        uint8_t kind = 0;
        if (!reader.getU8(kind) || !reader.getU8(memorial.stage)) return false;
        memorial.kind = static_cast<FarewellKind>(kind);
    }
    return true;
}

bool writeSnapshot(ByteWriter& writer, const Pet::PetSnapshotV1& snapshot) {
    return writer.putU64(snapshot.petId) &&
           writer.putBytes(snapshot.name, sizeof(snapshot.name)) &&
           writer.putU8(static_cast<uint8_t>(snapshot.speciesId)) &&
           writer.putU8(static_cast<uint8_t>(snapshot.lifeStage)) &&
           writer.putU8(snapshot.satiety) && writer.putU8(snapshot.mood) &&
           writer.putU8(snapshot.cleanliness) && writer.putU8(snapshot.level) &&
           writer.putU16(snapshot.exp) &&
           writer.putU8(static_cast<uint8_t>(snapshot.healthState)) &&
           writer.putU64(snapshot.ageSeconds) &&
           writer.putU32(snapshot.satietyRemainderSeconds) &&
           writer.putU32(snapshot.cleanlinessRemainderSeconds) &&
           writer.putU32(snapshot.moodRemainderSeconds) &&
           writer.putU32(snapshot.dangerSeconds) &&
           writer.putU32(snapshot.sickAwakeSeconds) &&
           writeTimestamp(writer, snapshot.bornAt) &&
           writeTimestamp(writer, snapshot.diedAt) &&
           writer.putU8(static_cast<uint8_t>(snapshot.deathCause)) &&
           writer.putU8(static_cast<uint8_t>(snapshot.sleepMode)) &&
           writeTimestamp(writer, snapshot.sleepStartedAt) &&
           writeTimestamp(writer, snapshot.lastSleepSettledAt) && writer.putU8(snapshot.departed ? 1 : 0);
}

bool readSnapshot(ByteReader& reader, Pet::PetSnapshotV1& snapshot, uint16_t version) {
    uint8_t species = 0;
    uint8_t lifeStage = 0;
    uint8_t health = 0;
    uint8_t deathCause = 0;
    uint8_t sleepMode = 0;
    if (!reader.getU64(snapshot.petId) ||
        !reader.getBytes(snapshot.name, sizeof(snapshot.name)) ||
        !reader.getU8(species) || !reader.getU8(lifeStage) ||
        !reader.getU8(snapshot.satiety) || !reader.getU8(snapshot.mood) ||
        !reader.getU8(snapshot.cleanliness) || !reader.getU8(snapshot.level) ||
        !reader.getU16(snapshot.exp) || !reader.getU8(health) ||
        !reader.getU64(snapshot.ageSeconds) ||
        !reader.getU32(snapshot.satietyRemainderSeconds) ||
        !reader.getU32(snapshot.cleanlinessRemainderSeconds) ||
        !reader.getU32(snapshot.moodRemainderSeconds) ||
        !reader.getU32(snapshot.dangerSeconds) ||
        !reader.getU32(snapshot.sickAwakeSeconds) ||
        !readTimestamp(reader, snapshot.bornAt) ||
        !readTimestamp(reader, snapshot.diedAt) ||
        !reader.getU8(deathCause) || !reader.getU8(sleepMode) ||
        !readTimestamp(reader, snapshot.sleepStartedAt) ||
        !readTimestamp(reader, snapshot.lastSleepSettledAt)) {
        return false;
    }
    snapshot.speciesId = static_cast<Pet::SpeciesId>(species);
    snapshot.lifeStage = static_cast<Pet::LifeStage>(lifeStage);
    snapshot.healthState = static_cast<Pet::HealthState>(health);
    snapshot.deathCause = static_cast<Pet::DeathCause>(deathCause);
    snapshot.sleepMode = static_cast<Pet::SleepMode>(sleepMode);
    snapshot.departed = false;
    if (version == 2) {
        uint8_t departed = 0;
        if (!reader.getU8(departed) || departed > 1) return false;
        snapshot.departed = departed != 0;
    }
    return reader.position() == kHeaderSize + (version == 1 ? 92 : kPetSavePayloadSize);
}

bool encodeRecord(const Pet::PetSnapshotV1& snapshot, uint32_t sequence,
                  uint8_t (&record)[kPetSaveRecordSize]) {
    memset(record, 0, sizeof(record));
    ByteWriter writer(record, sizeof(record));
    if (!writer.putU32(kPetSaveMagic) ||
        !writer.putU16(kPetSaveSchemaVersion) ||
        !writer.putU16(kPetSavePayloadSize) ||
        !writer.putU32(sequence) || !writeSnapshot(writer, snapshot) ||
        writer.position() != kCrcOffset) {
        return false;
    }
    return writer.putU32(crc32(record, kCrcOffset)) &&
           writer.position() == sizeof(record);
}

bool decodeRecord(const uint8_t* record, size_t length,
                  Pet::PetSnapshotV1& snapshot, uint32_t& sequence) {
    ByteReader reader(record, length);
    uint32_t magic = 0;
    uint16_t version = 0;
    uint16_t payloadSize = 0;
    uint32_t savedCrc = 0;
    if (!reader.getU32(magic) || !reader.getU16(version) ||
        !reader.getU16(payloadSize) || !reader.getU32(sequence) ||
        magic != kPetSaveMagic || (version != 1 && version != kPetSaveSchemaVersion) ||
        payloadSize != (version == 1 ? 92 : kPetSavePayloadSize) ||
        length != kHeaderSize + payloadSize + sizeof(uint32_t) ||
        !readSnapshot(reader, snapshot, version) ||
        !reader.getU32(savedCrc) || savedCrc != crc32(record, length - sizeof(uint32_t))) {
        return false;
    }
    return Pet::isValidPetSnapshot(snapshot);
}

bool encodeMemorialRecord(const Memorials& memorials, uint32_t sequence,
                          uint8_t (&record)[kMemorialRecordSize]) {
    if (memorials.count() > kMemorialCapacity) return false;
    memset(record, 0, sizeof(record));
    ByteWriter writer(record, sizeof(record));
    if (!writer.putU32(kMemorialSaveMagic) ||
        !writer.putU16(kMemorialSaveVersion) ||
        !writer.putU16(kMemorialPayloadSize) ||
        !writer.putU32(sequence) || !writer.putU8(memorials.count())) {
        return false;
    }
    const MemorialRecord empty{};
    for (uint8_t i = 0; i < kMemorialCapacity; ++i) {
        const MemorialRecord* item = memorials.at(i);
        if (!writeMemorial(writer, item == nullptr ? empty : *item)) return false;
    }
    if (writer.position() != kMemorialCrcOffset) return false;
    return writer.putU32(crc32(record, kMemorialCrcOffset)) &&
           writer.position() == sizeof(record);
}

bool decodeMemorialRecord(const uint8_t* record, size_t length,
                          Memorials& memorials, uint32_t& sequence) {
    ByteReader reader(record, length);
    uint32_t magic = 0;
    uint16_t version = 0;
    uint16_t payloadSize = 0;
    uint8_t count = 0;
    if (!reader.getU32(magic) || !reader.getU16(version) ||
        !reader.getU16(payloadSize) || !reader.getU32(sequence) ||
        !reader.getU8(count) || magic != kMemorialSaveMagic ||
        (version != 1 && version != kMemorialSaveVersion) ||
        payloadSize != (version == 1 ? kMemorialPayloadSize - 2 * kMemorialCapacity : kMemorialPayloadSize) ||
        length != kHeaderSize + payloadSize + sizeof(uint32_t) || count > kMemorialCapacity) {
        return false;
    }
    memorials.clear();
    for (uint8_t i = 0; i < kMemorialCapacity; ++i) {
        MemorialRecord item{};
        if (!readMemorial(reader, item, version)) return false;
        if (i < count && (!isValidMemorial(item) ||
                          memorials.containsPet(item.petId) ||
                          !memorials.append(item))) return false;
    }
    uint32_t savedCrc = 0;
    if (!reader.getU32(savedCrc) ||
        savedCrc != crc32(record, length - sizeof(uint32_t))) return false;
    return true;
}

struct SlotRecord {
    bool present = false;
    bool valid = false;
    uint32_t sequence = 0;
    Pet::PetSnapshotV1 snapshot{};
};

struct MemorialSlotRecord {
    bool present = false;
    bool valid = false;
    uint32_t sequence = 0;
};

SlotRecord readSlot(Preferences& preferences, const char* key) {
    SlotRecord slot;
    const size_t length = preferences.getBytesLength(key);
    slot.present = length > 0;
    if (length != 108 && length != kPetSaveRecordSize) return slot;
    uint8_t record[kPetSaveRecordSize]{};
    if (preferences.getBytes(key, record, length) != length) return slot;
    slot.valid = decodeRecord(record, length, slot.snapshot, slot.sequence);
    return slot;
}

MemorialSlotRecord readMemorialSlot(Preferences& preferences, const char* key,
                                    Memorials& memorials,
                                    uint8_t (&record)[kMemorialRecordSize]) {
    MemorialSlotRecord slot;
    const size_t length = preferences.getBytesLength(key);
    slot.present = length > 0;
    if (length != kMemorialRecordSize - 2 * kMemorialCapacity && length != kMemorialRecordSize) return slot;
    if (preferences.getBytes(key, record, length) != length) return slot;
    slot.valid = decodeMemorialRecord(record, length, memorials, slot.sequence);
    return slot;
}

bool sequenceIsNewer(uint32_t candidate, uint32_t reference) {
    return static_cast<int32_t>(candidate - reference) > 0;
}

uint32_t elapsedSince(uint32_t now, uint32_t then) {
    // A synchronous checkpoint can finish after the loop's captured `now`.
    // Treat that small negative delta as zero, while supporting millis rollover.
    const uint32_t delta = now - then;
    return static_cast<int32_t>(delta) < 0 ? 0 : delta;
}

}  // namespace

bool Save::init() {
    initialized_ = preferences_.begin(kNamespace, false);
    if (initialized_) refreshCapacity();
    return initialized_;
}

void Save::refreshCapacity() {
    nvs_stats_t stats{};
    if (nvs_get_stats(nullptr, &stats) != ESP_OK) {
        capacityKnown_ = false;
        return;
    }
    const bool wasLow = capacityLow();
    const bool first = !capacityKnown_;
    capacityKnown_ = true;
    freeEntries_ = stats.free_entries;
    if (first || wasLow != capacityLow()) {
        Serial.printf("NVS capacity: used=%u free=%u total=%u namespaces=%u%s\n",
            static_cast<unsigned>(stats.used_entries), static_cast<unsigned>(stats.free_entries),
            static_cast<unsigned>(stats.total_entries), static_cast<unsigned>(stats.namespace_count),
            capacityLow() ? " LOW" : "");
    }
}

bool Save::failedSaveAttempt() {
    if (saveFailures_ < 4) ++saveFailures_;
    const uint32_t delays[] = {5000, 10000, 30000, 60000};
    retryDelayMs_ = delays[saveFailures_ - 1];
    lastSaveAttemptAtMs_ = millis();
    refreshCapacity();
    return false;
}

bool Save::isInitialized() const { return initialized_; }

LoadStatus Save::load(Pet::PetData& pet) {
    if (!initialized_) return LoadStatus::Unavailable;
    const SlotRecord slotA = readSlot(preferences_, kSlotAKey);
    const SlotRecord slotB = readSlot(preferences_, kSlotBKey);
    if (!slotA.valid && !slotB.valid) {
        if (slotA.present || slotB.present) {
            writesBlocked_ = true;
            return LoadStatus::Invalid;
        }
        return LoadStatus::Empty;
    }
    const bool useA = slotA.valid &&
        (!slotB.valid || sequenceIsNewer(slotA.sequence, slotB.sequence));
    const SlotRecord& selected = useA ? slotA : slotB;
    if (!pet.restore(selected.snapshot)) {
        writesBlocked_ = true;
        return LoadStatus::Invalid;
    }
    activeSlotA_ = useA;
    hasActiveSlot_ = true;
    sequence_ = selected.sequence;
    return LoadStatus::Loaded;
}

bool Save::save(const Pet::PetData& pet) {
    if (!initialized_ || writesBlocked_) return false;
    const Pet::PetSnapshotV1 snapshot = pet.snapshot();
    if (!Pet::isValidPetSnapshot(snapshot)) return failedSaveAttempt();
    const uint32_t nextSequence = sequence_ + 1;
    uint8_t record[kPetSaveRecordSize]{};
    if (!encodeRecord(snapshot, nextSequence, record)) return failedSaveAttempt();
    const bool targetSlotA = !hasActiveSlot_ || !activeSlotA_;
    const char* targetKey = targetSlotA ? kSlotAKey : kSlotBKey;
    if (preferences_.putBytes(targetKey, record, sizeof(record)) != sizeof(record)) return failedSaveAttempt();
    const SlotRecord verified = readSlot(preferences_, targetKey);
    if (!verified.valid || verified.sequence != nextSequence) return failedSaveAttempt();
    activeSlotA_ = targetSlotA;
    hasActiveSlot_ = true;
    sequence_ = nextSequence;
    dirty_ = false;
    lastSaveAtMs_ = millis();
    lastSaveAttemptAtMs_ = lastSaveAtMs_;
    saveFailures_ = 0;
    retryDelayMs_ = 0;
    refreshCapacity();
    return true;
}

LoadStatus Save::loadMemorials(Memorials& memorials) {
    if (!initialized_) return LoadStatus::Unavailable;
    const MemorialSlotRecord slotA = readMemorialSlot(preferences_, kMemorialSlotAKey, memorialScratchA_, memorialBytes_);
    const MemorialSlotRecord slotB = readMemorialSlot(preferences_, kMemorialSlotBKey, memorialScratchB_, memorialBytes_);
    if (!slotA.valid && !slotB.valid) {
        if (slotA.present || slotB.present) {
            memorialWritesBlocked_ = true;
            return LoadStatus::Invalid;
        }
        memorials.clear();
        return LoadStatus::Empty;
    }
    const bool useA = slotA.valid &&
        (!slotB.valid || sequenceIsNewer(slotA.sequence, slotB.sequence));
    const MemorialSlotRecord& selected = useA ? slotA : slotB;
    memorials = useA ? memorialScratchA_ : memorialScratchB_;
    activeMemorialSlotA_ = useA;
    hasActiveMemorialSlot_ = true;
    memorialSequence_ = selected.sequence;
    return LoadStatus::Loaded;
}

bool Save::saveMemorials(const Memorials& memorials) {
    if (!initialized_ || memorialWritesBlocked_) return false;
    const uint32_t nextSequence = memorialSequence_ + 1;
    if (!encodeMemorialRecord(memorials, nextSequence, memorialBytes_)) return false;
    const bool targetSlotA = !hasActiveMemorialSlot_ || !activeMemorialSlotA_;
    const char* targetKey = targetSlotA ? kMemorialSlotAKey : kMemorialSlotBKey;
    if (preferences_.putBytes(targetKey, memorialBytes_, sizeof(memorialBytes_)) != sizeof(memorialBytes_)) return false;
    const MemorialSlotRecord verified = readMemorialSlot(preferences_, targetKey, memorialScratchA_, memorialBytes_);
    if (!verified.valid || verified.sequence != nextSequence) return false;
    activeMemorialSlotA_ = targetSlotA;
    hasActiveMemorialSlot_ = true;
    memorialSequence_ = nextSequence;
    return true;
}

bool Save::appendMemorial(const Pet::PetData& pet, Memorials& memorials) {
    if (!pet.isEnded()) return false;
    if (memorials.containsPet(pet.petId())) return true;
    memorialScratchB_ = memorials;
    if (!memorialScratchB_.append(pet) || !saveMemorials(memorialScratchB_)) return false;
    memorials = memorialScratchB_;
    return true;
}

bool Save::removeMemorial(uint8_t index, Memorials& memorials) {
    memorialScratchB_ = memorials;
    if (!memorialScratchB_.remove(index) || !saveMemorials(memorialScratchB_)) return false;
    memorials = memorialScratchB_;
    return true;
}

void Save::markDirty(uint32_t nowMs) {
    if (!dirty_) dirtySinceMs_ = nowMs;
    dirty_ = true;
}

void Save::update(const Pet::PetData& pet, uint32_t nowMs) {
    if (!initialized_ || writesBlocked_) return;
    if (saveFailures_ && elapsedSince(nowMs, lastSaveAttemptAtMs_) < retryDelayMs_) return;
    const uint32_t sinceLastSave = elapsedSince(nowMs, lastSaveAtMs_);
    if (!dirty_) {
        if (sinceLastSave < kPeriodicSaveIntervalMs) return;
        markDirty(nowMs - kDeferredSaveMs);
    }
    if (nowMs - dirtySinceMs_ < kDeferredSaveMs) return;
    if (hasActiveSlot_ && sinceLastSave < kMinimumSaveIntervalMs) return;
    save(pet);
}

}  // namespace Storage
