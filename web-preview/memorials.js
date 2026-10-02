/* Local observed history, separated by data source and device. Never invent BLE history. */
(()=>{'use strict';
const uuid=()=>crypto.randomUUID?crypto.randomUUID():Date.now().toString(36)+'-'+Math.random().toString(36).slice(2);
function valid(r){return Array.isArray(r)&&r.every(x=>x&&typeof x.key==='string'&&typeof x.deviceId==='string'&&['mock','ble'].includes(x.source)&&typeof x.name==='string'&&typeof x.ageSeconds==='string'&&/^\d+$/.test(x.ageSeconds)&&['egg','baby','adult'].includes(x.stage)&&typeof x.dead==='boolean');}
const records=PetLocalStore.read('memorials',valid)||[];
function remember(p,source,session){
 const deviceId=source==='mock'?'展示花園':p.deviceId;if(!deviceId)return false;
 const identity=source==='mock'?p.petId:(p.saveGeneration? p.saveGeneration+':'+p.petId:session+':'+p.petId);
 const key=source+':'+deviceId+':'+identity,dead=p.condition==='dead'||p.health==='dead';
 const record={key,source,deviceId,name:p.name,petId:p.petId||'',ageSeconds:String(p.ageSeconds),stage:p.stage,dead,provisional:source==='ble'&&!p.saveGeneration,receivedAt:new Date().toISOString()};
 const at=records.findIndex(r=>r.key===key);if(at<0)records.push(record);else if(!records[at].dead)records[at]=record;
 return PetLocalStore.write('memorials',records);
}
function duration(seconds){const n=BigInt(seconds),h=n/3600n,m=n%3600n/60n,s=n%60n;return (h?h+' 小時 ':'')+(m?m+' 分 ':'')+s+' 秒';}
function render(preferred){
 const select=document.querySelector('#cemetery-device'),previous=preferred||select.value;select.replaceChildren();
 const groups=new Map([['mock:展示花園','展示花園（模擬）']]);for(const r of records)groups.set(r.source+':'+r.deviceId,r.deviceId+(r.source==='ble'?'（BLE 本機紀錄）':''));
 for(const [value,label] of groups){const o=document.createElement('option');o.value=value;o.textContent=label;select.append(o);}if(groups.has(previous))select.value=previous;renderCards();
}
function renderCards(){const selected=document.querySelector('#cemetery-device').value,list=document.querySelector('#memorial-list');list.replaceChildren();
 const items=records.filter(r=>r.source+':'+r.deviceId===selected).slice().reverse();
 if(!items.length){const p=document.createElement('p');p.textContent='這個花園尚未保存寵物紀錄。';list.append(p);}
 for(const r of items){const card=document.createElement('article');card.className='memorial-card';const stone=document.createElement('div');stone.className='mini-stone';stone.textContent=r.dead?'RIP':'陪伴中';const name=document.createElement('h3');name.textContent=r.name;const info=document.createElement('p');info.textContent='代號：'+r.petId+' · '+{egg:'蛋',baby:'幼鳥',adult:'成鳥'}[r.stage];const age=document.createElement('p');age.textContent='陪伴時長（有效年齡）：'+duration(r.ageSeconds);card.append(stone,name,info,age);if(r.provisional){const note=document.createElement('small');note.textContent='舊協定未提供存檔世代；此為本次連線觀察紀錄，跨重新連線可能分成多筆。';card.append(note);}list.append(card);}
}
window.PetMemorials={uuid,remember,duration,hasDemoName:name=>records.some(r=>r.source==='mock'&&r.dead&&r.name===name),open(preferred){render(preferred);document.querySelector('#cemetery-dialog').showModal();}};
document.querySelector('#cemetery-device').onchange=renderCards;
})();
