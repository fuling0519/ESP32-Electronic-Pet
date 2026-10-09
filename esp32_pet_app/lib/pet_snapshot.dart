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
    this.isDeparted = false,
    this.speciesId = 1,
    required this.sleep,
    this.level, this.exp, this.expToNextLevel,
  });

  final String id, name, stage, ageSeconds, health, sleep;
  final int satiety, mood, cleanliness;
  final int speciesId;
  String get speciesName => speciesId == 1 ? '小鳥' : speciesId == 2 ? '小飛龍' : '未知種類';
  String get portraitEmoji => isDeparted ? '👋' : isDead ? '†' : stage == 'egg' ? '🥚' :
      speciesId == 2 ? '🐉' : speciesId == 1 ? (stage == 'baby' ? '🐣' : '🐦') : '？';
  final int? level, exp, expToNextLevel;
  final bool isDead, isDeparted;

  static PetSnapshot? parse(Object? value) {
    if (value is! Map<String, dynamic>) return null;
    final id = value['id'];
    final name = value['name'];
    final species = value.containsKey('species_id') ? value['species_id'] : 1;
    if (species is! int || species < 1 || species > 255) return null;
    final stage = value['life_stage'];
    final age = value['age_seconds'];
    final health = value['health'];
    final sleep = value['sleep'];
    final dead = value['is_dead'];
    final departed = value.containsKey('is_departed') ? value['is_departed'] : false;
    if (departed is! bool || (departed && (dead == true || stage == 'egg' || sleep != 'awake'))) return null;
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
      speciesId: species,
      satiety: satiety, mood: mood, cleanliness: cleanliness,
      ageSeconds: age, health: health as String, isDead: dead, isDeparted: departed,
      sleep: sleep as String,
      level: level as int?, exp: exp as int?, expToNextLevel: next as int?,
    );
  }
}
