/* Local memorial album, separated by data source and device. Never invent BLE history. */
(()=>{'use strict';
const uuid=()=>crypto.randomUUID?crypto.randomUUID():Date.now().toString(36)+'-'+Math.random().toString(36).slice(2);
function valid(r){return Array.isArray(r)&&r.every(x=>x&&typeof x.key==='string'&&typeof x.deviceId==='string'&&['mock','ble'].includes(x.source)&&typeof x.name==='string'&&typeof x.ageSeconds==='string'&&/^\d+$/.test(x.ageSeconds)&&['egg','baby','adult'].includes(x.stage)&&typeof x.dead==='boolean'&&(x.speciesId===undefined||Number.isInteger(x.speciesId)&&x.speciesId>=1&&x.speciesId<=255)&&(x.departed===undefined||typeof x.departed==='boolean')&&!(x.dead&&x.departed));}
const records=PetLocalStore.read('memorials',valid)||[];
const validPhoto=url=>typeof url==='string'&&url.length<=400000&&/^data:image\/png;base64,[A-Za-z0-9+/]+={0,2}$/.test(url);
// Scene photos (garden plus pet, 2026-10-24) replace the older framed pet-only snapshots once. Device pets
// (source 'ble') are matched by device id + pet id; every departed record of that pet (one per connection session
// until the memorial sync merges them) gets the photo. It is Web-only extra data the ESP32 never sees.
function savePhoto(p,url){
 if(!validPhoto(url))return false;
 const ble=!!p?.deviceId&&records.some(r=>r.source==='ble'&&r.deviceId===p.deviceId&&r.petId===p.petId&&r.departed);
 const match=r=>ble?r.source==='ble'&&r.deviceId===p.deviceId&&r.petId===p.petId&&r.departed:r.source==='mock'&&r.petId===p.petId&&(r.dead||r.departed);
 const todo=records.map((r,i)=>match(r)?i:-1).filter(i=>i>=0);if(!todo.length)return false;
 const missing=todo.filter(i=>!(validPhoto(records[i].photo)&&records[i].photoKind==='scene'));if(!missing.length)return true;
 const next=records.slice();for(const i of missing)next[i]={...next[i],photo:url,photoKind:'scene'};
 if(!PetLocalStore.write('memorials',next))return false;records.splice(0,records.length,...next);return true;
}
function remember(p,source,session){
 const deviceId=source==='mock'?'展示花園':p.deviceId;if(!deviceId)return false;
 const identity=source==='mock'?p.petId:(p.saveGeneration? p.saveGeneration+':'+p.petId:session+':'+p.petId);
 const key=source+':'+deviceId+':'+identity,dead=p.healthState==='dead'||p.condition==='dead'||p.health==='dead',departed=p.isDeparted===true;
 const record={speciesId:p.speciesId??1,departed,kind:departed?'departed':dead?'dead':'observed',key,source,deviceId,name:p.name,petId:p.petId||'',ageSeconds:String(p.ageSeconds),stage:p.stage,dead,provisional:source==='ble'&&!p.saveGeneration,receivedAt:new Date().toISOString()};
 const next=records.slice(),at=next.findIndex(r=>r.key===key);
 // The album shows when the pet was first seen and its level at the end; earlier sightings keep the first date.
 record.firstSeenAt=at>=0?(next[at].firstSeenAt||next[at].receivedAt):record.receivedAt;if(Number.isInteger(p.level))record.level=p.level;
 if(source==='mock'){record.previewSpeciesId=p.previewSpeciesId??(at>=0?next[at].previewSpeciesId:undefined);}if(at<0)next.push(record);else if(!next[at].dead&&!next[at].departed)next[at]=record;else if(source==='mock'&&p.previewSpeciesId&&!next[at].previewSpeciesId)next[at]={...next[at],previewSpeciesId:p.previewSpeciesId};
 if(!PetLocalStore.write('memorials',next))return false;records.splice(0,records.length,...next);return true;
}
// Postmark date: when this browser first saw the pet leave, else today.
function postDate(iso){const d=iso?new Date(iso):new Date(),day=isNaN(d)?new Date():d;return day.getFullYear()+'.'+String(day.getMonth()+1).padStart(2,'0')+'.'+String(day.getDate()).padStart(2,'0');}
function farewellDate(p){return postDate(records.find(r=>r.departed&&r.petId===p?.petId&&(!p.deviceId||r.deviceId===p.deviceId))?.receivedAt);}
const gardenDay=typeof Image==='undefined'?null:Object.assign(new Image(),{src:'assets/garden-detailed-day.png?v=cute-farm-v2-20261016'});
function duration(seconds){const n=BigInt(seconds),h=n/3600n,m=n%3600n/60n,s=n%60n;return (h?h+' 小時 ':'')+(m?m+' 分 ':'')+s+' 秒';}
function setHidden(keys,hidden=true){
 const set=new Set(keys),next=records.map(r=>set.has(r.key)?{...r,hiddenFromAlbum:hidden}:r);
 if(!PetLocalStore.write('memorials',next))return false;
 records.splice(0,records.length,...next);renderCards();return true;
}
// Once the undo window has passed, a deleted record keeps only what stops it being re-created for the
// current pet; photos, dates and level are dropped so deleted postcards stop filling browser storage.
const REQUIRED=['key','source','deviceId','name','petId','ageSeconds','stage','dead','departed','speciesId','kind','receivedAt'];
const bulky=r=>r.hiddenFromAlbum&&Object.keys(r).some(k=>!REQUIRED.includes(k)&&k!=='hiddenFromAlbum');
function purge(keys){
 const set=keys?new Set(keys):null,next=records.map(r=>(!set||set.has(r.key))&&bulky(r)?Object.fromEntries([...REQUIRED.filter(k=>k in r).map(k=>[k,r[k]]),['hiddenFromAlbum',true]]):r);
 if(next.every((r,i)=>r===records[i])||!PetLocalStore.write('memorials',next))return false;
 records.splice(0,records.length,...next);return true;
}
// --- Scrapbook album: departed pets as postcards, resting pets as rest cards, newest first.
// Pets still alive (or never ended) are kept only as background data for their first-seen date.
const $=s=>document.querySelector(s);
const SPECIES={1:'小鳥',2:'小飛龍'},STAGE_LINE={egg:'還是一顆蛋',baby:'長到幼年',adult:'長到成年'};
const speciesOf=r=>r.previewSpeciesId??r.speciesId??1,ended=r=>r.dead||r.departed;
const make=(tag,className,text)=>{const el=document.createElement(tag);if(className)el.className=className;if(text!==undefined)el.textContent=text;return el;};
let garden='mock:展示花園',pendingDelete=null,toastTimer=null,toastKeys=null,selecting=false;const selected=new Set();
function dateRange(r){
 const end=postDate(r.receivedAt);if(!r.firstSeenAt)return end+' 告別';const start=postDate(r.firstSeenAt);
 return start===end?end:start.slice(0,4)===end.slice(0,4)?start+' – '+end.slice(5):start+' – '+end;
}
// A small fixed tilt per card, so the page looks hand-pasted without shuffling between visits.
function tilt(r){let h=0;for(const ch of r.key)h=(h*31+ch.codePointAt(0))>>>0;return ((h%5)-2)*.6;}
function artFor(r,scale=2){
 const art=make('canvas','album-art');art.width=300*scale;art.height=192*scale;art.setAttribute('role','img');art.setAttribute('aria-label',r.name+(r.departed?' 的遠行明信片':' 的安息卡'));
 if(!art.getContext)return art;
 const species=speciesOf(r),saved=r.departed&&validPhoto(r.photo)&&r.photoKind==='scene'&&typeof Image!=='undefined'?Object.assign(new Image(),{src:r.photo}):null;
 const paint=()=>{const ctx=art.getContext('2d');if(!ctx)return;ctx.setTransform(scale,0,0,scale,0,0);ctx.clearRect(0,0,300,192);
  if(r.departed&&window.PetFarewellView){const photo=saved?.complete&&saved.naturalWidth?saved:PetFarewellView.scene(species,r.stage,gardenDay);
   PetFarewellView.postcard(ctx,{photo,name:r.name,age:duration(r.ageSeconds),date:postDate(r.receivedAt),petId:r.petId,species,stage:r.stage});}
  else if(r.dead&&window.PetDeathView)PetDeathView.restCard(ctx,{species,stage:r.stage,name:r.name,petId:r.petId,label:r.name+' · '+(SPECIES[species]||'未知種類')+' · 長眠',background:gardenDay});};
 paint();saved?.addEventListener?.('load',paint);gardenDay?.addEventListener?.('load',paint,{once:true});for(const ms of [400,1200])setTimeout(paint,ms);
 return art;
}
function details(r,order){
 return [r.name+' · '+(SPECIES[speciesOf(r)]||'未知種類'),'第 '+order+' 位夥伴 · '+(r.departed?'遠行':'長眠'),dateRange(r),
  ['陪伴 '+duration(r.ageSeconds),STAGE_LINE[r.stage],Number.isInteger(r.level)?'Lv '+r.level:null].filter(Boolean).join(' · ')];
}
function metaBlock(r,order){
 const [title,...lines]=details(r,order),meta=make('div','album-meta');meta.append(make('h3','',title));for(const line of lines)meta.append(make('p','',line));
 if(r.provisional)meta.append(make('small','','舊協定未提供存檔世代；此為本次連線觀察紀錄，跨重新連線可能分成多筆。'));
 return meta;
}
function card(r,order){
 const c=make('article','album-card '+(r.departed?'departed':'dead'));c.setAttribute('style','--tilt:'+tilt(r)+'deg');
 // While selecting, the whole card (picture, text and tick) toggles it instead of opening the enlarged view.
 const open=make('button','album-open');open.type='button';open.append(artFor(r));
 if(selecting){const on=selected.has(r.key),toggle=()=>{on?selected.delete(r.key):selected.add(r.key);renderCards();};
  c.className+=' selecting'+(on?' selected':'');c.setAttribute('role','checkbox');c.setAttribute('aria-checked',String(on));c.setAttribute('aria-label',r.name);c.setAttribute('tabindex','0');
  c.onclick=toggle;c.onkeydown=e=>{if(e.key===' '||e.key==='Enter'){e.preventDefault();toggle();}};open.tabIndex=-1;}
 else{open.setAttribute('aria-label','放大看 '+r.name+' 的'+(r.departed?'明信片':'安息卡'));open.onclick=()=>view(r,order);}
 const del=make('button','album-delete','刪除');del.type='button';del.setAttribute('aria-label','刪除 '+r.name+' 的紀念');del.onclick=()=>askDelete([r]);
 c.append(open,metaBlock(r,order));if(selecting)c.append(make('span','album-check','✓'));else c.append(del);return c;
}
function view(r,order){$('#album-viewer-body').replaceChildren(artFor(r,3),metaBlock(r,order));$('#album-viewer').showModal?.();}
function askDelete(list){
 if(!list.length)return;pendingDelete=list;
 $('#album-delete-text').textContent=list.length===1?'要刪除 '+list[0].name+' 的紀念嗎？':'要刪除選取的 '+list.length+' 筆紀念嗎？';$('#album-delete').showModal?.();
}
function toast(list){
 const t=$('#album-toast'),keys=list.map(r=>r.key);
 // A newer deletion ends the previous undo window straight away.
 clearTimeout(toastTimer);if(toastKeys)purge(toastKeys);toastKeys=keys;
 $('#album-toast-text').textContent=list.length===1?'已刪除 '+list[0].name+' 的紀念':'已刪除 '+list.length+' 筆紀念';t.hidden=false;
 $('#album-undo').onclick=()=>{clearTimeout(toastTimer);toastKeys=null;setHidden(keys,false);t.hidden=true;};
 toastTimer=setTimeout(()=>{t.hidden=true;toastKeys=null;purge(keys);},6000);
}
function actions(shown){
 const bar=$('#album-actions');bar.replaceChildren();
 const button=(text,onclick,className='album-action')=>{const b=make('button',className,text);b.type='button';b.onclick=onclick;bar.append(b);return b;};
 if(!selecting){if(shown.length)button('選取',()=>{selecting=true;selected.clear();renderCards();});return;}
 const all=shown.length>0&&shown.every(r=>selected.has(r.key));
 button(all?'取消全選':'全選',()=>{for(const r of shown)all?selected.delete(r.key):selected.add(r.key);renderCards();});
 const picked=shown.filter(r=>selected.has(r.key)),del=button('刪除（'+picked.length+'）',()=>askDelete(picked),'album-action danger');del.disabled=!picked.length;
 button('取消',()=>{selecting=false;selected.clear();renderCards();});
}
const gardenName=key=>key.startsWith('mock:')?'展示花園':key.slice(key.indexOf(':')+1);
function render(preferred){
 if(preferred)garden=preferred;
 const gardens=new Set([garden,...records.filter(ended).map(r=>r.source+':'+r.deviceId)]),select=$('#cemetery-device');select.replaceChildren();
 for(const key of gardens){const o=make('option','',gardenName(key));o.value=key;select.append(o);}select.value=garden;
 // The garden switch only appears when this browser keeps more than one garden.
 $('#album-garden').hidden=gardens.size<2;renderCards();
}
function renderCards(){
 const list=$('#memorial-list'),category=$('#memorial-category').value||'all';list.replaceChildren();
 $('#album-device-note').hidden=!garden.startsWith('ble:');
 const ordered=records.filter(r=>ended(r)&&r.source+':'+r.deviceId===garden).sort((a,b)=>String(a.receivedAt).localeCompare(String(b.receivedAt)));
 const order=new Map(ordered.map((r,i)=>[r.key,i+1])),visible=ordered.filter(r=>!r.hiddenFromAlbum);
 const tabs=$('#album-tabs');tabs.replaceChildren();
 for(const [value,label,count] of [['all','全部',visible.length],['departed','遠行',visible.filter(r=>r.departed).length],['dead','長眠',visible.filter(r=>r.dead).length]]){
  const b=make('button','album-tab'+(value===category?' active':''),label+(value==='all'?'':' '+count));b.type='button';b.setAttribute('aria-pressed',String(value===category));
  b.onclick=()=>{$('#memorial-category').value=value;selected.clear();renderCards();};tabs.append(b);}
 const shown=visible.filter(r=>category==='all'||(category==='departed')===!!r.departed).reverse();
 if(!shown.length)list.append(make('p','album-empty',category==='departed'?'這裡還沒有遠行的明信片。':category==='dead'?'這裡還沒有長眠的夥伴。':'花園裡還沒有回憶，好好陪伴現在的夥伴吧。'));
 for(const r of shown)list.append(card(r,order.get(r.key)));
 actions(shown);
}
window.PetMemorials={uuid,remember,duration,savePhoto,farewellDate,records:()=>JSON.parse(JSON.stringify(records)),hasDemoName:name=>records.some(r=>r.source==='mock'&&(r.dead||r.departed)&&r.name===name),open(preferred){render(preferred);$('#cemetery-dialog').showModal();}};
$('#cemetery-device').onchange=()=>{garden=$('#cemetery-device').value;renderCards();};
$('#album-delete-confirm').onclick=()=>{const list=pendingDelete;pendingDelete=null;$('#album-delete').close?.();if(list&&setHidden(list.map(r=>r.key))){selecting=false;selected.clear();renderCards();toast(list);}};
$('#cemetery-dialog').addEventListener('close',()=>{selecting=false;selected.clear();});
// Deletions whose undo window was cut short by closing the page are cleaned up on the next load.
purge();
})();
