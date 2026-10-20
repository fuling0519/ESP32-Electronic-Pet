/* Local observed history, separated by data source and device. Never invent BLE history. */
(()=>{'use strict';
const uuid=()=>crypto.randomUUID?crypto.randomUUID():Date.now().toString(36)+'-'+Math.random().toString(36).slice(2);
function valid(r){return Array.isArray(r)&&r.every(x=>x&&typeof x.key==='string'&&typeof x.deviceId==='string'&&['mock','ble'].includes(x.source)&&typeof x.name==='string'&&typeof x.ageSeconds==='string'&&/^\d+$/.test(x.ageSeconds)&&['egg','baby','adult'].includes(x.stage)&&typeof x.dead==='boolean'&&(x.speciesId===undefined||Number.isInteger(x.speciesId)&&x.speciesId>=1&&x.speciesId<=255)&&(x.departed===undefined||typeof x.departed==='boolean')&&!(x.dead&&x.departed));}
const records=PetLocalStore.read('memorials',valid)||[];
const validPhoto=url=>typeof url==='string'&&url.length<=400000&&/^data:image\/png;base64,[A-Za-z0-9+/]+={0,2}$/.test(url);
function savePhoto(p,url){
 if(!validPhoto(url))return false;
 const at=records.findIndex(r=>r.source==='mock'&&r.petId===p.petId&&(r.dead||r.departed));
 if(at<0)return false;if(validPhoto(records[at].photo))return true;
 const next=records.slice();next[at]={...next[at],photo:url};
 if(!PetLocalStore.write('memorials',next))return false;records.splice(0,records.length,...next);return true;
}
function remember(p,source,session){
 const deviceId=source==='mock'?'展示花園':p.deviceId;if(!deviceId)return false;
 const identity=source==='mock'?p.petId:(p.saveGeneration? p.saveGeneration+':'+p.petId:session+':'+p.petId);
 const key=source+':'+deviceId+':'+identity,dead=p.healthState==='dead'||p.condition==='dead'||p.health==='dead',departed=p.isDeparted===true;
 const record={speciesId:p.speciesId??1,departed,kind:departed?'departed':dead?'dead':'observed',key,source,deviceId,name:p.name,petId:p.petId||'',ageSeconds:String(p.ageSeconds),stage:p.stage,dead,provisional:source==='ble'&&!p.saveGeneration,receivedAt:new Date().toISOString()};
 const next=records.slice(),at=next.findIndex(r=>r.key===key);if(source==='mock'){record.previewSpeciesId=p.previewSpeciesId??(at>=0?next[at].previewSpeciesId:undefined);}if(at<0)next.push(record);else if(!next[at].dead&&!next[at].departed)next[at]=record;else if(source==='mock'&&p.previewSpeciesId&&!next[at].previewSpeciesId)next[at]={...next[at],previewSpeciesId:p.previewSpeciesId};
 if(!PetLocalStore.write('memorials',next))return false;records.splice(0,records.length,...next);return true;
}
function duration(seconds){const n=BigInt(seconds),h=n/3600n,m=n%3600n/60n,s=n%60n;return (h?h+' 小時 ':'')+(m?m+' 分 ':'')+s+' 秒';}
function hideCard(key,hidden=true){
 const next=records.map(r=>r.key===key?{...r,hiddenFromAlbum:hidden}:r);
 if(!PetLocalStore.write('memorials',next))return false;
 records.splice(0,records.length,...next);renderCards();return true;
}
function render(preferred){
 const select=document.querySelector('#cemetery-device'),previous=preferred||select.value;select.replaceChildren();
 const groups=new Map([['mock:展示花園','展示花園（模擬）']]);for(const r of records)groups.set(r.source+':'+r.deviceId,r.deviceId+(r.source==='ble'?'（BLE 本機紀錄）':''));
 for(const [value,label] of groups){const o=document.createElement('option');o.value=value;o.textContent=label;select.append(o);}if(groups.has(previous))select.value=previous;renderCards();
}
function renderCards(){const selected=document.querySelector('#cemetery-device').value,list=document.querySelector('#memorial-list');list.replaceChildren();
 const category=document.querySelector('#memorial-category').value||'dead';
 const group=records.filter(r=>r.source+':'+r.deviceId===selected&&(r.departed?'departed':r.dead?'dead':'observed')===category);
 const items=group.filter(r=>!r.hiddenFromAlbum).slice().reverse();
 for(const r of group.filter(r=>r.hiddenFromAlbum)){const undo=document.createElement('button');undo.textContent='復原明信片：'+r.name;undo.onclick=()=>hideCard(r.key,false);list.append(undo);}
 if(!items.length){const p=document.createElement('p');p.textContent='這個花園尚未保存寵物紀錄。';list.append(p);}
 for(const r of items){const card=document.createElement('article');card.className=r.dead?'memorial-card':'memorial-card postcard-card';const photo=document.createElement('div');photo.className=r.dead?'memorial-portrait':'postcard-photo';const stone=document.createElement('div');stone.className=r.dead?'mini-stone':'postcard-stamp';stone.textContent=r.departed?'遠行':r.dead?'長眠':'陪伴中';const name=document.createElement('h3');name.textContent=r.name+' · '+({1:'小鳥',2:'小飛龍'}[r.previewSpeciesId??r.speciesId??1]||'未知種類');const info=document.createElement('p');info.textContent='代號：'+r.petId+' · '+{egg:'蛋',baby:'幼年',adult:'成年'}[r.stage];const age=document.createElement('p');age.textContent='陪伴時長（有效年齡）：'+duration(r.ageSeconds);if(r.dead)card.append(stone,name,info,age,photo);else card.append(stone,photo,name,info,age);const caption=document.createElement('p');caption.className='postcard-caption';caption.textContent=r.dead?'把陪伴留在這張明信片裡':r.departed?'帶著回憶，向下一段旅程出發':'花園裡的陪伴時光';if(!r.dead)card.append(caption);const savedPhoto=!r.dead&&validPhoto(r.photo);if(savedPhoto){const image=document.createElement('img');image.src=r.photo;image.alt=r.name+'的告別照片';photo.append(image);}if(!savedPhoto&&(r.previewSpeciesId??r.speciesId??1)===2){const art=document.createElement('canvas');art.width=128;art.height=88;const frame=WyvernView.frameFor({stage:r.stage,sleepMode:r.dead?'normal':'awake',healthState:'healthy',satiety:100,mood:80,cleanliness:100},0,null,0,true);WyvernView.paint(art.getContext('2d'),frame.bits,0,0,2);photo.append(art);}if(!savedPhoto&&(r.previewSpeciesId??r.speciesId??1)===1&&r.stage!=='egg'&&window.BirdView){const art=document.createElement('canvas');art.width=192;art.height=132;art.setAttribute('aria-label',(r.stage==='baby'?'幼鳥':'成鳥')+(r.dead?'長眠':'遠行'));BirdView.still(art.getContext('2d'),r.stage,r.dead,96,66,1.8,true);photo.append(art);}if(r.provisional){const note=document.createElement('small');note.textContent='舊協定未提供存檔世代；此為本次連線觀察紀錄，跨重新連線可能分成多筆。';card.append(note);}const remove=document.createElement('button');remove.textContent='移除明信片';remove.setAttribute('aria-label','移除 '+r.name+' 的明信片');remove.onclick=()=>hideCard(r.key);card.append(remove);list.append(card);}
}
window.PetMemorials={uuid,remember,duration,savePhoto,hasDemoName:name=>records.some(r=>r.source==='mock'&&(r.dead||r.departed)&&r.name===name),open(preferred){render(preferred);document.querySelector('#cemetery-dialog').showModal();}};
document.querySelector('#cemetery-device').onchange=renderCards;document.querySelector('#memorial-category').onchange=renderCards;
})();
