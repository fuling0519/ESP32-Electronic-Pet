/* UI-only demo. No BLE, API, account or device-authoritative state. */
(()=>{'use strict';
const root=document.querySelector('#aviary'),canvas=document.querySelector('#bird'),ctx=canvas.getContext('2d'),status=document.querySelector('#status'),bubble=document.querySelector('#bubble');

const characterSheet=new Image();characterSheet.src='assets/tit-game-smooth.png';
let mode='mock';const sources=window.PetSources;
const defaults={satiety:60,mood:55,cleanliness:45};let state={...defaults},action=null,actionStart=0,night=false,resetToken=0;
const reduced=matchMedia('(prefers-reduced-motion: reduce)');
const savedPreferences=PetLocalStore.read('preferences',PetLocalStore.validPreferences);
let stage=sources.mock.pet.stage,condition=sources.mock.pet.condition,backdrop=savedPreferences?.backdrop||'smooth';
if(backdrop==='simple')backdrop='smooth';
if(savedPreferences){mode=savedPreferences.mode;night=savedPreferences.night;}
function savePreferences(){PetLocalStore.write('preferences',{mode,night,backdrop});}
const palette={P:'#282523',W:'#fffaf0',G:'#896448',L:'#896448'};
function update(){const snapshot=sources[mode].read();state=snapshot.pet?{...snapshot.pet}:{};for(const k of Object.keys(defaults)){const meter=document.querySelector('#'+k);if(snapshot.pet)meter.value=state[k];else meter.removeAttribute('value');meter.hidden=!snapshot.pet;document.querySelector('#'+k+'-value').textContent=snapshot.pet?state[k]:'—';}}
function busy(value){document.querySelectorAll('[data-action]').forEach(b=>b.disabled=mode!=='mock'||value||condition==='sleep'||condition==='dead'||stage==='egg'||(condition==='sick'&&b.dataset.action==='play')||(b.dataset.action==='treat'&&condition!=='sick'));}
function speak(text){bubble.textContent=text;bubble.hidden=false;}
function sickMessage(){speak(stage==='baby'?'啾…我不舒服，想休息一下…':'有點難受…可以照顧我嗎？');}
function begin(kind){if(mode!=='mock')return;if(action||condition==='dead'||stage==='egg'||condition==='sleep'||condition==='dead'||stage==='egg'||(condition==='sick'&&kind==='play'))return;if(kind==='treat'){if(condition!=='sick')return;action=kind;actionStart=performance.now();busy(true);speak('乖乖休息，正在照顧你…');status.textContent='正在治療生病的小鳥（展示）…';return;}const key=kind==='feed'?'satiety':kind==='clean'?'cleanliness':'mood';if(state[key]===100){speak(kind==='feed'?'吃飽囉！':kind==='clean'?'羽毛已經很乾淨了！':'今天心情超好！');status.textContent='展示'+({satiety:'飽食度',cleanliness:'清潔度',mood:'心情'})[key]+'已達 100。';return;}action=kind;actionStart=performance.now();busy(true);speak(kind==='feed'?'開飯啦！':kind==='clean'?'整理一下羽毛～':'一起玩吧！');status.textContent=kind==='feed'?'牠正在啄食種子……':kind==='clean'?'牠正在整理羽毛，髒污逐漸消失……':'牠開心地跳躍、拍拍翅膀……';}
function finish(){if(mode!=='mock'||!action)return;const kind=action;if(kind==='treat'){sources.mock.command(kind);action=null;document.querySelector('#condition').value='awake';preview();update();speak('舒服多了，謝謝你！');status.textContent='展示治療完成，已恢復健康；飽食度與清潔度保持原值。';const token=++resetToken;setTimeout(()=>{if(token===resetToken&&!action)bubble.hidden=true;},1800);return;}const {key,actual}=sources.mock.command(kind);action=null;busy(false);update();speak(kind==='feed'?'謝謝你，好好吃！':kind==='clean'?'乾乾淨淨！':'再一起玩！');status.textContent='展示'+({satiety:'飽食度',cleanliness:'清潔度',mood:'心情'})[key]+' +'+actual+'。';const token=++resetToken;setTimeout(()=>{if(token===resetToken&&!action){if(condition==='sick')sickMessage();else bubble.hidden=true;}},1800);}
function block(x,y,w,h,color){ctx.fillStyle=color;ctx.fillRect(Math.round(x),Math.round(y),w,h);}
function heart(x,y){const rows=['.PP.PP.','PPPPPPP','PPPPPPP','.PPPPP.','..PPP..','...P...'];rows.forEach((r,j)=>[...r].forEach((c,i)=>{if(c==='P')block(x+i,y+j,1,1,'#e8a2b0');}));}
function foodBowl(elapsed){
  const cx=255,cy=185,remaining=1-Math.min(1,Math.max(0,(elapsed-.5)/2));
  function ellipse(x,y,rx,ry,fill){ctx.beginPath();ctx.ellipse(x,y,rx,ry,0,0,Math.PI*2);ctx.fillStyle=fill;ctx.fill();}
  ctx.save();ellipse(cx,211,49,7,'rgba(63,44,28,.16)');
  const outside=ctx.createLinearGradient(0,177,0,210);outside.addColorStop(0,'#c6b59f');outside.addColorStop(.5,'#aa9580');outside.addColorStop(1,'#80705f');
  ctx.beginPath();ctx.moveTo(cx-43,cy-1);ctx.bezierCurveTo(cx-49,cy+8,cx-48,cy+20,cx-34,cy+23);ctx.bezierCurveTo(cx-10,cy+32,cx+31,cy+29,cx+43,cy+17);ctx.quadraticCurveTo(cx+49,cy+9,cx+43,cy-1);ctx.closePath();ctx.fillStyle=outside;ctx.fill();ctx.strokeStyle='#766553';ctx.lineWidth=1.4;ctx.stroke();
  ellipse(cx,cy,44,20,'#d2c3ae');ellipse(cx,cy,39,16,'#847360');ellipse(cx,cy+1,35,13,'#a18d72');
  ctx.save();ctx.beginPath();ctx.ellipse(cx,cy,35,13,0,0,Math.PI*2);ctx.clip();
  const count=Math.ceil(64*remaining);for(let i=0;i<count;i++){const theta=i*2.39996,r=Math.sqrt((i+.5)/64),px=cx+Math.cos(theta)*r*34,py=cy+Math.sin(theta)*r*12;
    const grain=ctx.createRadialGradient(px-1,py-1,.2,px,py,3);grain.addColorStop(0,i%4===0?'#e4cf91':'#f1cf73');grain.addColorStop(1,i%4===0?'#8b7349':'#b68a3f');ellipse(px,py,3,2.5,grain);ctx.strokeStyle='#947348';ctx.lineWidth=.5;ctx.stroke();}
  ctx.restore();ctx.beginPath();ctx.ellipse(cx,cy,42,18,0,0,Math.PI);ctx.strokeStyle='#e3d5c1';ctx.lineWidth=3;ctx.stroke();ctx.restore();
}
function resizeBird(){const scale=Math.max(2,(canvas.clientWidth||384)/384*(devicePixelRatio||1));canvas.width=Math.round(384*scale);canvas.height=Math.round(256*scale);ctx.setTransform(canvas.width/384,0,0,canvas.height/256,0,0);}
new ResizeObserver(resizeBird).observe(canvas);resizeBird();
function draw(now){ctx.clearRect(0,0,384,256);if(drawLifecycle(now)){requestAnimationFrame(draw);return;}if(mode!=='mock'&&(!sources.ble.read().pet||stage==='egg'||sources.ble.read().pet.health==='dead')){requestAnimationFrame(draw);return;}const elapsed=action?(now-actionStart)/1000:0,moving=!reduced.matches&&(mode==='mock'||sources.ble.read().connection==='ready');let x=105,y=87,angle=0;
if(moving){y+=Math.floor(Math.sin(now/(condition==='awake'?650:1500)));if(condition==='sick'){x+=Math.sin(now/95)*2;angle=Math.sin(now/180)*.035;}if(action==='feed')angle=.16+Math.max(0,Math.sin(elapsed*10))*.5;if(action==='play'){y-=Math.abs(Math.sin(elapsed*5))*20;x+=Math.sin(elapsed*4)*8;}if(action==='clean')angle=Math.sin(elapsed*12)*.07;}
const flightPhase=(now%15000)/1000,flying=stage==='adult'&&condition==='awake'&&!action&&moving&&flightPhase>=5&&flightPhase<10;
if(flying){const p=(flightPhase-5)/5;y-=Math.sin(Math.PI*p)*44;x+=Math.sin(p*Math.PI*2)*18;angle+=Math.sin(p*Math.PI*2)*.08;}
const walking=stage==='baby'&&condition==='awake'&&!action&&moving,walkPhase=(now%12000)/1000,stepping=walking&&(walkPhase<4||(walkPhase>=6&&walkPhase<10));
if(stage==='baby'){x+=5;y+=8;if(walking){x+=walkPhase<4?walkPhase*6:walkPhase<6?24:walkPhase<10?24-(walkPhase-6)*6:0;if(stepping)y-=Math.floor(Math.abs(Math.sin(now/160))*2);}}
if(action==='feed'){x+=Math.min(1,elapsed/.4)*26;y+=stage==='baby'?12:2;}
const baby=stage==='baby',pose=action==='feed'?1:condition!=='awake'||!moving?0:Math.floor(now/6500)%4;
const view=pose===3?1:pose,dw=baby?98:125,dh=baby?94:112;
// Feet stay on the branch; the sheet provides the actual illustrated character.
ctx.save();ctx.fillStyle='rgba(69,48,33,.18)';ctx.beginPath();ctx.ellipse(x+65,190,baby?24:32,flying?3:5,0,0,Math.PI*2);ctx.fill();ctx.restore();
if(characterSheet.complete&&characterSheet.naturalWidth){
  const factor=characterSheet.naturalWidth/2172,sh=characterSheet.naturalHeight;const frames=[[0,680],[690,750],[1480,692]],frame=frames[view];const sx=frame[0]*factor,sw=frame[1]*factor;
  const feet= baby?y+85:y+101;
  ctx.save();ctx.translate(x+65,feet-dh/2);ctx.rotate(angle);
  const turn=moving&&condition==='awake'&&!action?Math.min((now%6500)/250,1):1;
  ctx.scale((pose===3?-1:1)*(action==='feed'?1:.92+.08*turn),1);
  ctx.imageSmoothingEnabled=true;ctx.imageSmoothingQuality='high';
  ctx.shadowColor='rgba(56,38,22,.16)';ctx.shadowBlur=5;ctx.shadowOffsetY=3;
  const width=dh*sw/sh;ctx.drawImage(characterSheet,sx,0,sw,sh,-width/2,-dh/2,width,dh);
  ctx.restore();
}
if(condition==='sleep'){ctx.fillStyle=palette.P;ctx.font='bold 12px monospace';for(let i=0;i<3;i++)ctx.fillText('z',x+90+i*12,y+12-i*10-(moving?(now/180+i*4)%9:0));}
if(action==='feed')foodBowl(elapsed);
if(action==='treat'){ctx.save();ctx.fillStyle='#fffaf0';ctx.strokeStyle='#896448';ctx.lineWidth=1.5;ctx.beginPath();ctx.roundRect(248,151,24,31,5);ctx.fill();ctx.stroke();ctx.fillStyle='#896448';ctx.fillRect(252,146,16,6);ctx.fillStyle='#acd1bc';ctx.fillRect(258,160,4,13);ctx.fillRect(253,165,14,4);if(moving){ctx.globalAlpha=Math.sin(elapsed*Math.PI/2.5)*.8;for(let i=0;i<3;i++){const sy=130-i*13-(elapsed*12%12);ctx.fillRect(145+i*23,sy,2,8);ctx.fillRect(142+i*23,sy+3,8,2);}}ctx.restore();}
if(action==='clean')for(let i=0;i<8;i++)block(134+(i*17)%72,110+(i*13+elapsed*22)%54,3,3,'#e9fbec');
if(action==='play')for(let i=0;i<3;i++)heart(145+i*24,82-(moving?(elapsed*17+i*10)%30:0));
if(night)for(let i=0;i<14;i++){ctx.globalAlpha=moving?.4+.6*Math.abs(Math.sin(now/800+i)):1;block(132+(i*29)%221,8+(i*13)%62,2,2,'#fff2bb');}ctx.globalAlpha=1;
if(action&&elapsed>=2.5)finish();requestAnimationFrame(draw);}
document.querySelectorAll('[data-action]').forEach(b=>b.addEventListener('click',()=>b.dataset.action==='treat'?healPet():begin(b.dataset.action)));
function setNight(value){night=value;root.classList.toggle('night',night);applyBackdrop();document.querySelector('#day').setAttribute('aria-pressed',String(!night));document.querySelector('#night').setAttribute('aria-pressed',String(night));root.querySelector('h1').textContent=night?'小鳥的月光花園':'小鳥的午後花園';savePreferences();}
function applyBackdrop(){root.classList.toggle('simple-background',backdrop==='simple'||backdrop==='smooth');root.classList.toggle('smooth-background',backdrop==='smooth');document.querySelector('#background').src=backdrop==='smooth'?'assets/garden-smooth.png':backdrop==='simple'?'assets/garden-simple.png':'assets/garden-detailed-'+(night?'night':'day')+'.png';}
document.querySelector('#backdrop').onchange=()=>{backdrop=document.querySelector('#backdrop').value;applyBackdrop();savePreferences();};applyBackdrop();
document.querySelector('#day').onclick=()=>setNight(false);document.querySelector('#night').onclick=()=>setNight(true);
let deathKey=null,deathStart=0,lastAgeTick=performance.now(),bleHistorySession=PetMemorials.uuid();
function currentPet(){return sources[mode].read().pet;}
function syncLifecycle(){
 const p=currentPet(),dead=p&&(p.condition==='dead'||p.health==='dead');
 document.querySelector('#afterlife-menu').hidden=!dead;document.querySelector('#tombstone').hidden=true;
 if(p){PetMemorials.remember(p,mode,bleHistorySession);if(mode==='mock'){document.querySelector('#pet-name').textContent=p.name;document.querySelector('#pet-age').textContent='陪伴時長：'+PetMemorials.duration(String(p.ageSeconds))+'（展示有效年齡）';}}
 const key=dead?mode+':'+(p.deviceId||'mock')+':'+p.petId:null;
 if(key!==deathKey){deathKey=key;deathStart=performance.now();}
 if(dead){action=null;resetToken++;bubble.hidden=true;document.querySelector('#illness-notice').hidden=true;root.classList.remove('is-sick');document.querySelector('#tomb-name').textContent=p.name+' · '+(p.petId||'未知代號');document.querySelector('#tomb-age').textContent='陪伴 '+PetMemorials.duration(String(p.ageSeconds));}
 const stageOrder=['egg','baby','adult'],stageRank=stageOrder.indexOf(p?.stage);for(const option of document.querySelector('#stage').options){const rank=stageOrder.indexOf(option.value);option.disabled=rank<stageRank||rank>stageRank+1;}
 for(const id of ['stage','condition','reset'])document.querySelector('#'+id).disabled=mode!=='mock'||!!dead;
 document.querySelector('#adopt-egg').disabled=mode!=='mock';document.querySelector('#adoption-note').textContent=mode==='ble'?'裝置模式請在 ESP32 上領養新蛋，網頁等待新狀態回報。':'新蛋使用新代號，舊紀錄留在墓園。';
}
function drawLifecycle(now){
 const p=currentPet();if(!p)return false;
 if(p.condition==='dead'||p.health==='dead'){
  const t=reduced.matches?1:Math.min(1,(now-deathStart)/5000);document.querySelector('#afterlife-menu').hidden=t<1;document.querySelector('#tombstone').hidden=t<1;
  if(t<1){ctx.save();ctx.globalAlpha=1-t;
   if(characterSheet.complete&&characterSheet.naturalWidth){const f=characterSheet.naturalWidth/2172;ctx.drawImage(characterSheet,0,0,680*f,characterSheet.naturalHeight,140,82,100,106);}
   // Cover the original pupils, then paint two crossed eyes.
   ctx.fillStyle='#fffaf0';ctx.strokeStyle='#57483a';ctx.lineWidth=2;ctx.lineCap='round';for(const ex of [180,202]){ctx.beginPath();ctx.ellipse(ex,115,7,7,0,0,Math.PI*2);ctx.fill();ctx.beginPath();ctx.moveTo(ex-3,112);ctx.lineTo(ex+3,118);ctx.moveTo(ex+3,112);ctx.lineTo(ex-3,118);ctx.stroke();}
   ctx.restore();ctx.save();ctx.globalAlpha=Math.sin(Math.PI*t)*.95;const gy=90-t*60;ctx.translate(190,gy);ctx.beginPath();ctx.moveTo(-15,14);ctx.lineTo(-15,0);ctx.bezierCurveTo(-15,-22,15,-22,15,0);ctx.lineTo(15,14);ctx.quadraticCurveTo(10,8,5,14);ctx.quadraticCurveTo(0,8,-5,14);ctx.quadraticCurveTo(-10,8,-15,14);ctx.fillStyle='#fffaf0';ctx.fill();ctx.strokeStyle='#96877f';ctx.lineWidth=1.2;ctx.stroke();ctx.fillStyle='#57483a';for(const ex of [-5,5]){ctx.beginPath();ctx.ellipse(ex,-1,1.8,2.5,0,0,Math.PI*2);ctx.fill();}ctx.restore();
  }return true;
 }
 document.querySelector('#tombstone').hidden=true;
 if(p.stage==='egg'){ctx.save();ctx.translate(190,163);ctx.rotate(reduced.matches?0:Math.sin(now/1000)*.035);const g=ctx.createRadialGradient(-8,-18,2,0,0,34);g.addColorStop(0,'#fffdf5');g.addColorStop(1,'#ceb99e');ctx.beginPath();ctx.moveTo(0,-34);ctx.bezierCurveTo(29,-34,36,30,0,30);ctx.bezierCurveTo(-36,30,-29,-34,0,-34);ctx.fillStyle=g;ctx.fill();ctx.strokeStyle='#896448';ctx.lineWidth=1.5;ctx.stroke();ctx.restore();return true;}return false;
}
document.querySelector('#open-cemetery').onclick=document.querySelector('#visit-graves').onclick=()=>PetMemorials.open(mode==='mock'?'mock:展示花園':currentPet()?.deviceId?'ble:'+currentPet().deviceId:undefined);
document.querySelector('#adopt-egg').onclick=()=>{if(mode==='mock'&&currentPet()?.condition==='dead')document.querySelector('#adopt-dialog').showModal();};
document.querySelector('#confirm-adopt').onclick=()=>{if(mode!=='mock'||currentPet()?.condition!=='dead')return;if(!PetMemorials.remember(currentPet(),'mock',bleHistorySession)){document.querySelector('#adopt-dialog p').textContent='墓園保存失敗，請先處理下方本地儲存提示，再領養新蛋。';return;}sources.mock.adopt();document.querySelector('#adopt-dialog').close();deathKey=null;document.querySelector('#stage').value='egg';document.querySelector('#condition').value='awake';preview();update();};
// Only page-active demo time advances. Hardware age always comes from BLE.
setInterval(()=>{const now=performance.now(),seconds=Math.min(2,Math.floor((now-lastAgeTick)/1000));lastAgeTick=now;if(document.hidden||mode!=='mock'||sources.mock.pet.condition==='dead')return;const p=sources.mock.pet;p.ageSeconds+=seconds;if(p.stage==='egg'&&p.ageSeconds>=300){p.stage='baby';document.querySelector('#stage').value='baby';preview();}else if(p.stage==='baby'&&p.ageSeconds>=3900&&p.condition!=='sick'){p.stage='adult';document.querySelector('#stage').value='adult';preview();}PetLocalStore.write('demo',p);document.querySelector('#pet-age').textContent='陪伴時長：'+PetMemorials.duration(String(p.ageSeconds))+'（展示有效年齡）';},1000);
let healthTransition=0;
function syncHealth(health){
 const badge=document.querySelector('#health-badge'),label=document.querySelector('#health-label'),button=document.querySelector('#treat-button'),sick=health==='sick',token=++healthTransition;
 badge.className='health-badge '+(sick?'sick':health==='healthy'?'healthy':'unknown');label.textContent=sick?'生病':health==='healthy'?'健康':health==='dead'?'已離世':'尚無資料';
 if(sick){button.hidden=false;button.classList.remove('treatment-hidden');button.setAttribute('aria-hidden','false');}
 else{button.disabled=true;button.classList.add('treatment-hidden');button.setAttribute('aria-hidden','true');if(document.activeElement===button)document.querySelector('#condition').focus();setTimeout(()=>{if(token===healthTransition)button.hidden=true;},reduced.matches?0:420);}
}
function setSickState(){if(mode!=='mock'||action)return;document.querySelector('#condition').value='sick';preview();update();}
function healPet(){if(mode!=='mock'||condition!=='sick'||action)return;begin('treat');if(action==='treat')root.classList.add('healing');}
window.setSickState=setSickState;window.healPet=healPet;
function preview(){if(mode!=='mock')return;action=null;root.classList.remove('healing');resetToken++;stage=document.querySelector('#stage').value;condition=document.querySelector('#condition').value;sources.mock.preview(stage,condition);stage=sources.mock.pet.stage;condition=sources.mock.pet.condition;document.querySelector('#stage').value=stage;document.querySelector('#condition').value=condition;syncLifecycle();busy(false);bubble.hidden=true;root.classList.toggle('is-sick',condition==='sick');document.querySelector('#illness-notice').hidden=condition!=='sick';syncHealth(condition==='dead'?'dead':condition==='sick'?'sick':'healthy');if(condition==='sick')sickMessage();document.querySelector('#pet-description').textContent=(stage==='baby'?'幼鳥':'成鳥')+' · '+(condition==='sick'?'生病':'健康')+' · '+(condition==='sleep'?'一般睡眠':'清醒');canvas.setAttribute('aria-label',(stage==='baby'?'幼鳥':'成鳥')+' Tamama，'+(condition==='sleep'?'正在睡覺':condition==='sick'?'生病中':'清醒'));if(condition==='dead')document.querySelector('#pet-description').textContent='已離世 · 陪伴紀錄已留存';else if(stage==='egg')document.querySelector('#pet-description').textContent='蛋 · 等待孵化';status.textContent=condition==='dead'?'謝謝你的陪伴，下一步可進入墓園或領養新蛋。':stage==='egg'?'新蛋正在孵化中：此頁有效時間累計 5 分鐘孵化；可用生命階段選單預覽。':condition==='sleep'?'牠閉上眼睛，正在休息。切回「清醒」就可以繼續照顧。':condition==='sick'?'生病中：牠不太舒服，需要休息與治療。陪玩暫停；餵食與清潔不會治好疾病。（展示）':stage==='baby'?'圓胖的幼鳥正在等你！可以餵食、清潔或陪牠玩。':'牠正在棲架上等你。試著餵牠一點種子吧！';}
document.querySelector('#stage').onchange=preview;document.querySelector('#condition').onchange=preview;
document.querySelector('#reset').onclick=()=>{if(mode!=='mock')return;sources.mock.reset();deathKey=null;state={...defaults};document.querySelector('#stage').value='adult';document.querySelector('#condition').value='awake';preview();update();};
function switchMode(){root.classList.remove('healing');mode=document.querySelector('#mode').value;action=null;resetToken++;bubble.hidden=true;root.classList.remove('is-sick');document.querySelector('#illness-notice').hidden=true;
for(const id of ['stage','condition','reset'])document.querySelector('#'+id).disabled=mode!=='mock';
const demo=mode==='mock';document.querySelector('#mode-label').textContent=demo?'✿ 展示模式':'◎ 裝置模式';document.querySelector('#source-label').textContent=demo?'模擬資料 · 不會影響 ESP32':'BLE 資料 · 尚未連接裝置';document.querySelector('#device-empty').hidden=demo;
document.querySelector('#pet-name').textContent=demo?sources.mock.pet.name:'等待裝置';document.querySelector('#pet-age').textContent=demo?'有效年齡：1 小時（展示）':'有效年齡：—';document.querySelector('#mode-footer').textContent=demo?'本頁互動只作用於展示小鳥。':'裝置模式以 ESP32 回報為準；本版只讀取，不發送照顧命令。';
document.querySelector('#last-received').hidden=demo;if(demo)preview();else renderDevice();update();busy(false);savePreferences();}
function renderDevice(){if(mode!=='ble')return;const snap=sources.ble.read(),p=snap.pet;const labels={disconnected:'未連線',selecting:'選擇裝置中',connecting:'連線中',waiting:'等待資料',ready:'裝置直連（唯讀）',cancelled:'已取消',error:'連線異常'};
 document.querySelector('#source-label').textContent='BLE 資料 · '+(snap.stale&&p?'離線／舊資料':labels[snap.connection]);
 document.querySelector('#ble-state').textContent=snap.message;
 document.querySelector('#last-received').textContent=p?'最後接收：'+new Date(snap.receivedAt).toLocaleString()+' · 裝置 ID：'+(p.deviceId||'舊韌體未提供'):'最後接收：—';
 document.querySelector('#pet-name').textContent=p?p.name:'等待裝置';
 document.querySelector('#pet-age').textContent=p?'有效年齡：'+p.ageSeconds+' 秒（裝置回報）':'有效年齡：—';
 document.querySelector('#pet-description').textContent=p?({egg:'蛋',baby:'幼鳥',adult:'成鳥'}[p.stage]+' · '+{healthy:'健康',sick:'生病',dead:'已離世'}[p.health]+' · '+(p.condition==='sleep'?'睡眠':'清醒')+(snap.stale?' · 舊資料':'')):'尚無硬體資料';
 const noArt=!p;document.querySelector('#device-empty').hidden=!noArt;
 document.querySelector('#device-empty').textContent=!p?snap.message:p.health==='dead'?'裝置回報：小鳥已離世': '裝置回報：蛋階段，等待孵化';
 if(p){stage=p.stage;condition=p.health==='dead'?'dead':p.condition;}syncLifecycle();root.classList.toggle('is-sick',p?.health==='sick');document.querySelector('#illness-notice').hidden=p?.health!=='sick';
 syncHealth(p?.health??'unknown');status.textContent=snap.message+(p&&snap.stale?'。目前顯示最後收到的舊資料，不代表即時狀態。':'')+' 真實照顧命令尚未開放。';
 canvas.setAttribute('aria-label',p?p.name+'，'+document.querySelector('#pet-description').textContent:'尚無硬體資料');update();busy(false);
}
sources.ble.subscribe(()=>{renderDevice();refreshConnectionButtons();const snap=sources.ble.read();if(snap.pet&&snap.connection==='ready')PetMemorials.remember(snap.pet,'ble',bleHistorySession);});
function refreshConnectionButtons(){const s=sources.ble.read();document.querySelector('#ble-state').textContent=s.message;document.querySelector('#ble-search').disabled=['selecting','connecting','waiting','ready'].includes(s.connection);document.querySelector('#ble-disconnect').disabled=['disconnected','cancelled','error'].includes(s.connection);}
document.querySelector('#ble-search').onclick=()=>{document.querySelector('#mode').value='ble';switchMode();sources.ble.connect();};
document.querySelector('#ble-disconnect').onclick=()=>sources.ble.disconnect();
document.querySelector('#mode').onchange=switchMode;
document.querySelector('#connection').onclick=()=>{window.checkBleReadiness();document.querySelector('#device-dialog').showModal();};
document.querySelector('#stage').value=stage;document.querySelector('#condition').value=condition;document.querySelector('#backdrop').value=backdrop;document.querySelector('#mode').value=mode;setNight(night);switchMode();PetLocalStore.report();requestAnimationFrame(draw);
})();





































