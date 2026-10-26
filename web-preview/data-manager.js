/* Application boundary: UI never receives adapter objects or mutable storage. */
(()=>{'use strict';
const copy=x=>JSON.parse(JSON.stringify(x));
function normalize(p){
 if(!p)return null;
 const healthState=p.healthState||p.health||(p.condition==='dead'?'dead':p.condition==='sick'?'sick':'healthy');
 const result={speciesId:1,isDeparted:false,...p,healthState,sleepMode:p.sleepMode||(p.condition==='sleep'?'normal':'awake')};delete result.condition;delete result.health;return result;
}
function presentation(p){if(!p)return null;return {...p,health:p.healthState,condition:p.isDeparted?'departed':p.healthState==='dead'?'dead':p.sleepMode==='normal'?'sleep':p.healthState==='sick'?'sick':'awake'};}
class DataManager {
 constructor(adapters){this.adapters=adapters;this.mode='mock';}
 select(mode){if(!['mock','ble'].includes(mode))throw new Error('未知資料來源');if(this.mode!==mode)this.cancelGame();this.mode=mode;}
 read(mode=this.mode){const s=this.adapters[mode].read();return copy({pet:normalize(s.pet),metadata:{source:mode,connection:s.connection,receivedAt:s.receivedAt??null,stale:s.stale??false,message:s.message||'',activity:s.activity??null,webGame:!!s.webGame,gameEnd:s.gameEnd??null,games:s.games??null,petEvents:!!s.petEvents,care:!!s.care,lifecycle:!!s.lifecycle,careSending:!!s.careSending,soundControl:!!s.soundControl,sound:s.sound??null,memorialSync:!!s.memorialSync,rules:s.rules??null,memorials:s.memorials??null}});}
 view(mode=this.mode){const s=this.read(mode);return {...s.metadata,pet:presentation(s.pet)};}
 // Device mode with the care feature: a promise of the device's action_result ({action,changes}); the pet itself only
 // changes through the next status snapshot. Mock stays synchronous and unchanged.
 command(kind){if(this.mode==='ble'&&this.read('ble').metadata.care)return this.adapters.ble.care(kind).then(copy);if(this.mode!=='mock')throw new Error('BLE 僅提供唯讀查詢');return this.adapters.mock.command(kind);}
 preview(stage,condition){if(this.mode!=='mock')throw new Error('裝置狀態由 ESP32 決定');this.adapters.mock.preview(stage,condition);}
 demoNeed(key,value){if(this.mode!=='mock')throw new Error('裝置數值由 ESP32 決定');this.adapters.mock.demoNeed(key,value);}
 reset(){if(this.mode!=='mock')throw new Error('不可重設裝置');this.adapters.mock.reset();}
 // Device mode with the lifecycle feature: depart / adopt return the ESP32's action_result promise ({action,changes});
 // the page confirms first and the next snapshot shows the result. Mock stays synchronous.
 adopt(){if(this.mode==='ble'&&this.read('ble').metadata.lifecycle)return this.adapters.ble.lifecycle('adopt').then(copy);if(this.mode!=='mock')throw new Error('請在裝置領養');const p=this.view().pet;if(p.healthState!=='dead'&&!p.isDeparted)throw new Error('目前個體尚未死亡');if(!PetMemorials.remember(p,'mock',''))throw new Error('墓碑保存失敗');this.adapters.mock.adopt();}
 startGame(kind,now){if(this.mode!=='mock')throw new Error('裝置遊戲不可遠端操作');return this.adapters.mock.startGame(kind,now);}
 gameState(now){return this.mode==='mock'?this.adapters.mock.gameState(now):null;}
 answerGame(choice,now){if(this.mode!=='mock')throw new Error('裝置遊戲不可遠端操作');return this.adapters.mock.answerGame(choice,now);}
 cancelGame(){this.adapters.mock.cancelGame();}
 // Web play with the real pet: the device decides and settles; the page only relays the game commands.
 deviceGame(cmd,fields){if(this.mode!=='ble')throw new Error('只有裝置模式能和電子雞陪玩');return this.adapters.ble.game(cmd,fields);}
 // Best effort, also after leaving device mode, so the ESP32 is not left waiting for a heartbeat timeout.
 cancelDeviceGame(session){try{return this.adapters.ble.game('game_cancel',{session}).catch(()=>{});}catch{return Promise.resolve();}}
 farewell(){if(this.mode==='ble'&&this.read('ble').metadata.lifecycle)return this.adapters.ble.lifecycle('depart').then(copy);if(this.mode!=='mock')throw new Error('請在裝置送別');this.adapters.mock.farewell();}
 tick(seconds){if(this.mode==='mock')this.adapters.mock.tick(seconds);}
 subscribe(fn){return this.adapters.ble.subscribe(()=>fn(this.read('ble')));}
 // Device pet_event (PROTOCOL.md 養成同步): a copy that only picks an animation; pet state still comes from snapshots.
 onPetEvent(fn){return this.adapters.ble.onPetEvent(e=>fn(copy(e)));}
 // Device speaker mute (sound feature): the action_result promise {action:'set_sound',changes:{muted}}; the state shown comes from the next snapshot.
 setDeviceSound(muted){if(this.mode!=='ble')throw new Error('裝置模式才能調整電子雞的聲音');return this.adapters.ble.setSound(muted).then(copy);}
 // Device album (memorials feature, any page mode): the full list {hash,items} and one deletion's action_result promise.
 // memorial-sync.js reconciles them with the Web album; the page confirms a deletion before it is queued.
 deviceMemorials(){return this.adapters.ble.getMemorials().then(copy);}
 deleteDeviceMemorial(petId){return this.adapters.ble.deleteMemorial(petId).then(copy);}
 connect(){return this.adapters.ble.connect();}
 disconnect(){return this.adapters.ble.disconnect();}
}
window.PetDomain={normalize,presentation};window.DataManager=DataManager;window.petDataManager=new DataManager(window.PetSources);
})();
