#include "pet/PetName.h"

#include <string.h>

namespace Pet {
namespace {

constexpr char kPrefixes[][3] = {
    "Ba", "Be", "Bo", "Da", "Do", "Fu", "Ha", "Ho",
    "Ka", "Ki", "Ko", "Ku", "Ma", "Mi", "Mo", "Na",
    "Ni", "No", "Pa", "Pi", "Po", "Ta", "Te", "To",
};

constexpr char kSuffixes[][3] = {
    "ka", "ki", "ko", "ma", "mi", "mo", "na", "ni", "no",
    "pa", "pi", "po", "ra", "ri", "ro", "ru", "ta", "to",
};

constexpr uint8_t kPrefixCount = sizeof(kPrefixes) / sizeof(kPrefixes[0]);
constexpr uint8_t kSuffixCount = sizeof(kSuffixes) / sizeof(kSuffixes[0]);
static_assert(kPetNameMaxLength >= 6, "ABB names need six characters");

bool sameSyllable(const char* prefix, const char* suffix) {
    return static_cast<char>(prefix[0] + ('a' - 'A')) == suffix[0] &&
           prefix[1] == suffix[1];
}

}  // namespace

void generateAbbName(uint32_t randomValue,
                     char (&name)[kPetNameMaxLength + 1]) {
    memset(name, 0, sizeof(name));
    const uint8_t prefixIndex = randomValue % kPrefixCount;
    uint8_t suffixIndex = (randomValue / kPrefixCount) % kSuffixCount;
    if (sameSyllable(kPrefixes[prefixIndex], kSuffixes[suffixIndex])) {
        suffixIndex = (suffixIndex + 1) % kSuffixCount;
    }
    name[0] = kPrefixes[prefixIndex][0];
    name[1] = kPrefixes[prefixIndex][1];
    name[2] = kSuffixes[suffixIndex][0];
    name[3] = kSuffixes[suffixIndex][1];
    name[4] = kSuffixes[suffixIndex][0];
    name[5] = kSuffixes[suffixIndex][1];
}

}  // namespace Pet
