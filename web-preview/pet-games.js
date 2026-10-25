/* Species-independent garden games (toy play, chorus); rewards only on a completed session. */
(()=>{'use strict';
// Toy play (web only): the garden view runs the throws; each finished throw is reported as answer(1 caught, 0 fetched).
// Not a contest, so the reward is fixed once all throws are done.
class PetToyGame {
 constructor(pet,now,throws=5){this.kind='toy';this.petId=pet.petId;this.stage=pet.stage;this.total=throws;this.round=1;this.throws=0;this.catches=0;this.phase='play';this.settled=false;this.cancelled=false;this.startedAt=now;}
 advance(){return this;}
 answer(caught,now){if(this.phase!=='play'||![0,1].includes(caught))return;this.throws++;this.catches+=caught;if(this.throws>=this.total)this.phase='complete';else this.round=this.throws+1;}
 rewards(){return {mood:6,exp:10};}
}
// Garden chorus (web only): the garden view plays each round; answer(1) means the round was sung back right.
// Rewards follow the web memory-notes tiers, by rounds right out of three.
class PetChorusGame {
 constructor(pet,now,rounds=3){this.kind='chorus';this.petId=pet.petId;this.stage=pet.stage;this.total=rounds;this.round=1;this.played=0;this.correct=0;this.phase='play';this.settled=false;this.cancelled=false;this.startedAt=now;}
 advance(){return this;}
 answer(right,now){if(this.phase!=='play'||![0,1].includes(right))return;this.played++;this.correct+=right;if(this.played>=this.total)this.phase='complete';else this.round=this.played+1;}
 rewards(){return this.correct>=3?{mood:10,exp:15}:this.correct===2?{mood:6,exp:10}:{mood:3,exp:5};}
}
window.PetToyGame=PetToyGame;window.PetChorusGame=PetChorusGame;
})();
