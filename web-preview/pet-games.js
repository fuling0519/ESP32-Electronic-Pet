/* Species-independent demo games; rewards only on a completed session. */
(()=>{'use strict';
class PetGame {
 constructor(kind,pet,now,random=()=>crypto.getRandomValues(new Uint32Array(1))[0]){
  if(!['rps','memory'].includes(kind))throw new Error('未知遊戲');
  this.kind=kind;this.petId=pet.petId;this.stage=pet.stage;this.random=random;this.round=1;this.wins=0;this.losses=0;this.score=0;this.settled=false;this.cancelled=false;this.next(now);
 }
 next(now){this.phase=this.kind==='rps'?'choose':this.round===1?'prepare':'show';this.startedAt=now;this.index=0;if(this.kind==='rps')this.opponent=this.random()%3;if(this.kind==='memory'){this.noteIndex=0;this.noteOn=true;this.nextAt=now+250;if(this.phase==='prepare')this.prepareUntil=now+5000;this.sequence=Array.from({length:[2,3,5][this.round-1]},()=>this.random()%4);}}
 advance(now){
  if(this.kind==='rps'&&this.phase==='countdown'&&now>=this.until){this.phase='round-result';this.readyAt=now+800;if(this.round===3)this.phase='complete';}
  if(this.kind==='memory'){
   if(this.phase==='prepare'&&now>=this.prepareUntil){this.phase='show';this.noteOn=true;this.nextAt=now+250;}else if(this.phase==='show'&&now>=this.nextAt){if(this.noteOn){this.noteOn=false;this.nextAt=now+450;}else if(++this.noteIndex===this.sequence.length){this.phase='answer';this.until=now+5000;}else{this.noteOn=true;this.nextAt=now+250;}}
   if(this.phase==='answer'&&now>=this.until){this.correct=false;this.readyAt=now+800;this.phase=this.round===3?'complete':'round-result';}
   if(this.phase==='last-note'&&now>=this.until){this.correct=true;this.score++;this.readyAt=now+800;this.phase=this.round===3?'complete':'round-result';}
  }
  return this;
 }
 answer(choice,now){
  this.advance(now);if(!Number.isInteger(choice)||choice<0||choice>(this.kind==='rps'?2:3))return;
  if(this.kind==='rps'&&this.phase==='choose'){
   this.player=choice;const delta=(choice-this.opponent+3)%3;this.outcome=delta===0?'draw':delta===1?'win':'loss';if(delta===1)this.wins++;if(delta===2)this.losses++;this.phase='countdown';this.until=now+1050;
  }else if(this.kind==='memory'&&this.phase==='answer'){
   if(choice!==this.sequence[this.index]){this.correct=false;this.readyAt=now+800;this.phase=this.round===3?'complete':'round-result';}
   else if(++this.index===this.sequence.length){this.phase='last-note';this.until=now+180;}
   else this.until=now+5000;
  }
 }
 continue(now){this.advance(now);if(this.phase==='round-result'&&now>=this.readyAt){this.round++;this.next(now);}}
 rewards(){if(this.kind==='rps')return this.wins>this.losses?{mood:10,exp:15}:this.wins===this.losses?{mood:6,exp:10}:{mood:3,exp:5};return {mood:this.score===3?10:this.score===2?6:3,exp:this.score===3?15:this.score===2?10:5};}
}
// Toy play (web only): the garden view runs the throws; each finished throw is reported as answer(1 caught, 0 fetched).
// Not a contest, so the reward is fixed once all throws are done.
class PetToyGame {
 constructor(pet,now,throws=5){this.kind='toy';this.petId=pet.petId;this.stage=pet.stage;this.total=throws;this.round=1;this.throws=0;this.catches=0;this.phase='play';this.settled=false;this.cancelled=false;this.startedAt=now;}
 advance(){return this;}
 answer(caught,now){if(this.phase!=='play'||![0,1].includes(caught))return;this.throws++;this.catches+=caught;if(this.throws>=this.total)this.phase='complete';else this.round=this.throws+1;}
 continue(){}
 rewards(){return {mood:6,exp:10};}
}
// Garden chorus (web only): the garden view plays each round; answer(1) means the round was sung back right.
// Rewards follow the web memory-notes tiers, by rounds right out of three.
class PetChorusGame {
 constructor(pet,now,rounds=3){this.kind='chorus';this.petId=pet.petId;this.stage=pet.stage;this.total=rounds;this.round=1;this.played=0;this.correct=0;this.phase='play';this.settled=false;this.cancelled=false;this.startedAt=now;}
 advance(){return this;}
 answer(right,now){if(this.phase!=='play'||![0,1].includes(right))return;this.played++;this.correct+=right;if(this.played>=this.total)this.phase='complete';else this.round=this.played+1;}
 continue(){}
 rewards(){return this.correct>=3?{mood:10,exp:15}:this.correct===2?{mood:6,exp:10}:{mood:3,exp:5};}
}
window.PetGame=PetGame;window.PetToyGame=PetToyGame;window.PetChorusGame=PetChorusGame;
})();
