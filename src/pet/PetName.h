#pragma once

#include <stdint.h>

#include "pet/PetSnapshot.h"

namespace Pet {

// Produces a six-letter ASCII name using the ABB syllable pattern. The same
// random value always produces the same name, which keeps this easy to test;
// device code should supply values from esp_random().
void generateAbbName(uint32_t randomValue,
                     char (&name)[kPetNameMaxLength + 1]);

}  // namespace Pet
