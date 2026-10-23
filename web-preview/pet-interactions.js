/* Mock-only gestures. No raw adapter access or BLE writes. */
(()=>{'use strict';
const scene=document.querySelector('.scene'),canvas=document.querySelector('#bird');
const care=document.querySelector('.care');if(care){scene.append(care);const feed=care.querySelector('[data-action="feed"]');if(feed){feed.setAttribute('aria-label','餵食');feed.innerHTML='<span class="bowl-icon" aria-hidden="true"></span>';}}
const panel=document.createElement('section');panel.className='pet-interactions';panel.setAttribute('aria-label','寵物觸碰互動');
panel.innerHTML='<strong>觸碰互動</strong><button type="button" id="tool-pet" aria-pressed="true">✋ 摸頭／順毛</button><button type="button" id="tool-poke" aria-pressed="false">╱ 樹枝戳戳</button><button type="button" id="touch-once">互動一下</button><p id="touch-help">選工具後輕點角色，或在頭上滑一下順毛。每次心情＋3／－3，間隔3秒，不給EXP。</p><p id="touch-result" role="status" aria-live="polite"></p>';
scene.after(panel);
const overlay=document.createElement('canvas');overlay.width=384;overlay.height=256;overlay.className='touch-effects';overlay.setAttribute('aria-hidden','true');scene.append(overlay);const pen=overlay.getContext('2d');
let tool='pet',down=null,effect=null,hover=null,lastTapAt=0,tapTimer=null;
const result=document.querySelector('#touch-result'),petButton=document.querySelector('#tool-pet'),pokeButton=document.querySelector('#tool-poke'),once=document.querySelector('#touch-once');
const statusDialog=document.createElement('dialog');
statusDialog.id='pet-status-dialog';
statusDialog.innerHTML='<h2>寵物狀態</h2><div class="pet-status-card"><div class="pet-status-portrait"><img id="status-pet-image" alt="目前寵物"></div><div class="pet-status-main"><strong id="status-pet-name"></strong><span>健康：<b id="status-pet-health"></b></span><span>成長狀態：<b id="status-pet-stage"></b></span><span>清醒狀態：<b id="status-pet-sleep"></b></span><span>種類：<b id="status-pet-species"></b></span><span>年齡：<b id="status-age"></b></span></div></div><div class="pet-status-grid"><span>飽食度</span><div class="status-meter-value"><strong id="status-satiety"></strong><progress id="status-satiety-progress" max="100" value="0"></progress></div><span>心情</span><div class="status-meter-value"><strong id="status-mood"></strong><progress id="status-mood-progress" max="100" value="0"></progress></div><span>清潔度</span><div class="status-meter-value"><strong id="status-cleanliness"></strong><progress id="status-cleanliness-progress" max="100" value="0"></progress></div><span>經驗值</span><div class="status-exp-value"><strong id="status-exp"></strong><progress id="status-exp-progress" max="100" value="0"></progress></div></div><form method="dialog"><button>關閉</button></form>';
document.body.append(statusDialog);
const statusShowModal=statusDialog.showModal.bind(statusDialog);statusDialog.showModal=()=>{const image=document.querySelector('#status-pet-image');if(image&&image.src.includes('white-idle-source-v4.png'))image.src='assets/pets/bird/white-idle-v2.png';statusShowModal();};
function select(value){tool=value;petButton.setAttribute('aria-pressed',String(tool==='pet'));pokeButton.setAttribute('aria-pressed',String(tool==='poke'));canvas.style.cursor=tool==='pet'?'grab':'crosshair';}
petButton.onclick=()=>select('pet');pokeButton.onclick=()=>select('poke');select('pet');
function allowed(){const s=petDataManager.read(),p=s.pet;return s.metadata.source==='mock'&&p&&['baby','adult'].includes(p.stage)&&p.healthState==='healthy'&&p.sleepMode==='awake'&&!p.isDeparted&&!document.querySelector('[data-action="feed"]').disabled&&!document.querySelector('#game-dialog').open&&!document.querySelector('#farewell-dialog').open;}
function touchTarget(){return window.WyvernAdultView?.interactionTarget(petDataManager.read().pet)||{x:190,y:130,rx:100,ry:90};}
function hit(p){const t=touchTarget();return ((p.x-t.x)/t.rx)**2+((p.y-t.y)/t.ry)**2<=1;}
function point(e){const r=canvas.getBoundingClientRect();return {x:(e.clientX-r.left)*384/r.width,y:(e.clientY-r.top)*256/r.height};}
function openStatus(){const p=petDataManager.read().pet;if(!p)return;const species=p.speciesId===2?'小飛龍':'小鳥',stage=p.stage==='baby'?'幼年':p.stage==='adult'?'成年':'蛋',health=p.isDeparted?'已遠行':p.healthState==='dead'?'長眠':p.healthState==='sick'?'生病':'健康',sleep=p.sleepMode==='normal'?'睡眠':'清醒';const age=p.ageSeconds==null?'—':Math.floor(Number(p.ageSeconds)/86400)+' 天';const set=(id,value)=>{const el=document.querySelector(id);if(el)el.textContent=value;};const setMeter=(id,value)=>{const el=document.querySelector(id);if(el){const numeric=Number(value);el.value=Number.isFinite(numeric)?Math.max(0,Math.min(100,numeric)):0;}};set('#status-pet-name',p.name||'未命名');set('#status-pet-health',health);set('#status-pet-stage',stage);set('#status-pet-sleep',sleep);set('#status-pet-species',species);set('#status-age',age);set('#status-satiety',p.satiety??'—');setMeter('#status-satiety-progress',p.satiety);set('#status-mood',p.mood??'—');setMeter('#status-mood-progress',p.mood);set('#status-cleanliness',p.cleanliness??'—');setMeter('#status-cleanliness-progress',p.cleanliness);const expMax=Number(p.expToNextLevel),expValue=Number(p.exp);const hasExp=Number.isFinite(expMax)&&expMax>0&&Number.isFinite(expValue);const expPercent=hasExp?Math.max(0,Math.min(100,expValue/expMax*100)):0;set('#status-exp',p.expToNextLevel===0?'MAX':(p.exp==null?'—':p.exp+' / '+p.expToNextLevel));const expProgress=document.querySelector('#status-exp-progress');if(expProgress){expProgress.max=100;expProgress.value=expPercent;expProgress.setAttribute('aria-label',hasExp?'經驗值 '+p.exp+' / '+p.expToNextLevel:'經驗值');}const image=document.querySelector('#status-pet-image');if(image){image.src=p.speciesId===2?'assets/pets/wyvern/adult/idle-preview-v1.png':'assets/pets/bird/white-idle-source-v4.png';image.alt=species+' '+stage;}statusDialog.showModal();}
function interact(at=null){if(!allowed()){result.textContent='目前不能觸碰：請在展示模式、清醒健康且動作結束後互動。';return;}if(!at){const t=touchTarget();at={x:t.x,y:t.y-20};}
 try{const r=petDataManager.command(tool),now=performance.now(),petId=petDataManager.read().pet.petId;
  // A held pat already has hearts running; keep them instead of restarting.
  if(tool!=='pet'||!heartsAlive(now))effect={kind:tool,start:now,until:tool==='pet'?1200:Infinity,petId};result.textContent=(tool==='pet'?'牠開心地接受摸摸，喜歡你的陪伴！':'牠嚇了一跳，輕一點啦！')+' 心情 '+(r.actual>=0?'+':'')+r.actual+'。';document.dispatchEvent(new CustomEvent('pet-interaction-complete',{detail:{kind:tool}}));}catch(e){result.textContent=e.message;}}
once.onclick=()=>interact();
canvas.addEventListener('pointerdown',e=>{if(e.button!==0||!allowed())return;const p=point(e);if(!hit(p))return;down={...p,id:e.pointerId};canvas.setPointerCapture(e.pointerId);e.preventDefault();if(tool==='pet'){effect={kind:'pet',start:performance.now(),until:Infinity,petId:petDataManager.read().pet.petId};document.dispatchEvent(new CustomEvent('pet-touch-hold',{detail:{holding:true}}));}});
canvas.addEventListener('pointermove',e=>{hover=point(e);});
canvas.addEventListener('pointerup',e=>{if(!down||e.pointerId!==down.id)return;const p=point(e);down=null;releaseHearts();document.dispatchEvent(new CustomEvent('pet-touch-hold',{detail:{holding:false}}));if(!hit(p))return;const now=performance.now();if(now-lastTapAt<380){clearTimeout(tapTimer);tapTimer=null;lastTapAt=0;openStatus();return;}lastTapAt=now;clearTimeout(tapTimer);tapTimer=setTimeout(()=>{tapTimer=null;interact(p);},380);});
canvas.addEventListener('pointercancel',()=>{down=null;releaseHearts();document.dispatchEvent(new CustomEvent('pet-touch-hold',{detail:{holding:false}}));});canvas.addEventListener('pointerleave',()=>{hover=null;});
const reduced=matchMedia('(prefers-reduced-motion: reduce)');
// Head centre and heart spread per role; adult wyvern follows its live (flight) target.
const HEADS={'2:adult':{dx:0,dy:-22,spread:56,bangDy:-48},'2:baby':{dx:0,dy:-18,spread:46,bangDy:-40},'1:adult':{dx:-10,dy:-14,spread:62,bangDy:-44},'1:baby':{dx:-10,dy:-8,spread:54,bangDy:-36}};
// Touch is mock-only, where ?preview=wyvern|bird overrides the shown species (as in app.js).
function shownSpecies(p){const m=/(?:[?&])preview=(wyvern|bird)(?:&|$)/.exec(location.search||'');return m?(m[1]==='wyvern'?2:1):p.speciesId;}
// Reactions draw the bird at the centre (its walk pauses), so anchor to the touch target.
function head(p){const t=touchTarget(),o=HEADS[shownSpecies(p)+':'+(window.PetArtStage?.shown(p)||p.stage)]||HEADS['2:adult'];return {x:t.x+o.dx,y:t.y+o.dy,spread:o.spread,bangDy:o.bangDy};}
function heartsAlive(now){return effect?.kind==='pet'&&now-effect.start<effect.until+PetEffects.HEART_LIFE;}
function releaseHearts(){if(effect?.kind==='pet'&&effect.until===Infinity)effect.until=performance.now()-effect.start;}
function draw(now){pen.clearRect(0,0,384,256);once.disabled=!allowed();
 if(hover&&tool==='poke'&&allowed()){pen.strokeStyle='#795033';pen.lineWidth=3;pen.beginPath();pen.moveTo(hover.x+3,hover.y+3);pen.lineTo(hover.x+28,hover.y+32);pen.stroke();}
 if(effect){const p=petDataManager.read().pet,ms=now-effect.start,end=effect.kind==='pet'?effect.until+PetEffects.HEART_LIFE:PetEffects.SHOCK_MS;
 if(ms>=end||!p||p.petId!==effect.petId||p.healthState==='dead'||p.isDeparted)effect=null;else{const h=head(p),r=reduced.matches;
 if(effect.kind==='pet')PetEffects.hearts(pen,ms,{x:h.x,y:h.y,spread:h.spread,until:effect.until,reduced:r});
 else PetEffects.shock(pen,ms,{x:h.x+h.spread-6,y:h.y+h.bangDy,reduced:r});}}
 requestAnimationFrame(draw);}
requestAnimationFrame(draw);
})();
