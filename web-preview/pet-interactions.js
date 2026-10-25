/* Touch gestures and the status dialog for either source. Touch is a visual reaction only, like the firmware's
 * copper pad: it never changes needs, mood or EXP, so it needs no adapter access or BLE writes. */
(()=>{'use strict';
const scene=document.querySelector('.scene'),canvas=document.querySelector('#bird');
// The care dock floats over the garden; on narrow screens it would cover the pet, so it sits below the message bar instead.
const care=document.querySelector('.care'),narrow=matchMedia('(max-width:650px)');
function placeCare(){if(!care)return;if(narrow.matches)(scene.nextElementSibling?.id==='status'?scene.nextElementSibling:scene).after(care);else scene.append(care);}
placeCare();narrow.addEventListener?.('change',placeCare);
// Same split as the firmware's copper pad: a release before PETTING_MS is a poke, holding past it pets.
const PETTING_MS=700;
function classify(heldMs){return heldMs>=PETTING_MS?'pet':'poke';}
const overlay=document.createElement('canvas');overlay.width=384;overlay.height=256;overlay.className='touch-effects';overlay.setAttribute('aria-hidden','true');scene.append(overlay);const pen=overlay.getContext('2d');
let down=null,effect=null,holdTimer=null,lastFill=0,orbKey='';
const result=document.querySelector('#status'),petButton=document.querySelector('#touch-pet'),pokeButton=document.querySelector('#touch-poke');
// Care buttons are always disabled in device mode, so there only a growth transition counts as busy.
function allowed(){const s=petDataManager.read(),p=s.pet,busy=s.metadata.source==='mock'?document.querySelector('[data-action="feed"]').disabled:!!window.PetGrowthView?.active;return !!p&&['baby','adult'].includes(p.stage)&&p.healthState==='healthy'&&p.sleepMode==='awake'&&!p.isDeparted&&!busy&&!window.PetToyPlay?.active()&&!window.PetChorus?.active()&&!document.querySelector('#game-dialog').open&&!document.querySelector('#farewell-dialog').open;}
// The bird stands left of centre with its tail to the right, so its touch area and effects follow the drawn torso.
function birdAnchors(p){const at=shownSpecies(p)===1&&window.PetBirdPlacement?.();return at&&window.WhiteBirdView?.anchors?.(at.stage,at.cx,at.cy,3,'idle');}
function touchTarget(){const p=petDataManager.read().pet,a=birdAnchors(p);return a?{x:a.x,y:a.y,rx:100,ry:90}:window.WyvernAdultView?.interactionTarget(p)||{x:190,y:130,rx:100,ry:90};}
function hit(p){const t=touchTarget();return ((p.x-t.x)/t.rx)**2+((p.y-t.y)/t.ry)**2<=1;}
function point(e){const r=canvas.getBoundingClientRect();return {x:(e.clientX-r.left)*384/r.width,y:(e.clientY-r.top)*256/r.height};}
function fillStatus(){const p=petDataManager.read().pet;if(!p)return false;const species=p.speciesId===2?'小飛龍':'小鳥',stage=p.stage==='baby'?'幼年':p.stage==='adult'?'成年':'蛋',health=p.isDeparted?'已遠行':p.healthState==='dead'?'長眠':p.healthState==='sick'?'生病':'健康',sleep=p.sleepMode==='normal'?'睡眠':'清醒';const age=p.ageSeconds==null?'—':Math.floor(Number(p.ageSeconds)/86400)+' 天';const set=(id,value)=>{const el=document.querySelector(id);if(el)el.textContent=value;};const setMeter=(id,value)=>{const el=document.querySelector(id);if(el){const numeric=Number(value);el.value=Number.isFinite(numeric)?Math.max(0,Math.min(100,numeric)):0;}};set('#status-pet-name',p.name||'未命名');set('#status-pet-health',health);set('#status-pet-stage',stage);set('#status-pet-sleep',sleep);set('#status-pet-species',species);set('#status-age',age);set('#status-satiety',p.satiety??'—');setMeter('#status-satiety-progress',p.satiety);set('#status-mood',p.mood??'—');setMeter('#status-mood-progress',p.mood);set('#status-cleanliness',p.cleanliness??'—');setMeter('#status-cleanliness-progress',p.cleanliness);const expMax=Number(p.expToNextLevel),expValue=Number(p.exp);const hasExp=Number.isFinite(expMax)&&expMax>0&&Number.isFinite(expValue);const expPercent=hasExp?Math.max(0,Math.min(100,expValue/expMax*100)):0;set('#status-exp',p.expToNextLevel===0?'MAX':(p.exp==null?'—':p.exp+' / '+p.expToNextLevel));const expProgress=document.querySelector('#status-exp-progress');if(expProgress){expProgress.max=100;expProgress.value=expPercent;expProgress.setAttribute('aria-label',hasExp?'經驗值 '+p.exp+' / '+p.expToNextLevel:'經驗值');}const image=document.querySelector('#status-pet-image');if(image){renderPortrait(image,p);image.setAttribute('aria-label',species+' '+stage);}return true;}
// Portraits are drawn from the live sprites (species and stage), then cropped to the
// character's visible pixels and centred, so no art file's padding leaves it off-centre.
// Measured with visibleBox() on http; file:// pages cannot read canvas pixels back, so they use these.
const PORTRAIT_BOXES={'1:adult':[279,129,248,247],'1:baby':[299,153,199,199],'2:adult':[240,102,279,253],'2:baby':[300,170,160,184],egg:[316,232,128,168]};
function visibleBox(a){let px;try{px=a.getImageData(0,0,768,512).data;}catch{return null;}
 let x0=768,y0=512,x1=-1,y1=-1;
 for(let y=0;y<512;y++)for(let x=0;x<768;x++)if(px[(y*768+x)*4+3]>16){if(x<x0)x0=x;if(x>x1)x1=x;if(y<y0)y0=y;if(y>y1)y1=y;}
 return x1<0?null:[x0,y0,x1-x0+1,y1-y0+1];}
function renderPortrait(target,p){
 const art=document.createElement('canvas');art.width=768;art.height=512;const a=art.getContext('2d');a.scale(2,2);
 let drawn=false;
 if(p.stage==='egg')drawn=!!window.PetEggView&&(window.PetEggView.draw(a,0,0,true,false,p.petId),true);
 else if(p.speciesId===2)drawn=!!window.WyvernAdultView?.paint(a,'idle',0,true,{stage:p.stage});
 else drawn=!!window.BirdView?.draw(a,{stage:p.stage,healthState:'healthy',sleepMode:'awake',satiety:100,mood:80,cleanliness:100},0,null,0,false,190,126,3);
 if(!drawn)return false;
 const box=visibleBox(a)||PORTRAIT_BOXES[p.stage==='egg'?'egg':p.speciesId+':'+p.stage];if(!box)return false;
 const [x0,y0,w,h]=box,pad=.1,fit=Math.min(target.width*(1-2*pad)/w,target.height*(1-2*pad)/h),t=target.getContext('2d');
 t.clearRect(0,0,target.width,target.height);t.imageSmoothingEnabled=true;t.imageSmoothingQuality='high';
 t.drawImage(art,x0,y0,w,h,(target.width-w*fit)/2,(target.height-h*fit)/2,w*fit,h*fit);return true;}
function portraitKey(p){return p.petId+':'+p.speciesId+':'+p.stage;}
// The status card is a light-dismiss popover: the orb toggles it, a click elsewhere or Esc closes it.
const statusButton=document.querySelector('#open-status'),statusCard=document.querySelector('#pet-status-card'),orbImage=document.querySelector('#status-orb-image');
// Page coordinates, so the card stays under the orb while the page scrolls.
function placeCard(){const r=statusButton.getBoundingClientRect(),w=statusCard.offsetWidth,gap=8;
 statusCard.style.left=scrollX+Math.max(gap,Math.min(r.left,document.documentElement.clientWidth-w-gap))+'px';statusCard.style.top=scrollY+r.bottom+gap+'px';}
statusCard?.addEventListener('beforetoggle',e=>{if(e.newState==='open'&&!fillStatus())e.preventDefault();});
statusCard?.addEventListener('toggle',e=>{if(e.newState==='open')placeCard();});
document.querySelector('#open-cemetery')?.addEventListener('click',()=>statusCard.hidePopover?.());
window.addEventListener('resize',()=>{if(statusCard?.matches?.(':popover-open'))placeCard();});
function interact(kind){if(!allowed()){result.textContent='目前不能觸碰：請在寵物清醒健康且動作結束後互動。';return false;}
 const now=performance.now(),petId=petDataManager.read().pet.petId;
 // A held pat already has hearts running; keep them instead of restarting.
 if(kind!=='pet'||!heartsAlive(now))effect={kind,start:now,until:kind==='pet'?1200:Infinity,petId};result.textContent=kind==='pet'?'牠開心地接受摸摸，喜歡你的陪伴！':'牠嚇了一跳，輕一點啦！';document.dispatchEvent(new CustomEvent('pet-interaction-complete',{detail:{kind}}));return true;}
petButton.onclick=()=>interact('pet');pokeButton.onclick=()=>interact('poke');

function startPetting(){holdTimer=null;if(!down)return;const now=performance.now();
 // Hearts start with the hold and stay until release, like the firmware's PettingStarted.
 effect={kind:'pet',start:now,until:Infinity,petId:petDataManager.read().pet.petId};
 if(!interact('pet')){effect=null;down=null;return;}down.petting=true;
 document.dispatchEvent(new CustomEvent('pet-touch-hold',{detail:{holding:true}}));}
function endTouch(){clearTimeout(holdTimer);holdTimer=null;const was=down;down=null;
 if(was?.petting){releaseHearts();document.dispatchEvent(new CustomEvent('pet-touch-hold',{detail:{holding:false}}));}return was;}
canvas.addEventListener('pointerdown',e=>{if(e.button!==0||!allowed())return;const p=point(e);if(!hit(p))return;down={...p,id:e.pointerId,at:performance.now(),petting:false};e.preventDefault();clearTimeout(holdTimer);holdTimer=setTimeout(startPetting,PETTING_MS);try{canvas.setPointerCapture(e.pointerId);}catch{}});
canvas.addEventListener('pointermove',e=>{const p=point(e);canvas.style.cursor=allowed()&&hit(p)?'pointer':'';});
canvas.addEventListener('pointerup',e=>{if(!down||e.pointerId!==down.id)return;const held=performance.now()-down.at,was=endTouch();if(!was.petting&&classify(held)==='poke')interact('poke');});
canvas.addEventListener('pointercancel',endTouch);
// A long press on phones would otherwise open the context menu and cancel the pat.
canvas.addEventListener('contextmenu',e=>{if(allowed())e.preventDefault();});
const reduced=matchMedia('(prefers-reduced-motion: reduce)');
// Head centre and heart spread per role; adult wyvern follows its live (flight) target. The bird's head comes from
// its measured eye line (birdAnchors), so its dx/dy only apply when no placement is known.
const HEADS={'2:adult':{dx:0,dy:-22,spread:56,bangDy:-48},'2:baby':{dx:0,dy:-18,spread:46,bangDy:-40},'1:adult':{dx:-10,dy:-14,spread:62,bangDy:-44},'1:baby':{dx:-10,dy:-8,spread:54,bangDy:-36}};
// In demo mode the species picker or ?preview=wyvern|bird overrides the shown species (as in app.js).
function shownSpecies(p){const picked=window.PetArtStage?.species?.(p);if(picked)return picked;const m=/(?:[?&])preview=(wyvern|bird)(?:&|$)/.exec(location.search||'');return m?(m[1]==='wyvern'?2:1):p.speciesId;}
// Reactions draw the bird at the centre (its walk pauses), so anchor to the touch target.
function head(p){const o=HEADS[shownSpecies(p)+':'+(window.PetArtStage?.shown(p)||p.stage)]||HEADS['2:adult'],a=birdAnchors(p);if(a)return {x:a.headX,y:a.headY,spread:o.spread,bangDy:o.bangDy};const t=touchTarget();return {x:t.x+o.dx,y:t.y+o.dy,spread:o.spread,bangDy:o.bangDy};}
function heartsAlive(now){return effect?.kind==='pet'&&now-effect.start<effect.until+PetEffects.HEART_LIFE;}
function releaseHearts(){if(effect?.kind==='pet'&&effect.until===Infinity)effect.until=performance.now()-effect.start;}
// The emote bubble steps aside while a touch reaction plays, so its icon never sits under the hearts or the exclamation.
function reacting(now=performance.now()){if(down?.petting)return true;if(!effect)return false;const ms=now-effect.start;return ms>=0&&ms<(effect.kind==='pet'?effect.until+PetEffects.HEART_LIFE:PetEffects.SHOCK_MS);}
function draw(now){pen.clearRect(0,0,384,256);petButton.disabled=pokeButton.disabled=!allowed();const shown=petDataManager.read().pet;statusButton.hidden=!shown;if(shown){const key=portraitKey(shown);if(orbKey!==key&&renderPortrait(orbImage,shown))orbKey=key;if(statusCard.matches?.(':popover-open')&&now-lastFill>500){lastFill=now;fillStatus();}}else statusCard.hidePopover?.();
 if(effect){const p=petDataManager.read().pet,ms=now-effect.start,end=effect.kind==='pet'?effect.until+PetEffects.HEART_LIFE:PetEffects.SHOCK_MS;
 if(ms>=end||!p||p.petId!==effect.petId||p.healthState==='dead'||p.isDeparted)effect=null;else{const h=head(p),r=reduced.matches;
 if(effect.kind==='pet')PetEffects.hearts(pen,ms,{x:h.x,y:h.y,spread:h.spread,until:effect.until,reduced:r});
 else PetEffects.shock(pen,ms,{x:h.x+h.spread-6,y:h.y+h.bangDy,reduced:r});}}
 requestAnimationFrame(draw);}
requestAnimationFrame(draw);
window.PetTouch={PETTING_MS,classify,reacting};
window.PetPortrait={render:renderPortrait,boxes:PORTRAIT_BOXES};
})();
