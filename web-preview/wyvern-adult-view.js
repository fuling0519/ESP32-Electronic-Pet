/* Web-only illustrated adult art. Domain state and firmware masks stay separate. */
(()=>{'use strict';
const base='assets/pets/wyvern/adult/';
const images=new Map();
function image(name){
 if(!images.has(name)){const im=new Image();im.src=base+name;images.set(name,im);}
 return images.get(name);
}
// Shared scale/anchor within each animation: never fit each frame independently.
const poses={
 idle:{file:'idle-subtle-sheet-v2.png',cell:777,columns:2,frames:[0,1,2,3],times:[220,160,220,320],anchor:[388,659],scale:126/522},
 sad:{file:'emotions-sheet-v1.png',cell:470,columns:2,frames:[0,1],times:[2200,240],anchor:[234,444],scale:126/423},
 sick:{file:'emotions-sheet-v1.png',cell:470,columns:2,frames:[2,3],times:[1400,900],anchor:[234,444],scale:120/399},
 happy:{file:'emotions-sheet-v1.png',cell:470,columns:2,frames:[4,5],times:[1800,650],anchor:[234,444],scale:126/368},
 sleep:{files:['sleep-00.png','sleep-01.png'],times:[1100,900],rect:[0,0,887,640],anchor:[456,588],scale:180/832},
 dead:{file:'dead-still-v1.png',times:[1],rect:[0,0,1684,934],anchor:[848,856],scale:200/1592},
 // Scale matches idle's eye spacing; horn-to-feet would shrink this pose's head.
 shock:{file:'shock-still-v1.png',times:[1],rect:[0,0,1235,1025],anchor:[601,1000],scale:126/924},
 eat:{file:'eat-sheet-v2-clean.png',cell:767,columns:2,frames:[0,1,2,3],times:[375,375,375,375],anchor:[383,721],scale:126/508},
 flight:{file:'flight-sheet-v1.png',cellWidth:560,cellHeight:448,columns:3,frames:[0,1,2,3,4,5],times:[170,130,130,170,130,170],anchor:[280,310],scale:172/517}
};
for(const pose of Object.values(poses))for(const file of pose.files||[pose.file])image(file);
// Eating: three bites. Each bite = head down 450 ms, then chewing 400 ms; the bowl
// loses one step of food as the head comes up. Smile only after the bowl is empty.
const EAT={down:450,chew:400,bites:3};
function eatSample(ms,reduced=false){
 const biting=EAT.bites*(EAT.down+EAT.chew);ms=Math.max(0,ms);
 if(ms>=biting)return {frame:3,bowl:3};
 const k=Math.floor(ms/(EAT.down+EAT.chew)),down=ms%(EAT.down+EAT.chew)<EAT.down;
 return {frame:reduced?2:down?1:2,bowl:down?k:k+1};
}
let preview='auto',reaction=null,lastState='',stateStart=0;
let travelStart=null,lastNow=null,travelPet=null,lastTravel=null,returning=null,target=null,controlsEnabled=null;
function resetMotion(){travelStart=null;lastNow=null;travelPet=null;lastTravel=null;returning=null;target=null;}
const smooth=t=>{t=Math.max(0,Math.min(1,t));return t*t*(3-2*t);};
// Every leg eases to zero speed before facing changes. Body-center anchoring
// keeps both mirrored extents within the 384x256 scene, including wing tips.
function travelAt(elapsed,withRest=true){
 const rest=withRest?24000:0,cycle=rest+6480;
 let t=Math.max(0,elapsed)%cycle-rest;
 if(t<0)return {active:false,x:190,lift:0,facing:1,blend:0};
 if(t<600)return {active:true,x:190,lift:smooth(t/600),facing:1,blend:smooth(t/220)};
 t-=600;
 if(t<1200)return {active:true,x:190+80*smooth(t/1200),lift:1,facing:1,blend:1};
 t-=1200;
 if(t<240)return {active:true,x:270,lift:1,facing:t<120?1:-1,blend:1};
 t-=240;
 if(t<2400)return {active:true,x:270-160*smooth(t/2400),lift:1,facing:-1,blend:1};
 t-=2400;
 if(t<240)return {active:true,x:110,lift:1,facing:t<120?-1:1,blend:1};
 t-=240;
 if(t<1200)return {active:true,x:110+80*smooth(t/1200),lift:1,facing:1,blend:1};
 t-=1200;
 return {active:true,x:190,lift:1-smooth(t/600),facing:1,blend:1-smooth((t-380)/220)};
}
const controls=document.querySelector('.preview-controls');
let label,select,note;
if(controls){
 label=document.createElement('label');label.textContent='小龍素材 ';label.hidden=true;
 select=document.createElement('select');select.id='wyvern-art-state';select.setAttribute('aria-label','小龍素材預覽');
 for(const [value,text] of [['auto','跟隨寵物'],['idle','待機'],['sad','傷心'],['sick','生病'],['happy','開心'],['shock','驚嚇（被戳）'],['sleep','蜷縮睡眠'],['dead','死亡靜態'],['flight','低空飛行（六格循環）'],['eat','吃飯（四格）']]){
  const option=document.createElement('option');option.value=value;option.textContent=text;select.appendChild(option);
 }
 select.onchange=()=>{preview=select.value;lastState='';resetMotion();};label.appendChild(select);controls.appendChild(label);
 note=document.createElement('small');note.hidden=true;note.textContent='只切換外觀，不改養成數值。健康清醒待機時偶爾低空飛行，往左自動翻面。';controls.appendChild(note);
}
function eligible(p){return p&&p.stage==='adult'&&!p.isDeparted&&p.healthState!=='dead';}
function syncControls(p,enabled){
 if(label)label.hidden=!enabled;if(note)note.hidden=!enabled;
 if(controlsEnabled!==enabled||!eligible(p))resetMotion();controlsEnabled=enabled;
 if(!enabled){preview='auto';if(select)select.value='auto';reaction=null;}
}
document.addEventListener('pet-interaction-complete',e=>{
 const p=window.petDataManager?.read().pet;
 if(eligible(p)&&e.detail)reaction={kind:e.detail.kind,petId:p.petId,start:performance.now()};
});
function resolve(p,now,action){
 if(p.healthState==='dead')return 'dead';
 if(p.sleepMode==='normal')return 'sleep';
 if(p.healthState==='sick'||action==='treat')return 'sick';
 if(reaction&&(reaction.petId!==p.petId||now-reaction.start>=1200))reaction=null;
 if(reaction&&!action)return reaction.kind==='pet'?'happy':'shock';
 if(action==='feed')return 'eat';
 if(action==='clean')return 'idle';
 if(preview!=='auto'&&!action&&preview!=='flight')return preview;
 if(p.webExpression==='happy')return 'happy';
 if(window.PetSadView?.active(p)||PetRules.updateSad({...p,sadReasons:{...p.sadReasons}}))return 'sad';
 if(preview==='flight'&&!action)return 'flight';
 return 'idle';
}
function paint(ctx,state,elapsed=0,reduced=false,{x=190,y=177,size=1,facing=1,opacity=1,frame=null}={}){
 const pose=poses[state];if(!pose)return false;
 const cycle=pose.times.reduce((a,b)=>a+b,0);let phase=Math.max(0,elapsed)%cycle,index=0;
 if(frame!==null)index=Math.max(0,Math.min(pose.times.length-1,frame));
 else if(!reduced)while(index<pose.times.length-1&&phase>=pose.times[index])phase-=pose.times[index++];
 const im=image(pose.files?pose.files[index]:pose.file);
 if(!im.complete||!im.naturalWidth)return false;
 let rect=pose.rect;
 if(pose.cell||pose.cellWidth){const frame=pose.frames[index],cw=pose.cellWidth||pose.cell,ch=pose.cellHeight||pose.cell;rect=[frame%pose.columns*cw,Math.floor(frame/pose.columns)*ch,cw,ch];}
 const [sx,sy,w,h]=rect,scale=pose.scale*size;
 // Crop only at draw time; source files remain intact. Preserve each pose's proportions.
 ctx.save();ctx.imageSmoothingEnabled=false;
 ctx.globalAlpha=(ctx.globalAlpha??1)*opacity;
 if(facing<0){ctx.translate(x,y);ctx.scale(-1,1);ctx.drawImage(im,sx,sy,w,h,-pose.anchor[0]*scale,-pose.anchor[1]*scale,w*scale,h*scale);}
 else ctx.drawImage(im,sx,sy,w,h,x-pose.anchor[0]*scale,y-pose.anchor[1]*scale,w*scale,h*scale);
 ctx.restore();
 return {state,index};
}
function paintTravel(ctx,elapsed,reduced=false,{withRest=false}={}){
 if(reduced)return paint(ctx,'idle',0,true);
 const m=travelAt(elapsed,withRest),offset=-22*m.lift;
 // Stay on the ground while an image is loading; never launch a fallback pose.
 const im=image(poses.flight.file);
 if(!m.active||!im.complete||!im.naturalWidth)return paint(ctx,'idle',elapsed,false);
 if(m.blend<1)paint(ctx,'idle',elapsed,false,{opacity:1-m.blend});
 const result=paint(ctx,'flight',elapsed,false,{x:m.x,y:142+offset,facing:m.facing,opacity:m.blend});
 return {...result,x:m.x,offset,facing:m.facing};
}
function draw(ctx,p,now,action,elapsed,reduced){
 if(p.stage!=='adult'){resetMotion();return false;}
 if(travelPet!==p.petId||lastNow!==null&&(now<lastNow||now-lastNow>500))resetMotion();
 travelPet=p.petId;lastNow=now;
 const state=resolve(p,now,action),key=p.petId+':'+state;
 if(lastState!==key){lastState=key;stateStart=now;}
 const blocked=document.hidden||document.querySelector('dialog[open]')||window.PetGrowthView?.active;
 const flightArt=image(poses.flight.file),artReady=flightArt.complete&&flightArt.naturalWidth>0;
 const mayTravel=artReady&&!reduced&&!blocked&&!action&&!reaction&&eligible(p)&&p.healthState==='healthy'&&p.sleepMode==='awake'&&
  (state==='idle'&&preview==='auto'||state==='flight');
 if(mayTravel&&!returning){
  if(travelStart===null)travelStart=now;
  const result=paintTravel(ctx,now-travelStart,false,{withRest:preview!=='flight'});
  lastTravel=result?.state==='flight'?{x:result.x-190,y:result.offset}:null;
  target={petId:p.petId,x:result?.x??190,y:130+(result?.offset??0),rx:100,ry:90};
  return result;
 }
 if(travelStart!==null){
  if(lastTravel&&!reduced)returning={start:now,from:lastTravel};
  travelStart=null;lastTravel=null;
 }
 let dx=0,dy=0;
 if(reduced)returning=null;
 if(returning){const t=smooth((now-returning.start)/250);dx=returning.from.x*(1-t);dy=returning.from.y*(1-t);if(t===1)returning=null;}
 const startled=reaction?.kind==='poke'&&!action&&state==='shock'&&!reduced;
 const offset=startled?Math.max(0,1-(now-reaction.start)/1200)*Math.abs(Math.sin((now-reaction.start)/85))*5:0;
 target={petId:p.petId,x:190+dx,y:130+dy-offset,rx:100,ry:90};
 if(state==='eat'){
  // Feeding follows the action clock; the menu preview loops with a short pause on the smile.
  const ms=action==='feed'?elapsed*1000:(now-stateStart)%3600,s=eatSample(ms,reduced);
  const result=paint(ctx,'eat',0,reduced,{x:190+dx,y:177+dy-offset,frame:s.frame});
  return result&&{...result,bowlSeconds:s.bowl*.75+.01};
 }
 return paint(ctx,state==='flight'?'idle':state,now-stateStart,reduced,{x:190+dx,y:177+dy-offset});
}
document.addEventListener('visibilitychange',resetMotion);
window.WyvernAdultView={eatSample,paint,paintTravel,travelAt,draw,resolve,syncControls,
 interactionTarget(p){return target?.petId===p?.petId?{...target}:null;}};
})();
