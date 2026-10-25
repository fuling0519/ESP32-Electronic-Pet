/* One Web play session with the real pet over BLE (PROTOCOL.md, Web 陪玩), shared by toy play and the garden chorus:
 * start, heartbeat, finish (waiting out the device's minimum time and retrying lost writes), cancel, and the checks
 * that end a session early. The ESP32 decides and settles; the page only reports the score. */
(()=>{'use strict';
const ERRORS={busy:'電子雞正在忙，請先讓它回到主畫面。',not_allowed:'寵物現在不能陪玩。',pet_changed:'電子雞上的寵物換了，請等狀態更新後再試。',invalid_session:'電子雞已經結束這局了。',invalid_value:'電子雞不接受這個遊戲或成績。'};
const REASONS={device_cancel:'在電子雞上結束了陪玩，這局沒有結算。',heartbeat_timeout:'太久沒收到網頁的訊號，電子雞結束了這局，沒有結算。',pet_state:'寵物的狀態變了，這局沒有結算。'};
function errorText(e){return ERRORS[e?.code]||e?.message||'電子雞沒有回應。';}
let starting=false;
// The games this firmware can start; the first web-play firmware did not list them and only had toy play.
function games(){const v=petDataManager.view('ble');return Array.isArray(v.games)?v.games:v.webGame?['toy']:[];}
// Why a device game (or, without a game, any device game) cannot start now; '' when it can.
function blocker(game){
 const v=petDataManager.view('ble'),p=petDataManager.read('ble').pet;
 if(!v.webGame)return v.connection==='ready'?'這台電子雞的韌體還不支援網頁陪玩。':'請先連接電子雞。';
 if(game&&!games().includes(game))return '這台電子雞的韌體還不支援這個遊戲，請更新韌體。';
 if(v.activity==='device_game')return '牠正在電子雞上陪你玩，等那邊玩完吧。';
 if(v.activity!=='idle')return '請先讓電子雞回到主畫面。';
 if(!p||!['baby','adult'].includes(p.stage)||p.healthState!=='healthy'||p.sleepMode!=='awake'||p.isDeparted)return '寵物現在不能陪玩。';
 return starting?'正在請電子雞準備陪玩。':'';
}
// Opens the session on the device and starts the heartbeat. onLost(text) runs if the device refuses a heartbeat;
// a heartbeat lost in transport is not fatal (the next one, or the device's own timeout, settles it).
async function start(game,onLost){
 const why=blocker(game);if(why)throw new Error(why);
 const pet=petDataManager.read('ble').pet;starting=true;
 let s;try{s=await petDataManager.deviceGame('game_start',{game,pet_id:pet.petId});}catch(e){throw new Error(errorText(e));}finally{starting=false;}
 const session={id:s.session,game,pet,minMs:Number(s.min_ms)||0,startedAt:performance.now(),ended:false,timer:null};
 if(petDataManager.mode!=='ble'){close(session,true);throw new Error('已離開裝置模式。');}
 session.timer=setInterval(()=>{if(session.ended)return;petDataManager.deviceGame('game_heartbeat',{session:session.id}).catch(e=>{if(e?.code&&!session.ended){close(session);onLost(errorText(e)+' 這局沒有結算。');}});},Number(s.heartbeat_ms)||5000);
 return session;
}
// '' while the session can go on; otherwise why it ended (leaving device mode also cancels it on the device).
function check(session){
 if(session.ended)return '';
 if(petDataManager.mode!=='ble'){close(session,true);return '已離開裝置模式，這局沒有結算。';}
 const v=petDataManager.view('ble');
 if(v.connection!=='ready'){close(session);return '藍牙斷線，這局沒有結算。';}
 if(v.gameEnd?.session===session.id){close(session);return REASONS[v.gameEnd.reason]||'電子雞結束了這局，沒有結算。';}
 return '';
}
// Resolves with the device's game_result. too_early waits out the device's minimum time; an error without a code
// means the write or reply was lost, and resending is safe because the device replays a settled session's result.
function finish(session,score){
 let retries=3;
 return new Promise((resolve,reject)=>{
  const attempt=()=>{
   if(session.ended)return;
   petDataManager.deviceGame('game_finish',{session:session.id,score}).then(r=>{if(session.ended)return;close(session);resolve(r);},e=>{
    if(session.ended)return;
    if(e?.code==='too_early'){setTimeout(attempt,Math.max(300,session.minMs-(performance.now()-session.startedAt)+300));return;}
    if(!e?.code&&retries-->0){setTimeout(attempt,600);return;}
    close(session);reject(new Error(errorText(e)+' 這局沒有結算。'));});
  };
  attempt();
 });
}
function close(session,cancel=false){
 if(session.ended)return;session.ended=true;clearInterval(session.timer);
 if(cancel)petDataManager.cancelDeviceGame(session.id);
}
window.PetDeviceSession={blocker,start,check,finish,close,games,errorText,starting:()=>starting};
})();
