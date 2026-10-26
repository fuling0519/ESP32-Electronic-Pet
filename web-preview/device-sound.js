/* Device-mode remote sound switch and touch events over BLE (PROTOCOL.md 養成同步 phase 5: the sound feature's
 * set_sound command and the petted / poked pet_events). The button is for the ESP32's own speaker and is separate from the
 * page's own sound button (game-ui.js); its label comes from the last status snapshot only, never from the click.
 * The touch events only replay the web touch reaction (pet-interactions.js PetTouch.remote); they send and change nothing. */
(()=>{'use strict';
const ERRORS={busy:'電子雞正在調整聲音設定，等它回到主畫面再試。',save_failed:'電子雞沒能保存聲音設定，請看最新狀態後再試。',op_conflict:'電子雞記得另一個相同編號的操作，這次沒有執行，請再按一次。',invalid_value:'這台電子雞不接受這個聲音設定，請更新韌體。',missing_field:'這台電子雞不接受這個聲音設定，請更新韌體。',timeout:'沒有收到電子雞的聲音設定結果，結果不明，請看最新狀態。'};
const OK_ACTIVITY=['idle','menu','sleeping','ended'];
function errorText(e){return ERRORS[e?.code]||e?.message||'電子雞沒有回應。';}
function view(){return petDataManager.view('ble');}
function label(sound){return '電子雞聲音：'+(sound==='on'?'開':sound==='muted'?'靜音':'—');}
// Why the button cannot be used now; '' when it can. Needs the feature and a snapshot that says on / muted.
function blocker(){
 const v=view();
 if(!v.soundControl)return v.connection==='ready'?'這台電子雞的韌體還不支援從網頁調整聲音。':'請先連接電子雞。';
 if(v.careSending)return '正在等電子雞回覆上一個操作。';
 if(!v.sound)return '還沒收到電子雞的聲音狀態。';
 if(v.activity==='device_game')return '牠正在電子雞上陪你玩，等那邊玩完吧。';
 if(!OK_ACTIVITY.includes(v.activity))return '電子雞正在忙，等它回到主畫面再調整聲音。';
 return '';
}
// Resolves with {action,changes} after action_result done; rejects with the Traditional Chinese reason.
async function run(){
 const why=blocker();if(why)throw new Error(why);
 try{return await petDataManager.setDeviceSound(view().sound==='on');}catch(e){const out=new Error(errorText(e));out.code=e?.code||null;throw out;}
}
function doneText(r){return r?.changes?.muted===true?'電子雞已設為靜音。':r?.changes?.muted===false?'電子雞的聲音已開啟。':'電子雞的聲音設定好了。';}
// Shown only in device mode with the feature; the label follows the snapshot, the tooltip says why it is blocked.
function sync(){
 const row=document.querySelector('#device-sound'),b=document.querySelector('#device-sound-button');if(!row||!b)return;
 const v=view(),on=petDataManager.mode==='ble'&&v.soundControl;row.hidden=!on;if(!on)return;
 const why=blocker();b.textContent=label(v.sound);b.disabled=v.careSending;b.classList?.toggle('is-blocked',!!why);b.setAttribute('aria-disabled',String(!!why));b.title=why||'切換電子雞自己的喇叭（不是網頁聲音）';
 b.setAttribute('aria-label',label(v.sound)+'，按一下切換');
}
document.querySelector('#device-sound-button')?.addEventListener('click',()=>{
 const status=document.querySelector('#status'),why=blocker();if(why){status.textContent=why;return;}
 const p=run();status.textContent='正在請電子雞'+(view().sound==='on'?'靜音':'開啟聲音')+'……';sync();
 p.then(r=>{status.textContent=doneText(r);},e=>{status.textContent=e.message;}).finally(sync);
});
petDataManager.subscribe(sync);
// petted / poked: replay the touch reaction when the web would also allow a touch; otherwise skip silently with one log line.
petDataManager.onPetEvent(e=>{
 if(e.event!=='petted'&&e.event!=='poked')return;
 const why=window.PetTouch?.remote?window.PetTouch.remote(e.event==='petted'?'pet':'poke'):'touch not loaded';
 if(why)console.info('[pet_event]',e.event,'no animation: '+why);
});
sync();
window.PetDeviceSound={blocker,run,sync,label,doneText,errorText};
})();
