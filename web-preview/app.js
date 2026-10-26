/* Presentation only; application state and adapter access go through DataManager. */
(()=>{'use strict';
// file:// blocks canvas pixel reads (halo, motes, farewell photo), so point to the localhost launcher.
if(typeof location!=='undefined'&&location.protocol==='file:'){const w=document.querySelector('#file-warning');if(w)w.hidden=false;}
const root=document.querySelector('#aviary'),canvas=document.querySelector('#bird'),ctx=canvas.getContext('2d'),status=document.querySelector('#status'),bubble=document.querySelector('#bubble');

const characterSheet=new Image();characterSheet.src='assets/tit-game-smooth.png';
const birdHybrid=new Image();birdHybrid.src='assets/bird-sage-v4.png';
function drawBirdAppearance(cx,cy,scale=3){
 const w=64*scale,h=44*scale,fit=Math.min(w/birdHybrid.naturalWidth,h/birdHybrid.naturalHeight),dw=birdHybrid.naturalWidth*fit,dh=birdHybrid.naturalHeight*fit;
 ctx.imageSmoothingEnabled=false;ctx.drawImage(birdHybrid,cx-dw/2,cy-dh/2,dw,dh);
}
let speciesPreview=typeof location!=='undefined'&&/(?:[?&])preview=wyvern(?:&|$)/.test(location.search||'')?'2':typeof location!=='undefined'&&/(?:[?&])preview=bird(?:&|$)/.test(location.search||'')?'1':'auto',reminderPending=false,lastReminder=-Infinity,previousSad=null,previousHealth=null,previousSleep=null;let mode='ble';const manager=window.petDataManager;
const defaults={satiety:60,mood:55,cleanliness:45};let state={...defaults},action=null,actionStart=0,treatmentResult=null,night=false,resetToken=0;
const reduced=matchMedia('(prefers-reduced-motion: reduce)');
let postcardAttemptAt=-Infinity,postcardSavedFor=null;
const savedPreferences=PetLocalStore.read('preferences',PetLocalStore.validPreferences);
let stage=manager.view('mock').pet.stage,artStage='auto',artAction='auto',showcaseAllowed=false,condition=manager.view('mock').pet.condition,backdrop='garden';
// Legacy background preferences resolve to the new day/night pixel farm pair.
if(savedPreferences){mode=savedPreferences.mode;night=savedPreferences.night;}
function savePreferences(){PetLocalStore.write('preferences',{mode,night,backdrop});}
const palette={P:'#282523',W:'#fffaf0',G:'#896448',L:'#896448'};
// The farm is named after the current pet; with no pet data it stays generic.
function syncTitle(){const name=manager.view().pet?.name;root.querySelector('h1').textContent=(name||'寵物')+'的'+(night?'夜晚':'午後')+'農場';}
function update(){const snapshot=manager.view();const p=snapshot.pet;syncTitle();if(p){const shown=mode==='mock'&&speciesPreview!=='auto'?Number(speciesPreview):p.speciesId;window.PetGrowthView?.observe(p,shown);document.querySelector('#pet-species').textContent='種類：'+(p.stage==='egg'?'孵化後揭曉':{1:'小鳥',2:'小飛龍'}[shown]||'未知種類');document.querySelector('#pet-exp').textContent=p.level==null?'等級／EXP：—':'Lv'+p.level+' · '+(p.expToNextLevel===0?'MAX':p.exp+' / '+p.expToNextLevel+' EXP');document.querySelector('#exp-progress').hidden=p.level==null;document.querySelector('#exp-progress').max=p.expToNextLevel||1;document.querySelector('#exp-progress').value=p.expToNextLevel===0?1:p.exp||0;}else{document.querySelector('#pet-species').textContent='種類：—';document.querySelector('#pet-exp').textContent='等級／EXP：—';document.querySelector('#exp-progress').hidden=true;}const reasons=PetRules.sadnessReasons(snapshot.pet),hint=document.querySelector('#sad-reason'),left=deviceCountdown(snapshot);hint.hidden=!reasons.length&&!left;hint.textContent=(reasons.length?'傷心原因：'+reasons.join('；'):'')+(reasons.length&&snapshot.pet.sleepMode==='normal'?'（睡眠中顯示睡眠造型）':'')+(reasons.length&&left?'。':'')+left;state=snapshot.pet?{...snapshot.pet}:{};for(const k of Object.keys(defaults)){const meter=document.querySelector('#'+k);if(snapshot.pet)meter.value=state[k];else meter.removeAttribute('value');meter.hidden=!snapshot.pet;document.querySelector('#'+k+'-value').textContent=snapshot.pet?state[k]:'—';}}
// Device sickness / death countdown (PROTOCOL.md 養成同步): needs the snapshot's seconds and device_info.rules, else ''.
let countdownShown=false;
function deviceCountdown(snapshot){const live=snapshot.source==='ble'&&!snapshot.stale&&snapshot.receivedAt?Math.max(0,Math.floor((Date.now()-Date.parse(snapshot.receivedAt))/1000)):0;const text=snapshot.source==='ble'?PetRules.countdown(snapshot.pet,snapshot.rules,live):'';countdownShown=!!text;return text;}
// In device mode only 陪玩 shows, and only when the firmware supports BLE play. It stays clickable while play is not
// possible right now (dimmed, reason as tooltip), so a click can say why; device-session.js decides.
// With the care feature the device-mode care buttons show too; device-care.js decides which ones work right now.
function busy(value){const devicePlay=mode!=='mock'&&!!manager.view('ble').webGame,deviceCare=mode!=='mock'&&!!manager.view('ble').care,playing=!!window.PetToyPlay?.active()||!!window.PetChorus?.active();root.classList.toggle('device-play',devicePlay);root.classList.toggle('device-care',deviceCare);
 document.querySelectorAll('[data-action]').forEach(b=>b.disabled=mode!=='mock'?!(devicePlay&&b.dataset.action==='play'&&!value&&!playing&&!manager.view('ble').careSending):value||!!window.PetGrowthView?.active||condition==='sleep'||(condition==='dead'||condition==='departed')||stage==='egg'||(condition==='sick'&&b.dataset.action==='play')||(b.dataset.action==='treat'&&condition!=='sick'));
 const play=document.querySelector('[data-action="play"]'),why=devicePlay&&!play.disabled?window.PetDeviceSession?.blocker()||'':'';
 if(play?.classList){play.classList.toggle('is-blocked',!!why);play.setAttribute('aria-disabled',String(!!why));play.title=why;}
 if(deviceCare)window.PetDeviceCare?.mark(document.querySelectorAll('[data-action]'),value||!!action||playing);}
function shownWyvern(){const p=manager.read().pet;return (mode==='mock'&&speciesPreview!=='auto'?Number(speciesPreview):p?.speciesId)===2;}
// The emote bubble shows what the pet needs right now (sickness first, then hunger, dirt, mood), or a short
// thank-you after care. Nothing shows while it is busy, reacting to a touch, asleep, in a game or not a hatched living pet.
const EMOTE_LABEL={potion:'生病了，需要治療',food:'肚子餓了',sponge:'身體髒髒的，想洗澡',heartbreak:'心情不好',heart:'好開心，謝謝你！',note:'乾乾淨淨好舒服～'};
let emoteShown=null,thanks=null,thanksUntil=0;
function thank(key){thanks=key;thanksUntil=performance.now()+1800;}
// Level-up celebration: queued per pet and started once care, its thank-you bubble, growth, games and touch reactions are done.
function celebrate(o){const p=mode==='mock'?manager.read().pet:manager.view('ble').pet;if(p?.petId)window.PetLevelUp?.request({petId:p.petId,...o},performance.now());}
function celebrateMock(gained){const level=manager.read().pet?.level;celebrate(Number.isInteger(level)?{level,from:Math.max(1,level-gained)}:{gained});}
function drawLevelUp(now,pet,species){const L=window.PetLevelUp;if(!L)return false;const live=mode==='mock'?manager.read().pet:manager.view('ble').pet;L.observe(live);
 const blocked=!!action||now<thanksUntil||!!window.PetGrowthView?.active||!!window.PetToyPlay?.active()||!!window.PetChorus?.active()||!!document.querySelector('#game-dialog')?.open||!!window.PetTouch?.reacting(now);
 const s=L.step(now,live,blocked);if(s==='start')window.PetWebSound?.levelUp?.();return !!s&&L.draw(ctx,now,{species,stage:pet?.stage,reduced:reduced.matches});}
function needEmote(p){
 if(!p||!['baby','adult'].includes(p.stage)||p.isDeparted||[p.healthState,p.health].includes('dead')||['sleep','dead','departed'].includes(condition))return null;
 if([p.healthState,p.health].includes('sick'))return 'potion';
 const sad=p.sadReasons||{satiety:p.satiety<=25,cleanliness:p.cleanliness<=25,mood:p.mood<=40};
 return sad.satiety?'food':sad.cleanliness?'sponge':sad.mood?'heartbreak':null;
}
function syncEmote(now,p,busyNow){
 const key=busyNow?null:now<thanksUntil?thanks:needEmote(p);
 if(!key){if(!bubble.hidden)bubble.hidden=true;emoteShown=null;return;}
 if(key===emoteShown&&!bubble.hidden)return;
 const art=window.PetEmote?.canvas(key);emoteShown=key;
 bubble.replaceChildren(art||EMOTE_LABEL[key]);bubble.classList.toggle('emote',!!art);bubble.setAttribute('role','img');bubble.setAttribute('aria-label',EMOTE_LABEL[key]);
 bubble.classList.toggle('baby',(artStage==='auto'?stage:artStage)==='baby');bubble.hidden=false;
}
// #status is the message bar under the garden; problems are highlighted there for a few seconds.
let noticeTimer=null;function notice(text){status.textContent=text;status.classList.add('alert');clearTimeout(noticeTimer);noticeTimer=setTimeout(()=>status.classList.remove('alert'),5000);}
// Device mode: 陪玩 opens the picker of games the ESP32 can settle, or says why play cannot start now.
function begin(kind){if(kind!=='play'&&mode!=='mock'){deviceCare(kind);return;}if(kind==='play'&&mode!=='mock'){const why=window.PetDeviceSession?.blocker();if(why)status.textContent=why;else window.openPetGames();return;}if(mode!=='mock'||window.PetGrowthView?.active)return;if(kind==='play'){window.openPetGames();return;}if(action||(condition==='dead'||condition==='departed')||stage==='egg'||condition==='sleep'||(condition==='dead'||condition==='departed')||stage==='egg'||(condition==='sick'&&kind==='play'))return;if(kind==='treat'){if(condition!=='sick')return;action=kind;treatmentResult=null;actionStart=performance.now();busy(true);bubble.hidden=true;status.textContent=shownWyvern()?'正在治療生病的小飛龍（展示）…':'正在治療生病的小鳥（展示）…';return;}action=kind;actionStart=performance.now();busy(true);status.textContent=kind==='feed'?(shownWyvern()?'牠正在低頭大口吃飯……':'牠正在啄食種子……'):kind==='clean'?'牠正在整理清潔，髒污逐漸消失……':'牠開心地跳躍、拍拍翅膀……';}
function finish(){if(!action)return;if(mode!=='mock'){const kind=action;action=null;treatmentResult=null;lastDeviceLine='';renderDevice();if(careLine){notice(careLine);careLine='';}thank(kind==='clean'?'note':'heart');return;}const kind=action;if(kind==='treat'){const result=treatmentResult||manager.command(kind);treatmentResult=null;if(result.levelsGained)celebrateMock(result.levelsGained);action=null;document.querySelector('#condition').value='awake';preview();update();thank('heart');status.textContent='展示治療完成，已恢復健康；飽食度與清潔度保持原值。';const token=++resetToken;setTimeout(()=>{if(token===resetToken&&!action)bubble.hidden=true;},1800);return;}const {key,actual,levelsGained}=manager.command(kind);if(levelsGained)celebrateMock(levelsGained);action=null;busy(false);update();if(kind!=='play')thank(kind==='clean'?'note':'heart');status.textContent='展示'+({satiety:'飽食度',cleanliness:'清潔度',mood:'心情'})[key]+' +'+actual+'。';const token=++resetToken;setTimeout(()=>{if(token===resetToken&&!action)bubble.hidden=true;},1800);}
function cleanDuration(){return window.CleaningAnimationView?.duration||4.2;}
// The bird is drawn at birdAt (left of centre, tail to the right), so foam and sponge follow its torso.
let birdAt=null;window.PetBirdPlacement=()=>birdAt;
function drawCleaning(elapsed,pet,shownSpecies){const view=window.CleaningAnimationView;if(!view||!view.ready?.())return false;const a=shownSpecies===1&&birdAt&&window.WhiteBirdView?.anchors(pet.stage,birdAt.cx,birdAt.cy,3,'base');view.draw(ctx,elapsed,{species:shownSpecies===2?'wyvern':'bird',stage:pet.stage,reduced:reduced.matches,at:a?{x:a.x,bottom:a.feet}:undefined});return true;}
function careReminder(p){const sad=PetRules.updateSad({...p});if(previousSad!==null&&((sad&&!previousSad)||(p.healthState==='sick'&&previousHealth!=='sick')||(sad&&previousSleep==='normal'&&p.sleepMode==='awake')))reminderPending=true;previousSad=sad;previousHealth=p.healthState;previousSleep=p.sleepMode;if(!sad||p.isDeparted||p.healthState==='dead')reminderPending=false;if(reminderPending&&p.sleepMode==='awake'&&!action&&!window.PetLevelUp?.active&&!window.PetLevelUp?.pending&&performance.now()-lastReminder>=300000&&!document.querySelector('#game-dialog').open&&window.PetWebSound?.reminder()){lastReminder=performance.now();reminderPending=false;}}
// Dirt (dirt-view.js): the garden view grimes over from its edges and the pet turns dull over its own pixels, so
// the pet's part is drawn straight after the pet while the canvas holds nothing else. Cleaning washes off, with the
// rinse (2–3.4 s), only what the +30 clean will remove; the pet's part stays hidden during treatment as before.
const CLEAN_GAIN=30;
function dirtBox(p,shownSpecies){if(shownSpecies===1){const a=birdAt&&window.WhiteBirdView?.anchors(p.stage,birdAt.cx,birdAt.cy,3,action?'base':'idle');return a&&{cx:a.x,top:a.headY+a.height*.16,bottom:a.feet-a.height*.04,w:a.height*.64,head:a.feet-a.height*.9};}const t=window.WyvernAdultView?.interactionTarget(p)||{x:190,y:130},baby=p.stage==='baby';return {cx:t.x,top:t.y-(baby?12:20),bottom:t.y+(baby?44:46),w:baby?60:96,head:t.y-(baby?40:60)};}
function washProgress(){return action==='clean'?Math.max(0,Math.min(1,((performance.now()-actionStart)/1000-2)/1.4)):0;}
function syncScreenDirt(p){const live=p&&['baby','adult'].includes(p.stage)&&!p.isDeparted&&![p.healthState,p.health].includes('dead')&&condition!=='dead'&&condition!=='departed'&&!window.PetGrowthView?.active;window.PetDirt?.screen(document.querySelector('#aviary .scene'),live?PetDirt.level(p.cleanliness):0,{keep:action==='clean'&&live?PetDirt.level(Math.min(100,Number(p.cleanliness)+CLEAN_GAIN)):0,alpha:1-washProgress()});}
function drawDirt(p,shownSpecies){if(action==='treat'||!window.PetDirt||!['baby','adult'].includes(p?.stage))return;const washed=washProgress();PetDirt.draw(ctx,{level:PetDirt.level(p.cleanliness),keep:action==='clean'?PetDirt.level(Math.min(100,Number(p.cleanliness)+CLEAN_GAIN)):0,box:dirtBox(p,shownSpecies),now:performance.now(),alpha:1-washed,reduced:reduced.matches,smell:!action});}
function drawRecovery(elapsed){if(window.SharedPetView){SharedPetView.recovery(ctx,elapsed);return;}ctx.fillStyle='#fff3ac';for(const x of [80,306]){const y=95-Math.sin(Math.floor((elapsed-1.4)/.125)*Math.PI/8)*8;ctx.fillRect(x-6,y,13,2);ctx.fillRect(x,y-6,2,13);}}
function block(x,y,w,h,color){ctx.fillStyle=color;ctx.fillRect(Math.round(x),Math.round(y),w,h);}
function heart(x,y){const rows=['.PP.PP.','PPPPPPP','PPPPPPP','.PPPPP.','..PPP..','...P...'];rows.forEach((r,j)=>[...r].forEach((c,i)=>{if(c==='P')block(x+i,y+j,1,1,'#e8a2b0');}));}
const BOWL_ADULT_WYVERN={cx:190,bottom:200,width:60},BOWL_BABY_WYVERN={cx:190,bottom:196,width:48};
function foodBowl(elapsed,at={cx:255,bottom:215,width:64}){if(window.BlueSeedBowl?.draw(ctx,elapsed,at.cx,at.bottom,at.width))return;if(window.SharedPetView){SharedPetView.paint(ctx,'kBowlFrames',Math.min(2,Math.floor(elapsed)),224,176,3,'#f0d39a');return;}
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
// One action list drives both species' art previews; the bird has no separate travel pose.
function applyArtAction(shownSpecies){if(shownSpecies===1)window.BirdDemo?.setPreview({eat:'feed',shock:'startled',move:'idle'}[artAction]||artAction);else if(shownSpecies===2)window.WyvernAdultView?.setPreview(artAction==='move'?'flight':artAction);}
// Demo tools are chip groups mirroring hidden selects, so the existing select handlers stay the single source of state.
// Hatching and growing up show their own banner, so the stage caption stays quiet at that moment.
function quietGrowth(from,to){if(from==='egg'&&to==='baby'||from==='baby'&&to==='adult')status.textContent='';}
function nextStage(){const order=['egg','baby','adult'],p=manager.read().pet;return p&&!p.isDeparted&&p.healthState!=='dead'?order[order.indexOf(p.stage)+1]:undefined;}
function syncDemoTools(){if(mode!=='mock')return;const p=manager.read().pet,shown=speciesPreview!=='auto'?Number(speciesPreview):p?.speciesId;
 for(const group of document.querySelector('#demo-panel')?.querySelectorAll?.('.chips')||[]){const select=document.querySelector('#'+group.dataset.for);for(const chip of group.querySelectorAll('button')){const v=chip.dataset.value;chip.setAttribute('aria-pressed',String(v===select.value));chip.disabled=select.disabled||(['art-stage','art-action'].includes(select.id)&&!showcaseAllowed)||(select.id==='art-action'&&v==='move'&&shown===1)||(select.id==='condition'&&p?.stage==='egg'&&v!=='awake');}}
 document.querySelector('#grow').disabled=document.querySelector('#stage').disabled||!nextStage();
 const needOff=!p||!['baby','adult'].includes(p.stage)||p.isDeparted||p.healthState==='dead'||!!action;for(const b of document.querySelectorAll('[data-need]'))b.disabled=needOff;}
// Showcase-only stage override: drawing reads a copy, the save keeps its real stage.
function showcasePet(p,shownSpecies){const allow=mode==='mock'&&[1,2].includes(shownSpecies)&&['baby','adult'].includes(p?.stage)&&!p.isDeparted&&p.healthState!=='dead';showcaseAllowed=allow;if(!allow){artStage=artAction='auto';document.querySelector('#art-stage').value=document.querySelector('#art-action').value='auto';return p;}return artStage==='auto'?p:{...p,stage:artStage};}
window.PetArtStage={shown:p=>showcasePet(p,mode==='mock'&&speciesPreview!=='auto'?Number(speciesPreview):p?.speciesId)?.stage,species:p=>mode==='mock'&&speciesPreview!=='auto'?Number(speciesPreview):p?.speciesId};
function draw(now){ctx.clearRect(0,0,384,256);birdAt=null;window.PetToyPlay?.check(now);window.PetChorus?.check(now);syncDemoTools();syncScreenDirt(mode==='mock'?manager.read().pet:manager.view('ble').pet);syncEmote(now,mode==='mock'?manager.read().pet:manager.view('ble').pet,!!action||!!window.PetToyPlay?.active()||!!window.PetChorus?.active()||!!window.PetGrowthView?.active||!!window.PetLevelUp?.active||!!document.querySelector('#game-dialog')?.open||!!window.PetTouch?.reacting(now));if(drawLifecycle(now)){requestAnimationFrame(draw);return;}if(!action&&window.PetGrowthView?.draw(ctx,now,reduced.matches)){busy(true);requestAnimationFrame(draw);return;}busy(!!action);if(mode!=='mock'&&(!manager.view('ble').pet||stage==='egg'||manager.view('ble').pet.health==='dead')){requestAnimationFrame(draw);return;}const elapsed=action?(now-actionStart)/1000:0;if(action==='treat'&&elapsed>=1.2&&!treatmentResult&&mode==='mock'){try{treatmentResult=manager.command('treat');}catch(error){action=null;notice(error.message);}}let pet=manager.read().pet;const shownSpecies=mode==='mock'&&speciesPreview!=='auto'?Number(speciesPreview):pet?.speciesId;const living=["baby","adult"].includes(pet?.stage)&&!pet.isDeparted&&pet.healthState!=="dead";window.BirdDemo?.syncControls(pet,mode==="mock"&&shownSpecies===1&&living,shownSpecies===1&&living);window.WyvernAdultView?.syncControls(pet,mode==="mock"&&shownSpecies===2&&living,shownSpecies===2&&living);pet=showcasePet(pet,shownSpecies);applyArtAction(shownSpecies);
// Garden games (toy play, chorus) draw the pet themselves; care stays locked until they end.
if(window.PetToyPlay?.draw(ctx,now,pet)||window.PetChorus?.draw(ctx,now,pet)){busy(true);requestAnimationFrame(draw);return;}
if(drawLevelUp(now,pet,shownSpecies)){busy(true);requestAnimationFrame(draw);return;}
const shownStage=pet?.stage??stage;if(shownSpecies===2){ctx.save();const shown=WyvernView.draw(ctx,action==='treat'&&elapsed>=1.4?{...pet,webExpression:'happy',healthState:'healthy',mood:100,satiety:100,cleanliness:100,sadReasons:{mood:false,satiety:false,cleanliness:false}}:pet,action==='treat'&&elapsed>=1.4?0:now,action==='treat'&&elapsed>=1.4?null:action,elapsed,reduced.matches);ctx.restore();drawDirt(pet,shownSpecies);if(action==='treat'&&elapsed>=1.4)drawRecovery(elapsed);if(shown?.state==='eat')foodBowl(shown.bowlSeconds,pet.stage==='baby'?BOWL_BABY_WYVERN:BOWL_ADULT_WYVERN);else if(action==='feed')foodBowl(elapsed,pet.stage==='adult'?BOWL_ADULT_WYVERN:undefined);if(action==='treat'&&elapsed<1.4&&window.PotionView?.draw(ctx,elapsed,264,74,reduced.matches)){}else if(action==='treat'&&elapsed<1.4&&window.SharedPetView){SharedPetView.paint(ctx,'kPotionFrames',SharedPetView.potionFrame(elapsed),pet.stage==='baby'?232:262,30,3);}else if(action==='treat'&&elapsed<1.4){ctx.fillStyle='#acd1bc';ctx.fillRect(pet.stage==='baby'?232:262,30,18,30);}if(action==='clean'&&!drawCleaning(elapsed,pet,shownSpecies)){ctx.fillStyle='rgba(192,226,235,.85)';ctx.fillRect(94,177-Math.min(132,elapsed/1.2*132),192,Math.min(132,elapsed/1.2*132));}if(action&&elapsed>=(action==='feed'?3:action==='clean'?cleanDuration():3.4))finish();requestAnimationFrame(draw);return;}if(shownSpecies!==1){ctx.fillStyle='#57483a';ctx.font='16px sans-serif';ctx.fillText('未知種類 · 不猜測外觀',100,140);requestAnimationFrame(draw);return;}const moving=!window.BirdDemo?.idleActive(pet)&&!window.BirdDemo?.previewing()&&!reduced.matches&&!(window.PetSadView?.active(pet)&&!action)&&(mode==='mock'||manager.view('ble').connection==='ready');let x=105,y=87,angle=0;
if(moving){y+=Math.floor(Math.sin(now/(condition==='awake'?650:1500)));if(condition==='sick'){x+=Math.sin(now/95)*2;angle=Math.sin(now/180)*.035;}if(action==='feed')angle=.16+Math.max(0,Math.sin(elapsed*10))*.5;if(action==='play'){y-=Math.abs(Math.sin(elapsed*5))*20;x+=Math.sin(elapsed*4)*8;}if(action==='clean')angle=Math.sin(elapsed*12)*.07;}
const flightPhase=(now%15000)/1000,flying=shownStage==='adult'&&condition==='awake'&&!action&&moving&&flightPhase>=5&&flightPhase<10;
if(flying){const p=(flightPhase-5)/5;y-=Math.sin(Math.PI*p)*44;x+=Math.sin(p*Math.PI*2)*18;angle+=Math.sin(p*Math.PI*2)*.08;}
const walking=shownStage==='baby'&&condition==='awake'&&!action&&moving,walkPhase=(now%12000)/1000,stepping=walking&&(walkPhase<4||(walkPhase>=6&&walkPhase<10));
if(shownStage==='baby'){x+=5;y+=8;if(walking){x+=walkPhase<4?walkPhase*6:walkPhase<6?24:walkPhase<10?24-(walkPhase-6)*6:0;if(stepping)y-=Math.floor(Math.abs(Math.sin(now/160))*2);}}
if(action==='feed'){if(window.WhiteBirdFeed?.ready()){const l=WhiteBirdFeed.layout(pet.stage);x=l.cx-65;y=l.cy-50;}else{x+=Math.min(1,elapsed/.4)*26;y+=shownStage==='baby'?12:2;}}
const baby=shownStage==='baby',pose=action==='feed'?1:condition!=='awake'||!moving?0:Math.floor(now/6500)%4;
const view=pose===3?1:pose,dw=baby?98:125,dh=baby?94:112;
// Feet stay on the branch; the sheet provides the actual illustrated character.
// The ground shadow is added behind the pet after its dirt, which must only land on the pet's own pixels.
const drawShadow=()=>{ctx.save();ctx.globalCompositeOperation='destination-over';ctx.fillStyle='rgba(69,48,33,.18)';ctx.beginPath();ctx.ellipse(x+65,action==='feed'&&window.WhiteBirdFeed?.ready()?WhiteBirdFeed.layout(pet.stage).feet:190,baby?24:32,flying?3:5,0,0,Math.PI*2);ctx.fill();ctx.restore();};
birdAt={cx:x+65,cy:y+50,stage:pet.stage};window.WhiteBirdActivity?.forget?.(ctx);if((pet.stage==='baby'||pet.stage==='adult')&&window.BirdView&&BirdView.draw(ctx,pet,now,action,elapsed,false,x+65,y+50,3)){}else if(pet.healthState==='sick'&&window.PetSadView?.active(pet)&&action!=='feed'&&!(action==='treat'&&elapsed>=1.4)){PetSadView.bird(ctx,pet,now,action==='treat');}else if(birdHybrid.complete&&birdHybrid.naturalWidth){ctx.save();ctx.imageSmoothingEnabled=false;ctx.translate(x+65,y+50);ctx.rotate(angle);drawBirdAppearance(0,0,baby?2.5:3);ctx.restore();}else if(characterSheet.complete&&characterSheet.naturalWidth){
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
// Walking to the pond or drinking moves the bird inside its activity player; follow it there.
const walked=window.WhiteBirdActivity?.placed?.(ctx);if(walked)birdAt={...birdAt,...walked};
drawDirt(pet,shownSpecies);drawShadow();
if(condition==='sleep'){window.PetEffects?.zzz(ctx,now,{x:x+92,y:y+(window.WhiteBirdSleepData?9:14),reduced:reduced.matches});}
if(action==='feed'&&!window.WhiteBirdFeed?.bowl(ctx,pet,elapsed))foodBowl(elapsed);
if(action==='treat'&&elapsed<1.4&&window.PotionView?.draw(ctx,elapsed,264,74,reduced.matches)){}else if(action==='treat'&&elapsed<1.4&&window.SharedPetView){SharedPetView.paint(ctx,'kPotionFrames',SharedPetView.potionFrame(elapsed),232,30,3);}else if(action==='treat'&&elapsed<1.4){ctx.save();ctx.fillStyle='#fffaf0';ctx.strokeStyle='#896448';ctx.lineWidth=1.5;ctx.beginPath();ctx.roundRect(248,151,24,31,5);ctx.fill();ctx.stroke();ctx.fillStyle='#896448';ctx.fillRect(252,146,16,6);ctx.fillStyle='#acd1bc';ctx.fillRect(258,160,4,13);ctx.fillRect(253,165,14,4);if(moving){ctx.globalAlpha=Math.sin(elapsed*Math.PI/2.5)*.8;for(let i=0;i<3;i++){const sy=130-i*13-(elapsed*12%12);ctx.fillRect(145+i*23,sy,2,8);ctx.fillRect(142+i*23,sy+3,8,2);}}ctx.restore();}
if(action==='treat'&&elapsed>=1.4)drawRecovery(elapsed);if(action==='clean'&&!drawCleaning(elapsed,pet,shownSpecies)){const height=[33,66,99,132][Math.min(3,Math.floor(elapsed/.4))];ctx.fillStyle='rgba(192,226,235,.85)';ctx.fillRect(94,177-height,192,height);}
if(action==='play')for(let i=0;i<3;i++)heart(145+i*24,82-(moving?(elapsed*17+i*10)%30:0));
if(night)for(let i=0;i<14;i++){ctx.globalAlpha=moving?.4+.6*Math.abs(Math.sin(now/800+i)):1;block(132+(i*29)%221,8+(i*13)%62,2,2,'#fff2bb');}ctx.globalAlpha=1;
if(action&&elapsed>=(action==='feed'?3:action==='clean'?cleanDuration():3.4))finish();requestAnimationFrame(draw);}
document.querySelectorAll('[data-action]').forEach(b=>b.addEventListener('click',()=>b.dataset.action==='treat'&&mode==='mock'?healPet():begin(b.dataset.action)));
function setNight(value){night=value;root.classList.toggle('night',night);applyBackdrop();const theme=document.querySelector('#theme-toggle'),label=night?'切換到午後':'切換到夜晚';theme.textContent=night?'☀':'☾';theme.setAttribute('aria-label',label);theme.title=label;syncTitle();savePreferences();}
function applyBackdrop(){root.classList.remove('simple-background','smooth-background');const image=document.querySelector('#background');image.src='assets/garden-detailed-'+(night?'night':'day')+'.png?v=cute-farm-v2-20261016';image.alt=night?'像素農場傍晚夜景，暖黃燈光與靛藍溪流':'像素農場午後，明亮草地與湛藍溪流';}

applyBackdrop();
document.querySelector('#theme-toggle').onclick=()=>setNight(!night);
// seenAlive: pets watched alive this session; a pet already departed when first seen opens on its postcard.
let deathKey=null,deathStart=0,seenAlive=new Set(),shutterFor=null,lastAgeTick=performance.now(),bleHistorySession=PetMemorials.uuid();
function currentPet(){return manager.view().pet;}
function syncLifecycle(){
 const p=currentPet(),dead=p&&(p.isDeparted||p.condition==='dead'||p.health==='dead');
 // drawLifecycle shows the menu once the farewell or grave animation has settled.
 if(!dead)document.querySelector('#afterlife-menu').hidden=true;
 if(p){PetMemorials.remember(mode==='mock'&&speciesPreview!=='auto'?{...p,previewSpeciesId:Number(speciesPreview)}:p,mode,bleHistorySession);if(mode==='mock'){document.querySelector('#pet-name').textContent=p.name;document.querySelector('#pet-age').textContent='陪伴時長：'+PetMemorials.duration(String(p.ageSeconds))+'（展示有效年齡）';}}
 const key=dead?mode+':'+(p.deviceId||'mock')+':'+p.petId:null;if(p&&!dead)seenAlive.add(mode+':'+(p.deviceId||'mock')+':'+p.petId);
 if(key!==deathKey){deathKey=key;deathStart=performance.now();if(key&&p.isDeparted&&!seenAlive.has(key))deathStart=-Infinity;if(key&&!p.isDeparted)window.PetWebSound?.death?.();}
 if(dead){action=null;resetToken++;bubble.hidden=true;root.classList.remove('is-sick');document.querySelector('#afterlife-age').textContent=(p.isDeparted?'遠行 · ':'長眠 · ')+p.name+' · 陪伴 '+PetMemorials.duration(String(p.ageSeconds));}
 const stageOrder=['egg','baby','adult'],stageRank=stageOrder.indexOf(p?.stage);for(const option of document.querySelector('#stage').options){const rank=stageOrder.indexOf(option.value);option.disabled=rank<stageRank||rank>stageRank+1;}
 for(const id of ['stage','condition','reset'])document.querySelector('#'+id).disabled=mode!=='mock'||!!dead;
 const L=mode==='ble'&&window.PetDeviceLifecycle;syncDeviceLifecycle(L);document.querySelector('#adopt-egg').disabled=mode!=='mock'&&!(L&&L.shown('adopt'));document.querySelector('#farewell').disabled=mode!=='mock'||!!dead||!p||p.stage==='egg'||p.sleepMode!=='awake';document.querySelector('#species-preview').disabled=mode!=='mock';document.querySelector('#adoption-note').textContent=mode==='ble'?(L&&L.shown('adopt')?'新蛋的種類和名字由電子雞決定，舊紀錄留在電子雞的紀念冊。':'裝置模式請在 ESP32 上領養新蛋，網頁等待新狀態回報。'):'新蛋使用新代號，舊紀錄留在紀念冊。';
}
function drawLifecycle(now){
 const original=currentPet();if(!original||original.isDeparted||original.healthState==="dead")window.BirdDemo?.syncControls(original,false);if(!original||original.isDeparted||original.healthState==="dead")window.WyvernAdultView?.syncControls(original,false);const p=original&&mode==='mock'&&speciesPreview!=='auto'?{...original,speciesId:Number(speciesPreview)}:original;if(!p)return false;
 if(p.isDeparted||p.condition==='dead'||p.health==='dead'){
  if(p.isDeparted){
   // Viewfinder, shutter, departure and the postcard all come from PetFarewellView.
   const shown=window.PetFarewellView?.draw(ctx,{species:p.speciesId,stage:p.stage,name:p.name,petId:p.petId,age:PetMemorials.duration(String(p.ageSeconds)),date:PetMemorials.farewellDate(p),elapsed:now-deathStart,now,reduced:reduced.matches,background:document.querySelector('#background')});
   document.querySelector('#afterlife-menu').hidden=!(shown?.rest??true);
   if(shown?.phase==='shoot'&&shutterFor!==deathKey){shutterFor=deathKey;window.PetWebSound?.shutter?.();}
   // Device pets keep the scene photo too, on their Web album record (device id + pet id).
   const url=shown?.rest&&postcardSavedFor!==deathKey&&now-postcardAttemptAt>=5000?PetFarewellView.photo():null;
   if(url){postcardAttemptAt=now;try{if(PetMemorials.savePhoto(p,url))postcardSavedFor=deathKey;}catch(error){/* Optional photo failure must not interrupt the farewell or its saved record. */}}
   return true;
  }
  // Soul, grave and resting garden all come from PetDeathView; the menu waits for the grave to settle.
  const shown=window.PetDeathView?.draw(ctx,{species:p.speciesId,stage:p.stage,name:p.name,elapsed:now-deathStart,now,reduced:reduced.matches,night});
  document.querySelector('#afterlife-menu').hidden=!(shown?.rest??true);return true;
 }
 if(p.stage==='egg'&&window.PetEggView){const age=Number(p.ageSeconds),visualAge=mode==='mock'?(age<150?0:age<225?240:270):age;PetEggView.draw(ctx,visualAge,now,reduced.matches,mode==='mock',p.petId);return true;}if(p.stage==='egg'){ctx.save();ctx.translate(190,163);ctx.rotate(reduced.matches?0:Math.sin(now/1000)*.035);const g=ctx.createRadialGradient(-8,-18,2,0,0,34);g.addColorStop(0,'#fffdf5');g.addColorStop(1,'#ceb99e');ctx.beginPath();ctx.moveTo(0,-34);ctx.bezierCurveTo(29,-34,36,30,0,30);ctx.bezierCurveTo(-36,30,-29,-34,0,-34);ctx.fillStyle=g;ctx.fill();ctx.strokeStyle='#896448';ctx.lineWidth=1.5;ctx.stroke();ctx.restore();return true;}return false;
}
document.querySelector('#open-cemetery').onclick=document.querySelector('#visit-graves').onclick=()=>PetMemorials.open(mode==='mock'?'mock:展示花園':currentPet()?.deviceId?'ble:'+currentPet().deviceId:undefined);
// The adopt dialog keeps its demo text; device mode swaps in the ESP32 wording while it is open.
const adoptText=[document.querySelector('#adopt-dialog h2').textContent,document.querySelector('#adopt-dialog p').textContent];
document.querySelector('#adopt-egg').onclick=()=>{const ble=mode==='ble';if(ble){const why=window.PetDeviceLifecycle?.blocker('adopt');if(why!==''){status.textContent=why||'這台電子雞的韌體還不支援從網頁領養。';return;}}else if(currentPet()?.condition!=='dead'&&!currentPet()?.isDeparted)return;
 document.querySelector('#adopt-dialog h2').textContent=ble?'領養新蛋？':adoptText[0];document.querySelector('#adopt-dialog p').textContent=ble?currentPet().name+' 的紀錄會留在電子雞的紀念冊。新蛋的種類和名字由電子雞決定，網頁不能指定。':adoptText[1];document.querySelector('#adopt-dialog').showModal();};
document.querySelector('#confirm-adopt').onclick=()=>{if(mode==='ble'){document.querySelector('#adopt-dialog').close();deviceLifecycle('adopt');return;}if(mode!=='mock'||(currentPet()?.condition!=='dead'&&!currentPet()?.isDeparted))return;try{manager.adopt();previousSad=null;reminderPending=false;}catch(error){document.querySelector('#adopt-dialog p').textContent=error.message;return;}document.querySelector('#adopt-dialog').close();deathKey=null;document.querySelector('#stage').value='egg';document.querySelector('#condition').value='awake';preview();update();};
// Only page-active demo time advances. Hardware age always comes from BLE.
document.addEventListener('visibilitychange',()=>{lastAgeTick=performance.now();if(document.hidden)manager.cancelGame();});
// Device-mode games pass the ESP32's levels_gained; showcase games settle in the mock and are read back here.
document.addEventListener('pet-game-complete',e=>{const gained=e.detail?.levelsGained??(mode==='mock'?manager.gameState(performance.now())?.levelsGained:0);if(gained)mode==='mock'?celebrateMock(gained):celebrate({gained});update();});
setInterval(()=>{const now=performance.now(),seconds=Math.floor((now-lastAgeTick)/1000);if(seconds<1)return;lastAgeTick+=seconds*1000;if(!document.hidden&&mode!=='mock'&&countdownShown)renderDevice();if(document.hidden||mode!=='mock')return;const before=manager.view().pet;manager.tick(before.stage==='egg'?Math.min(seconds*50,Math.max(0,300-before.ageSeconds)):seconds);const p=manager.view().pet;careReminder(p);stage=p.stage;condition=p.condition;document.querySelector('#stage').value=stage;document.querySelector('#condition').value=condition;if(before.stage!==stage||before.condition!==condition||before.healthState!==p.healthState){action=null;preview();quietGrowth(before.stage,stage);}update();syncLifecycle();},1000);
let healthTransition=0;
function syncHealth(health){
 const badge=document.querySelector('#health-badge'),label=document.querySelector('#health-label'),button=document.querySelector('#treat-button'),sick=health==='sick',token=++healthTransition;
 badge.className='health-badge '+(sick?'sick':health==='healthy'?'healthy':'unknown');label.textContent=sick?'生病':health==='healthy'?'健康':health==='dead'?'已離世':health==='departed'?'已遠行':'尚無資料';
 if(sick){button.hidden=false;button.classList.remove('treatment-hidden');button.setAttribute('aria-hidden','false');}
 else{button.disabled=true;button.classList.add('treatment-hidden');button.setAttribute('aria-hidden','true');if(document.activeElement===button)document.querySelector('#condition').focus();setTimeout(()=>{if(token===healthTransition)button.hidden=true;},reduced.matches?0:420);}
}
function setSickState(){if(mode!=='mock'||action)return;document.querySelector('#condition').value='sick';preview();update();}
function healPet(){if(mode!=='mock'||condition!=='sick'||action)return;begin('treat');if(action==='treat')root.classList.add('healing');}
window.setSickState=setSickState;window.healPet=healPet;
function preview(){if(mode!=='mock')return;action=null;treatmentResult=null;root.classList.remove('healing');resetToken++;stage=document.querySelector('#stage').value;condition=document.querySelector('#condition').value;manager.preview(stage,condition);stage=manager.view('mock').pet.stage;condition=manager.view('mock').pet.condition;document.querySelector('#stage').value=stage;document.querySelector('#condition').value=condition;syncLifecycle();busy(false);bubble.hidden=true;root.classList.toggle('is-sick',manager.read().pet.healthState==='sick');syncHealth(manager.read().pet.isDeparted?'departed':manager.read().pet.healthState);document.querySelector('#pet-description').textContent=(stage==='baby'?'幼年':'成年')+' · '+(manager.read().pet.healthState==='sick'?'生病':'健康')+' · '+(condition==='sleep'?'一般睡眠':'清醒');canvas.setAttribute('aria-label',(stage==='baby'?'幼年':'成年')+' '+manager.read().pet.name+'，'+(condition==='sleep'?'正在睡覺':condition==='sick'?'生病中':'清醒'));if(condition==='departed')document.querySelector('#pet-description').textContent='已遠行 · 紀念冊已保存';else if(condition==='dead')document.querySelector('#pet-description').textContent='已離世 · 陪伴紀錄已留存';else if(stage==='egg')document.querySelector('#pet-description').textContent='蛋 · 等待孵化';status.textContent=condition==='dead'?'謝謝你的陪伴，下一步可進入紀念冊或領養新蛋。':stage==='egg'?'新蛋正在孵化中：展示模式約3秒後開始裂痕孵化，約6秒後進入出生轉場；裝置依正式時間。':condition==='sleep'?'牠閉上眼睛，正在休息。切回「清醒」就可以繼續照顧。':condition==='sick'?'生病中：牠不太舒服，需要休息與治療。陪玩暫停；餵食與清潔不會治好疾病。（展示）':stage==='baby'?'幼年寵物正在等你！可以餵食、清潔或陪牠玩。':(shownWyvern()?'牠正在草地上等你。試著餵牠吃點東西吧！':'牠正在棲架上等你。試著餵牠一點種子吧！');}
document.querySelector('#stage').onchange=preview;document.querySelector('#art-stage').onchange=()=>{artStage=document.querySelector('#art-stage').value;};document.querySelector('#art-action').onchange=()=>{artAction=document.querySelector('#art-action').value;};
document.querySelector('#demo-panel').addEventListener('click',e=>{const chip=e.target.closest?.('.chips button');if(!chip||chip.disabled)return;const select=document.querySelector('#'+chip.closest('.chips').dataset.for);select.value=chip.dataset.value;select.onchange();syncDemoTools();});
document.querySelector('#grow').onclick=()=>{const next=nextStage(),from=manager.read().pet?.stage;if(mode!=='mock'||!next)return;document.querySelector('#stage').value=next;preview();update();quietGrowth(from,next);syncDemoTools();};document.querySelector('#condition').onchange=preview;
// Demo needs: drop one value to 10 (below every sad threshold) or refill all three to 100.
for(const b of document.querySelectorAll('[data-need]'))b.onclick=()=>{if(mode!=='mock'||action)return;const n=b.dataset.need;for(const k of n==='all'?['satiety','cleanliness','mood']:[n])manager.demoNeed(k,n==='all'?100:10);update();};
document.querySelector('#reset').onclick=()=>{if(mode==='mock')document.querySelector('#reset-dialog').showModal();};
// Reset overwrites the demo pet without a memorial entry, so it only runs after the dialog's confirmation.
document.querySelector('#confirm-reset').onclick=()=>{if(mode!=='mock')return;document.querySelector('#reset-dialog').close();manager.reset();previousSad=null;reminderPending=false;deathKey=null;state={...defaults};document.querySelector('#stage').value='adult';document.querySelector('#condition').value='awake';preview();update();};
function switchMode(){root.classList.remove('healing');mode=document.querySelector('#mode').value;manager.select(mode);lastAgeTick=performance.now();action=null;resetToken++;bubble.hidden=true;root.classList.remove('is-sick');
for(const id of ['stage','condition','reset'])document.querySelector('#'+id).disabled=mode!=='mock';
const demo=mode==='mock';root.classList.toggle('device-mode',!demo);document.querySelector('#mode-toggle').textContent=demo?'回到裝置':'進入展示模式';document.querySelector('#mode-label').textContent=demo?'✿ 展示模式':'◎ 裝置模式';document.querySelector('#source-label').textContent=demo?'不影響 ESP32':'尚未連接裝置';document.querySelector('#device-empty').hidden=demo;
document.querySelector('#pet-name').textContent=demo?manager.view('mock').pet.name:'等待裝置';document.querySelector('#pet-age').textContent=demo?'有效年齡：1 小時（展示）':'有效年齡：—';
document.querySelector('#last-received').hidden=demo;if(demo)preview();else renderDevice();update();busy(false);savePreferences();}
const DEATH_CAUSE={untreated_sickness:'生病未治療'};
// Snapshots arrive whenever the pet changes; only a new connection message replaces the line, so play messages stay.
let lastDeviceLine='',lifecycleHold=0;
function renderDevice(){if(mode!=='ble')return;const snap=manager.view('ble'),p=snap.pet;const labels={disconnected:'未連線',selecting:'選擇裝置中',connecting:'連線中',waiting:'等待資料',ready:snap.care?'裝置直連':'裝置直連（唯讀）',cancelled:'已取消',error:'連線異常'};
 document.querySelector('#source-label').textContent=snap.stale&&p?'離線／舊資料':labels[snap.connection];
 document.querySelector('#ble-state').textContent=snap.message;
 document.querySelector('#last-received').textContent=p?'最後接收：'+new Date(snap.receivedAt).toLocaleString()+' · 裝置 ID：'+(p.deviceId||'舊韌體未提供'):'最後接收：—';
 document.querySelector('#pet-name').textContent=p?p.name:'等待裝置';
 document.querySelector('#pet-age').textContent=p?'有效年齡：'+p.ageSeconds+' 秒（裝置回報）':'有效年齡：—';
 document.querySelector('#pet-description').textContent=p?({egg:'蛋',baby:'幼年',adult:'成年'}[p.stage]+' · '+{healthy:'健康',sick:'生病',dead:'已離世'}[p.health]+(p.health==='dead'&&p.deathCause?'（'+(DEATH_CAUSE[p.deathCause]||'原因未知')+'）':'')+' · '+(p.isDeparted?'已遠行':p.condition==='sleep'?'睡眠':'清醒')+(snap.stale?' · 舊資料':'')):'尚無硬體資料';
 const noArt=!p;document.querySelector('#device-empty').hidden=!noArt;
 document.querySelector('#device-empty-message').textContent=!p?snap.message:p.health==='dead'?'裝置回報：寵物已離世': '裝置回報：蛋階段，等待孵化';
 if(p){stage=p.stage;condition=p.health==='dead'?'dead':p.condition;}syncLifecycle();root.classList.toggle('is-sick',!p?.isDeparted&&p?.health==='sick');
 if(p?.isDeparted)document.querySelector('#pet-description').textContent='已遠行'+(snap.stale?' · 舊資料':'');syncHealth(p?.isDeparted?'departed':p?.health??'unknown');{const alert=p?.isDeparted?'牠已踏上旅程。':p?.health==='dead'?'寵物已離世（'+(DEATH_CAUSE[p.deathCause]||'原因未知')+'）。':p?.stage==='egg'?p.name+' 還在蛋裡，等牠在電子雞上孵化吧。':deviceCountdown(snap)?deviceCountdown(snap)+'。':'';const old=p&&snap.stale?'目前顯示最後收到的舊資料，不代表即時狀態。':'',line=alert?alert+old:snap.message+(old?'。'+old:'')+(snap.care?'':' 真實照顧命令尚未開放。');if(!action&&line!==lastDeviceLine&&performance.now()>=lifecycleHold){lastDeviceLine=line;status.textContent=line;}}
 canvas.setAttribute('aria-label',p?p.name+'，'+document.querySelector('#pet-description').textContent:'尚無硬體資料');update();busy(false);
}
// Device pet_event: plays the matching care animation only (the snapshot still decides the state). Hatching and growing
// up already play from the snapshot's stage change; death, farewell, adoption and the album follow the next snapshot.
// Anything already playing (care, growth, a game) wins and the event is skipped.
const EVENT_ACTION={fed:'feed',cleaned:'clean',treated:'treat'},EVENT_TEXT={feed:'電子雞上餵了牠，牠正在吃飯……',clean:'電子雞上幫牠清潔，髒污逐漸消失……',treat:'電子雞上幫牠治療……'},EVENT_DONE={feed:'電子雞上餵好了。',clean:'電子雞上清潔好了。',treat:'電子雞上治療好了。'};
function deviceEvent(e){
 if(mode!=='ble')return;const p=manager.view('ble').pet,living=p&&['baby','adult'].includes(p.stage)&&!p.isDeparted&&p.health!=='dead';
 const playing=!!action||!!window.PetGrowthView?.active||!!window.PetToyPlay?.active()||!!window.PetChorus?.active();
 // Level-up queues behind care, growth and games instead of being skipped (see level-up-view.js).
 if(e.event==='level_up'){const why=!living?'pet not living':'';if(!why)celebrate(Number.isInteger(e.level)?{level:e.level}:{gained:1});console.info('[pet_event] level_up',why?'no animation: '+why:playing?'queued':'celebration');return;}
 const kind=EVENT_ACTION[e.event],why=!kind?'':!living?'pet not living':playing?'another animation playing':p.sleepMode!=='awake'?'asleep':'';if(why)console.info('[pet_event]',e.event,'no animation: '+why);if(!kind||why)return;
 action=kind;actionStart=performance.now();treatmentResult=null;bubble.hidden=true;busy(true);status.textContent=EVENT_TEXT[kind];careLine=EVENT_DONE[kind];
}
manager.onPetEvent(deviceEvent);
// Web care on the device (PROTOCOL.md 養成同步 care): the ESP32 plays its own animation (D1); the garden one plays only
// after action_result done, with the real deltas as the status line. Meters still change only from the next snapshot,
// and the device's pet_event for this web command (source web) is skipped by the BLE layer, so nothing plays twice.
let careLine='';
// Touch reactions wait while a garden care animation plays in device mode (the mock checks its disabled care buttons).
window.PetGardenBusy=()=>!!action;
function deviceCare(kind){
 const why=action?'等目前的動畫播完再照顧吧。':window.PetDeviceCare?window.PetDeviceCare.blocker(kind):'這台電子雞的韌體還不支援網頁照顧。';if(why){status.textContent=why;return;}
 const run=PetDeviceCare.run(kind);status.textContent=PetDeviceCare.sendingText(kind);busy(false);
 run.then(r=>{if(mode!=='ble')return;const p=manager.view('ble').pet,line=PetDeviceCare.doneText(r),kind=PetDeviceCare.ANIMATION[r.action];
  if(r.changes.levels_gained>0)celebrate({gained:r.changes.levels_gained});
  const free=kind&&!action&&!window.PetGrowthView?.active&&!window.PetToyPlay?.active()&&!window.PetChorus?.active()&&p&&['baby','adult'].includes(p.stage)&&p.sleepMode==='awake'&&!p.isDeparted&&p.health!=='dead';
  if(free){action=kind;actionStart=performance.now();treatmentResult=null;bubble.hidden=true;careLine=line;status.textContent=line;busy(true);}else{notice(line);busy(false);}},
  e=>{if(mode!=='ble')return;notice(e.message);busy(false);});
}
manager.subscribe(()=>{renderDevice();refreshConnectionButtons();const snap=manager.view('ble');if(snap.pet&&snap.connection==='ready')PetMemorials.remember(snap.pet,'ble',bleHistorySession);});
function refreshConnectionButtons(){const s=manager.view('ble');document.querySelector('#ble-state').textContent=s.message;document.querySelector('#ble-search').disabled=['selecting','connecting','waiting','ready'].includes(s.connection);document.querySelector('#ble-disconnect').disabled=['disconnected','cancelled','error'].includes(s.connection);}
document.querySelector('#ble-search').onclick=()=>{document.querySelector('#mode').value='ble';switchMode();manager.connect();};
document.querySelector('#ble-disconnect').onclick=()=>manager.disconnect();
document.querySelector('#mode').onchange=switchMode;
document.querySelector('#mode-toggle').onclick=()=>{document.querySelector('#mode').value=mode==='mock'?'ble':'mock';switchMode();};
document.querySelector('#empty-search').onclick=()=>document.querySelector('#ble-search').onclick();
document.querySelector('#connection').onclick=()=>{window.checkBleReadiness();document.querySelector('#device-dialog').showModal();};
document.addEventListener('pet-interaction-complete',()=>{update();syncLifecycle();});document.querySelector('#species-preview').value=speciesPreview;
document.querySelector('#species-preview').onchange=()=>{speciesPreview=document.querySelector('#species-preview').value;update();};
document.querySelector('#farewell').onclick=()=>{const p=manager.read().pet;if(mode!=='mock'||p.isDeparted||p.healthState==='dead'||p.stage==='egg'||p.sleepMode!=='awake')return;document.querySelector('#farewell-description').textContent=p.name+' · 陪伴 '+PetMemorials.duration(String(p.ageSeconds));document.querySelector('#farewell-dialog').showModal();};
document.querySelector('#confirm-farewell').onclick=()=>{if(mode==='ble'){document.querySelector('#farewell-dialog').close();deviceLifecycle('depart');return;}try{manager.farewell();document.querySelector('#farewell-dialog').close();condition='departed';action=null;root.classList.remove('is-sick');syncHealth('departed');document.querySelector('#pet-description').textContent='已遠行 · 紀念冊已保存';syncLifecycle();busy(false);update();status.textContent='紀念冊已保存，牠已踏上旅程。';}catch(error){document.querySelector('#farewell-description').textContent=error.message;}};
// Device farewell / adoption (PROTOCOL.md 養成同步, lifecycle): the ESP32 does not ask again (D3), so the dialog is the only
// confirmation, defaulting to 留下. The action_result only names the outcome; the next snapshot (departed, then the egg)
// drives the farewell sequence and the egg view, once per pet (deathKey). The web command's own pet_event is skipped.
function syncDeviceLifecycle(L){window.PetDeviceSound?.sync();const row=document.querySelector('#device-lifecycle'),b=document.querySelector('#device-farewell');row.hidden=!(L&&L.shown('depart'));root.classList.toggle('device-lifecycle',!!L&&!!manager.view('ble').lifecycle);
 for(const [el,kind] of [[b,'depart'],[document.querySelector('#adopt-egg'),'adopt']]){const why=L&&L.shown(kind)?L.blocker(kind):'';el.classList.toggle('is-blocked',!!why);el.setAttribute('aria-disabled',String(!!why));el.title=why;}}
document.querySelector('#device-farewell').onclick=()=>{const L=window.PetDeviceLifecycle,why=mode==='ble'&&L?L.blocker('depart'):'裝置模式才能從網頁送別電子雞上的寵物。';if(why){status.textContent=why;return;}
 document.querySelector('#farewell-description').textContent=L.farewellInfo();document.querySelector('#farewell-dialog').showModal();document.querySelector('#farewell-dialog form button').focus();};
function deviceLifecycle(kind){const L=window.PetDeviceLifecycle,why=L?L.blocker(kind):'這台電子雞的韌體還不支援。';if(why){status.textContent=why;return;}
 const run=L.run(kind);status.textContent=L.sendingText(kind);syncLifecycle();
 run.then(r=>{if(mode==='ble'){lifecycleHold=performance.now()+8000;notice(L.doneText(r));}},e=>{if(mode==='ble'){lifecycleHold=performance.now()+8000;notice(e.message);}}).finally(()=>{if(mode==='ble')syncLifecycle();});}
document.querySelector('#stage').value=stage;document.querySelector('#condition').value=condition;document.querySelector('#mode').value=mode;setNight(night);switchMode();PetLocalStore.report();requestAnimationFrame(draw);
})();
