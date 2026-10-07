#include <assert.h>
#include <stdio.h>
#include <initializer_list>
#include "pet/SadQuickTest.h"

int main() {
    for (bool baby : {false, true}) {
        Pet::PetData pet;
        if (baby) { pet.startNewEgg(); pet.advanceSeconds(Pet::PetData::kEggHatchAgeSeconds); }
        auto stage = pet.lifeStage(); auto identity = pet.petId();
        assert(Pet::applySadTestPreset(pet,'1') && pet.mood()==40);
        assert(Pet::applySadTestPreset(pet,'6') && pet.mood()==44);
        assert(Pet::applySadTestPreset(pet,'7') && pet.mood()==45);
        assert(Pet::applySadTestPreset(pet,'2') && pet.satiety()==25 && pet.mood()==80);
        assert(Pet::applySadTestPreset(pet,'3') && pet.cleanliness()==25 && pet.satiety()==80);
        assert(Pet::applySadTestPreset(pet,'4') && pet.isSick());
        assert(Pet::applySadTestPreset(pet,'5') && !pet.isSick() && pet.mood()==40 && pet.satiety()==25 && pet.cleanliness()==25);
        assert(Pet::applySadTestPreset(pet,'8') && pet.satiety()==29 && pet.cleanliness()==29 && pet.mood()==40);
        assert(Pet::applySadTestPreset(pet,'9') && pet.satiety()==30 && pet.cleanliness()==30);
        assert(Pet::applySadTestPreset(pet,'0') && pet.mood()==80 && pet.satiety()==80 && pet.cleanliness()==100);
        assert(pet.lifeStage()==stage && pet.petId()==identity);
        auto revision=pet.displayRevision();
        assert(!Pet::applySadTestPreset(pet,'x') && pet.displayRevision()==revision);
        assert(pet.beginNormalSleep()); revision=pet.displayRevision();
        assert(!Pet::applySadTestPreset(pet,'1') && pet.displayRevision()==revision);
        pet.wake(); pet.setDead(true); revision=pet.displayRevision();
        assert(!Pet::applySadTestPreset(pet,'0') && pet.displayRevision()==revision);
        pet.startNewEgg(); revision=pet.displayRevision();
        assert(!Pet::applySadTestPreset(pet,'1') && pet.displayRevision()==revision);
    }
    puts("PASS: quick test presets, recovery boundaries, identity/stage preservation and invalid/sleep/ended/egg rejection.");
}
