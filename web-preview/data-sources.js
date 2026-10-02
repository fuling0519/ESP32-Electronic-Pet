/* Domain data only; persistence never writes back to firmware. */
(()=>{'use strict';
const newPetId=()=>crypto.randomUUID?crypto.randomUUID():Date.now().toString(36)+'-'+Math.random().toString(36).slice(2);
function generateAbbName(){
  const prefixes='Ba Be Bo Da Do Fu Ha Ho Ka Ki Ko Ku Ma Mi Mo Na Ni No Pa Pi Po Ta Te To'.split(' '),suffixes='ka ki ko ma mi mo na ni no pa pi po ra ri ro ru ta to'.split(' ');
  let name;for(let attempt=0;attempt<9;attempt++){const value=crypto.getRandomValues(new Uint32Array(1))[0],a=prefixes[value%prefixes.length];let bIndex=Math.floor(value/prefixes.length)%suffixes.length;if(a.toLowerCase()===suffixes[bIndex])bIndex=(bIndex+1)%suffixes.length;const b=suffixes[bIndex];name=a+b+b;if(!window.PetMemorials?.hasDemoName(name))break;}return name;
}
class MockSource {
  constructor(){this.pet=PetLocalStore.read('demo',PetLocalStore.validPet)||this.defaults();if(!this.pet.petId){this.pet.petId=newPetId();PetLocalStore.write('demo',this.pet);}}
  defaults(){return {petId:newPetId(),name:'Tamama',stage:'adult',condition:'awake',ageSeconds:3600,satiety:60,mood:55,cleanliness:45};}
  reset(){this.pet=this.defaults();PetLocalStore.resetDemo(this.pet);}
  read(){return {source:'mock',connection:'demo',pet:{...this.pet}};}
  preview(stage,condition){if(this.pet.condition==='dead')return;const order=['egg','baby','adult'],current=order.indexOf(this.pet.stage),next=order.indexOf(stage);if(next<current||next>current+1||next<0)return;this.pet.stage=stage;this.pet.condition=condition;PetLocalStore.write('demo',this.pet);}
  adopt(){this.pet={...this.defaults(),name:generateAbbName(),stage:'egg',ageSeconds:0,satiety:60,mood:55,cleanliness:45};PetLocalStore.write('demo',this.pet);}
  command(kind){if(this.pet.condition==='dead'||this.pet.stage==='egg')throw new Error('目前不能照顧');if(kind==='treat'){if(this.pet.condition!=='sick')throw new Error('健康時不需治療');this.pet.condition='awake';PetLocalStore.write('demo',this.pet);return {treated:true};}const rules={feed:['satiety',20],clean:['cleanliness',30],play:['mood',15]},rule=rules[kind];if(!rule)throw new Error('不支援的展示操作');const [key,gain]=rule,actual=Math.min(gain,100-this.pet[key]);this.pet[key]+=actual;PetLocalStore.write('demo',this.pet);return {key,actual};}
}
class BleSource {
  read(){return {source:'ble',connection:'disconnected',pet:null};}
  command(){throw new Error('尚未連接裝置，真實 BLE 命令尚未開放。');}
}
window.PetSources={mock:new MockSource(),ble:new BleSource()};
})();




