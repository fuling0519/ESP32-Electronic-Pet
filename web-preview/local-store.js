/* Versioned records; demo, preferences and future device cache use separate keys. */
(()=>{'use strict';
const problems=new Map();
function report(){const el=document.querySelector('#storage-status');if(el)el.textContent=problems.size?[...problems.values()].join(' '):'資料保存在此瀏覽器；清除網站資料會移除存檔。';}
function read(key,validate){try{const raw=localStorage.getItem('aviary.'+key);if(raw===null)return null;const record=JSON.parse(raw);if(record.version!==1||!validate(record.data))throw new Error('invalid');return record.data;}catch(e){problems.set(key,'本地存檔無法讀取或版本不相容，暫用預設資料；原存檔保留。');report();return null;}}
function write(key,data){if(problems.has(key))return false;try{localStorage.setItem('aviary.'+key,JSON.stringify({version:1,data}));report();return true;}catch(e){problems.set(key,'本地保存失敗，目前變更僅保留在此頁面。');report();return false;}}
function validPet(p){return p&&typeof p.name==='string'&&p.name.length>0&&['egg','baby','adult'].includes(p.stage)&&['awake','sleep','sick','dead'].includes(p.condition)&&Number.isSafeInteger(p.ageSeconds)&&p.ageSeconds>=0&&['satiety','mood','cleanliness'].every(k=>Number.isInteger(p[k])&&p[k]>=0&&p[k]<=100);}
function validPreferences(p){return p&&['mock','ble'].includes(p.mode)&&['smooth','simple','garden'].includes(p.backdrop)&&typeof p.night==='boolean';}
function resetDemo(data){try{localStorage.removeItem('aviary.demo');problems.delete('demo');return write('demo',data);}catch(e){problems.set('demo','無法重設本地展示存檔。');report();return false;}}
window.PetLocalStore={read,write,validPet,validPreferences,resetDemo,report};
})();


