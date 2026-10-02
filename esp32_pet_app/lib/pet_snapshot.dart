class PetSnapshot {
  const PetSnapshot({
    required this.id,
    required this.name,
    required this.stage,
    required this.satiety,
    required this.mood,
    required this.cleanliness,
    required this.ageSeconds,
    required this.health,
    required this.isDead,
    required this.sleep,
    this.level, this.exp, this.expToNextLevel,
  });

  final String id, name, stage, ageSeconds, health, sleep;
  final int satiety, mood, cleanliness;
  final int? level, exp, expToNextLevel;
  final bool isDead;

  static PetSnapshot? parse(Object? value) {
    if (value is! Map<String, dynamic>) return null;
    final id = value['id'];
    final name = value['name'];
    final stage = value['life_stage'];
    final age = value['age_seconds'];
    final health = value['health'];
    final sleep = value['sleep'];
    final dead = value['is_dead'];
    final satiety = value['satiety'];
    final mood = value['mood'];
    final cleanliness = value['cleanliness'];
    final level = value['level'];
    final exp = value['exp'];
    final next = value['exp_to_next_level'];
    final hasProgress = value.containsKey('level') || value.containsKey('exp') ||
        value.containsKey('exp_to_next_level');
    // Old firmware omits the entire optional group. Partial/invalid groups fail.
    if (hasProgress && (level is! int || level < 1 || level > 255 ||
        exp is! int || exp < 0 || exp > 65535 || next is! int || next < 0 ||
        next > 65535 || (next == 0 ? exp != 0 : exp >= next))) {
      return null;
    }
    if (id is! String || !RegExp(r'^\d+$').hasMatch(id) ||
        name is! String || name.isEmpty ||
        !{'egg', 'baby', 'adult'}.contains(stage) ||
        age is! String || !RegExp(r'^\d+$').hasMatch(age) ||
        !{'healthy', 'sick', 'dead'}.contains(health) ||
        dead is! bool || dead != (health == 'dead') ||
        !{'awake', 'normal'}.contains(sleep) ||
        satiety is! int || satiety < 0 || satiety > 100 ||
        mood is! int || mood < 0 || mood > 100 ||
        cleanliness is! int || cleanliness < 0 || cleanliness > 100) {
      return null;
    }
    return PetSnapshot(
      id: id, name: name, stage: stage as String,
      satiety: satiety, mood: mood, cleanliness: cleanliness,
      ageSeconds: age, health: health as String, isDead: dead,
      sleep: sleep as String,
      level: level as int?, exp: exp as int?, expToNextLevel: next as int?,
    );
  }
}
