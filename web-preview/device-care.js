/* Device-mode care over BLE (PROTOCOL.md 養成同步, care): which care buttons the ESP32 accepts right now, the request,
 * and the Traditional Chinese lines for its result. The ESP32 decides and settles; the page never adds the deltas
 * itself, it only names them and waits for the next status snapshot. 'rest' is the one 睡覺／起床 button. */
(()=>{'use strict';
const ERRORS={busy:'電子雞正在忙，請先讓它回到主畫面。',pet_changed:'電子雞上的寵物換了，請等狀態更新後再試。',not_allowed:'寵物現在的狀態不能這樣照顧。',save_failed:'電子雞沒能保存這次照顧，請看最新狀態後再試。',op_conflict:'電子雞記得另一個相同編號的操作，這次沒有執行，請再按一次。',invalid_value:'這台電子雞不接受這個照顧動作，請更新韌體。',missing_field:'這台電子雞不接受這個照顧動作，請更新韌體。',timeout:'沒有收到電子雞的照顧結果，結果不明，請看最新狀態。'};
const SENDING={feed:'正在請電子雞餵牠……',clean:'正在請電子雞幫牠清潔……',treat:'正在請電子雞幫牠治療……',sleep:'正在請電子雞讓牠睡覺……',wake:'正在叫醒牠……'};
const DONE={feed:'餵好了',clean:'清潔好了',treat:'治療好了',sleep:'牠在電子雞上睡著了',wake:'牠醒來了'};
const LABEL={satiety:'飽食度',cleanliness:'清潔度',exp:'EXP'};
// The garden animation each care action replays after done; sleep and wake follow the next snapshot only.
const ANIMATION={feed:'feed',clean:'clean',treat:'treat'};
function errorText(e){return ERRORS[e?.code]||e?.message||'電子雞沒有回應。';}
function view(){return petDataManager.view('ble');}
// The action a care button sends: rest is wake while the device says the pet sleeps, else sleep.
function actionFor(kind){return kind==='rest'?(view().pet?.sleepMode==='normal'?'wake':'sleep'):kind;}
// Why this care button cannot be used now; '' when it can (PROTOCOL.md care table, activity before pet conditions).
function blocker(kind){
 const v=view(),p=v.pet,act=actionFor(kind);
 if(!v.care)return v.connection==='ready'?'這台電子雞的韌體還不支援網頁照顧。':'請先連接電子雞。';
 if(v.careSending)return '正在等電子雞回覆上一個照顧動作。';
 if(!['feed','clean','treat','sleep','wake'].includes(act))return '這個照顧動作不能從網頁使用。';
 if(v.activity==='device_game')return '牠正在電子雞上陪你玩，等那邊玩完吧。';
 if(v.activity==='ended'||!p||p.isDeparted||p.healthState==='dead')return '寵物現在不能照顧。';
 if(act==='wake')return v.activity==='sleeping'?'':'請先讓電子雞回到睡眠畫面。';
 if(v.activity==='sleeping')return '牠正在電子雞上睡覺，先讓牠起床吧。';
 if(!['idle','menu'].includes(v.activity))return '請先讓電子雞回到主畫面。';
 if(!['baby','adult'].includes(p.stage))return '還是蛋，等牠孵化後再照顧吧。';
 if(p.sleepMode!=='awake')return '牠正在睡覺，先讓牠起床吧。';
 if(act==='treat'&&p.healthState!=='sick')return '牠沒有生病，不需要治療。';
 return '';
}
// Device-mode button state: disabled while anything plays (hold) or a result is pending; a blocked button stays
// clickable but dimmed with the reason as tooltip, like 陪玩, so a click can say why.
function mark(buttons,hold){
 const v=view();
 for(const b of buttons){const kind=b.dataset?.action;if(!kind||kind==='play')continue;
  b.disabled=!v.care||hold||v.careSending||kind==='treat'&&v.pet?.healthState!=='sick';
  const why=b.disabled?'':blocker(kind);
  if(b.classList){b.classList.toggle('is-blocked',!!why);b.setAttribute('aria-disabled',String(!!why));b.title=why;}
  if(kind==='rest'){const text=actionFor(kind)==='wake'?'起床':'睡覺',small=b.querySelector?.('small');if(small)small.textContent=text;b.setAttribute?.('aria-label',text);}}
}
const plus=n=>'+'+n;
// "餵好了：飽食度 +20、EXP +3。" from the device's real deltas; full meters say so instead of +0.
function doneText(r){
 const c=r.changes||{},parts=Object.keys(LABEL).filter(k=>Number.isInteger(c[k])).map(k=>LABEL[k]+' '+plus(c[k]));
 let line=DONE[r.action]||'電子雞完成了照顧';
 if(parts.length)line+=parts.every(x=>/ \+0$/.test(x))&&['feed','clean'].includes(r.action)?'（已經滿了，數值沒有增加）':'：'+parts.join('、');
 if(c.levels_gained>0)line+='，升了 '+c.levels_gained+' 級';if(c.grew)line+='，還長大了';
 return line+'。';
}
// Resolves with {action,changes} after action_result done; rejects with the Traditional Chinese reason.
async function run(kind){
 const why=blocker(kind);if(why)throw new Error(why);
 const act=actionFor(kind);
 try{return await petDataManager.command(act);}catch(e){const out=new Error(errorText(e));out.code=e?.code||null;throw out;}
}
window.PetDeviceCare={actionFor,blocker,mark,run,doneText,errorText,sendingText:kind=>SENDING[actionFor(kind)]||'正在送出照顧動作……',ANIMATION};
})();
