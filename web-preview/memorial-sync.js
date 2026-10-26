/* 養成同步 phase 4 (PROTOCOL.md 紀念冊, docs/features/device-sync-ble-plan.md §8.3, D3/D4): two-way memorial sync
 * between the ESP32 album and this browser's device records. Runs after connecting and whenever memorial_hash differs
 * from the last synced one: queued deletions, the full list, then add / update (keeping the Web-only photo, dates and
 * level) / remove what the ESP32 no longer has. A record is device id + pet id, also checking name and species. Demo
 * records never take part. The ESP32 does not ask again (D3), so the album confirms before a deletion is queued. */
(()=>{'use strict';
const GRACE_MS=6000,RETRY_MS=10000,DELETE_ACTIVITY=['idle','menu','sleeping','ended'];
const validItem=q=>q&&typeof q.deviceId==='string'&&typeof q.petId==='string'&&/^\d+$/.test(q.petId)&&typeof q.name==='string'&&Number.isInteger(q.speciesId)&&typeof q.key==='string';
function validState(s){return !!s&&!!s.devices&&typeof s.devices==='object'&&!Array.isArray(s.devices)&&Object.values(s.devices).every(d=>d&&(d.hash===null||typeof d.hash==='string')&&typeof d.removalOk==='boolean')&&Array.isArray(s.queue)&&s.queue.every(validItem);}
// devices[device_id] = {hash: last synced memorial_hash, removalOk: the one-time removal summary was answered, syncedAt}.
// queue = deletions waiting for the ESP32 (D4), each tied to its own device_id.
const state=PetLocalStore.read('memorial-sync',validState)||{devices:{},queue:[]};
const save=()=>PetLocalStore.write('memorial-sync',state);
const M=()=>window.PetMemorials,D=()=>window.petDataManager,$=s=>document.querySelector(s);
const ended=r=>r.dead||r.departed,mine=(r,deviceId)=>r.source==='ble'&&r.deviceId===deviceId;
const notBefore=new Map(),notes=new Map(),queueNotes=new Map();let sending=null,retryTimer=null,running=false,again=false,failAt=-Infinity,wasReady=false,lastDevice=null,asking=null;
const view=()=>D().view('ble');
function live(){const v=view();return v.connection==='ready'&&v.memorialSync&&v.pet?.deviceId&&v.memorials?v:null;}
const connected=deviceId=>live()?.pet.deviceId===deviceId;
const known=deviceId=>!!state.devices[deviceId];
// The device pet the latest snapshot shows as ended (its ESP32 record may still be saving).
function current(r){const p=view().pet;return !!p&&r?.source==='ble'&&(p.isDeparted||p.healthState==='dead')&&M().same(r,{deviceId:p.deviceId,petId:p.petId,name:p.name,speciesId:p.speciesId??1});}
const queued=r=>state.queue.find(q=>M().same(r,q));
function stateOf(r){if(r?.source!=='ble')return null;const q=queued(r);return !q?null:q===sending?'sending':'pending';}
const due=deviceId=>state.queue.filter(q=>q.deviceId===deviceId&&!(notBefore.get(q.key)>Date.now()));
// Two album lines per device: the last list sync, and the last problem with queued deletions (kept apart so one
// does not hide the other).
function setNote(deviceId,text,queue=false){(queue?queueNotes:notes).set(deviceId,text);M().sync.refresh();}
const ERRORS={busy:'電子雞正在忙，待同步的刪除先保留，等牠回到主畫面會再送出。',timeout:'沒有收到電子雞的刪除結果，先保留在待同步，稍後再試。',
 save_failed:'電子雞沒能保存這次刪除，先保留在待同步，稍後再試。',op_conflict:'電子雞記得另一個相同編號的操作，這次沒有刪除，稍後再試。'};
const errorText=e=>ERRORS[e?.code]||'這次沒能從電子雞刪除紀念，先保留在待同步，稍後再試。';

// Pure: the album after syncing one device's list. Device has / Web lacks: a new record with device fields only.
// Both: device fields from the ESP32, Web extras kept, per-connection duplicates merged into one. Web has / device
// lacks: removed (deleted on the ESP32), except the current ended pet whose device record may still be saving, alive
// pets kept as first-seen background, and only when allowRemove. Hidden local deletion stubs go silently.
function reconcile(records,deviceId,items,current,allowRemove){
 const used=new Set(),added=[],updated=[],merged=[],validPhoto=M().validPhoto;
 const cur=current&&current.deviceId===deviceId&&current.petId&&(current.isDeparted||current.healthState==='dead'||current.health==='dead')?{deviceId,petId:current.petId,name:current.name,speciesId:current.speciesId??1}:null;
 const synced=items.map((q,i)=>{
  const it={...q,deviceId},group=records.filter(r=>mine(r,deviceId)&&M().same(r,it));for(const r of group)used.add(r);
  (group.length?updated:added).push(q.name);if(group.length>1)merged.push(q.name);
  const byDate=(a,b)=>String(a.receivedAt??'').localeCompare(String(b.receivedAt??'')),ends=group.filter(ended).sort(byDate),prev=group.find(r=>r.synced);
  const photo=group.find(r=>validPhoto(r.photo)&&r.photoKind==='scene')||group.find(r=>validPhoto(r.photo));
  const first=group.map(r=>r.firstSeenAt||r.receivedAt).filter(Boolean).sort()[0],receivedAt=prev?.receivedAt??ends[0]?.receivedAt;
  const level=[...ends].reverse().find(r=>Number.isInteger(r.level))?.level??group.find(r=>Number.isInteger(r.level))?.level;
  const rec={key:'ble:'+deviceId+':memorial:'+q.petId+':'+q.speciesId+':'+q.name,source:'ble',deviceId,name:q.name,petId:q.petId,speciesId:q.speciesId,ageSeconds:q.ageSeconds,stage:q.stage,
   dead:q.kind==='resting',departed:q.kind==='departed',kind:q.kind==='resting'?'dead':'departed',protected:q.protected,synced:true,deviceOrder:i,provisional:false};
  if(receivedAt)rec.receivedAt=receivedAt;if(first)rec.firstSeenAt=first;if(Number.isInteger(level))rec.level=level;if(photo){rec.photo=photo.photo;rec.photoKind=photo.photoKind;}
  return rec;});
 const keep=[],removed=[],candidates=[];
 for(const r of records){if(!mine(r,deviceId)){keep.push(r);continue;}if(used.has(r))continue;
  if(!ended(r)||cur&&M().same(r,cur)){keep.push(r);continue;}
  if(r.hiddenFromAlbum){removed.push(r);continue;}
  if(allowRemove)removed.push(r);else{candidates.push(r);keep.push(r);}}
 return {next:[...keep,...synced],added,updated,merged,removed:removed.filter(r=>!r.hiddenFromAlbum),candidates};
}
const names=list=>[...new Set(list.map(r=>r.name))].join('、');
function apply(deviceId,list){
 const st=state.devices[deviceId]||{hash:null,removalOk:false},res=reconcile(M().records(),deviceId,list.items,view().pet,st.removalOk);
 if(!M().sync.commit(res.next))throw new Error('紀念冊無法保存到這個瀏覽器');
 state.devices[deviceId]={hash:list.hash,removalOk:st.removalOk||!res.candidates.length,syncedAt:new Date().toISOString()};save();
 const parts=[];if(res.added.length)parts.push('從電子雞加入 '+res.added.length+' 筆');if(res.removed.length)parts.push('移除電子雞上已刪除的 '+res.removed.length+' 筆（'+names(res.removed)+'）');
 setNote(deviceId,parts.length?'剛剛同步：'+parts.join('，')+'。':'已和電子雞同步。');
 if(res.candidates.length)ask(deviceId,res.candidates,list);
}
// One-time summary before the first sync of a device removes anything (plan §8.3). It asks rather than only informs:
// those records (often from before two-way sync, some with Web-only photos) cannot come back, while keeping them for
// now changes nothing on the ESP32. 先保留 leaves them and asks again on the next connection.
function ask(deviceId,candidates,list){
 const d=$('#memorial-sync-dialog');asking={deviceId,list};
 $('#memorial-sync-title').textContent='同步後會移除 '+candidates.length+' 筆';
 $('#memorial-sync-text').textContent='電子雞的紀念冊裡已經沒有這些紀錄（可能在電子雞上刪除，或電子雞重新開始過）：'+names(candidates)+'。同步後網頁也會移除，照片等網頁上的資料無法復原。';
 if(!d.open)d.showModal?.();
}
$('#memorial-sync-confirm').onclick=()=>{const a=asking;asking=null;$('#memorial-sync-dialog').close?.();if(!a)return;
 state.devices[a.deviceId]={...state.devices[a.deviceId],removalOk:true};save();try{apply(a.deviceId,a.list);}catch(e){setNote(a.deviceId,'紀念冊同步沒有完成：'+e.message+'。');}};
$('#memorial-sync-keep').onclick=()=>{const a=asking;asking=null;$('#memorial-sync-dialog').close?.();if(a)setNote(a.deviceId,'先保留了電子雞上已經沒有的紀念，下次連線會再詢問。');};
$('#memorial-sync-dialog').addEventListener?.('close',()=>{if(asking)$('#memorial-sync-keep').onclick();});

// A queued deletion is settled: the queue item and every Web record of that device pet go.
function settle(q){state.queue=state.queue.filter(x=>x!==q);save();M().sync.commit(M().records().filter(r=>!M().same(r,q)));}
async function sendQueue(deviceId){
 let n=0;if(due(deviceId).length)queueNotes.delete(deviceId);
 for(const q of due(deviceId)){
  const v=live();if(v?.pet.deviceId!==deviceId)break;
  if(!DELETE_ACTIVITY.includes(v.activity)){setNote(deviceId,'待同步的刪除會在電子雞回到主畫面後送出。',true);break;}
  if(v.careSending)break;
  sending=q;M().sync.refresh();
  try{await D().deleteDeviceMemorial(q.petId);settle(q);n++;M().sync.notify('已從電子雞刪除 '+q.name+' 的紀念，無法復原。');}
  catch(e){
   // not_found: already gone on the ESP32, counts as deleted. not_allowed: the protected one, shown again.
   if(e?.code==='not_found'){settle(q);n++;M().sync.notify('已刪除 '+q.name+' 的紀念（電子雞上已經沒有這筆），無法復原。');}
   else if(e?.code==='not_allowed'){state.queue=state.queue.filter(x=>x!==q);save();n++;const t=q.name+' 是電子雞上目前告別、還沒領養新蛋的夥伴，不能刪除，已恢復顯示。';setNote(deviceId,t,true);M().sync.notify(t);}
   else{failAt=Date.now();const t=e?.code?errorText(e):'待同步的刪除先保留，稍後再試。';setNote(deviceId,t,true);M().sync.notify(t);break;}}
  finally{sending=null;M().sync.refresh();}
 }
 return n;
}
// Queued items whose pet id the fresh list no longer names with the same name and species are already deleted on
// the ESP32 (or the id now belongs to another pet after an NVS rebuild): they are settled, never sent.
function settleGone(deviceId,items){for(const q of state.queue.filter(q=>q.deviceId===deviceId))if(!items.some(i=>M().same({...i,source:'ble',deviceId},q)))settle(q);}
async function run(force){
 if(running){again=true;return;}const v=live();if(!v)return;const deviceId=v.pet.deviceId,st=state.devices[deviceId];running=true;
 try{
  // After a failure nothing is retried for RETRY_MS (a new connection or an ESP32 event clears the pause).
  if(Date.now()-failAt<RETRY_MS&&!force)return;
  const stale=force||!st||st.hash!==v.memorials.hash;let list=null;
  // When the ESP32 album changed since the last sync, its list is read before queued deletions are sent, so a pet
  // id reused after an NVS rebuild is never deleted by mistake (a safe extension of the §8.3 order).
  if(stale&&due(deviceId).length){list=await D().deviceMemorials();settleGone(deviceId,list.items);}
  const sent=await sendQueue(deviceId);
  if(sent||stale&&!list)list=await D().deviceMemorials();
  if(list&&connected(deviceId))apply(deviceId,list);
 }catch(e){failAt=Date.now();setNote(deviceId,'紀念冊同步沒有完成：'+(e?.message||'電子雞沒有回應')+'，稍後會再試。');}
 finally{running=false;M().sync.refresh();
  if(again){again=false;Promise.resolve().then(()=>run(false));}
  else if(due(deviceId).length&&connected(deviceId)&&failAt>Date.now()-RETRY_MS){clearTimeout(retryTimer);retryTimer=setTimeout(()=>run(false),RETRY_MS+10);}}
}
// After a status: a new connection always runs (asking again while the removal summary is unanswered); otherwise a
// memorial_hash different from the last synced one, or due deletions once the ESP32 can take them, with a retry pause.
function onStatus(){
 const v=live();if(!v){wasReady=false;return;}
 const id=v.pet.deviceId,fresh=!wasReady||lastDevice!==id;wasReady=true;lastDevice=id;const st=state.devices[id];
 if(fresh){failAt=-Infinity;run(!!st&&!st.removalOk);return;}
 if(Date.now()-failAt<RETRY_MS)return;
 if(!st||st.hash!==v.memorials.hash||due(id).length&&DELETE_ACTIVITY.includes(v.activity)&&!v.careSending)run(false);
}
D().subscribe(onStatus);
// The ESP32's own memorial events re-read the list even before the snapshot with the new hash arrives. This page's
// deletions come back with source web and are dropped by the BLE layer, so nothing runs twice.
D().onPetEvent(e=>{if(e.event==='memorial_added'||e.event==='memorial_deleted')run(true);});
// Album side (memorials.js): queue after the confirm dialog; undo works until the deletion is sent.
function request(list){
 const at=Date.now()+GRACE_MS;
 for(const r of list){if(r.source!=='ble'||!r.synced||r.protected||queued(r))continue;
  state.queue.push({deviceId:r.deviceId,petId:r.petId,name:r.name,speciesId:r.speciesId??1,key:r.key});notBefore.set(r.key,at);}
 save();const deviceId=list[0]?.deviceId;setTimeout(()=>{if(connected(deviceId))run(false);},GRACE_MS+10);
 return {connected:connected(deviceId)};
}
function cancel(keys){
 const cancelled=[],sent=[];
 for(const key of keys){const q=state.queue.find(x=>x.key===key);if(!q)continue;if(q===sending){sent.push(q);continue;}state.queue=state.queue.filter(x=>x!==q);notBefore.delete(key);cancelled.push({name:q.name});}
 save();return {cancelled,sent};
}
function note(deviceId){
 const n=state.queue.filter(q=>q.deviceId===deviceId).length;
 return [connected(deviceId)?'紀念冊和這台電子雞雙向同步：在這裡或電子雞上刪除，兩邊都會刪掉。':'紀念冊和這台電子雞雙向同步；現在沒有連線，顯示上次同步的內容。',
  '照片、日期和等級只存在這個瀏覽器。',n?n+' 筆刪除待同步，連上電子雞後會自動送出。':'',queueNotes.get(deviceId)||'',notes.get(deviceId)||''].filter(Boolean).join('');
}
window.PetMemorialSync={reconcile,run,request,cancel,state:stateOf,known,connected,current,note,pending:deviceId=>state.queue.filter(q=>q.deviceId===deviceId).map(q=>({...q}))};
})();
