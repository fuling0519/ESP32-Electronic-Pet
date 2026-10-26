// Pet sync phase 5 on the real UiController and DeviceSettings (fake NVS): set_sound parsing,
// activity rules (busy on the Volume screen), op replay and conflict, save-failure rollback,
// the status "sound" field following the saved mode (joystick included), and touch pet_events
// emitted once per accepted gesture and never for rejected touches.
#define main legacyUiRegressionMain
#include "rps_ui_native.cpp"
#undef main
#include "ble/CareCommand.h"
#include "ble/Messages.h"
#include "ble/OpCache.h"
#include "ble/PetEvents.h"
#include "ble/SoundCommand.h"
#include "storage/DeviceSettings.h"

FakeNvs nvs;
uint32_t millis() { return 100; }

using Ui::Activity;
using Ui::ScreenId;
using Ev = Hardware::InputEvent;
using TouchEvent = Hardware::TouchEvent;
static uint32_t t = 0;
static void step(Ui::UiController& ui, Ev e = Ev::None, uint32_t dt = 30) { t += dt; ui.update(e, t); ui.render(); }
static Pet::PetData living(uint64_t id) {
    Pet::PetData pet;
    assert(pet.startNewEgg(id, "Tamama", Pet::SpeciesId::Bird));
    auto s = pet.snapshot(); s.lifeStage = Pet::LifeStage::Adult; s.ageSeconds = Pet::PetData::kAdultAgeSeconds;
    assert(pet.restore(s));
    pet.setMood(80); pet.setSatiety(80); pet.setCleanliness(80);
    return pet;
}
static void home(Ui::UiController& ui) {
    t = 0; ui.init(0); step(ui, Ev::None, 1500); step(ui);
    assert(ui.screen() == ScreenId::Home && ui.activity() == Activity::Idle);
}

static const char* parse(const char* text, Ble::SoundRequest& r) {
    StaticJsonDocument<256> d; assert(!deserializeJson(d, text));
    return Ble::parseSoundCommand(d.as<JsonVariantConst>(), 9, r);
}
static void parsing() {
    Ble::SoundRequest r;
    assert(strcmp(parse(R"({"muted":true,"op":"00000000000000a1"})", r), "ok") == 0 && r.muted && r.op == 0xa1 && r.commandId == 9);
    assert(strcmp(parse(R"({"muted":false,"op":"00000000000000a1"})", r), "ok") == 0 && !r.muted);
    assert(strcmp(parse(R"({"op":"00000000000000a1"})", r), "missing_field") == 0);
    assert(strcmp(parse(R"({"muted":"true","op":"00000000000000a1"})", r), "missing_field") == 0);
    assert(strcmp(parse(R"({"muted":1,"op":"00000000000000a1"})", r), "missing_field") == 0);
    assert(strcmp(parse(R"({"muted":null,"op":"00000000000000a1"})", r), "missing_field") == 0);
    assert(strcmp(parse(R"({"muted":true})", r), "missing_field") == 0);
    assert(strcmp(parse(R"({"muted":true,"op":5})", r), "missing_field") == 0);
    assert(strcmp(parse(R"({"muted":true,"op":"00000000000000A1"})", r), "invalid_value") == 0);
    assert(strcmp(parse(R"({"muted":true,"op":"a1"})", r), "invalid_value") == 0);
    // pet_id is neither needed nor checked.
    assert(strcmp(parse(R"({"muted":true,"op":"00000000000000a1","pet_id":"x"})", r), "ok") == 0);
    for (const char* a : {"idle", "menu", "sleeping", "ended"}) assert(strcmp(Ble::soundEligibility(a), "ok") == 0);
    for (const char* a : {"busy", "device_game", "web_game", "", "Idle"}) assert(strcmp(Ble::soundEligibility(a), "busy") == 0);
    assert(strcmp(Ble::soundEligibility(nullptr), "busy") == 0);
}

static void results() {
    char json[1025]; uint16_t len = 0;
    assert(Ble::makeSoundResultJson(21, 0x9f2c41d07ab3e815ULL, Ble::soundDone(true), json, sizeof(json), len));
    assert(strcmp(json, R"({"v":1,"type":"action_result","id":21,"op":"9f2c41d07ab3e815","action":"set_sound","outcome":"done","changes":{"muted":true}})") == 0);
    assert(Ble::makeSoundResultJson(22, 1, Ble::soundDone(false), json, sizeof(json), len) && strstr(json, R"("changes":{"muted":false})"));
    assert(Ble::makeSoundResultJson(23, 1, Ble::soundFailed("save_failed"), json, sizeof(json), len));
    assert(strcmp(json, R"({"v":1,"type":"action_result","id":23,"op":"0000000000000001","action":"set_sound","outcome":"failed","code":"save_failed"})") == 0);
}

static void ops() {
    using M = Ble::OpCache::Match;
    Ble::OpCache c;
    assert(c.check(5, Ble::OpKind::Sound, 0, 1, 1) == M::New);
    c.begin(5, Ble::OpKind::Sound, 0, 1, 1);
    assert(c.check(5, Ble::OpKind::Sound, 0, 1, 2) == M::Pending && c.find(5)->commandId == 2);
    assert(c.check(5, Ble::OpKind::Sound, 0, 0, 3) == M::Conflict);  // Same op, other state.
    assert(c.check(5, Ble::OpKind::Care, 0, 1, 3) == M::Conflict);   // Same op, other command kind.
    const auto* e = c.complete(5, Ble::soundFailed("save_failed"));
    assert(e && e->done && !e->sound.done && strcmp(e->sound.code, "save_failed") == 0);
    assert(c.check(5, Ble::OpKind::Sound, 0, 1, 4) == M::Done && c.find(5)->commandId == 4);
    char json[1025]; uint16_t len = 0;
    assert(Ble::makeSoundResultJson(4, 5, c.find(5)->sound, json, sizeof(json), len) && strstr(json, R"("id":4)") && strstr(json, "save_failed"));
    c.begin(6, Ble::OpKind::Sound, 0, 0, 7); c.complete(6, Ble::soundDone(false));
    assert(c.find(6)->sound.done && !c.find(6)->sound.muted);
}

// The steps of Application::handleSoundRequest after the op cache.
struct Device {
    Hardware::Sound& sound; Storage::DeviceSettings& settings; Ui::UiController& ui;
    const char* activity() const { return Ui::activityName(ui.activity()); }
    const char* request(bool muted, Ble::SoundOutcome& out) {
        const char* code = Ble::soundEligibility(activity());
        if (strcmp(code, "ok") != 0) return code;
        const bool wasMuted = sound.volume() == 0;
        out = Ble::applySound(sound, settings, muted);
        if (out.done && wasMuted && !muted) sound.playConfirm();
        return code;
    }
};

// What the main loop feeds Link::setSoundMuted, through the real status builder.
static std::string statusSoundField(const Storage::DeviceSettings& settings, const Pet::PetData& pet) {
    Ble::StatusSources src; src.sound = settings.volume() == 0 ? "muted" : "on";
    char json[1025]; uint16_t len = 0;
    assert(Ble::makeStatusJson(pet, "esp32-AABBCCDDEEFF", "idle", src, json, sizeof(json), len));
    StaticJsonDocument<1024> d; assert(!deserializeJson(d, static_cast<const char*>(json)));
    return d["sound"].as<const char*>();
}

static void setSound() {
    nvs = {};
    Hardware::Display display; Storage::Memorials records;
    Hardware::Sound snd; Storage::DeviceSettings settings;
    assert(settings.init()); snd.setVolume(settings.volume());
    auto pet = living(10);
    Ui::UiController ui(display, snd, pet, records, randomMove); home(ui);
    Device d{snd, settings, ui};
    char json[1025];
    // Without a source the field is absent; with one it is "on" or "muted".
    { uint16_t len = 0; Ble::StatusSources none; assert(Ble::makeStatusJson(pet, "esp32-A", "idle", none, json, sizeof(json), len) && !strstr(json, "\"sound\"")); }
    assert(statusSoundField(settings, pet) == "on");

    // Plain Home: accepted silently, no screen change, saved and applied; muting plays nothing.
    Ble::SoundOutcome o;
    const unsigned confirms = confirmSounds, cancels = cancelSounds, failures = failureSounds;
    assert(strcmp(d.request(true, o), "ok") == 0 && o.done && o.muted);
    assert(ui.screen() == ScreenId::Home && snd.volume() == 0 && settings.volume() == 0);
    assert(confirmSounds == confirms && cancelSounds == cancels && failureSounds == failures);
    assert(statusSoundField(settings, pet) == "muted");
    Storage::DeviceSettings reboot; assert(reboot.init() && reboot.volume() == 0);
    // Same state again: done without another NVS write.
    const unsigned writes = nvs.writes;
    assert(strcmp(d.request(true, o), "ok") == 0 && o.done && nvs.writes == writes);
    // Un-muting: a short confirm tone, like the joystick's save.
    assert(strcmp(d.request(false, o), "ok") == 0 && o.done && !o.muted && snd.volume() == 1 && settings.volume() == 1);
    assert(confirmSounds == confirms + 1 && ui.screen() == ScreenId::Home);
    assert(statusSoundField(settings, pet) == "on");

    // A menu page also accepts silently and stays where it is.
    step(ui, Ev::Press); assert(ui.screen() == ScreenId::MainMenu && ui.activity() == Activity::Menu);
    assert(strcmp(d.request(true, o), "ok") == 0 && o.done && ui.screen() == ScreenId::MainMenu && snd.volume() == 0);
    assert(strcmp(d.request(false, o), "ok") == 0 && snd.volume() == 1);
    step(ui, Ev::LongPress); assert(ui.screen() == ScreenId::Home);

    // Save failure: failed/save_failed, in-memory and stored modes unchanged, status unchanged.
    nvs.failAllWrites = true;
    assert(strcmp(d.request(true, o), "ok") == 0 && !o.done && strcmp(o.code, "save_failed") == 0);
    assert(snd.volume() == 1 && settings.volume() == 1 && statusSoundField(settings, pet) == "on");
    uint16_t len = 0; assert(Ble::makeSoundResultJson(1, 2, o, json, sizeof(json), len) && strstr(json, R"("outcome":"failed","code":"save_failed")"));
    nvs.failAllWrites = false;
    nvs.failWrite = nvs.writes + 1; nvs.torn = true;  // A torn record: still failed and rolled back.
    assert(strcmp(d.request(true, o), "ok") == 0 && !o.done && snd.volume() == 1 && settings.volume() == 1);
    nvs.failWrite = 0; nvs.torn = false;
    assert(strcmp(d.request(true, o), "ok") == 0 && o.done && snd.volume() == 0);  // A retry works again.
    assert(strcmp(d.request(false, o), "ok") == 0 && snd.volume() == 1);

    // The Volume screen holds an unsaved choice: busy, nothing changes, status shows the saved mode.
    step(ui, Ev::Press);
    for (int i = 0; i < 8 && ui.menuIndex() != 8; ++i) step(ui, Ev::Down);
    assert(ui.menuIndex() == 8); step(ui, Ev::Press);
    assert(ui.screen() == ScreenId::Volume && ui.activity() == Activity::Busy);
    step(ui, Ev::Left); assert(snd.volume() == 0 && settings.volume() == 1);  // Audition only.
    const unsigned writes2 = nvs.writes;
    assert(strcmp(d.request(true, o), "busy") == 0 && strcmp(d.request(false, o), "busy") == 0);
    assert(snd.volume() == 0 && settings.volume() == 1 && nvs.writes == writes2);
    assert(statusSoundField(settings, pet) == "on");  // Not the audition.
    // Joystick save: the status field follows the saved mode.
    step(ui, Ev::Down); step(ui, Ev::Press);
    assert(ui.takeAction() == Ui::UiAction::SaveVolume);
    ui.onVolumeSaveResult(settings.saveVolume(snd.volume()));
    assert(ui.screen() == ScreenId::MainMenu && settings.volume() == 0);
    assert(statusSoundField(settings, pet) == "muted");
    // A failed joystick save keeps the old saved mode in the status.
    step(ui, Ev::Press); step(ui, Ev::Left);
    nvs.failAllWrites = true;
    step(ui, Ev::Down); step(ui, Ev::Press); assert(ui.takeAction() == Ui::UiAction::SaveVolume);
    ui.onVolumeSaveResult(settings.saveVolume(snd.volume()));
    assert(ui.screen() == ScreenId::Volume && settings.volume() == 0);
    assert(statusSoundField(settings, pet) == "muted");
    nvs.failAllWrites = false;
    step(ui, Ev::LongPress); assert(snd.volume() == 0);

    // Sleeping accepts (normal sleep screen, idle).
    auto sleeper = living(11); Hardware::Sound snd2; Ui::UiController su(display, snd2, sleeper, records, randomMove); home(su);
    assert(sleeper.beginNormalSleep()); su.onSleepStarted(t + 10); step(su, Ev::None, 4000);
    Device sd{snd, settings, su};
    assert(su.activity() == Activity::Sleeping && strcmp(sd.request(false, o), "ok") == 0 && o.done && snd.volume() == 1);
}

static void touchEvents() {
    Hardware::Display display; Storage::Memorials records;
    Ble::PetEventType type = Ble::PetEventType::Fed;
    // Pure mapping: only accepted gesture starts.
    assert(Ble::touchPetEvent(TouchEvent::Poke, true, type) && type == Ble::PetEventType::Poked);
    assert(Ble::touchPetEvent(TouchEvent::PettingStarted, true, type) && type == Ble::PetEventType::Petted);
    for (TouchEvent e : {TouchEvent::None, TouchEvent::PettingReleased, TouchEvent::Cancelled})
        assert(!Ble::touchPetEvent(e, true, type));
    for (TouchEvent e : {TouchEvent::Poke, TouchEvent::PettingStarted}) assert(!Ble::touchPetEvent(e, false, type));

    Hardware::Sound snd; auto pet = living(123);
    Ui::UiController ui(display, snd, pet, records, randomMove); home(ui);
    assert(ui.touchContextAllowed());
    const auto before = pet.snapshot();
    unsigned emitted = 0; char json[1025]; std::string names;
    // The main loop's rule: emit when the UI accepted the touch.
    auto touch = [&](TouchEvent e, uint32_t now) {
        Ble::PetEventType ty;
        if (!Ble::touchPetEvent(e, ui.onTouchEvent(e, now), ty)) return false;
        Ble::PetEvent ev; ev.type = ty; ev.petId = pet.petId();
        uint16_t len = 0; assert(Ble::makePetEventJson(ev, json, sizeof(json), len));
        ++emitted; names += Ble::petEventName(ty); names += ' ';
        return true;
    };
    assert(touch(TouchEvent::Poke, t + 10));
    assert(strcmp(json, R"({"v":1,"type":"pet_event","pet_id":"123","event":"poked","source":"device"})") == 0);
    assert(!touch(TouchEvent::Poke, t + 20) && !touch(TouchEvent::PettingStarted, t + 30));  // Already reacting.
    assert(emitted == 1);
    step(ui, Ev::None, 2000);  // The poke animation is over.
    assert(touch(TouchEvent::PettingStarted, t));
    assert(strcmp(json, R"({"v":1,"type":"pet_event","pet_id":"123","event":"petted","source":"device"})") == 0);
    assert(!touch(TouchEvent::PettingStarted, t + 10) && !touch(TouchEvent::Poke, t + 20));
    assert(!touch(TouchEvent::PettingReleased, t + 400));  // Accepted by the UI, but no event for a release.
    assert(!touch(TouchEvent::Cancelled, t + 500));
    assert(emitted == 2 && names == "poked petted ");
    // Never changes pet state.
    const auto after = pet.snapshot();
    assert(after.mood == before.mood && after.satiety == before.satiety && after.cleanliness == before.cleanliness &&
           after.exp == before.exp && after.ageSeconds == before.ageSeconds);
    // Rejected: a menu page, then sleeping.
    step(ui, Ev::None, 2000);
    step(ui, Ev::Press); assert(!ui.touchContextAllowed());
    assert(!touch(TouchEvent::Poke, t + 10) && !touch(TouchEvent::PettingStarted, t + 20));
    step(ui, Ev::LongPress);
    assert(pet.beginNormalSleep()); ui.onSleepStarted(t + 1);
    assert(!ui.touchContextAllowed() && !touch(TouchEvent::Poke, t + 2) && !touch(TouchEvent::PettingStarted, t + 3));
    assert(emitted == 2);
    // The detector's per-type web mask has room for both new types (16 bits).
    Ble::PetEventDetector detector; detector.markFromWeb(Ble::PetEventType::Poked);
    // Admission: touch events use the ordinary event policy (dropped when the queue is nearly full).
    assert(Ble::petEventAdmitted(true, true, 5, 8) && !Ble::petEventAdmitted(true, true, 6, 8) &&
           !Ble::petEventAdmitted(false, true, 0, 8) && !Ble::petEventAdmitted(true, false, 0, 8));
}

static void deviceInfo() {
    char json[1025]; uint16_t len = 0;
    assert(Ble::makeDeviceInfoJson(7, "ESP32-PET", "esp32-AABBCCDDEEFF", json, sizeof(json), len));
    assert(strstr(json, R"("features":["web_game","pet_events","care","lifecycle","memorials","sound"])"));
}

int main() {
    info.tile_width = 16; info.tile_height = 8; info.pixel_width = 128; info.pixel_height = 64;
    gfx.u8x8.display_info = &info;
    u8g2_SetupBuffer(&gfx, pixels.data(), 8, u8g2_ll_hvline_vertical_top_lsb, U8G2_R0);
    u8g2_SetDrawColor(&gfx, 1);
    parsing(); results(); ops(); setSound(); touchEvents(); deviceInfo();
    puts("PASS: set_sound parsing, activity rules, op replay/conflict, save-failure rollback, status sound field, touch pet_events.");
}
