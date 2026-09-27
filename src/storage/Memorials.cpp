#include "storage/Memorials.h"

#include <string.h>

#include "pet/PetData.h"

namespace Storage {

bool isValidMemorial(const MemorialRecord& memorial) {
    return memorial.petId != 0 && Pet::isValidPetName(memorial.name) &&
           memorial.speciesId == Pet::SpeciesId::Bird;
}

uint8_t Memorials::count() const { return count_; }

const MemorialRecord* Memorials::at(uint8_t index) const {
    return index < count_ ? &records_[index] : nullptr;
}

bool Memorials::containsPet(uint64_t petId) const {
    for (uint8_t i = 0; i < count_; ++i) {
        if (records_[i].petId == petId) return true;
    }
    return false;
}

bool Memorials::containsName(const char* name) const {
    if (name == nullptr) return false;
    for (uint8_t i = 0; i < count_; ++i) {
        if (strcmp(records_[i].name, name) == 0) return true;
    }
    return false;
}

uint64_t Memorials::highestPetId() const {
    uint64_t highest = 0;
    for (uint8_t i = 0; i < count_; ++i) {
        if (records_[i].petId > highest) highest = records_[i].petId;
    }
    return highest;
}

bool Memorials::append(const MemorialRecord& memorial) {
    if (!isValidMemorial(memorial)) return false;
    if (containsPet(memorial.petId)) return true;
    if (count_ >= kMemorialCapacity) return false;
    records_[count_++] = memorial;
    return true;
}

bool Memorials::append(const Pet::PetData& pet) {
    if (!pet.isDead()) return false;
    MemorialRecord memorial{};
    memorial.petId = pet.petId();
    memcpy(memorial.name, pet.name(), sizeof(memorial.name));
    memorial.speciesId = pet.speciesId();
    memorial.ageSeconds = pet.ageSeconds();
    return append(memorial);
}

bool Memorials::remove(uint8_t index) {
    if (index >= count_) return false;
    for (uint8_t i = index + 1; i < count_; ++i) {
        records_[i - 1] = records_[i];
    }
    --count_;
    records_[count_] = MemorialRecord{};
    return true;
}

void Memorials::clear() {
    memset(records_, 0, sizeof(records_));
    count_ = 0;
}

}  // namespace Storage
