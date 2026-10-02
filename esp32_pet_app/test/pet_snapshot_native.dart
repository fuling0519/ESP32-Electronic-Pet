import 'dart:io';
import 'package:esp32_pet_app/pet_snapshot.dart';

void main() {
  final base = <String, dynamic>{
    'id': '1', 'name': 'Tamama', 'life_stage': 'baby',
    'age_seconds': '300', 'health': 'healthy', 'is_dead': false,
    'sleep': 'awake', 'satiety': 80, 'mood': 100, 'cleanliness': 100,
  };
  assert(PetSnapshot.parse(base)?.level == null);
  final progress = {...base, 'level': 2, 'exp': 10, 'exp_to_next_level': 75};
  final pet = PetSnapshot.parse(progress)!;
  assert(pet.level == 2 && pet.exp == 10 && pet.expToNextLevel == 75);
  assert(PetSnapshot.parse({...base, 'level': 20, 'exp': 0, 'exp_to_next_level': 0}) != null);
  assert(PetSnapshot.parse({...base, 'level': 2}) == null);
  for (final invalid in <Map<String, dynamic>>[
    {'level': 0}, {'level': 256}, {'level': '2'},
    {'exp': -1}, {'exp': 75}, {'exp': 10.5},
    {'exp_to_next_level': -1}, {'exp_to_next_level': 0},
    {'exp_to_next_level': 65536}, {'exp_to_next_level': null},
  ]) {
    assert(PetSnapshot.parse({...progress, ...invalid}) == null);
  }
  assert(PetSnapshot.parse({...progress, 'health': 'dead'}) == null);
  stdout.writeln('PASS: old firmware, progress, MAX, partial groups and invalid BLE values.');
}
