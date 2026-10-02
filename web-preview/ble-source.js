/* BLE v1 transport -> domain snapshot. Only the two read-only queries are sent. */
(()=>{'use strict';
const UUID={service:'7d2a0001-8b7c-4f3a-9c2d-1e5f6a7b8c90',command:'7d2a0002-8b7c-4f3a-9c2d-1e5f6a7b8c90',event:'7d2a0003-8b7c-4f3a-9c2d-1e5f6a7b8c90'};
const decimal=x=>typeof x==='string'&&/^\d+$/.test(x);
function mapStatus(msg){
  const p=msg?.pet;
  if(msg?.v!==1||msg.type!=='status'||!p||!decimal(p.id)||!decimal(p.age_seconds)||typeof p.name!=='string'||!p.name.trim()||!['egg','baby','adult'].includes(p.life_stage)||!['healthy','sick','dead'].includes(p.health)||typeof p.is_dead!=='boolean'||p.is_dead!==(p.health==='dead')||!['awake','normal'].includes(p.sleep)||!['satiety','mood','cleanliness'].every(k=>Number.isInteger(p[k])&&p[k]>=0&&p[k]<=100)||msg.device_id!=null&&!/^esp32-pet-[A-F0-9]{12}$/.test(msg.device_id))throw new Error('裝置狀態格式或版本不相容，未套用資料。');
  return {name:p.name,petId:p.id,ageSeconds:p.age_seconds,stage:p.life_stage,health:p.health,condition:p.sleep==='normal'?'sleep':p.health==='sick'?'sick':'awake',satiety:p.satiety,mood:p.mood,cleanliness:p.cleanliness,deviceId:msg.device_id??null};
}
function validCache(c){try{return c&&typeof c.receivedAt==='string'&&Number.isFinite(Date.parse(c.receivedAt))&&JSON.stringify(mapStatus(c.wire))===JSON.stringify(c.pet);}catch{return false;}}
class BleSource {
  constructor(){this.cache=PetLocalStore.read('device-snapshot',validCache);this.connection='disconnected';this.message='尚未連接裝置';this.listeners=new Set();this.session=0;this.nextId=1;this.partial=null;this.pending=new Map();}
  read(){return {source:'ble',connection:this.connection,pet:this.cache?.pet??null,receivedAt:this.cache?.receivedAt??null,message:this.message,stale:this.connection!=='ready'};}
  subscribe(fn){this.listeners.add(fn);return ()=>this.listeners.delete(fn);}
  emit(){for(const fn of this.listeners)fn(this.read());}
  command(){throw new Error('此版本僅提供 BLE 唯讀，照顧命令未開放。');}
  stop(){clearTimeout(this.fragmentTimer);this.partial=null;for(const p of this.pending.values()){clearTimeout(p.timer);p.reject(new Error('連線已中止'));}this.pending.clear();}
  disconnect(){++this.session;this.stop();if(this.event){this.event.removeEventListener('characteristicvaluechanged',this.onValue);this.event=null;}if(this.device){this.device.removeEventListener('gattserverdisconnected',this.onDisconnect);this.device.gatt?.disconnect();}this.connection='disconnected';this.message='已斷線，保留最後收到的資料';this.emit();}
  async connect(){
    if(['selecting','connecting','waiting'].includes(this.connection))return;
    this.disconnect();const session=++this.session;
    if(location.protocol==='file:'||!isSecureContext||!navigator.bluetooth?.requestDevice){this.connection='error';this.message='請使用支援 Web Bluetooth 的瀏覽器，從 HTTPS 或 localhost 開啟。';this.emit();return;}
    this.connection='selecting';this.message='請在瀏覽器視窗選擇 ESP32-PET';this.emit();
    let selected;
    try{
      selected=await navigator.bluetooth.requestDevice({filters:[{services:[UUID.service]},{namePrefix:'ESP32-PET'}],optionalServices:[UUID.service]});
      if(session!==this.session)return;
      this.device=selected;this.cache=null;this.info=null;this.connection='connecting';this.message='正在連線';this.emit();
      this.onDisconnect=()=>{if(session!==this.session)return;this.disconnect();};selected.addEventListener('gattserverdisconnected',this.onDisconnect);
      const server=await selected.gatt.connect();if(session!==this.session){selected.gatt.disconnect();return;}
      const service=await server.getPrimaryService(UUID.service);const command=await service.getCharacteristic(UUID.command);const event=await service.getCharacteristic(UUID.event);
      if(session!==this.session)return;this.writer=command;this.event=event;this.onValue=e=>{if(session===this.session)this.receive(e.target.value);};event.addEventListener('characteristicvaluechanged',this.onValue);await event.startNotifications();
      if(session!==this.session)return;this.connection='waiting';this.message='已連線，等待裝置資訊與完整狀態';this.emit();
      await this.query('get_device_info');if(session!==this.session)return;await this.query('get_status');
    }catch(e){if(session!==this.session)return;this.disconnect();this.connection=e.name==='NotFoundError'?'cancelled':'error';this.message=e.name==='NotFoundError'?'已取消選擇，或沒有找到裝置；可以重新搜尋。':e.name==='SecurityError'||e.name==='NotAllowedError'?'藍牙權限被拒絕或受瀏覽器限制。':e.message||'連線失敗，請確認裝置和藍牙。';this.emit();}
  }
  async query(cmd){
    const id=this.nextId++,session=this.session;
    const response=new Promise((resolve,reject)=>{this.pending.set(id,{cmd,resolve,reject,timer:setTimeout(()=>{this.pending.delete(id);reject(new Error('等待裝置回覆逾時，請重新連線。'));},8000)});});
    // Attach a handler before async writes to avoid an unhandled timeout rejection.
    response.catch(()=>{});
    try{const bytes=new TextEncoder().encode(JSON.stringify({v:1,id,cmd})),count=Math.ceil(bytes.length/16),messageId=id%65536;
      for(let i=0;i<count;i++){if(session!==this.session)throw new Error('連線已中止');const data=new Uint8Array(4+Math.min(16,bytes.length-i*16));data[0]=messageId&255;data[1]=messageId>>8;data[2]=i;data[3]=count;data.set(bytes.slice(i*16,i*16+16),4);await this.writer.writeValueWithResponse(data);}await response;
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
    if(msg.v!==1)throw new Error('不支援的 BLE 協定版本');
    if(msg.type==='command_result'){const p=this.pending.get(msg.id);if(p&&msg.ok!==true){clearTimeout(p.timer);this.pending.delete(msg.id);p.reject(new Error('裝置拒絕查詢：'+String(msg.code)));}return;}
    if(msg.type==='device_info'){if(msg.protocol_version!==1||msg.device_id!=null&&!/^esp32-pet-[A-F0-9]{12}$/.test(msg.device_id)||typeof msg.firmware_version!=='string'||msg.max_message_bytes!==1024)throw new Error('裝置資訊不相容');this.info=msg;}
    else if(msg.type==='status'){const pet=mapStatus(msg);if(this.info?.device_id&&pet.deviceId&&this.info.device_id!==pet.deviceId)throw new Error('裝置 ID 不一致');this.cache={wire:msg,pet,receivedAt:new Date().toISOString()};PetLocalStore.write('device-snapshot',this.cache);this.connection='ready';this.message='已收到 ESP32 完整狀態（唯讀）';}
    else return;
    for(const [id,p] of this.pending){if(p.cmd===(msg.type==='device_info'?'get_device_info':'get_status')){clearTimeout(p.timer);this.pending.delete(id);p.resolve(msg);}}
    this.emit();
  }
}
window.PetSources.ble=new BleSource();
})();
