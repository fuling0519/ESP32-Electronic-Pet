import '../../domain/models/web_pet.dart';

/// BLE v1 wire decoder. Firmware JSON keys and wire encodings stay confined
/// to this file; the rest of the Web app consumes WebPetSnapshot.
class BleSnapshotAdapter {
  const BleSnapshotAdapter();

  WebPetSnapshot? decode({
    required WebDeviceId? deviceId,
    required Object? payload,
    required DateTime receivedAt,
  }) {
    if (payload is! Map<String, dynamic>) return null;
    final id = payload['id'];
    final name = payload['name'];
    final stage = switch (payload['life_stage']) {
      'egg' => PetLifeStage.egg,
      'baby' => PetLifeStage.baby,
      'adult' => PetLifeStage.adult,
      _ => null,
    };
    final ageSeconds = payload['age_seconds'];
    final health = switch (payload['health']) {
      'healthy' => PetHealthState.healthy,
      'sick' => PetHealthState.sick,
      'dead' => PetHealthState.dead,
      _ => null,
    };
    final sleep = switch (payload['sleep']) {
      'awake' => PetSleepState.awake,
      'normal' => PetSleepState.sleeping,
      'deep_sleep' => PetSleepState.deepSleep,
      _ => PetSleepState.unknown,
    };
    final isDead = payload['is_dead'];
    final satiety = payload['satiety'];
    final mood = payload['mood'];
    final cleanliness = payload['cleanliness'];

    if (id is! String ||
        !RegExp(r'^\d+$').hasMatch(id) ||
        name is! String ||
        name.isEmpty ||
        stage == null ||
        ageSeconds is! String ||
        !RegExp(r'^\d+$').hasMatch(ageSeconds) ||
        health == null ||
        isDead is! bool ||
        isDead != (health == PetHealthState.dead) ||
        sleep == PetSleepState.unknown ||
        satiety is! int || satiety < 0 || satiety > 100 ||
        mood is! int || mood < 0 || mood > 100 ||
        cleanliness is! int || cleanliness < 0 || cleanliness > 100) {
      return null;
    }

    final age = int.tryParse(ageSeconds);
    if (age == null) return null;

    return WebPetSnapshot(
      key: WebPetKey(deviceId: deviceId, petId: id),
      name: name,
      lifeStage: stage,
      satietyPercent: satiety,
      moodPercent: mood,
      cleanlinessPercent: cleanliness,
      age: Duration(seconds: age),
      health: health,
      isDead: isDead,
      sleep: sleep,
      receivedAt: receivedAt,
    );
  }
}
