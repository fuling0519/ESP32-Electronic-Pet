/* Toy play in the garden (web only): fling the ball, the pet walks (or flies) after it, catches or picks it up and
 * brings it back. The session and its one-time reward go through DataManager; this file only draws and runs the throws.
 * Demo mode settles on the local pet. Device mode plays with the real pet over BLE: the ESP32 opens the session, needs a
 * heartbeat, and computes the reward from the reported score (PROTOCOL.md, Web 陪玩). */
(()=>{'use strict';
const P=window.ToyPhysics,scene=document.querySelector('.scene'),canvas=document.querySelector('#bird'),status=document.querySelector('#status');
const reduced=matchMedia('(prefers-reduced-motion: reduce)');
const hud=document.createElement('div');hud.className='toy-hud';hud.hidden=true;
const count=document.createElement('span'),exit=document.createElement('button');
exit.type='button';exit.textContent='結束陪玩';exit.title='未完成不結算';hud.append(count,exit);scene.append(hud);
const REACT_MS=180,CHEER_MS=800,PICKUP_MS=320,RETURN_MS=520,FINALE_MS=2400;
const TOTAL_THROWS=5;
let play=null,starting=false,lastActivity=null;
function homeFor(species,stage){return species===2?{x:190,y:177}:{x:172,y:stage==='adult'?199:195};}
function say(text){status.textContent=text;}
// Device mode counts throws here; demo mode counts them in the data layer's session.
function counts(){if(play?.device)return {throws:play.throws,catches:play.catches,total:TOTAL_THROWS};const g=petDataManager.gameState(performance.now());return g?.kind==='toy'?{throws:g.throws,catches:g.catches,total:g.totalThrows}:null;}
function showCount(){const c=counts();if(c)count.textContent='第 '+Math.min(c.throws+1,c.total)+'／'+c.total+' 球 · 接住 '+c.catches;}
// Device sessions (start, heartbeat, finish, cancel, early-end checks) live in device-session.js, shared with the chorus.
function begin(p,device){
 const now=performance.now(),species=window.PetArtStage?.species?.(p)||p.speciesId,stage=window.PetArtStage?.shown?.(p)||p.stage,home=homeFor(species,stage);
 play={species,stage,home,move:P.moves(species,stage),pet:{...home,facing:-1,lift:0},ball:{...P.HAND,z:0},state:'ready',since:now,last:now,caught:false,device,throws:0,catches:0};
 hud.hidden=false;showCount();say('按住球，往花園甩出去！（鍵盤：Enter 丟球、Esc 結束）');
}
function start(){
 if(play||starting||window.PetChorus?.active())return false;
 if(petDataManager.mode==='ble'){startDevice();return true;}
 try{petDataManager.startGame('toy',performance.now());}catch(e){say(e.message);return false;}
 begin(petDataManager.read().pet,null);return true;
}
async function startDevice(){
 starting=true;say('正在請電子雞準備陪玩……');let device;
 try{device=await window.PetDeviceSession.start('toy',text=>{if(play?.device===device)end(text);});}catch(e){starting=false;say(e.message);return;}
 starting=false;begin(device.pet,device);
}
function end(text){if(play?.device)window.PetDeviceSession.close(play.device);play=null;hud.hidden=true;if(text)say(text);document.dispatchEvent(new CustomEvent('pet-toy-end'));}
function setState(state,now){play.state=state;play.since=now;}
function launch(from,to,now){play.flight=P.flight(from,to);play.thrownAt=now;play.caught=false;setState('chase',now);say('去吧！');}
// The bird's walking hop, shared by its drawing and the ball it carries.
function hop(now,pose,s){return pose==='walk'?Math.abs(Math.sin(now/90))*2*s:0;}
// Where the ball sits (its bottom) when the pet holds it, from the pet's feet (pet.x,pet.y), facing, size and the pose
// actually drawn ('walk', 'stand', 'happy' or 'peck').
function mouth(pose,now=0){
 const {pet,species,stage}=play,s=P.depthScale(pet.y,play.home.y),f=pet.facing;
 // The bird's beak, measured on each sheet as fractions of its height: the walking sheet faces the way it goes, while
 // the mirrored standing/happy art ends up with its beak slightly on the other side. The ball (18 px, unscaled) hangs
 // with its top just under the beak's centre, so the beak still shows above it.
 if(species===1){
  const h=(stage==='adult'?132:106)*s;if(pose==='peck')return {x:pet.x+f*.3*h,y:pet.y-.02*h};
  const walk=pose==='walk',top=(walk?.585:.6)*h-1;
  return {x:pet.x+f*(walk?.23:-.05)*h,y:pet.y-hop(now,pose,s)-(top-18)};
 }
 // The baby wyvern's walking art turns its face the way it goes; its idle and happy art look straight ahead.
 if(stage==='baby')return {x:pet.x+(pose==='walk'?f*8*s:0),y:pet.y-30*s};
 return pet.lift>.05?{x:pet.x+f*42*s,y:pet.y-(35+22*pet.lift)*s-2*s}:{x:pet.x,y:pet.y-46*s};
}
function face(target){const dx=target.x-play.pet.x;if(Math.abs(dx)>.5)play.pet.facing=dx>0?1:-1;}
// Device mode: the ESP32 settles the session (device-session.js waits out its minimum time and retries lost writes).
function finishDevice(){
 const device=play.device,score={throws:play.throws,catches:play.catches};
 setState('settling',performance.now());say('正在請電子雞結算……');
 window.PetDeviceSession.finish(device,score).then(r=>{if(play?.device!==device)return;setState('finale',performance.now());count.textContent='完成！接住 '+score.catches+'／'+TOTAL_THROWS;
  say('玩得好開心！心情 +'+r.mood_gain+' · EXP +'+r.exp_gain+(r.levels_gained?' · 升級！':''));document.dispatchEvent(new CustomEvent('pet-game-complete',{detail:{levelsGained:r.levels_gained||0}}));},e=>{if(play?.device===device)end(e.message);});
}
function report(now){
 if(play.device){play.throws++;play.catches+=play.caught?1:0;if(play.throws>=TOTAL_THROWS){finishDevice();return;}play.ball={...P.HAND,z:0};setState('ready',now);showCount();say('再丟一次！');return;}
 let g;try{g=petDataManager.answerGame(play.caught?1:0,now);}catch(e){end(e.message);return;}
 if(g?.phase==='complete'){setState('finale',now);say('玩得好開心！心情 +'+g.reward.mood+' · EXP +'+g.reward.exp+(g.levelsGained?' · 升級！':''));document.dispatchEvent(new Event('pet-game-complete'));count.textContent='完成！接住 '+g.catches+'／'+g.totalThrows;return;}
 play.ball={...P.HAND,z:0};setState('ready',now);showCount();say('再丟一次！');
}
// Called every frame, even when the garden shows something else (growth, death, device mode), so a session the data
// layer cancelled never leaves the page stuck in play.
function check(now){
 syncDeviceActivity();
 if(!play||play.state==='finale')return !!play;
 if(play.device){
  const why=window.PetDeviceSession.check(play.device);if(why){end(why);return false;}
  return true;
 }
 const g=petDataManager.gameState(now);
 if(!g||g.kind!=='toy'||g.phase==='cancelled'){end('陪玩中斷，這次沒有結算。');return false;}
 return true;
}
function update(now){
 const dt=Math.max(0,Math.min(.05,(now-play.last)/1000)),{pet,move}=play;play.last=Math.max(play.last,now);
 if(!check(now))return false;
 const ms=now-play.since;
 // Only the adult wyvern leaves the ground: up while chasing or carrying, down at home.
 if(move.flies){const up=['chase','cheer','pickup'].includes(play.state)||play.state==='carry'&&!play.atHome;pet.lift=Math.max(0,Math.min(1,pet.lift+(up?4:-4)*dt));}
 if(play.state==='ready'||play.state==='drag')face(play.ball);
 else if(play.state==='chase'){
  const t=(now-play.thrownAt)/1000,b=play.flight.at(t);play.ball=b;
  if(t*1000<REACT_MS){face(b);return true;}
  const target=b.phase==='air'?play.flight.to:b,next=P.step(pet,target,move.speed,dt);
  face(target);pet.x=next.x;pet.y=next.y;play.moving=!next.arrived;
  if(b.phase==='air'&&t>=play.flight.air*.25&&P.reaches(pet,b,move.reach)){play.caught=true;setState('cheer',now);say('接住了！好厲害！');}
  else if(b.phase!=='air'&&P.reaches(pet,{...b,z:0},Infinity)){setState('pickup',now);say('撿到了～');}
 }else if((play.state==='cheer'&&ms>=CHEER_MS)||(play.state==='pickup'&&ms>=PICKUP_MS)){play.atHome=false;setState('carry',now);}
 else if(play.state==='carry'){
  const next=P.step(pet,play.home,move.speed*.8,dt);face(play.home);pet.x=next.x;pet.y=next.y;play.moving=!next.arrived;play.atHome=next.arrived;
  if(next.arrived&&pet.lift<=.05){play.from=mouth(play.caught?'happy':'stand');face(P.HAND);setState('return',now);}
 }else if(play.state==='return'&&ms>=RETURN_MS){report(now);if(!play)return false;}
 else if(play.state==='finale'&&ms>=FINALE_MS){end('');return false;}
 if(play.state!=='chase'&&play.state!=='carry')play.moving=false;
 return true;
}
// A plain pink ball shaded like a sphere: light from the upper left, stepped into a pixel-art colour ramp with a soft
// checker between neighbouring tones, a small highlight, a bounce light on the lower rim and a dark outline. No stripes.
// Drawn once into an 18x18 sprite; its bottom touches (x, y - z).
const BALL_SIZE=18,BALL_RAMP=['#a04f6a','#c76a85','#e2869d','#f2a2b5','#fbc6d2','#fff4f6'];
const ballSprite=(()=>{
 const c=document.createElement('canvas');c.width=c.height=BALL_SIZE;const g=c.getContext('2d'),r=BALL_SIZE/2,L=[-.5,-.6,.62],len=Math.hypot(...L);
 for(let y=0;y<BALL_SIZE;y++)for(let x=0;x<BALL_SIZE;x++){
  const dx=(x+.5-r)/r,dy=(y+.5-r)/r,d=Math.hypot(dx,dy);if(d>1)continue;
  if(d>1-1.4/r){g.fillStyle='#6e3f56';g.fillRect(x,y,1,1);continue;}
  const nz=Math.sqrt(1-d*d),light=(dx*L[0]+dy*L[1]+nz*L[2])/len,rim=Math.max(0,dy)*Math.max(0,dx+.2)*(1-nz);
  // Map light to the ramp in solid bands; only a thin seam at each band edge is checkered, so the surface reads
  // smooth instead of as a grid. The rim adds a faint reflected light from the grass.
  const raw=Math.max(0,Math.min(4.99,(light+.35)*3.6+rim*1.6)),frac=raw%1;
  let level=Math.floor(raw);if(frac>.44&&frac<.56)level+=(x+y)%2;else if(frac>=.56)level+=1;
  if(light>.9&&dx<-.15&&dy<-.15)level=5;
  g.fillStyle=BALL_RAMP[Math.max(0,Math.min(5,level))];g.fillRect(x,y,1,1);
 }
 return c;
})();
function drawShadow(ctx,x,y,rx,ry){ctx.save();ctx.fillStyle='rgba(69,48,33,.18)';ctx.beginPath();ctx.ellipse(x,y,rx,ry,0,0,Math.PI*2);ctx.fill();ctx.restore();}
function drawBall(ctx,b,shadow=true){
 if(shadow){const k=1-Math.min(b.z,60)/120;drawShadow(ctx,b.x,b.y,9*k,3*k);}
 ctx.save();ctx.imageSmoothingEnabled=false;ctx.drawImage(ballSprite,Math.round(b.x-BALL_SIZE/2),Math.round(b.y-b.z-BALL_SIZE));ctx.restore();
}
function mirrored(ctx,x,flip,paint){ctx.save();if(flip){ctx.translate(x,0);ctx.scale(-1,1);ctx.translate(-x,0);}const r=paint();ctx.restore();return r;}
function drawPet(ctx,now,p,pose){
 const {pet,species,stage}=play,s=P.depthScale(pet.y,play.home.y),f=pet.facing;
 const model={...p,stage,healthState:'healthy',sleepMode:'awake',mood:100,satiety:100,cleanliness:100,sadReasons:{mood:false,satiety:false,cleanliness:false}};
 if(species===1){
  const h=(stage==='adult'?132:106)*s,feet=pet.y-hop(now,pose,s);
  drawShadow(ctx,pet.x,pet.y,(stage==='adult'?30:24)*s,4*s);
  // The bird art faces left; turning right mirrors it around its body.
  if(pose==='walk'||pose==='peck')return window.WhiteBirdActivity?.pose(ctx,stage,pose==='peck'?3:Math.floor(now/150)%2,pet.x,feet,3*s,f>0);
  return mirrored(ctx,pet.x,f>0,()=>WhiteBirdView.draw(ctx,model,now,pose==='happy'?'happy':null,0,false,pet.x+.053*h,feet-h*.47,3*s));
 }
 const flying=stage==='adult'&&pet.lift>.05;
 drawShadow(ctx,pet.x,pet.y,(stage==='adult'?34:26)*s*(flying?.7:1),4*s*(flying?.7:1));
 if(flying)return WyvernAdultView.paint(ctx,'flight',now,false,{x:pet.x,y:pet.y-(35+22*pet.lift)*s,size:s,facing:f,stage});
 const state=pose==='happy'?'happy':pose==='walk'&&stage==='baby'?'walk':'idle';
 return WyvernAdultView.paint(ctx,state,now,false,{x:pet.x,y:pet.y,size:s,facing:f,stage});
}
function draw(ctx,now,p){
 if(!play)return false;
 if(!update(now))return false;
 const st=play.state,pose=play.moving?'walk':st==='pickup'&&play.species===1&&now-play.since<PICKUP_MS*.7?'peck':['cheer','return','finale','settling'].includes(st)&&(st!=='return'||play.caught)?'happy':'stand';
 const held=['cheer','carry'].includes(st)||st==='pickup'&&now-play.since>=PICKUP_MS*.5
 // Return: the pet nudges the ball back to the player's hand in a small arc.
 let ball=play.ball;
 if(st==='return'){const u=Math.min(1,(now-play.since)/RETURN_MS);ball={x:play.from.x+(P.HAND.x-play.from.x)*u,y:play.from.y+(P.HAND.y-play.from.y)*u,z:Math.sin(u*Math.PI)*18};}
 // The returning ball leaves from the mouth, so it stays in front of the pet instead of vanishing behind its body.
 const behind=!held&&st!=='return'&&ball.y<play.pet.y;
 if(behind&&st!=='finale')drawBall(ctx,ball);
 drawPet(ctx,now,p,pose);
 if(held){const m=mouth(pose,now);drawBall(ctx,{x:m.x,y:m.y,z:0},false);}
 else if(!behind&&st!=='finale')drawBall(ctx,ball);
 if(st==='cheer'||st==='finale'){const m=mouth(pose,now);window.PetEffects?.hearts(ctx,now-play.since,{x:m.x,y:m.y-26,spread:44,until:st==='finale'?FINALE_MS-600:500,reduced:reduced.matches});}
 return true;
}
function point(e){const r=canvas.getBoundingClientRect();return {x:(e.clientX-r.left)*384/r.width,y:(e.clientY-r.top)*256/r.height};}
let samples=[];
canvas.addEventListener('pointerdown',e=>{
 if(!play||play.state!=='ready'||e.button!==0)return;const pt=point(e);
 if(Math.hypot(pt.x-play.ball.x,pt.y-(play.ball.y-9))>30)return;
 e.preventDefault();e.stopImmediatePropagation();try{canvas.setPointerCapture(e.pointerId);}catch{}
 play.dragId=e.pointerId;samples=[{...pt,t:performance.now()}];setState('drag',performance.now());say('甩出去！');
});
canvas.addEventListener('pointermove',e=>{
 if(play?.state==='ready'){const pt=point(e);canvas.style.cursor=Math.hypot(pt.x-play.ball.x,pt.y-(play.ball.y-9))<=30?'grab':'';}
 if(play?.state!=='drag'||e.pointerId!==play.dragId)return;
 const pt=point(e),now=performance.now();canvas.style.cursor='grabbing';
 play.ball={x:Math.max(8,Math.min(376,pt.x)),y:Math.max(P.GROUND.top,Math.min(250,pt.y)),z:0};
 samples.push({...play.ball,t:now});samples=samples.filter(q=>now-q.t<=120);
});
function release(e){
 if(play?.state!=='drag'||e.pointerId!==play.dragId)return;canvas.style.cursor='';
 const now=performance.now(),first=samples[0],last=samples.at(-1),dt=Math.max(.016,(last.t-first.t)/1000);
 const target=samples.length>1?P.landing(last,{x:(last.x-first.x)/dt,y:(last.y-first.y)/dt}):null;
 if(!target){play.ball={...P.HAND,z:0};setState('ready',now);say('再用力一點，往花園甩出去！');return;}
 launch({x:last.x,y:last.y},target,now);
}
canvas.addEventListener('pointerup',release);
canvas.addEventListener('pointercancel',e=>{if(play?.state==='drag'&&e.pointerId===play.dragId){play.ball={...P.HAND,z:0};setState('ready',performance.now());}});
document.addEventListener('keydown',e=>{
 if(!play||e.isComposing||['INPUT','SELECT','TEXTAREA'].includes(e.target?.tagName))return;
 if(e.key==='Escape'){e.preventDefault();stop();return;}
 if(play.state==='ready'&&(e.key==='Enter'||e.key===' ')&&e.target?.tagName!=='BUTTON'){
  e.preventDefault();throwTo({x:120+Math.random()*200,y:150+Math.random()*55});
 }
});
function stop(){if(!play)return;if(play.device)window.PetDeviceSession.close(play.device,true);else petDataManager.cancelGame();end('已結束陪玩，未完成不結算。');}
// While someone plays on the ESP32 itself, the garden says so once (and again when it is over).
function syncDeviceActivity(){
 const activity=petDataManager.mode==='ble'?petDataManager.view('ble').activity:null;
 if(activity===lastActivity)return;
 if(activity==='device_game')say('牠正在電子雞上陪你玩～這段時間網頁先等等。');
 else if(lastActivity==='device_game'&&activity)say('電子雞那邊玩完了！');
 lastActivity=activity;
}
exit.onclick=stop;
// throwTo is for keyboard use and demo checks: the same launch a fling makes, from the hand.
function throwTo(target,now=performance.now()){if(play?.state!=='ready')return false;launch(P.HAND,P.clampGround(target),now);return true;}
// Picker icon: the same shaded ball in mid-flight over its shadow, with a dotted arc behind it.
function icon(canvas){const g=canvas.getContext('2d');g.clearRect(0,0,canvas.width,canvas.height);
 g.fillStyle='#d9b9c4';for(const [x,y] of [[6,30],[10,24],[15,19]])g.fillRect(x,y,2,2);
 drawBall(g,{x:31,y:36,z:9});}
window.PetToyPlay={start,stop,draw,check,throwTo,icon,active:()=>!!play||starting};
})();
