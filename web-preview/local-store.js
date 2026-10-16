/* Versioned records; demo, preferences and future device cache use separate keys. */
(()=>{'use strict';
const problems=new Map(),blocked=new Set(),retries=new Map();
function report(){const el=document.querySelector('#storage-status');if(el)el.textContent=problems.size?[...problems.values()].join(' '):'資料保存在此瀏覽器；清除網站資料會移除存檔。';}
function read(key,validate){try{const raw=localStorage.getItem('aviary.'+key);if(raw===null)return null;const record=JSON.parse(raw);if(record.version!==1||!validate(record.data))throw new Error('invalid');if(key==='demo'){const p=record.data;p.healthState=p.healthState||(p.condition==='dead'?'dead':p.condition==='sick'?'sick':'healthy');p.sleepMode=p.sleepMode||(p.condition==='sleep'?'normal':'awake');p.condition=p.healthState==='dead'?'dead':p.sleepMode==='normal'?'sleep':p.healthState==='sick'?'sick':'awake';}return record.data;}catch(e){blocked.add(key);problems.set(key,'本地存檔無法讀取或版本不相容，暫用預設資料；原存檔保留。');report();return null;}}
function write(key,data,force=false){
 if(blocked.has(key))return false;
 const retry=retries.get(key);if(!force&&retry&&Date.now()<retry.at)return false;
 try{localStorage.setItem('aviary.'+key,JSON.stringify({version:1,data:key==='demo'?canonicalDemo(data):data}));retries.delete(key);problems.delete(key);report();return true;}
 catch(e){const attempt=(retry?.attempt||0)+1,delay=[5,10,30,60][Math.min(attempt-1,3)];retries.set(key,{attempt,at:Date.now()+delay*1000});problems.set(key,'本地保存失敗，'+delay+' 秒後可再試；變更仍在此頁面，請勿關閉。');report();return false;}
}
function canonicalDemo(p){const data={...p,healthState:p.healthState||(p.condition==='dead'?'dead':p.condition==='sick'?'sick':'healthy'),sleepMode:p.sleepMode||(p.condition==='sleep'?'normal':'awake')};delete data.condition;delete data.health;return data;}
function validPet(p){
 if(!p||typeof p.name!=='string'||!p.name.trim()||!['egg','baby','adult'].includes(p.stage)||!Number.isSafeInteger(p.ageSeconds)||p.ageSeconds<0)return false;
 const health=p.healthState||(p.condition==='dead'?'dead':p.condition==='sick'?'sick':'healthy'),sleep=p.sleepMode||(p.condition==='sleep'?'normal':'awake');
 if(p.healthState!==undefined?!['healthy','sick','dead'].includes(health)||!['awake','normal'].includes(p.sleepMode):!['awake','sleep','sick','dead'].includes(p.condition))return false;
 if(p.stage==='egg'&&(health!=='healthy'||sleep!=='awake')||health==='dead'&&sleep!=='awake')return false;
 if(p.speciesId!==undefined&&![1,2].includes(p.speciesId)||p.isDeparted!==undefined&&typeof p.isDeparted!=='boolean'||p.isDeparted&&(health==='dead'||p.stage==='egg'||sleep!=='awake'))return false;
 if(p.level!==undefined&&(!Number.isInteger(p.level)||p.level<1||p.level>20||!Number.isInteger(p.exp)||p.exp<0||p.level===20&&p.exp!==0||p.level<20&&p.exp>=50+25*(p.level-1)))return false;
 if(p.petId!==undefined&&(typeof p.petId!=='string'||!p.petId))return false;
 if(!['satiety','mood','cleanliness'].every(k=>Number.isInteger(p[k])&&p[k]>=0&&p[k]<=100))return false;
 if(p.timers){const limits={satiety:p.stage==='baby'?60:600,cleanliness:p.stage==='baby'?90:900,mood:p.stage==='baby'?120:1200,recovery:p.recoveryFormat===2?2400:600,danger:21601,sick:86401};if(!Object.entries(limits).every(([k,limit])=>Number.isSafeInteger(p.timers[k])&&p.timers[k]>=0&&p.timers[k]<limit))return false;}
 return true;
}
function validPreferences(p){return p&&['mock','ble'].includes(p.mode)&&['smooth','simple','garden'].includes(p.backdrop)&&typeof p.night==='boolean';}
function resetDemo(data){try{localStorage.removeItem('aviary.demo');problems.delete('demo');blocked.delete('demo');retries.delete('demo');return write('demo',data);}catch(e){problems.set('demo','無法重設本地展示存檔。');report();return false;}}
window.PetLocalStore={read,write,validPet,validPreferences,resetDemo,report};
})();


