/* BLE v1 transport -> domain snapshot. Sends the two read-only queries, plus the Web play game commands when the
 * firmware declares the web_game feature (see PROTOCOL.md, Web 陪玩). Accepts 養成同步 phase 1 (status additions,
 * device_info.rules, pet_event); care, lifecycle and memorial commands are still not sent. */
(()=>{'use strict';
const UUID={service:'7d2a0001-8b7c-4f3a-9c2d-1e5f6a7b8c90',command:'7d2a0002-8b7c-4f3a-9c2d-1e5f6a7b8c90',event:'7d2a0003-8b7c-4f3a-9c2d-1e5f6a7b8c90'};
const decimal=x=>typeof x==='string'&&/^\d+$/.test(x);
const GAME_COMMANDS=['game_start','game_heartbeat','game_finish','game_cancel'];
const SAD_REASONS=['mood','satiety','cleanliness','sick'],DEATH_CAUSES=['untreated_sickness'],uint32=x=>Number.isInteger(x)&&x>=0&&x<=4294967295;
// pet_event names (PROTOCOL.md 養成同步); unknown ones are ignored. Events only pick an animation, never change state.
const PET_EVENTS=['fed','cleaned','treated','slept','woke','hatched','grew','level_up','got_sick','died','departed','adopted','memorial_added','memorial_deleted'];
// device_info.rules: optional positive integers the page shows countdowns and limits with, never hard-coded.
const RULES=['sick_after_seconds','death_after_sick_seconds','memorial_limit'];
function mapRules(r){
  if(r===undefined)return null;
  if(!r||typeof r!=='object'||Array.isArray(r)||RULES.some(k=>r[k]!==undefined&&(!uint32(r[k])||r[k]<1)))throw new Error('裝置規則欄位不相容');
  return Object.fromEntries(RULES.filter(k=>r[k]!==undefined).map(k=>[k,r[k]]));
}
function mapStatus(msg){
  const p=msg?.pet;
  if(msg?.v!==1||msg.type!=='status'||!p||!decimal(p.id)||!decimal(p.age_seconds)||typeof p.name!=='string'||!p.name.trim()||!['egg','baby','adult'].includes(p.life_stage)||!['healthy','sick','dead'].includes(p.health)||typeof p.is_dead!=='boolean'||p.is_dead!==(p.health==='dead')||!['awake','normal'].includes(p.sleep)||!['satiety','mood','cleanliness'].every(k=>Number.isInteger(p[k])&&p[k]>=0&&p[k]<=100)||msg.device_id!=null&&!/^esp32-[A-F0-9]{12}$/.test(msg.device_id))throw new Error('裝置狀態格式或版本不相容，未套用資料。');
  if(p.species_id!==undefined&&(!Number.isInteger(p.species_id)||p.species_id<1||p.species_id>255))throw new Error('種類欄位格式不相容');
  if(p.is_departed!==undefined&&typeof p.is_departed!=='boolean'||p.is_departed&&(p.is_dead||p.life_stage==='egg'||p.sleep!=='awake'))throw new Error('遠行狀態不相容');
  const expFields=['level','exp','exp_to_next_level'],has=expFields.filter(k=>p[k]!==undefined).length;
  if(has&&(has!==3||!Number.isInteger(p.level)||p.level<1||p.level>255||!['exp','exp_to_next_level'].every(k=>Number.isInteger(p[k])&&p[k]>=0&&p[k]<=65535)||p.exp_to_next_level===0&&p.exp!==0||p.exp_to_next_level>0&&p.exp>=p.exp_to_next_level))throw new Error('等級／經驗欄位不相容');
  // 養成同步 phase 1 (PROTOCOL.md): each field means support only when present; present but malformed rejects the snapshot.
  const terminal=p.is_dead||p.is_departed===true||p.life_stage==='egg';
  if(p.sad_reasons!==undefined&&(!Array.isArray(p.sad_reasons)||p.sad_reasons.some(r=>!SAD_REASONS.includes(r))||new Set(p.sad_reasons).size!==p.sad_reasons.length||terminal&&p.sad_reasons.length))throw new Error('傷心原因欄位不相容');
  if(['danger_seconds','sick_seconds'].some(k=>p[k]!==undefined&&!uint32(p[k]))||p.health==='healthy'&&p.sick_seconds>0)throw new Error('生病計時欄位不相容');
  if(p.death_cause!==undefined&&(typeof p.death_cause!=='string'||!p.is_dead))throw new Error('死亡原因欄位不相容');
  if((msg.memorial_count===undefined)!==(msg.memorial_hash===undefined)||msg.memorial_count!==undefined&&(!Number.isInteger(msg.memorial_count)||msg.memorial_count<0||msg.memorial_count>255||typeof msg.memorial_hash!=='string'||!/^[0-9a-f]{8}$/.test(msg.memorial_hash)))throw new Error('紀念冊摘要欄位不相容');
  // The firmware's buffered sad judgement replaces the local one; device:true tells PetRules not to recompute it.
  const sad=p.sad_reasons&&{device:true,...Object.fromEntries(SAD_REASONS.map(k=>[k,p.sad_reasons.includes(k)]))};
  return {...sad&&{sadReasons:sad},dangerSeconds:p.danger_seconds??null,sickSeconds:p.sick_seconds??null,deathCause:p.death_cause===undefined?null:DEATH_CAUSES.includes(p.death_cause)?p.death_cause:'unknown',speciesId:p.species_id??1,isDeparted:p.is_departed??false,level:p.level??null,exp:p.exp??null,expToNextLevel:p.exp_to_next_level??null,name:p.name,petId:p.id,ageSeconds:p.age_seconds,stage:p.life_stage,healthState:p.health,sleepMode:p.sleep,satiety:p.satiety,mood:p.mood,cleanliness:p.cleanliness,deviceId:msg.device_id??null};
}
function validCache(c){try{return c&&typeof c.receivedAt==='string'&&Number.isFinite(Date.parse(c.receivedAt))&&!!mapStatus(c.wire);}catch{return false;}}
class BleSource {
  constructor(){this.cache=PetLocalStore.read('device-snapshot',validCache);if(this.cache)this.cache.pet=mapStatus(this.cache.wire);this.connection='disconnected';this.message='尚未連接裝置';this.listeners=new Set();this.session=0;this.nextId=1;this.partial=null;this.pending=new Map();this.gameEnd=null;this.rules=null;this.eventListeners=new Set();}
  // activity is only meaningful from a live connection; an old cached snapshot never says the device is idle.
  read(){return {source:'ble',connection:this.connection,pet:this.cache?.pet??null,receivedAt:this.cache?.receivedAt??null,message:this.message,stale:this.connection!=='ready',
   activity:this.connection==='ready'&&typeof this.cache?.wire?.activity==='string'?this.cache.wire.activity:null,webGame:this.connection==='ready'&&!!this.info?.features?.includes?.('web_game'),gameEnd:this.gameEnd,
   games:this.connection==='ready'&&Array.isArray(this.info?.games)?this.info.games.filter(g=>typeof g==='string'):null,
   petEvents:this.connection==='ready'&&!!this.info?.features?.includes?.('pet_events'),rules:this.connection==='ready'?this.rules:null,
   // Device album summary, kept for the later memorial sync; nothing is fetched yet. A cached one is the last known.
   memorials:this.cache?.wire?.memorial_count!==undefined?{count:this.cache.wire.memorial_count,hash:this.cache.wire.memorial_hash}:null};}
  subscribe(fn){this.listeners.add(fn);return ()=>this.listeners.delete(fn);}
  emit(){for(const fn of this.listeners)fn(this.read());}
  // pet_event listeners get a copy of the event only; the snapshot stays the single source of pet state.
  onPetEvent(fn){this.eventListeners.add(fn);return ()=>this.eventListeners.delete(fn);}
  petEvent(msg){
    const r=this.read();if(!r.petEvents||!PET_EVENTS.includes(msg.event)||msg.source==='web'||!decimal(msg.pet_id))return;
    // adopted names the new pet; every other event must be about the pet the page is showing.
    if(msg.event!=='adopted'&&msg.pet_id!==r.pet?.petId)return;
    const e={event:msg.event,petId:msg.pet_id,source:typeof msg.source==='string'?msg.source:null};
    if(msg.event==='level_up'&&Number.isInteger(msg.level))e.level=msg.level;
    if(decimal(msg.memorial_pet_id))e.memorialPetId=msg.memorial_pet_id;
    for(const fn of this.eventListeners)fn({...e});
  }
  command(){throw new Error('此版本僅提供 BLE 唯讀，照顧命令未開放。');}
  stop(){clearTimeout(this.fragmentTimer);this.partial=null;for(const p of this.pending.values()){clearTimeout(p.timer);p.reject(new Error('連線已中止'));}this.pending.clear();}
  disconnect(){++this.session;this.stop();this.writer=null;this.info=null;this.rules=null;this.writeChain=null;if(this.event){this.event.removeEventListener('characteristicvaluechanged',this.onValue);this.event=null;}if(this.device){this.device.removeEventListener('gattserverdisconnected',this.onDisconnect);this.device.gatt?.disconnect();}this.connection='disconnected';this.message='已斷線，保留最後收到的資料';this.emit();}
  async connect(){
    if(['selecting','connecting','waiting'].includes(this.connection))return;
    this.disconnect();const session=++this.session;
    if(location.protocol==='file:'||!isSecureContext||!navigator.bluetooth?.requestDevice){this.connection='error';this.message='請使用支援 Web Bluetooth 的瀏覽器，從 HTTPS 或 localhost 開啟。';this.emit();return;}
    this.connection='selecting';this.message='請在瀏覽器視窗選擇 ESP32-PET';this.emit();
    let selected;
    try{
      selected=await navigator.bluetooth.requestDevice({filters:[{services:[UUID.service]},{namePrefix:'ESP32-PET'}],optionalServices:[UUID.service]});
      if(session!==this.session)return;
      this.device=selected;this.cache=null;this.info=null;this.rules=null;this.connection='connecting';this.message='正在連線';this.emit();
      this.onDisconnect=()=>{if(session!==this.session)return;this.disconnect();};selected.addEventListener('gattserverdisconnected',this.onDisconnect);
      const server=await selected.gatt.connect();if(session!==this.session){selected.gatt.disconnect();return;}
      const service=await server.getPrimaryService(UUID.service);const command=await service.getCharacteristic(UUID.command);const event=await service.getCharacteristic(UUID.event);
      if(session!==this.session)return;this.writer=command;this.event=event;this.onValue=e=>{if(session===this.session)this.receive(e.target.value);};event.addEventListener('characteristicvaluechanged',this.onValue);await event.startNotifications();
      if(session!==this.session)return;this.connection='waiting';this.message='已連線，等待裝置資訊與完整狀態';this.emit();
      await this.query('get_device_info');if(session!==this.session)return;await this.query('get_status');
    }catch(e){if(session!==this.session)return;this.disconnect();this.connection=e.name==='NotFoundError'?'cancelled':'error';this.message=e.name==='NotFoundError'?'已取消選擇，或沒有找到裝置；可以重新搜尋。':e.name==='SecurityError'||e.name==='NotAllowedError'?'藍牙權限被拒絕或受瀏覽器限制。':e.message||'連線失敗，請確認裝置和藍牙。';this.emit();}
  }
  async query(cmd){
    if(!['get_device_info','get_status'].includes(cmd))throw new Error('唯讀查詢限制');
    return this.request(cmd,{});
  }
  // Web play: resolves with game_session / game_result for start / finish, or the command_result otherwise.
  // A refusal rejects with an Error whose code is the protocol error code (busy, too_early, ...).
  async game(cmd,fields={}){
    if(!GAME_COMMANDS.includes(cmd))throw new Error('不支援的遊戲命令');
    if(this.connection!=='ready'||!this.info?.features?.includes?.('web_game'))throw new Error('這台電子雞目前不能網頁陪玩');
    // A new session must not inherit an end notice from an earlier one (session numbers restart with the device).
    if(cmd==='game_start')this.gameEnd=null;
    return this.request(cmd,fields);
  }
  async request(cmd,fields){
    const id=this.nextId++,session=this.session;
    const response=new Promise((resolve,reject)=>{this.pending.set(id,{cmd,resolve,reject,timer:setTimeout(()=>{this.pending.delete(id);reject(new Error('等待裝置回覆逾時，請重新連線。'));},8000)});});
    // Attach a handler before async writes to avoid an unhandled timeout rejection.
    response.catch(()=>{});
    try{const bytes=new TextEncoder().encode(JSON.stringify({...fields,v:1,id,cmd})),count=Math.ceil(bytes.length/16),messageId=id%65536;
      const write=async()=>{for(let i=0;i<count;i++){if(session!==this.session)throw new Error('連線已中止');const data=new Uint8Array(4+Math.min(16,bytes.length-i*16));data[0]=messageId&255;data[1]=messageId>>8;data[2]=i;data[3]=count;data.set(bytes.slice(i*16,i*16+16),4);await this.writer.writeValueWithResponse(data);}};
      // Web Bluetooth allows one GATT write at a time and the ESP32 reassembles one message at a time, so whole
      // messages are written in order (a heartbeat waits for a finish's fragments). Replies are awaited outside the chain.
      const written=(this.writeChain||Promise.resolve()).catch(()=>{}).then(write);this.writeChain=written;
      await written;return await response;
    }catch(e){const p=this.pending.get(id);if(p){clearTimeout(p.timer);this.pending.delete(id);p.reject(e);}throw e;}
  }
  receive(value){try{
    const a=new Uint8Array(value.buffer,value.byteOffset,value.byteLength);if(a.length<5)throw new Error('BLE 分片格式錯誤');const id=a[0]|a[1]<<8,index=a[2],count=a[3];if(!count||index>=count||a.length>20)throw new Error('BLE 分片索引或大小錯誤');
    if(!this.partial||this.partial.id!==id){if(index!==0)throw new Error('BLE 分片缺少起始片');clearTimeout(this.fragmentTimer);this.partial={id,count,chunks:[],size:0};this.fragmentTimer=setTimeout(()=>{this.partial=null;this.message='BLE 分片接收逾時，等待新的完整資料';this.emit();},3000);}
    const p=this.partial;if(count!==p.count)throw new Error('BLE 分片數不一致');const chunk=a.slice(4);
    if(index<p.chunks.length){if(chunk.length!==p.chunks[index].length||chunk.some((v,i)=>v!==p.chunks[index][i]))throw new Error('BLE 重複分片衝突');return;}
    if(index!==p.chunks.length||p.size+chunk.length>1024)throw new Error('BLE 分片缺漏或訊息超限');p.chunks.push(chunk);p.size+=chunk.length;if(p.chunks.length!==count)return;
    clearTimeout(this.fragmentTimer);this.partial=null;const bytes=new Uint8Array(p.size);let offset=0;for(const c of p.chunks){bytes.set(c,offset);offset+=c.length;}
    this.messageReceived(JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(bytes)));
  }catch(e){clearTimeout(this.fragmentTimer);this.partial=null;this.message=e.message;this.emit();}}
  messageReceived(msg){
    if(msg.v!==1){this.disconnect();throw new Error('不支援的 BLE 協定版本，已停止查詢');}
    if(msg.type==='command_result'){const p=this.pending.get(msg.id);if(!p)return;
     if(msg.ok!==true){clearTimeout(p.timer);this.pending.delete(msg.id);const e=new Error('裝置拒絕'+(GAME_COMMANDS.includes(p.cmd)?'遊戲命令':'查詢')+'：'+String(msg.code));e.code=String(msg.code);p.reject(e);}
     else if(p.cmd==='game_heartbeat'||p.cmd==='game_cancel'){clearTimeout(p.timer);this.pending.delete(msg.id);p.resolve(msg);}
     return;}
    // Game events answer the start/finish command with the same id; a device-side end is kept for the play view.
    if(msg.type==='game_session'||msg.type==='game_result'){const p=this.pending.get(msg.id);
     if(p&&p.cmd===(msg.type==='game_session'?'game_start':'game_finish')&&decimal(msg.session)){clearTimeout(p.timer);this.pending.delete(msg.id);p.resolve(msg);}return;}
    if(msg.type==='pet_event'){this.petEvent(msg);return;}
    if(msg.type==='game_session_end'){if(decimal(msg.session)){this.gameEnd={session:msg.session,reason:String(msg.reason)};this.emit();}return;}
    if(msg.type==='device_info'){if(msg.protocol_version!==1){this.disconnect();throw new Error('裝置協定版本不相容，已停止查詢');}if(msg.device_id!=null&&!/^esp32-[A-F0-9]{12}$/.test(msg.device_id)||typeof msg.firmware_version!=='string'||msg.max_message_bytes!==1024)throw new Error('裝置資訊不相容');const rules=mapRules(msg.rules);this.info=msg;this.rules=rules;}
    else if(msg.type==='status'){const pet=mapStatus(msg);if(this.info?.device_id&&pet.deviceId&&this.info.device_id!==pet.deviceId)throw new Error('裝置 ID 不一致');this.cache={wire:msg,pet,receivedAt:new Date().toISOString()};PetLocalStore.write('device-snapshot',this.cache);this.connection='ready';this.message='已收到 ESP32 完整狀態（唯讀）';}
    else return;
    for(const [id,p] of this.pending){if(p.cmd===(msg.type==='device_info'?'get_device_info':'get_status')){clearTimeout(p.timer);this.pending.delete(id);p.resolve(msg);}}
    this.emit();
  }
}
window.PetBle={mapStatus,mapRules,PET_EVENTS,BleSource};window.PetSources.ble=new BleSource();
})();
