/* Local memorial album, separated by data source and device. Never invent BLE history. */
(()=>{'use strict';
const uuid=()=>crypto.randomUUID?crypto.randomUUID():Date.now().toString(36)+'-'+Math.random().toString(36).slice(2);
// Records synced from the ESP32's album (memorial-sync.js, 養成同步 phase 4) carry synced:true, the device's protected flag
// and save order, and may have the stage 'unknown' (old tombstones without one).
function valid(r){return Array.isArray(r)&&r.every(x=>x&&typeof x.key==='string'&&typeof x.deviceId==='string'&&['mock','ble'].includes(x.source)&&typeof x.name==='string'&&typeof x.ageSeconds==='string'&&/^\d+$/.test(x.ageSeconds)&&(['egg','baby','adult'].includes(x.stage)||x.synced===true&&x.stage==='unknown')&&typeof x.dead==='boolean'&&['synced','protected'].every(k=>x[k]===undefined||typeof x[k]==='boolean')&&(x.deviceOrder===undefined||Number.isInteger(x.deviceOrder)&&x.deviceOrder>=0)&&(x.speciesId===undefined||Number.isInteger(x.speciesId)&&x.speciesId>=1&&x.speciesId<=255)&&(x.departed===undefined||typeof x.departed==='boolean')&&!(x.dead&&x.departed));}
const records=PetLocalStore.read('memorials',valid)||[];
// One device pet: device id + pet id, also checking name and species (a reused id after an NVS rebuild is another pet).
const same=(r,p)=>r.source==='ble'&&r.deviceId===p.deviceId&&r.petId===p.petId&&r.name===p.name&&(r.speciesId??1)===(p.speciesId??1);
const validPhoto=url=>typeof url==='string'&&url.length<=400000&&/^data:image\/png;base64,[A-Za-z0-9+/]+={0,2}$/.test(url);
// Scene photos (garden plus pet, 2026-10-24) replace the older framed pet-only snapshots once. Device pets
// (source 'ble') are matched by device id + pet id; every departed record of that pet (one per connection session
// until the memorial sync merges them; a known name must match too) gets the photo. It is Web-only extra data the ESP32 never sees.
function savePhoto(p,url){
 if(!validPhoto(url))return false;
 const mine=r=>r.source==='ble'&&r.deviceId===p.deviceId&&r.petId===p.petId&&r.departed&&(!p.name||r.name===p.name),ble=!!p?.deviceId&&records.some(mine);
 const match=r=>ble?mine(r):r.source==='mock'&&r.petId===p.petId&&(r.dead||r.departed);
 const todo=records.map((r,i)=>match(r)?i:-1).filter(i=>i>=0);if(!todo.length)return false;
 const missing=todo.filter(i=>!(validPhoto(records[i].photo)&&records[i].photoKind==='scene'));if(!missing.length)return true;
 const next=records.slice();for(const i of missing)next[i]={...next[i],photo:url,photoKind:'scene'};
 if(!PetLocalStore.write('memorials',next))return false;records.splice(0,records.length,...next);return true;
}
function remember(p,source,session){
 const deviceId=source==='mock'?'展示花園':p.deviceId;if(!deviceId)return false;
 // A pet already in the synced device album keeps that one record; later connections add no per-session copies.
 if(source==='ble'&&records.some(r=>r.synced&&same(r,{...p,speciesId:p.speciesId??1})))return true;
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
function postDate(iso){if(iso===null)return '';const d=iso?new Date(iso):new Date(),day=isNaN(d)?new Date():d;return day.getFullYear()+'.'+String(day.getMonth()+1).padStart(2,'0')+'.'+String(day.getDate()).padStart(2,'0');}
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
const SPECIES={1:'小鳥',2:'小飛龍'},STAGE_LINE={egg:'還是一顆蛋',baby:'長到幼年',adult:'長到成年',unknown:'電子雞沒有記下成長階段'};
// Art needs a stage; an unknown one (old device tombstone) is drawn as an adult, the text says it is unknown.
const artStage=r=>r.stage==='unknown'?'adult':r.stage;
const Sync=()=>window.PetMemorialSync||null;
const speciesOf=r=>r.previewSpeciesId??r.speciesId??1,ended=r=>r.dead||r.departed;
const make=(tag,className,text)=>{const el=document.createElement(tag);if(className)el.className=className;if(text!==undefined)el.textContent=text;return el;};
let garden='mock:展示花園',pendingDelete=null,toastTimer=null,toastKeys=null,selecting=false;const selected=new Set();
function dateRange(r){
 if(!r.receivedAt)return '從電子雞同步，告別日期不明';
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
  if(r.departed&&window.PetFarewellView){const photo=saved?.complete&&saved.naturalWidth?saved:PetFarewellView.scene(species,artStage(r),gardenDay);
   PetFarewellView.postcard(ctx,{photo,name:r.name,age:duration(r.ageSeconds),date:postDate(r.receivedAt??null),petId:r.petId,species,stage:artStage(r)});}
  else if(r.dead&&window.PetDeathView)PetDeathView.restCard(ctx,{species,stage:artStage(r),name:r.name,petId:r.petId,label:r.name+' · '+(SPECIES[species]||'未知種類')+' · 長眠',background:gardenDay});};
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
// Why a device record cannot be deleted right now ('' when it can): the device's protected one, one still being sent,
// or, in a synced device garden, the current ended pet the ESP32 has not saved into its album yet.
function lock(r){
 const S=Sync();if(r.source!=='ble'||!S)return '';
 if(r.synced&&r.protected)return '這是電子雞上目前告別、還沒領養新蛋的夥伴，領養新蛋後才能刪除。';
 if(S.state(r)==='sending')return '正在從電子雞刪除……';
 if(!r.synced&&S.known(r.deviceId)&&S.current(r))return '電子雞還沒把這筆存進紀念冊，存好並同步後才能刪除。';
 return '';
}
const pending=r=>Sync()?.state(r)==='pending';
function card(r,order){
 const why=lock(r),wait=pending(r),c=make('article','album-card '+(r.departed?'departed':'dead')+(wait?' pending':''));c.setAttribute('style','--tilt:'+tilt(r)+'deg');
 // While selecting, the whole card (picture, text and tick) toggles it instead of opening the enlarged view.
 const open=make('button','album-open');open.type='button';open.append(artFor(r));
 if(selecting){const on=selected.has(r.key),toggle=()=>{if(why||wait){note(why||'這筆已在等待同步刪除。');return;}on?selected.delete(r.key):selected.add(r.key);renderCards();};
  c.className+=' selecting'+(on?' selected':'');c.setAttribute('role','checkbox');c.setAttribute('aria-checked',String(on));c.setAttribute('aria-label',r.name);c.setAttribute('tabindex','0');
  c.onclick=toggle;c.onkeydown=e=>{if(e.key===' '||e.key==='Enter'){e.preventDefault();toggle();}};open.tabIndex=-1;}
 else{open.setAttribute('aria-label','放大看 '+r.name+' 的'+(r.departed?'明信片':'安息卡'));open.onclick=()=>view(r,order);}
 const meta=metaBlock(r,order);c.append(open,meta);
 // 待同步: deleted while not connected (D4); it stays visible, outside the counts, and can still be cancelled.
 if(wait){meta.append(make('small','album-pending',Sync().connected(r.deviceId)?'即將從電子雞刪除，送出前可以取消。':'待同步：連上電子雞後會自動刪除。'));const undo=make('button','album-delete','取消刪除');undo.type='button';undo.setAttribute('aria-label','取消刪除 '+r.name+' 的紀念');undo.onclick=()=>cancelSync([r.key]);if(!selecting)c.append(undo);return c;}
 if(why){meta.append(make('small','album-lock',why));return c;}
 const del=make('button','album-delete','刪除');del.type='button';del.setAttribute('aria-label','刪除 '+r.name+' 的紀念');del.onclick=()=>askDelete([r]);
 if(selecting)c.append(make('span','album-check','✓'));else c.append(del);return c;
}
function view(r,order){$('#album-viewer-body').replaceChildren(artFor(r,3),metaBlock(r,order));$('#album-viewer').showModal?.();}
// Synced device records are deleted on both sides (plan §11); the rest stay a Web-only deletion with undo.
const twoWay=r=>r.source==='ble'&&r.synced===true&&!!Sync();
function askDelete(list){
 list=list.filter(r=>!lock(r)&&!pending(r));if(!list.length)return;pendingDelete=list;
 $('#album-delete-text').textContent=list.length===1?'要刪除 '+list[0].name+' 的紀念嗎？':'要刪除選取的 '+list.length+' 筆紀念嗎？';
 const S=Sync(),device=list.some(twoWay);
 $('#album-delete-note').textContent=!device?'刪除後會從紀念冊移除。':'電子雞上的這筆紀念也會一起刪除，送出後無法復原。'+(S.connected(list[0].deviceId)?'':'現在沒有連上這台電子雞，會先標成「待同步」，連上後自動刪除。');
 $('#album-delete').showModal?.();
}
// One toast line at a time; undo is offered only while it can still change something.
function show(text,undo,ms=6000){
 const t=$('#album-toast');clearTimeout(toastTimer);if(toastKeys){const k=toastKeys;toastKeys=null;purge(k);}
 $('#album-toast-text').textContent=text;t.hidden=false;const u=$('#album-undo');u.hidden=!undo;u.onclick=undo?()=>{clearTimeout(toastTimer);t.hidden=true;undo();}:null;
 toastTimer=setTimeout(()=>{t.hidden=true;if(toastKeys){const k=toastKeys;toastKeys=null;purge(k);}},ms);
}
function toast(list){
 const keys=list.map(r=>r.key);
 // A newer deletion ends the previous undo window straight away.
 show(list.length===1?'已刪除 '+list[0].name+' 的紀念':'已刪除 '+list.length+' 筆紀念',()=>{toastKeys=null;setHidden(keys,false);});toastKeys=keys;
}
const note=text=>show(text,null,8000);
// Undo before the deletion is sent cancels it; once the ESP32 has it there is no undo.
function cancelSync(keys){
 const r=Sync().cancel(keys);renderCards();
 note(r.sent.length?'已經送到電子雞，無法復原。':r.cancelled.length===1?'已取消刪除 '+r.cancelled[0].name+' 的紀念。':'已取消刪除 '+r.cancelled.length+' 筆紀念。');
}
function syncDelete(list){
 const r=Sync().request(list);renderCards();
 const what=list.length===1?list[0].name+' 的紀念':list.length+' 筆紀念';
 show(r.connected?'即將從電子雞刪除 '+what+'，送出前可以復原。':'已標成待同步：'+what+'，連上電子雞後會自動刪除。',()=>cancelSync(list.map(x=>x.key)));
}
function actions(shown){
 const bar=$('#album-actions');bar.replaceChildren();
 const button=(text,onclick,className='album-action')=>{const b=make('button',className,text);b.type='button';b.onclick=onclick;bar.append(b);return b;};
 if(!selecting){if(shown.length)button('選取',()=>{selecting=true;selected.clear();renderCards();});return;}
 const pickable=shown.filter(r=>!lock(r)&&!pending(r)),all=pickable.length>0&&pickable.every(r=>selected.has(r.key));
 button(all?'取消全選':'全選',()=>{for(const r of pickable)all?selected.delete(r.key):selected.add(r.key);renderCards();});
 const picked=pickable.filter(r=>selected.has(r.key)),del=button('刪除（'+picked.length+'）',()=>askDelete(picked),'album-action danger');del.disabled=!picked.length;
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
 const noteEl=$('#album-device-note'),deviceId=garden.startsWith('ble:')?garden.slice(4):null;noteEl.hidden=!deviceId;
 noteEl.textContent=deviceId&&Sync()?.known(deviceId)?Sync().note(deviceId):LOCAL_NOTE;
 // Synced records follow the ESP32's save order; records it does not hold yet (this browser's own) come after.
 const rank=r=>Number.isInteger(r.deviceOrder)&&r.synced?r.deviceOrder:Infinity;
 const ordered=records.filter(r=>ended(r)&&r.source+':'+r.deviceId===garden).sort((a,b)=>rank(a)-rank(b)||String(a.receivedAt??'').localeCompare(String(b.receivedAt??'')));
 // 待同步 deletions are still shown (marked) but left out of every count.
 const order=new Map(ordered.map((r,i)=>[r.key,i+1])),listed=ordered.filter(r=>!r.hiddenFromAlbum),visible=listed.filter(r=>!pending(r));
 const tabs=$('#album-tabs');tabs.replaceChildren();
 for(const [value,label,count] of [['all','全部',visible.length],['departed','遠行',visible.filter(r=>r.departed).length],['dead','長眠',visible.filter(r=>r.dead).length]]){
  const b=make('button','album-tab'+(value===category?' active':''),label+(value==='all'?'':' '+count));b.type='button';b.setAttribute('aria-pressed',String(value===category));
  b.onclick=()=>{$('#memorial-category').value=value;selected.clear();renderCards();};tabs.append(b);}
 const shown=listed.filter(r=>category==='all'||(category==='departed')===!!r.departed).reverse();
 if(!shown.length)list.append(make('p','album-empty',category==='departed'?'這裡還沒有遠行的明信片。':category==='dead'?'這裡還沒有長眠的夥伴。':'花園裡還沒有回憶，好好陪伴現在的夥伴吧。'));
 for(const r of shown)list.append(card(r,order.get(r.key)));
 actions(shown);
}
const LOCAL_NOTE='這裡是這個瀏覽器陪伴過的紀錄，完整紀念冊請在 ESP32 上瀏覽。';
// For memorial-sync.js only: it replaces device records after reconciling; everything is checked like a stored album.
function commit(next){if(!valid(next)||!PetLocalStore.write('memorials',next))return false;records.splice(0,records.length,...JSON.parse(JSON.stringify(next)));renderOpen();return true;}
const renderOpen=()=>{if($('#cemetery-dialog').open)render();};
window.PetMemorials={uuid,remember,duration,savePhoto,farewellDate,same,validPhoto,records:()=>JSON.parse(JSON.stringify(records)),
 sync:{commit,refresh:renderOpen,notify:text=>{if($('#cemetery-dialog').open)note(text);}},hasDemoName:name=>records.some(r=>r.source==='mock'&&(r.dead||r.departed)&&r.name===name),open(preferred){render(preferred);$('#cemetery-dialog').showModal();}};
$('#cemetery-device').onchange=()=>{garden=$('#cemetery-device').value;renderCards();};
$('#album-delete-confirm').onclick=()=>{const list=pendingDelete;pendingDelete=null;$('#album-delete').close?.();if(!list)return;
 const device=list.filter(twoWay),local=list.filter(r=>!twoWay(r));selecting=false;selected.clear();
 if(local.length&&setHidden(local.map(r=>r.key))){renderCards();toast(local);}if(device.length)syncDelete(device);};
$('#cemetery-dialog').addEventListener('close',()=>{selecting=false;selected.clear();});
// Deletions whose undo window was cut short by closing the page are cleaned up on the next load.
purge();
})();
