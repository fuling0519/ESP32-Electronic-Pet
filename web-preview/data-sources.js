/* Domain data only; persistence never writes back to firmware. */
(()=>{'use strict';
const newPetId=()=>{
 if(crypto.randomUUID)return crypto.randomUUID();
 const bytes=crypto.getRandomValues(new Uint8Array(16));bytes[6]=(bytes[6]&15)|64;bytes[8]=(bytes[8]&63)|128;
 const hex=Array.from(bytes,b=>b.toString(16).padStart(2,'0')).join('');return hex.slice(0,8)+'-'+hex.slice(8,12)+'-'+hex.slice(12,16)+'-'+hex.slice(16,20)+'-'+hex.slice(20);
};
function generateAbbName(){
  const prefixes='Ba Be Bo Da Do Fu Ha Ho Ka Ki Ko Ku Ma Mi Mo Na Ni No Pa Pi Po Ta Te To'.split(' '),suffixes='ka ki ko ma mi mo na ni no pa pi po ra ri ro ru ta to'.split(' ');
  let name;for(let attempt=0;attempt<9;attempt++){const value=crypto.getRandomValues(new Uint32Array(1))[0],a=prefixes[value%prefixes.length];let bIndex=Math.floor(value/prefixes.length)%suffixes.length;if(a.toLowerCase()===suffixes[bIndex])bIndex=(bIndex+1)%suffixes.length;const b=suffixes[bIndex];name=a+b+b;if(!window.PetMemorials?.hasDemoName(name))break;}return name;
}
class MockSource {
  constructor(){
    this.pet=PetLocalStore.read('demo',PetLocalStore.validPet)||this.defaults();
    this.pet.healthState=this.pet.healthState||(this.pet.condition==='dead'?'dead':this.pet.condition==='sick'?'sick':'healthy');
    this.pet.sleepMode=this.pet.sleepMode||(this.pet.condition==='sleep'?'normal':'awake');
    delete this.pet.condition;delete this.pet.health;this.pet.speciesId??=1;this.pet.isDeparted??=false;this.pet.level??=1;this.pet.exp??=0;this.pet.expToNextLevel=PetRules.threshold(this.pet.level);this.pendingAdoption=null;this.game=null;
    // Recovery units changed from seconds to weighted units in the 10/9 baseline.
    if(this.pet.timers&&this.pet.recoveryFormat!==2)this.pet.timers.recovery=0;this.pet.recoveryFormat=2;
    if(!this.pet.petId){this.pet.petId=newPetId();this.persist();}
  }
  defaults(){return {petId:newPetId(),name:'Tamama',speciesId:1,isDeparted:false,level:1,exp:0,expToNextLevel:50,recoveryFormat:2,stage:'adult',healthState:'healthy',sleepMode:'awake',ageSeconds:3600,satiety:60,mood:55,cleanliness:45};}
  reset(){this.pendingAdoption=null;this.game=null;this.pet=this.defaults();PetLocalStore.resetDemo(this.pet);}
  read(){return {source:'mock',connection:'demo',pet:JSON.parse(JSON.stringify(this.pet))};}
  preview(stage,condition){
    const p=this.pet,order=['egg','baby','adult'],current=order.indexOf(p.stage),next=order.indexOf(stage);
    if(p.isDeparted||p.healthState==='dead'||next<current||next>current+1||next<0||!['awake','sleep','sick','dead'].includes(condition))return;
    if(stage==='egg'&&condition!=='awake')return;
    p.stage=stage;
    if(condition==='dead'){p.healthState='dead';p.sleepMode='awake';}
    else if(condition==='sick'){p.healthState='sick';p.sleepMode='awake';}
    else {if(condition==='sleep'&&p.sleepMode!=='normal'&&p.timers)p.timers.mood=0;p.sleepMode=condition==='sleep'?'normal':'awake';}
    this.persist();
  }
  persist(){PetLocalStore.write('demo',this.pet);}
  tick(seconds){PetRules.tick(this.pet,seconds);this.persist();}
  adopt(){
    if(!this.pendingAdoption)this.pendingAdoption={...this.defaults(),petId:newPetId(),name:generateAbbName(),speciesId:1+(crypto.getRandomValues(new Uint32Array(1))[0]%2),stage:'egg',ageSeconds:0,satiety:80,mood:80,cleanliness:100};
    if(!PetLocalStore.write('demo',this.pendingAdoption,true))throw new Error('新蛋保存失敗，重試會保留同一個體、名字及種類。');
    this.pet=this.pendingAdoption;this.pendingAdoption=null;this.game=null;
  }
  farewell(){
    const p=this.pet;if(p.isDeparted||p.healthState==='dead'||p.stage==='egg'||p.sleepMode!=='awake')throw new Error('目前不能送別');
    const candidate={...p,isDeparted:true};
    if(!PetLocalStore.write('demo',candidate,true))throw new Error('送別保存失敗，請重試');
    this.pet=candidate;this.game=null;
    if(!PetMemorials.remember(candidate,'mock',''))throw new Error('送別快照已保存，紀念冊保存待重試；領養前會再次檢查。');
  }
  startGame(kind,now){
    const p=this.pet;if(p.isDeparted||p.healthState!=='healthy'||p.stage==='egg'||p.sleepMode!=='awake')throw new Error('目前不能遊玩');
    this.game=new PetGame(kind,p,now);return this.gameState(now);
  }
  gameState(now){
    const g=this.game,p=this.pet;if(!g)return null;
    if(g.petId!==p.petId||g.stage!==p.stage||p.healthState!=='healthy'||p.isDeparted||p.sleepMode!=='awake'){g.cancelled=true;g.phase='cancelled';}
    g.advance(now);
    if(g.phase==='complete'&&!g.settled&&!g.cancelled){g.settled=true;g.reward=g.rewards();g.reward.mood=Math.min(g.reward.mood,100-p.mood);p.mood+=g.reward.mood;const beforeExp=PetRules.totalExp(p);g.levelsGained=PetRules.addExp(p,g.reward.exp);g.reward.exp=PetRules.totalExp(p)-beforeExp;PetRules.updateSad(p);this.persist();}
    const state={kind:g.kind,phase:g.phase,round:g.round,score:g.score,index:g.index,correct:g.correct,reward:g.reward,levelsGained:g.levelsGained||0};
    if(g.kind==='rps'&&g.phase==='countdown')state.countdown=Math.max(1,Math.min(3,Math.ceil((g.until-now)/350)));
    if(g.kind==='memory'){state.totalRounds=3;state.noteCount=[2,3,5][g.round-1];if(g.phase==='prepare')state.prepareSeconds=Math.max(1,Math.ceil((g.prepareUntil-now)/1000));}
    // Opponent is hidden until reveal. Memory input never contains the answer sequence.
    if(g.phase==='round-result'||g.phase==='complete'){state.player=g.player;state.opponent=g.opponent;state.outcome=g.outcome;}
    state.demoIndex=g.noteIndex;if(g.phase==='show')state.lit=g.noteOn?g.sequence[g.noteIndex]:null;state.canContinue=g.phase==='round-result'&&now>=g.readyAt;
    return state;
  }
  answerGame(choice,now){if(this.gameState(now)?.phase==='cancelled')return this.gameState(now);this.game?.answer(choice,now);return this.gameState(now);}
  nextRound(now){this.game?.continue(now);return this.gameState(now);}
  cancelGame(){if(this.game){this.game.cancelled=true;this.game.phase='cancelled';}}
  // Demo tools only: drop or refill one need so its emote and sad face can be checked without waiting.
  demoNeed(key,value){if(!['satiety','cleanliness','mood'].includes(key))throw new Error('未知需求');const p=this.pet;p[key]=Math.max(0,Math.min(100,value));PetRules.updateSad(p);this.persist();}
  command(kind){
    const p=this.pet;
    if(p.isDeparted||p.healthState==='dead'||p.sleepMode!=='awake'||p.stage==='egg'||(kind==='play'&&p.healthState==='sick'))throw new Error('目前不能照顧');
    if(kind==='treat'){
      if(p.healthState!=='sick')throw new Error('健康時不需治療');
      p.healthState='healthy';if(p.timers){p.timers.danger=0;p.timers.sick=0;}
      if(p.stage==='baby'&&p.ageSeconds>=3900)p.stage='adult';const levelsGained=PetRules.addExp(p,5);PetRules.updateSad(p);this.persist();return {treated:true,levelsGained};
    }
    if(kind==='pet'||kind==='poke'){
      if(p.healthState!=='healthy')throw new Error('寵物不舒服，請先治療再互動');
      if(this.game&&!['complete','cancelled'].includes(this.game.phase))throw new Error('請先結束遊戲');
      const now=Date.now();if(this.lastInteraction?.petId===p.petId&&now-this.lastInteraction.at<3000)throw new Error('讓牠休息一下，互動間隔為3秒');
      const before=p.mood;p.mood=Math.max(0,Math.min(100,p.mood+(kind==='pet'?3:-3)));
      this.lastInteraction={petId:p.petId,at:now};PetRules.updateSad(p);this.persist();
      return {key:'mood',actual:p.mood-before,levelsGained:0,interaction:kind};
    }
    const rules={feed:['satiety',20],clean:['cleanliness',30]},rule=rules[kind];if(!rule)throw new Error('不支援的展示操作');
    const [key,gain]=rule,actual=Math.min(gain,100-p[key]);p[key]+=actual;
    if(p.healthState==='healthy'&&p.satiety>0&&p.cleanliness>0&&p.timers)p.timers.danger=0;
    const levelsGained=PetRules.addExp(p,actual>0?3:0);PetRules.updateSad(p);this.persist();return {key,actual,levelsGained};
  }
}
class BleSource {
  read(){return {source:'ble',connection:'disconnected',pet:null};}
  command(){throw new Error('尚未連接裝置，真實 BLE 命令尚未開放。');}
}
window.PetSources={mock:new MockSource(),ble:new BleSource()};
})();




