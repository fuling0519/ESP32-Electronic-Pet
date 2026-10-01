/* UI-only demo. No BLE, API, account or device-authoritative state. */
(()=>{'use strict';
const root=document.querySelector('#aviary'),canvas=document.querySelector('#bird'),ctx=canvas.getContext('2d'),status=document.querySelector('#status'),bubble=document.querySelector('#bubble');
const defaults={satiety:60,mood:55,cleanliness:45};let state={...defaults},action=null,actionStart=0,night=false,resetToken=0;
const reduced=matchMedia('(prefers-reduced-motion: reduce)');
let stage='adult',condition='awake',backdrop='simple';
const palette={P:'#282523',W:'#fffaf0',G:'#896448',L:'#896448'};
function update(){for(const k of Object.keys(defaults)){document.querySelector('#'+k).value=state[k];document.querySelector('#'+k+'-value').textContent=state[k];}}
function busy(value){document.querySelectorAll('[data-action]').forEach(b=>b.disabled=value||condition==='sleep'||(condition==='sick'&&b.dataset.action==='play'));}
function speak(text){bubble.textContent=text;bubble.hidden=false;}
function sickMessage(){speak(stage==='baby'?'啾…我不舒服，想休息一下…':'有點難受…可以照顧我嗎？');}
function begin(kind){if(action||condition==='sleep'||(condition==='sick'&&kind==='play'))return;const key=kind==='feed'?'satiety':kind==='clean'?'cleanliness':'mood';if(state[key]===100){speak(kind==='feed'?'吃飽囉！':kind==='clean'?'羽毛已經很乾淨了！':'今天心情超好！');status.textContent='展示'+({satiety:'飽食度',cleanliness:'清潔度',mood:'心情'})[key]+'已達 100。';return;}action=kind;actionStart=performance.now();busy(true);speak(kind==='feed'?'開飯啦！':kind==='clean'?'整理一下羽毛～':'一起玩吧！');status.textContent=kind==='feed'?'牠正在啄食種子……':kind==='clean'?'牠正在整理羽毛，髒污逐漸消失……':'牠開心地跳躍、拍拍翅膀……';}
function finish(){const kind=action,key=kind==='feed'?'satiety':kind==='clean'?'cleanliness':'mood',gain=kind==='feed'?20:kind==='clean'?30:15,actual=Math.min(gain,100-state[key]);state[key]+=actual;action=null;busy(false);update();speak(kind==='feed'?'謝謝你，好好吃！':kind==='clean'?'乾乾淨淨！':'再一起玩！');status.textContent='展示'+({satiety:'飽食度',cleanliness:'清潔度',mood:'心情'})[key]+' +'+actual+'。';const token=++resetToken;setTimeout(()=>{if(token===resetToken&&!action){if(condition==='sick')sickMessage();else bubble.hidden=true;}},1800);}
function block(x,y,w,h,color){ctx.fillStyle=color;ctx.fillRect(Math.round(x),Math.round(y),w,h);}
function heart(x,y){const rows=['.PP.PP.','PPPPPPP','PPPPPPP','.PPPPP.','..PPP..','...P...'];rows.forEach((r,j)=>[...r].forEach((c,i)=>{if(c==='P')block(x+i,y+j,1,1,'#e8a2b0');}));}
function draw(now){ctx.clearRect(0,0,384,256);const elapsed=action?(now-actionStart)/1000:0,moving=!reduced.matches;let x=105,y=87,angle=0;
if(moving){y+=Math.floor(Math.sin(now/(condition==='awake'?650:1500)));if(condition==='sick'){x+=Math.sin(now/95)*2;angle=Math.sin(now/180)*.035;}if(action==='feed')angle=Math.sin(elapsed*13)*.12;if(action==='play'){y-=Math.abs(Math.sin(elapsed*5))*20;x+=Math.sin(elapsed*4)*8;}if(action==='clean')angle=Math.sin(elapsed*12)*.07;}
const walking=stage==='baby'&&condition==='awake'&&!action&&moving;const walkPhase=(now%12000)/1000;const stepping=walking&&(walkPhase<4||(walkPhase>=6&&walkPhase<10));
if(stage==='baby'){x+=5;y+=8;if(walking){x+=walkPhase<4?walkPhase*6:walkPhase<6?24:walkPhase<10?24-(walkPhase-6)*6:0;if(stepping)y-=Math.floor(Math.abs(Math.sin(now/160))*2);}}
ctx.save();ctx.translate(Math.round(x+47),Math.round(y+69));ctx.rotate(angle);ctx.translate(-47,-69);const blink=moving&&(now%4200>4000);
// Round white-headed pixel bird, with separate front and side poses.
const front=action==='feed'?false:!moving||condition!=='awake'||now%14000<7000;
const baby=stage==='baby',blinkEye=blink||condition==='sleep';
const grid=4,cx=baby||front?16:18,bodyY=baby?12:12,rx=baby?11:(front?13:13),ry=baby?8:10;
const headX=baby||front?17:23,headY=baby?6:4,headRadius=baby?9:8;

function inside(i,j){const body=((i-cx)/rx)**2+((j-bodyY)/ry)**2<=1,head=((i-headX)/headRadius)**2+((j-headY)/(baby?7:7))**2<=1;const neck=!baby&&!front&&((i-23)/5)**2+((j-10)/6)**2<=1;return body||head||neck;}
// Side tail sits behind the body. The chick only has a small rear tip.
if(!front&&!baby){for(let i=0;i<7;i++){block(-16+i*5,77-i*2,8,5,palette.P);block(-14+i*5,78-i*2,6,2,palette.G);}}else if(baby&&!front){block(18,63,12,5,palette.G);block(18,63,5,3,palette.P);}
for(let j=0;j<22;j++)for(let i=0;i<31;i++){if(!inside(i,j))continue;const edge=!inside(i-1,j)||!inside(i+1,j)||!inside(i,j-1)||!inside(i,j+1);const color=edge?palette.P:(!baby&&j>bodyY+6)?palette.G:palette.W;block(i*grid,j*grid,grid,grid,color);}
const wingFlap=!baby&&condition==='awake'&&moving&&(action==='play'||(!action&&now%6500<1700));
function wing(wx,wy,mirror){ctx.save();ctx.translate(wx,wy);if(wingFlap)ctx.rotate(Math.sin(now/110)*.7*mirror);for(let j=0;j<9;j++)for(let i=0;i<5;i++){if(((i-2)/2.5)**2+((j-4)/4.5)**2<=1)block((i-2)*3,j*3,3,3,palette.G);}ctx.restore();}
if(!baby){if(front){wing(17,36,-1);wing(111,36,1);}else{ctx.save();ctx.translate(44,38);if(wingFlap)ctx.rotate(Math.sin(now/110)*.7);for(let wy=0;wy<9;wy++)for(let wx=0;wx<11;wx++){if(((wx-5)/6)**2+((wy-4)/5)**2<=1)block((wx-5)*4,wy*3,4,3,wy>6?palette.P:palette.G);}block(-13,23,24,3,palette.W);block(-17,28,25,3,palette.P);ctx.restore();}}else{if(front){wing(23,38,-1);wing(105,38,1);}else wing(44,36,-1);}
function eye(ex,ey){if(blinkEye){block(ex,ey+3,9,3,palette.P);}else{block(ex,ey,7,9,palette.P);block(ex+1,ey+1,2,2,palette.W);}}
if(front){eye(49,20);eye(79,20);block(63,31,6,4,palette.G);block(65,35,3,2,palette.G);}else{eye(baby?87:104,baby?20:14);block(baby?103:120,baby?31:25,9,4,palette.G);block(baby?112:129,baby?32:26,3,2,palette.G);}
const step=stepping?Math.floor(now/180)%2:0,footY=baby?80:88;
block(56,footY-7+step*3,3,7-step*3,palette.L);block(76,footY-7+(stepping?1-step:0)*3,3,7-(stepping?1-step:0)*3,palette.L);block(53+step*2,footY,9,2,palette.L);block(73-step*2,footY,9,2,palette.L);
const dirt=Math.ceil((100-state.cleanliness)/25)-1;for(let i=0;i<Math.max(0,dirt);i++){if(action==='clean'&&elapsed>(i+1)*.65)continue;block(55+i*10,52+i*5,5,4,palette.G);}ctx.restore();
if(condition==='sleep'){ctx.fillStyle=palette.P;ctx.font='bold 12px monospace';for(let i=0;i<3;i++)ctx.fillText('z',x+90+i*12,y+12-i*10-(moving?(now/180+i*4)%9:0));}
if(action==='feed'){block(213,167,24,3,'#ab829c');block(216,170,18,5,'#e9b5ae');for(let i=0;i<5;i++)block(217+i*4,163+(i%2)*2,2,2,'#edc176');}
if(action==='clean')for(let i=0;i<8;i++){const sy=110+(i*13+elapsed*22)%54;block(134+(i*17)%72,sy,3,3,'#e9fbec');}
if(action==='play')for(let i=0;i<3;i++)heart(145+i*24,82-(moving?(elapsed*17+i*10)%30:0));
if(night)for(let i=0;i<14;i++){const a=moving?.4+.6*Math.abs(Math.sin(now/800+i)):1;ctx.globalAlpha=a;block(132+(i*29)%221,8+(i*13)%62,2,2,'#fff2bb');}ctx.globalAlpha=1;
if(action&&elapsed>=2.5)finish();requestAnimationFrame(draw);}
document.querySelectorAll('[data-action]').forEach(b=>b.addEventListener('click',()=>begin(b.dataset.action)));
function setNight(value){night=value;root.classList.toggle('night',night);applyBackdrop();document.querySelector('#day').setAttribute('aria-pressed',String(!night));document.querySelector('#night').setAttribute('aria-pressed',String(night));root.querySelector('h1').textContent=night?'小鳥的月光花園':'小鳥的午後花園';}
function applyBackdrop(){root.classList.toggle('simple-background',backdrop==='simple');document.querySelector('#background').src=backdrop==='simple'?'assets/garden-simple.png':'assets/garden-'+(night?'night':'day')+'.png';}
document.querySelector('#backdrop').onchange=()=>{backdrop=document.querySelector('#backdrop').value;applyBackdrop();};applyBackdrop();
document.querySelector('#day').onclick=()=>setNight(false);document.querySelector('#night').onclick=()=>setNight(true);
function preview(){action=null;resetToken++;stage=document.querySelector('#stage').value;condition=document.querySelector('#condition').value;busy(false);bubble.hidden=true;root.classList.toggle('is-sick',condition==='sick');document.querySelector('#illness-notice').hidden=condition!=='sick';if(condition==='sick')sickMessage();document.querySelector('#pet-description').textContent=(stage==='baby'?'幼鳥':'成鳥')+' · '+(condition==='sick'?'生病':'健康')+' · '+(condition==='sleep'?'一般睡眠':'清醒');canvas.setAttribute('aria-label',(stage==='baby'?'幼鳥':'成鳥')+' Tamama，'+(condition==='sleep'?'正在睡覺':condition==='sick'?'生病中':'清醒'));status.textContent=condition==='sleep'?'牠閉上眼睛，正在休息。切回「清醒」就可以繼續照顧。':condition==='sick'?'生病中：牠不太舒服，需要休息與治療。陪玩暫停；餵食與清潔不會治好疾病。（展示）':stage==='baby'?'毛茸茸的幼鳥正在等你！可以餵食、清潔或陪牠玩。':'牠正在棲架上等你。試著餵牠一點種子吧！';}
document.querySelector('#stage').onchange=preview;document.querySelector('#condition').onchange=preview;
document.querySelector('#reset').onclick=()=>{state={...defaults};document.querySelector('#stage').value='adult';document.querySelector('#condition').value='awake';preview();update();};
document.querySelector('#connection').onclick=()=>document.querySelector('#device-dialog').showModal();update();requestAnimationFrame(draw);
})();












