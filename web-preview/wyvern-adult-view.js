/* Web-only illustrated wyvern art (adult and baby). Domain state and firmware masks stay separate. */
(()=>{'use strict';
const images=new Map();
function image(name){
 if(!images.has(name)){const im=new Image();im.src='assets/pets/wyvern/'+name;images.set(name,im);}
 return images.get(name);
}
// Shared scale/anchor within each animation: never fit each frame independently.
const adultPoses={
 idle:{file:'idle-subtle-sheet-v2.png',cell:777,columns:2,frames:[0,1,2,3],times:[220,160,220,320],anchor:[388,659],scale:126/522},
 sad:{file:'emotions-sheet-v1.png',cell:470,columns:2,frames:[0,1],times:[2200,240],anchor:[234,444],scale:126/423},
 sick:{file:'emotions-sheet-v1.png',cell:470,columns:2,frames:[2,3],times:[1400,900],anchor:[234,444],scale:120/399},
 // Happy art has a ~16% wider face than idle at equal height; squeeze x to idle's eye spacing.
 happy:{file:'emotions-sheet-v1.png',cell:470,columns:2,frames:[4,5],times:[1800,650],anchor:[234,444],scale:126/368,scaleX:0.86},
 // Matches idle's eye spacing (closed-eye arcs); 180/832 drew the head ~19% too big.
 sleep:{files:['sleep-00.png','sleep-01.png'],times:[1100,900],rect:[0,0,887,640],anchor:[456,588],scale:151/832},
 dead:{file:'dead-still-v1.png',times:[1],rect:[0,0,1684,934],anchor:[848,856],scale:200/1592},
 // Scale matches idle's eye spacing; horn-to-feet would shrink this pose's head.
 shock:{file:'shock-still-v1.png',times:[1],rect:[0,0,1235,1025],anchor:[601,1000],scale:126/924},
 eat:{file:'eat-sheet-v2-clean.png',cell:767,columns:2,frames:[0,1,2,3],times:[375,375,375,375],anchor:[383,721],scale:126/508},
 flight:{file:'flight-sheet-v1.png',cellWidth:560,cellHeight:448,columns:3,frames:[0,1,2,3,4,5],times:[170,130,130,170,130,170],anchor:[280,310],scale:172/517}
};
// Baby: ~73% of adult height (92 px). Each generated sheet drew the baby at its
// own size, so B and C scales are matched to sheet A by head width (B's head is
// ~5% narrower than A; C's walk frame head is 295 px against B's 220).
const BABY_A=92/296,BABY_B=BABY_A*231/219,BABY_C=BABY_B*220/295;
const babyA={file:'sheet-a-v2.png',cell:334,columns:4,anchor:[167,318],scale:BABY_A};
const babyB={file:'sheet-b-v2.png',cell:341,columns:4,anchor:[170,324],scale:BABY_B};
// Sleep/dead (front-view poses) matched to A by eye spacing: 107 vs 193.8 px.
const BABY_V5=BABY_A*107/193.8;
const babyV5={file:'sleep-dead-v5.png',cell:891,columns:2,anchor:[445,847],scale:BABY_V5};
const babyC={file:'sheet-c-v1.png',cell:816,columns:2,anchor:[408,776],scale:BABY_C};
const babyPoses={
 idle:{...babyA,frames:[0,1,2,3],times:[240,180,240,340]},
 sad:{...babyA,frames:[4,5],times:[2200,240]},
 sick:{...babyA,frames:[6,7],times:[1400,900]},
 happy:{...babyA,frames:[8,9],times:[1800,650]},
 // Same frame roles as adult: stand, bite, chew, smile (the happy squint).
 eat:{...babyA,frames:[0,10,11,9],times:[375,375,375,375]},
 shock:{...babyB,frames:[4],times:[1]},
 // B's third frame lifted the same foot as the first; C supplies the other foot.
 walk:{...babyB,frames:[0,1,{...babyC,frame:0},3],times:[200,160,200,160]},
 // Baby dozes sitting up and lies flopped flat when dead: lying/side poses kept
 // drifting toward the adult design. One still frame each; Zzz shows it asleep.
 // Mirrored so the head tilts to the viewer's right, as the user preferred.
 sleep:{...babyV5,frames:[0],times:[1],flip:true},
 dead:{...babyV5,frames:[1],times:[1]}
};
// A frame may be an object that borrows another sheet's file, cell and scale.
for(const [stage,set] of [['adult',adultPoses],['baby',babyPoses]])for(const pose of Object.values(set))for(const part of [pose,...(pose.frames||[]).filter(f=>typeof f==='object')])part.files=(part.files||[part.file]).map(f=>stage+'/'+f);
const POSES={adult:adultPoses,baby:babyPoses},poses=adultPoses;
for(const set of Object.values(POSES))for(const pose of Object.values(set))for(const part of [pose,...(pose.frames||[]).filter(f=>typeof f==='object')])for(const file of part.files)image(file);
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
// Baby walks on the ground instead of flying: slower, shorter route, same rest.
function walkAt(elapsed,withRest=true){
 const rest=withRest?24000:0,cycle=rest+9840;
 let t=Math.max(0,elapsed)%cycle-rest;
 if(t<0)return {active:false,x:190,facing:1};
 if(t<2400)return {active:true,x:190+60*smooth(t/2400),facing:1};
 t-=2400;
 if(t<240)return {active:true,x:250,facing:t<120?1:-1,still:true};
 t-=240;
 if(t<4800)return {active:true,x:250-120*smooth(t/4800),facing:-1};
 t-=4800;
 if(t<240)return {active:true,x:130,facing:t<120?-1:1,still:true};
 t-=240;
 return {active:true,x:130+60*smooth(t/2160),facing:1};
}
// The page's demo tools pick the art state; 'flight' means walking for the baby.
const PREVIEW_STATES=['auto','idle','sad','sick','happy','shock','sleep','dead','flight','eat'];
function setPreview(state){
 const next=PREVIEW_STATES.includes(state)?state:'auto';
 if(next!==preview){preview=next;lastState='';resetMotion();}
}
function eligible(p){return p&&['adult','baby'].includes(p.stage)&&!p.isDeparted&&p.healthState!=='dead';}
// Touch reactions also play in device mode, so they have their own switch (defaulting to the demo one).
function syncControls(p,enabled,touch=enabled){
 if(controlsEnabled!==enabled||!eligible(p))resetMotion();controlsEnabled=enabled;
 if(!enabled)preview='auto';if(!touch)reaction=null;
}
// Head pat smiles from pointer-down; release restarts the 1.2 s window so the
// face stays happy through the tap delay until the interaction completes.
document.addEventListener('pet-touch-hold',e=>{
 const p=window.petDataManager?.read().pet;
 if(e.detail?.holding){if(eligible(p))reaction={kind:'pet',petId:p.petId,start:performance.now(),holding:true};}
 else if(reaction?.holding)reaction={...reaction,start:performance.now(),holding:false};
});
document.addEventListener('pet-interaction-complete',e=>{
 const p=window.petDataManager?.read().pet;
 if(eligible(p)&&e.detail)reaction={kind:e.detail.kind,petId:p.petId,start:performance.now()};
});
function resolve(p,now,action){
 if(p.healthState==='dead')return 'dead';
 if(p.sleepMode==='normal')return 'sleep';
 if(p.healthState==='sick'||action==='treat')return 'sick';
 if(reaction&&(reaction.petId!==p.petId||(!reaction.holding&&now-reaction.start>=1200)))reaction=null;
 if(reaction&&!action)return reaction.kind==='pet'?'happy':'shock';
 if(action==='feed')return 'eat';
 if(action==='clean')return 'idle';
 if(preview!=='auto'&&!action&&preview!=='flight')return preview;
 if(p.webExpression==='happy')return 'happy';
 if(window.PetSadView?.active(p)||PetRules.updateSad({...p,sadReasons:{...p.sadReasons}}))return 'sad';
 if(preview==='flight'&&!action)return 'flight';
 return 'idle';
}
function paint(ctx,state,elapsed=0,reduced=false,{x=190,y=177,size=1,facing=1,opacity=1,frame=null,stage='adult',stretch=1}={}){
 const pose=POSES[stage]?.[state];if(!pose)return false;
 const cycle=pose.times.reduce((a,b)=>a+b,0);let phase=Math.max(0,elapsed)%cycle,index=0;
 if(frame!==null)index=Math.max(0,Math.min(pose.times.length-1,frame));
 else if(!reduced)while(index<pose.times.length-1&&phase>=pose.times[index])phase-=pose.times[index++];
 const part=pose.frames?.[index],spec=typeof part==='object'?{...pose,...part}:pose,cellIndex=typeof part==='object'?part.frame:part;
 const im=image(spec.files[spec.files.length>1?index:0]);
 if(!im.complete||!im.naturalWidth)return false;
 let rect=spec.rect;
 if(spec.cell||spec.cellWidth){const cw=spec.cellWidth||spec.cell,ch=spec.cellHeight||spec.cell;rect=[cellIndex%spec.columns*cw,Math.floor(cellIndex/spec.columns)*ch,cw,ch];}
 const [sx,sy,w,h]=rect,scale=spec.scale*size,scaleX=scale*(spec.scaleX||1);
 if(spec.flip)facing=-facing;
 // Crop only at draw time; source files remain intact. Preserve each pose's proportions.
 ctx.save();ctx.imageSmoothingEnabled=false;
 ctx.globalAlpha=(ctx.globalAlpha??1)*opacity;
 // Mirror and the sleeping breath (stretch) both pivot on the anchor at the feet.
 if(facing<0||stretch!==1){ctx.translate(x,y);ctx.scale(facing<0?-1:1,stretch);ctx.drawImage(im,sx,sy,w,h,-spec.anchor[0]*scaleX,-spec.anchor[1]*scale,w*scaleX,h*scale);}
 else ctx.drawImage(im,sx,sy,w,h,x-spec.anchor[0]*scaleX,y-spec.anchor[1]*scale,w*scaleX,h*scale);
 ctx.restore();
 return {state,index};
}
function paintWalk(ctx,elapsed,reduced=false,{withRest=false}={}){
 const m=walkAt(elapsed,withRest);
 if(reduced||!m.active)return paint(ctx,'idle',elapsed,reduced,{stage:'baby'});
 // Rest the legs on a standing frame while turning around.
 const result=paint(ctx,'walk',elapsed,false,{stage:'baby',x:m.x,facing:m.facing,frame:m.still?1:null});
 return result&&{...result,state:'flight',x:m.x,offset:0,facing:m.facing};
}
function paintTravel(ctx,elapsed,reduced=false,{withRest=false,stage='adult'}={}){
 if(stage==='baby')return paintWalk(ctx,elapsed,reduced,{withRest});
 if(reduced)return paint(ctx,'idle',0,true);
 const m=travelAt(elapsed,withRest),offset=-22*m.lift;
 // Stay on the ground while an image is loading; never launch a fallback pose.
 const im=image(poses.flight.files[0]);
 if(!m.active||!im.complete||!im.naturalWidth)return paint(ctx,'idle',elapsed,false);
 if(m.blend<1)paint(ctx,'idle',elapsed,false,{opacity:1-m.blend});
 const result=paint(ctx,'flight',elapsed,false,{x:m.x,y:142+offset,facing:m.facing,opacity:m.blend});
 return {...result,x:m.x,offset,facing:m.facing};
}
function draw(ctx,p,now,action,elapsed,reduced){
 const stage=p.stage,set=POSES[stage];
 if(!set){resetMotion();return false;}
 const motionKey=p.petId+':'+stage;
 if(travelPet!==motionKey||lastNow!==null&&(now<lastNow||now-lastNow>500))resetMotion();
 travelPet=motionKey;lastNow=now;
 const state=resolve(p,now,action),key=motionKey+':'+state;
 if(!set[state==='flight'?'idle':state]){resetMotion();return false;}
 const baby=stage==='baby',groundY=baby?131:130,rx=baby?62:100,ry=baby?56:90;
 if(lastState!==key){lastState=key;stateStart=now;}
 const blocked=document.hidden||document.querySelector('dialog[open]')||window.PetGrowthView?.active;
 const moveArt=image((baby?set.walk:set.flight).files[0]),artReady=moveArt.complete&&moveArt.naturalWidth>0;
 const mayTravel=artReady&&!reduced&&!blocked&&!action&&!reaction&&eligible(p)&&p.healthState==='healthy'&&p.sleepMode==='awake'&&
  (state==='idle'&&preview==='auto'||state==='flight');
 if(mayTravel&&!returning){
  if(travelStart===null)travelStart=now;
  const result=paintTravel(ctx,now-travelStart,false,{withRest:preview!=='flight',stage});
  lastTravel=result?.state==='flight'?{x:result.x-190,y:result.offset}:null;
  target={petId:p.petId,x:result?.x??190,y:groundY+(result?.offset??0),rx,ry};
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
 target={petId:p.petId,x:190+dx,y:groundY+dy-offset,rx,ry};
 if(state==='eat'){
  // Feeding follows the action clock; the menu preview loops with a short pause on the smile.
  const ms=action==='feed'?elapsed*1000:(now-stateStart)%3600,s=eatSample(ms,reduced);
  const result=paint(ctx,'eat',0,reduced,{x:190+dx,y:177+dy-offset,frame:s.frame,stage});
  return result&&{...result,bowlSeconds:s.bowl*.75+.01};
 }
 // A head pat holds the squint smile; happy's open-eye frame reads almost like idle.
 const smile=reaction?.kind==='pet'&&!action&&state==='happy'?1:null;
 const stretch=state==='sleep'?window.PetEffects?.breath?.(now,reduced)??1:1;
 return paint(ctx,state==='flight'?'idle':state,now-stateStart,reduced,{x:190+dx,y:177+dy-offset,frame:smile,stage,stretch});
}
document.addEventListener('visibilitychange',resetMotion);
window.WyvernAdultView={eatSample,paint,paintTravel,travelAt,walkAt,draw,resolve,syncControls,setPreview,
 interactionTarget(p){return target?.petId===p?.petId?{...target}:null;}};
})();
