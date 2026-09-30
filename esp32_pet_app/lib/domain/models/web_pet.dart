/// Web-side domain records. These models deliberately know nothing about BLE
/// field names, packets, FlutterBluePlus, or ESP32 C++ types.
enum PetLifeStage { egg, baby, adult }
enum PetHealthState { healthy, sick, dead }
enum PetSleepState { awake, sleeping, deepSleep, unknown }

class WebDeviceId {
  const WebDeviceId._(this.value);

  final String value;

  static WebDeviceId? tryParse(Object? value) {
    if (value is! String ||
        !RegExp(r'^esp32-pet-[0-9A-F]{12}$').hasMatch(value) ||
        value == 'esp32-pet-000000000000' ||
        value == 'esp32-pet-FFFFFFFFFFFF') {
      return null;
    }
    return WebDeviceId._(value);
  }

  @override
  String toString() => value;
}

/// Cloud/application identity. saveGeneration is null only for legacy
/// firmware that has not started reporting a generation identifier yet.
class WebPetKey {
  const WebPetKey({
    required this.deviceId,
    required this.petId,
    this.saveGeneration,
  });

  final WebDeviceId? deviceId;
  final String petId;
  final String? saveGeneration;

  bool get safeForLongTermHistory =>
      deviceId != null && saveGeneration != null && saveGeneration!.isNotEmpty;
}

class WebDevice {
  const WebDevice({
    required this.id,
    required this.displayName,
    this.firmwareVersion,
    this.protocolVersion,
    this.lastSyncedAt,
  });

  final WebDeviceId id;
  final String displayName;
  final String? firmwareVersion;
  final int? protocolVersion;
  final DateTime? lastSyncedAt;
}

class WebDeviceOwnership {
  const WebDeviceOwnership({
    required this.accountId,
    required this.deviceId,
    required this.boundAt,
    this.unboundAt,
  });

  final String accountId;
  final WebDeviceId deviceId;
  final DateTime boundAt;
  final DateTime? unboundAt;
}

/// Canonical Web snapshot. Values are normalized and typed before UI or cloud
/// code consumes them; they are not a mirror of a firmware struct.
class WebPetSnapshot {
  const WebPetSnapshot({
    required this.key,
    required this.name,
    required this.lifeStage,
    required this.satietyPercent,
    required this.moodPercent,
    required this.cleanlinessPercent,
    required this.age,
    required this.health,
    required this.isDead,
    required this.sleep,
    required this.receivedAt,
    this.revision,
  });

  final WebPetKey key;
  final String name;
  final PetLifeStage lifeStage;
  final int satietyPercent;
  final int moodPercent;
  final int cleanlinessPercent;
  final Duration age;
  final PetHealthState health;
  final bool isDead;
  final PetSleepState sleep;
  final DateTime receivedAt;
  final int? revision;

  /// Stable Web/cloud naming. This API shape can evolve without changing BLE.
  Map<String, Object?> toCloudJson() {
    final deviceId = key.deviceId;
    if (deviceId == null) {
      throw StateError('Cannot sync a snapshot without a device ID');
    }
    return {
        'deviceId': deviceId.value,
        'pet': {
          'petId': key.petId,
          'saveGeneration': key.saveGeneration,
          'name': name,
          'lifeStage': lifeStage.name,
          'needs': {
            'satietyPercent': satietyPercent,
            'moodPercent': moodPercent,
            'cleanlinessPercent': cleanlinessPercent,
          },
          'ageSeconds': age.inSeconds.toString(),
          'health': health.name,
          'isDead': isDead,
          'sleep': sleep.name,
        },
        'revision': revision,
        'receivedAt': receivedAt.toUtc().toIso8601String(),
      };
  }
}

class WebPetEvent {
  const WebPetEvent({
    required this.eventId,
    required this.petKey,
    required this.kind,
    required this.age,
    required this.receivedAt,
    this.occurredAt,
  });

  final String eventId;
  final WebPetKey petKey;
  final String kind;
  final Duration age;
  final DateTime receivedAt;
  final DateTime? occurredAt;
}
