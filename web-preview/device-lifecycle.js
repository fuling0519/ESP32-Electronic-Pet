/* Device-mode farewell and adoption over BLE (PROTOCOL.md 養成同步, depart / adopt with the lifecycle feature). The ESP32
 * does not ask again (D3), so the page confirms first; it only sends after that and shows the result line. Departure,
 * the egg and the album still come from the next status snapshot, never from the action_result itself. */
(()=>{'use strict';
// memorial_full: a record can be deleted in either album (the web one syncs to the ESP32 since phase 4).
const FULL='電子雞的紀念冊已滿，請先刪除一筆紀念紀錄（在網頁紀念冊或 ESP32 上都可以）再試。';
const ERRORS={
 depart:{busy:'電子雞正在忙，請先讓它回到主畫面再送別。',not_allowed:'寵物現在的狀態不能送別（要是醒著的幼年或成年）。',timeout:'沒有收到電子雞的送別結果，結果不明，請看最新狀態。',save_failed:'電子雞沒能保存送別，請看最新狀態後再試。'},
 adopt:{busy:'電子雞正在忙，請等它回到結束畫面再領養。',not_allowed:'電子雞還在保存上一隻的紀念紀錄，或牠還沒結束，暫時不能領養。',timeout:'沒有收到電子雞的領養結果，結果不明，請看最新狀態。',save_failed:'電子雞沒能保存領養，請看最新狀態後再試。'},
 any:{memorial_full:FULL,pet_changed:'電子雞上的寵物換了，請等狀態更新後再試。',op_conflict:'電子雞記得另一個相同編號的操作，這次沒有執行，請再試一次。',invalid_value:'這台電子雞不接受這個命令，請更新韌體。',missing_field:'這台電子雞不接受這個命令，請更新韌體。'}};
const SENDING={depart:'正在請電子雞送牠出發……',adopt:'正在請電子雞準備新蛋……'};
function errorText(kind,e){return ERRORS[kind]?.[e?.code]||ERRORS.any[e?.code]||e?.message||'電子雞沒有回應。';}
function view(){return petDataManager.view('ble');}
const ended=p=>!!p&&(p.isDeparted||p.health==='dead'||p.healthState==='dead');
// Whether each entry is offered at all: the lifecycle feature, then a living baby/adult or an ended pet.
function shown(kind){const v=view(),p=v.pet;if(!v.lifecycle||!p)return false;return kind==='depart'?['baby','adult'].includes(p.stage)&&!ended(p):ended(p);}
// Why the offered entry cannot be used now; '' when it can (activity before pet conditions, like care).
function blocker(kind){
 const v=view(),p=v.pet;
 if(!v.lifecycle)return v.connection==='ready'?'這台電子雞的韌體還不支援從網頁'+(kind==='depart'?'送別':'領養')+'。':'請先連接電子雞。';
 if(v.careSending)return '正在等電子雞回覆上一個操作。';
 if(!shown(kind))return kind==='depart'?'寵物現在不能送別。':'目前的寵物還沒有結束，不能領養新蛋。';
 if(kind==='adopt')return v.activity==='ended'?'':'請等電子雞回到結束畫面再領養。';
 if(v.activity==='device_game')return '牠正在電子雞上陪你玩，等那邊玩完吧。';
 if(v.activity==='sleeping'||p.sleepMode!=='awake')return '牠正在睡覺，先讓牠起床吧。';
 if(!['idle','menu'].includes(v.activity))return '請先讓電子雞回到主畫面。';
 return '';
}
// The confirm dialog shows what the ESP32 itself would show: name and effective companion time.
function farewellInfo(){const p=view().pet;return p?p.name+' · 陪伴 '+PetMemorials.duration(String(p.ageSeconds)):'';}
function doneText(r){
 const c=r?.changes||{};
 if(r?.action==='depart')return c.memorial_saved===false?'牠已踏上旅程。電子雞還在保存紀念紀錄，稍後會自動完成。':'牠已踏上旅程，紀念紀錄保存在電子雞上。';
 // The species stays a surprise until the egg hatches, so only the name is shown.
 return c.name?'領養了新蛋：'+c.name+'。等牠在電子雞上孵化吧。':'領養了新蛋，等電子雞回報新狀態。';
}
// Resolves with {action,changes} after action_result done; rejects with the Traditional Chinese reason and code.
async function run(kind){
 const why=blocker(kind);if(why)throw new Error(why);
 try{return await (kind==='depart'?petDataManager.farewell():petDataManager.adopt());}catch(e){const out=new Error(errorText(kind,e));out.code=e?.code||null;throw out;}
}
window.PetDeviceLifecycle={shown,blocker,farewellInfo,run,doneText,errorText,sendingText:kind=>SENDING[kind]||'正在送出……',FULL};
})();
