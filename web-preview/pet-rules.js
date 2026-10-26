/* Shared Web demo rules from project-plan sections 3, 4.2.1 and 4.3.
 * Never applied to BLE state; for a device pet they only explain what the snapshot says. */
(()=>{'use strict';
const threshold=level=>level>=20?0:50+25*(level-1);
const totalExp=p=>{let total=p.exp;for(let level=1;level<p.level;level++)total+=threshold(level);return total;};
const lowCount=p=>Number(p.satiety<=50)+Number(p.cleanliness<=50);
function addExp(p,gain){
 const old=p.level;p.exp+=gain;
 while(p.level<20&&p.exp>=threshold(p.level)){p.exp-=threshold(p.level);p.level++;}
 if(p.level===20)p.exp=0;p.expToNextLevel=threshold(p.level);return p.level-old;
}
// A device snapshot with sad_reasons carries sadReasons.device: the firmware's buffered judgement is used as is.
function updateSad(p){
 if(p.sadReasons?.device)return ['mood','satiety','cleanliness','sick'].some(k=>p.sadReasons[k]);
 const flags=p.sadReasons||{mood:false,satiety:false,cleanliness:false};
 for(const [key,on,off] of [['mood',40,45],['satiety',25,30],['cleanliness',25,30]])flags[key]=flags[key]?p[key]<off:p[key]<=on;
 p.sadReasons=flags;return p.healthState==='sick'||Object.values(flags).some(Boolean);
}
function tick(p,seconds){
 if(!Number.isSafeInteger(seconds)||seconds<0)throw new Error('無效有效時間');
 const t=p.timers||(p.timers={satiety:0,mood:0,cleanliness:0,recovery:0,danger:0,sick:0});
 for(let i=0;i<seconds&&!p.isDeparted&&p.healthState!=='dead';i++){
  p.ageSeconds=Math.min(Number.MAX_SAFE_INTEGER,p.ageSeconds+1);
  if(p.stage==='egg'){if(p.ageSeconds>=300){p.stage='baby';p.satiety=80;p.mood=80;p.cleanliness=100;}continue;}
  const sleeping=p.sleepMode==='normal',sick=p.healthState==='sick',exposed=p.satiety===0||p.cleanliness===0,priorLow=lowCount(p);
  if(sick){if(++t.sick>=86400){p.healthState='dead';p.sleepMode='awake';}}
  else if(exposed){if(++t.danger>=21600)p.healthState='sick';}else t.danger=0;
  const rates=p.stage==='baby'?{satiety:60,cleanliness:90,mood:120}:{satiety:600,cleanliness:900,mood:1200};
  for(const key of ['satiety','cleanliness'])if(++t[key]>=rates[key]){t[key]-=rates[key];p[key]=Math.max(0,p[key]-1);}
  if(sleeping){t.recovery+=2-priorLow;if(t.recovery>=2400){t.recovery-=2400;p.mood=Math.min(100,p.mood+1);}}
  else if(++t.mood>=rates.mood){t.mood-=rates.mood;p.mood=Math.max(0,p.mood-1-lowCount(p));}
  if(p.stage==='baby'&&p.ageSeconds>=3900&&p.healthState==='healthy')p.stage='adult';
 }
 updateSad(p);
}
function sadnessReasons(p){
 if(!p||p.stage==='egg'||p.isDeparted||p.healthState==='dead')return [];
 const copy=JSON.parse(JSON.stringify(p));updateSad(copy);const reasons=[];
 for(const [key,label,target] of [['mood','心情',45],['satiety','飽食度',30],['cleanliness','清潔度',30]])if(copy.sadReasons[key])reasons.push(label+' '+p[key]+'，恢復至 '+target+' 可解除');
 if(copy.sadReasons.device?copy.sadReasons.sick:p.healthState==='sick')reasons.push('生病中，需要治療');return reasons;
}
// Device countdown to sickness or death, from the snapshot's seconds and device_info.rules; '' without both.
// While the snapshot is live the device keeps counting, so the time since it arrived is taken off (elapsed seconds).
function duration(s){const h=Math.floor(s/3600),m=Math.floor(s%3600/60);return h?h+' 小時'+(m?' '+m+' 分':''):m?m+' 分鐘':'不到 1 分鐘';}
function countdown(p,rules,elapsed=0){
 if(!p||!rules||p.stage==='egg'||p.isDeparted||p.healthState==='dead')return '';
 if(p.healthState==='sick'&&Number.isInteger(p.sickSeconds)&&rules.death_after_sick_seconds)return '若不治療，約 '+duration(Math.max(0,rules.death_after_sick_seconds-p.sickSeconds-elapsed))+'後會離世';
 if(p.healthState==='healthy'&&p.dangerSeconds>0&&rules.sick_after_seconds)return (p.satiety===0&&p.cleanliness===0?'飽食度與清潔度':p.satiety===0?'飽食度':'清潔度')+'歸零，約 '+duration(Math.max(0,rules.sick_after_seconds-p.dangerSeconds-elapsed))+'後會生病';
 return '';
}
window.PetRules={threshold,totalExp,lowCount,addExp,updateSad,tick,sadnessReasons,countdown};
})();
