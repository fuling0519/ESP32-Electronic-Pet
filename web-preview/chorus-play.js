/* Garden chorus (web only): four flowers in the foreground are the keys. Each round the pet sings a short tune,
 * each note lighting its flower; the player sings it back by tapping the flowers (or keys 1-4). Three rounds of
 * 3, 4 and 5 notes, no time pressure. Demo mode settles through DataManager; device mode plays with the real pet
 * over BLE (device-session.js): the ESP32 opens the session and computes the reward from the tunes sung right. */
(()=>{'use strict';
const scene=document.querySelector('.scene'),canvas=document.querySelector('#bird'),status=document.querySelector('#status');
const reduced=matchMedia('(prefers-reduced-motion: reduce)');
const hud=document.createElement('div');hud.className='toy-hud';hud.hidden=true;
const count=document.createElement('span'),exit=document.createElement('button');
exit.type='button';exit.textContent='結束陪玩';exit.title='未完成不結算';hud.append(count,exit);scene.append(hud);
// Do Re Mi Sol, the same pitches as the firmware's memory notes; each flower has its own colour ramp (dark to light).
const FLOWERS=[
 {x:110,freq:523,name:'Do',ramp:['#9c4466','#c45f82','#e07f9b','#f2a2b8','#fcc9d6','#fff2f6']},
 {x:162,freq:587,name:'Re',ramp:['#a06a17','#cc8f26','#e8b43f','#f5d36c','#fde9a6','#fff9e2']},
 {x:214,freq:659,name:'Mi',ramp:['#3b5a99','#567bc0','#7a9ede','#a3c0ef','#cfe0fb','#f2f7ff']},
 {x:266,freq:784,name:'Sol',ramp:['#64458f','#8562b5','#a582d2','#c3a6e7','#e1d1f7','#f8f3ff']}];
const BASE_Y=252,ROUND_NOTES=[3,4,5],INTRO_MS=1300,NOTE_ON=430,NOTE_GAP=230,LIGHT_MS=320,RESULT_MS=1500,FINALE_MS=2600;
const TOTAL_ROUNDS=3;
let play=null,starting=false;
function say(text){status.textContent=text;}
// The pet stands a little further back than on Home, so the flower row in front never covers its feet.
function homeFor(species,stage){return species===2?{x:190,y:160}:{x:172,y:stage==='adult'?180:176};}

// --- Pixel art, drawn once per colour and state -------------------------------------------------------------
// One sprite pixel is one canvas unit, the same grain as the toy ball; S scales the flower's geometry, not its pixels.
const S=1.5,W=Math.round(22*S),H=Math.round(34*S),CX=W/2,CY=10*S,BLOOM=Math.round(20*S);
function petals(){return [0,1,2,3,4].map(k=>{const a=-Math.PI/2+k*2*Math.PI/5;return {x:CX+Math.cos(a)*5.2*S,y:CY+Math.sin(a)*5.2*S};});}
// Shade one disc pixel like the ball: light from the upper left, stepped into the ramp with a thin checker seam.
function shade(dx,dy,x,y,bias=0){const d=Math.min(1,Math.hypot(dx,dy)),nz=Math.sqrt(1-d*d),L=[-.5,-.6,.62],len=Math.hypot(...L);
 const light=(dx*L[0]+dy*L[1]+nz*L[2])/len,raw=Math.max(0,Math.min(4.99,(light+.35)*3.4+bias)),frac=raw%1;
 let level=Math.floor(raw);if(frac>.44&&frac<.56)level+=(x+y)%2;else if(frac>=.56)level+=1;return Math.min(5,level);}
function flowerSprite(ramp,lit){
 const c=document.createElement('canvas');c.width=W;c.height=H;const g=c.getContext('2d'),P=petals(),colour=[];
 const set=(x,y,v)=>{colour[y*W+x]=v;};
 // Stem and two leaves first; the bloom is painted over them.
 const greens=['#2f6b34','#3f8a40','#58a64f','#7cc266','#a7dc8a'],stem=Math.floor(CX)-1;
 for(let y=Math.round(15*S);y<H;y++)set(stem,y,greens[1]),set(stem+1,y,greens[2]),set(stem+2,y,greens[2]);
 for(const [lx,ly] of [[CX-4.5*S,25*S],[CX+4.5*S,21*S]])for(let y=0;y<H;y++)for(let x=0;x<W;x++){
  const dx=(x+.5-lx)/(4.2*S),dy=(y+.5-ly)/(2.2*S);if(dx*dx+dy*dy<=1)set(x,y,greens[Math.max(0,Math.min(4,shade(dx*.8,dy,x,y)-1))]);}
 const bias=lit?1:0,heart=2.7*S,petal=4.6*S;
 for(let y=0;y<BLOOM;y++)for(let x=0;x<W;x++){
  const px=x+.5,py=y+.5,dc=Math.hypot(px-CX,py-CY);
  if(dc<=heart){set(x,y,['#a8641c','#d18a2b','#efb640','#f8d670','#fff0b0','#fffbe8'][Math.min(5,shade((px-CX)/heart,(py-CY)/heart,x,y,bias))]);continue;}
  let best=null,bd=99;for(const p of P){const d=Math.hypot(px-p.x,py-p.y);if(d<bd){bd=d;best=p;}}
  if(bd<=petal)set(x,y,ramp[shade((px-best.x)/petal,(py-best.y)/petal,x,y,bias)]);
 }
 // A dark outline around the bloom keeps it readable on the grass; a lit flower also gets a pale halo.
 const inBloom=(x,y)=>x>=0&&y>=0&&x<W&&y<BLOOM&&colour[y*W+x]&&!greens.includes(colour[y*W+x]);
 const outline=new Set();for(let y=0;y<=BLOOM;y++)for(let x=0;x<W;x++)if(!inBloom(x,y)&&[[1,0],[-1,0],[0,1],[0,-1]].some(([a,b])=>inBloom(x+a,y+b)))outline.add(y*W+x);
 if(lit)for(let y=0;y<=BLOOM+1;y++)for(let x=0;x<W;x++)if(!inBloom(x,y)&&!outline.has(y*W+x)&&[[2,0],[-2,0],[0,2],[0,-2],[1,1],[-1,-1],[1,-1],[-1,1]].some(([a,b])=>inBloom(x+a,y+b))){g.fillStyle='rgba(255,255,240,.6)';g.fillRect(x,y,1,1);}
 for(let i=0;i<colour.length;i++)if(colour[i]){g.fillStyle=colour[i];g.fillRect(i%W,Math.floor(i/W),1,1);}
 g.fillStyle=ramp[0];for(const i of outline)g.fillRect(i%W,Math.floor(i/W),1,1);
 return c;
}
const sprites=FLOWERS.map(f=>({idle:flowerSprite(f.ramp,false),lit:flowerSprite(f.ramp,true)}));
// An eighth note in the flower's colour (2x2 units per pixel, like the hearts), floating out of the pet and the flowers.
const NOTE=['...##','...#.#','...#..#','...#...','...#...','.###...','####...','.##....'];
function drawNote(ctx,x,y,colour,alpha){ctx.save();ctx.globalAlpha=alpha;for(let r=0;r<NOTE.length;r++)for(let c=0;c<NOTE[r].length;c++)if(NOTE[r][c]==='#'){
 const px=Math.round(x+c*2),py=Math.round(y+r*2);ctx.fillStyle='#4a3348';ctx.fillRect(px,py+2,2,2);ctx.fillStyle=colour;ctx.fillRect(px,py,2,2);}ctx.restore();}

// --- Game flow ----------------------------------------------------------------------------------------------
// Device mode counts rounds here; demo mode counts them in the data layer's session.
function counts(){if(play?.device)return {played:play.played,correct:play.correct,totalRounds:TOTAL_ROUNDS};const g=petDataManager.gameState(performance.now());return g?.kind==='chorus'?g:null;}
function showCount(){const g=counts();if(g)count.textContent='第 '+Math.min(g.played+1,g.totalRounds)+'／'+g.totalRounds+' 首 · 唱對 '+g.correct;}
function tune(length){const notes=[];for(let i=0;i<length;i++)notes.push(crypto.getRandomValues(new Uint32Array(1))[0]%FLOWERS.length);return notes;}
function begin(p,device){
 const now=performance.now(),species=window.PetArtStage?.species?.(p)||p.speciesId,stage=window.PetArtStage?.shown?.(p)||p.stage;
 play={species,stage,home:homeFor(species,stage),state:'intro',since:now,round:0,tune:[],index:0,lights:[],notes:[],mood:null,sung:-1,device,played:0,correct:0};
 hud.hidden=false;showCount();say('聽牠唱，再點花朵唱回去！（鍵盤 1～4，Esc 結束）');
}
function start(){
 if(play||starting||window.PetToyPlay?.active())return false;
 if(petDataManager.mode==='ble'){startDevice();return true;}
 try{petDataManager.startGame('chorus',performance.now());}catch(e){say(e.message);return false;}
 begin(petDataManager.read().pet,null);return true;
}
async function startDevice(){
 starting=true;say('正在請電子雞準備合唱……');let device;
 try{device=await window.PetDeviceSession.start('chorus',text=>{if(play?.device===device)end(text);});}catch(e){starting=false;say(e.message);return;}
 starting=false;begin(device.pet,device);
}
function end(text){if(play?.device)window.PetDeviceSession.close(play.device);play=null;hud.hidden=true;if(text)say(text);}
function setState(state,now){play.state=state;play.since=now;}
function light(i,now,fromPet){
 const f=FLOWERS[i];play.lights.push({i,until:now+LIGHT_MS});window.PetWebSound?.note?.(f.freq,fromPet?300:200);
 // Notes float from the pet's head while it sings, and up from the flower when the player taps.
 const from=fromPet?head():{x:f.x-7,y:BASE_Y-H-4};play.notes.push({x:from.x,y:from.y,tx:fromPet?f.x-7:f.x-7+(i%2?10:-10),ty:fromPet?BASE_Y-H-14:BASE_Y-H-40,colour:f.ramp[2],at:now});
}
function beginRound(now){play.tune=tune(ROUND_NOTES[play.round]);play.index=0;play.sung=-1;setState('sing',now);say('第 '+(play.round+1)+' 首：仔細聽～');}
function tap(i,now){
 if(play?.state!=='listen')return;
 light(i,now,false);
 if(i!==play.tune[play.index]){play.ok=false;play.mood='shock';play.lights.push({i:play.tune[play.index],until:now+RESULT_MS,hint:true});setState('result',now);say('啊，不是這朵～應該是亮起來的那朵。');return;}
 if(++play.index>=play.tune.length){play.ok=true;play.mood='happy';setState('result',now);say('唱對了！好好聽～');}
}
function finishDevice(){
 const device=play.device,score={correct:play.correct,total:TOTAL_ROUNDS};
 setState('settling',performance.now());play.mood=null;say('正在請電子雞結算……');
 window.PetDeviceSession.finish(device,score).then(r=>{if(play?.device!==device)return;setState('finale',performance.now());play.mood='happy';count.textContent='完成！唱對 '+score.correct+'／'+TOTAL_ROUNDS;
  say('一起唱得好開心！心情 +'+r.mood_gain+' · EXP +'+r.exp_gain+(r.levels_gained?' · 升級！':''));},e=>{if(play?.device===device)end(e.message);});
}
function report(now){
 if(play.device){play.played++;play.correct+=play.ok?1:0;play.mood=null;if(play.played>=TOTAL_ROUNDS){finishDevice();return;}play.round++;showCount();beginRound(now);return;}
 let g;try{g=petDataManager.answerGame(play.ok?1:0,now);}catch(e){end(e.message);return;}
 play.mood=null;
 if(g?.phase==='complete'){setState('finale',now);play.mood='happy';count.textContent='完成！唱對 '+g.correct+'／'+g.totalRounds;
  say('一起唱得好開心！心情 +'+g.reward.mood+' · EXP +'+g.reward.exp+(g.levelsGained?' · 升級！':''));document.dispatchEvent(new Event('pet-game-complete'));return;}
 play.round++;showCount();beginRound(now);
}
// Called every frame (also while the garden shows something else) so a cancelled session never leaves the page stuck.
function check(now){
 if(!play||play.state==='finale')return !!play;
 if(play.device){const why=window.PetDeviceSession.check(play.device);if(why){end(why);return false;}return true;}
 const g=petDataManager.gameState(now);
 if(petDataManager.mode!=='mock'||!g||g.kind!=='chorus'||g.phase==='cancelled'){end('合唱中斷，這次沒有結算。');return false;}
 return true;
}
function update(now){
 if(!check(now))return false;
 const ms=now-play.since;
 if(play.state==='intro'&&ms>=INTRO_MS)beginRound(now);
 else if(play.state==='sing'){
  const k=Math.floor(ms/(NOTE_ON+NOTE_GAP));
  if(k>play.sung&&k<play.tune.length&&ms%(NOTE_ON+NOTE_GAP)<NOTE_ON){play.sung=k;light(play.tune[k],now,true);}
  if(ms>=play.tune.length*(NOTE_ON+NOTE_GAP)+200){setState('listen',now);say('換你唱囉！依序點 '+play.tune.length+' 朵花。');}
 }else if(play.state==='result'&&ms>=RESULT_MS){report(now);if(!play)return false;}
 else if(play.state==='finale'&&ms>=FINALE_MS){end('');return false;}
 play.lights=play.lights.filter(l=>l.until>now);play.notes=play.notes.filter(n=>now-n.at<900);
 return true;
}
// --- Drawing ------------------------------------------------------------------------------------------------
function head(){const {home,species,stage}=play;if(species===1){const a=window.WhiteBirdView?.anchors(stage,home.x+.053*(stage==='adult'?132:106),home.y-(stage==='adult'?132:106)*.47,3,'idle');return a?{x:a.headX,y:a.headY-14}:{x:home.x,y:home.y-100};}
 return {x:home.x,y:home.y-(stage==='baby'?80:110)};}
function drawPet(ctx,now,p){
 const {home,species,stage}=play,singing=play.state==='sing',pose=play.mood||(singing?'happy':'idle');
 // A little hop on each sung note and when happy.
 const hop=(singing||play.mood==='happy')&&!reduced.matches?Math.round(Math.abs(Math.sin(now/130))*3):0,shake=play.mood==='shock'&&!reduced.matches?Math.round(Math.sin(now/40)*2):0;
 ctx.save();ctx.fillStyle='rgba(69,48,33,.18)';ctx.beginPath();ctx.ellipse(home.x,home.y,species===2?32:28,4,0,0,Math.PI*2);ctx.fill();ctx.restore();
 if(species===1){const h=stage==='adult'?132:106,model={...p,stage,healthState:'healthy',sleepMode:'awake',mood:100,satiety:100,cleanliness:100,sadReasons:{mood:false,satiety:false,cleanliness:false}};
  return WhiteBirdView.draw(ctx,model,now,pose==='happy'?'happy':null,0,false,home.x+.053*h+shake,home.y-hop-h*.47,3);}
 return WyvernAdultView.paint(ctx,pose==='shock'?'shock':pose==='happy'?'happy':'idle',now,false,{x:home.x+shake,y:home.y-hop,stage});
}
function draw(ctx,now,p){
 if(!play)return false;
 if(!update(now))return false;
 drawPet(ctx,now,p);
 FLOWERS.forEach((f,i)=>{const on=play.lights.some(l=>l.i===i),hint=play.lights.some(l=>l.i===i&&l.hint),bob=on&&!reduced.matches?-2:0;
  ctx.save();ctx.imageSmoothingEnabled=false;if(hint&&Math.floor(now/180)%2)ctx.globalAlpha=.75;
  ctx.drawImage(on?sprites[i].lit:sprites[i].idle,Math.round(f.x-W/2),BASE_Y-H+bob);ctx.restore();});
 for(const n of play.notes){const u=Math.min(1,(now-n.at)/900),e=1-(1-u)**2;drawNote(ctx,n.x+(n.tx-n.x)*e,n.y+(n.ty-n.y)*e-(reduced.matches?0:Math.sin(u*Math.PI)*8),n.colour,1-u*u);}
 if(play.mood==='happy'&&window.PetEffects){const h=head();PetEffects.hearts(ctx,now-play.since,{x:h.x,y:h.y,spread:44,until:play.state==='finale'?FINALE_MS-600:500,reduced:reduced.matches});}
 if(play.mood==='shock'&&window.PetEffects){const h=head();PetEffects.shock(ctx,now-play.since,{x:h.x+30,y:h.y-10,reduced:reduced.matches});}
 return true;
}
// --- Input --------------------------------------------------------------------------------------------------
function point(e){const r=canvas.getBoundingClientRect();return {x:(e.clientX-r.left)*384/r.width,y:(e.clientY-r.top)*256/r.height};}
function flowerAt(pt){return FLOWERS.findIndex(f=>Math.abs(pt.x-f.x)<=W/2+2&&pt.y>=BASE_Y-H-4&&pt.y<=BASE_Y+4);}
canvas.addEventListener('pointerdown',e=>{if(!play||e.button!==0)return;const i=flowerAt(point(e));if(i<0)return;e.preventDefault();e.stopImmediatePropagation();tap(i,performance.now());});
canvas.addEventListener('pointermove',e=>{if(play)canvas.style.cursor=play.state==='listen'&&flowerAt(point(e))>=0?'pointer':'';});
document.addEventListener('keydown',e=>{
 if(!play||e.isComposing||['INPUT','SELECT','TEXTAREA'].includes(e.target?.tagName))return;
 if(e.key==='Escape'){e.preventDefault();stop();return;}
 const i=['1','2','3','4'].indexOf(e.key);if(i>=0&&play.state==='listen'){e.preventDefault();tap(i,performance.now());}
});
function stop(){if(!play)return;if(play.device)window.PetDeviceSession.close(play.device,true);else petDataManager.cancelGame();end('已結束合唱，未完成不結算。');}
exit.onclick=stop;
window.PetChorus={start,stop,draw,check,tap,FLOWERS,active:()=>!!play||starting,state:()=>play&&{state:play.state,tune:[...play.tune],index:play.index,round:play.round}};
})();
