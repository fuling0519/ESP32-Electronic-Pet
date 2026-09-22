#include <Arduino.h>

#include "hardware/Display.h"
#include "hardware/Input.h"
#include "hardware/Sound.h"
#include "storage/Save.h"

Hardware::Display display;
Hardware::Input input;
Hardware::Sound sound;
Storage::Save save;

void setup() {
    Serial.begin(115200);
    display.init();
    input.init();
    sound.init();
    save.init();
}

void loop() {
    input.update();
    sound.update();
}
