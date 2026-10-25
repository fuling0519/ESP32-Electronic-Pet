/* Shared cleaning-art player. Preview only; no pet commands, storage, or rewards. */
(()=>{'use strict';
const duration=4.2,images=new Map(),base='assets/shared/cleaning/animation-v1/';
const clamp=t=>Math.max(0,Math.min(1,t)),ease=t=>{t=clamp(t);return t*t*(3-2*t);};
function definition(name){return window.CleaningAnimationData?.assets[name];}
function source(name){const d=definition(name);if(!d)return null;if(!images.has(name)){const im=new Image();im.src=base+d.file;images.set(name,im);}return images.get(name);}
function ready(){const names=['sponge','foam','shower','water','drops','sparkle'];return names.map(n=>{const im=source(n);return !!(im?.complete&&im.naturalWidth>0);}).every(Boolean);}
function sprite(ctx,name,index,x,y,width,alpha=1){
 const d=definition(name),im=source(name);if(!d||!im?.complete||!im.naturalWidth||alpha<=0)return false;
 const i=Math.max(0,Math.min(d.frameCount-1,index)),s=width/d.referenceWidth,a=d.anchor;
 ctx.save();ctx.imageSmoothingEnabled=false;ctx.globalAlpha*=clamp(alpha);
 ctx.drawImage(im,i%d.columns*d.cellWidth,Math.floor(i/d.columns)*d.cellHeight,d.cellWidth,d.cellHeight,Math.round(x-a[0]*s),Math.round(y-a[1]*s),d.cellWidth*s,d.cellHeight*s);
 ctx.restore();return true;
}
function phaseAt(t){return t<0?'待機':t<.3?'海綿進場':t<1.6?'擦洗起泡':t<2?'準備沖洗':t<3.4?'淋浴沖洗':t<duration?'水滴與亮點收尾':'完成';}
// `at` is where the renderer actually put the torso: {x, bottom}. Without it the pet is assumed centred at 190.
function pose(species,stage,at={}){
 const ratio=stage==='baby'?.82:1,bottom=at.bottom??(species==='bird'?126+(stage==='adult'?132:106)*.47:177);
 // Use the torso bounds, excluding tails, horns and outstretched wings.
 const bodyWidth=species==='bird'?(stage==='baby'?72:90):(stage==='baby'?76:96);
 return {cx:at.x??190,ratio,bottom,bodyWidth,bodyY:bottom-(species==='bird'?34:30)*ratio};
}
function spongePose(t,p){
 const u=clamp((t-.3)/1.3),halfSpan=p.bodyWidth*.22;
 const top=p.bodyY-10*p.ratio,bottom=p.bottom-18*p.ratio;
 // Three alternating strokes descend through the torso. Cosine slows each
 // horizontal reversal; the vertical easing joins entry and exit smoothly.
 const entering=ease(t/.3),leaving=ease((t-1.6)/.35);
 return {x:p.cx+halfSpan*Math.cos(u*Math.PI*3)+(1-entering)*104+leaving*112,
  y:top+(bottom-top)*ease(u)+leaving*12*p.ratio,
  width:60,alpha:entering*(1-leaving),active:t>=.3&&t<1.6};
}
function showerPose(t,p){
 const sd=definition('shower'),wd=definition('water'),width=sd?.suggestedWidth||58;
 // Keep the original jet length so the head clears the face, including on babies.
 const waterWidth=wd?.suggestedWidth||80,ws=waterWidth/(wd?.referenceWidth||334),ss=width/(sd?.referenceWidth||1049);
 const anchor=sd?.anchor||[536,512],nozzle=sd?.nozzle||[350,733],flowEnd=wd?.flowEnd||[-110,350];
 const halfSpan=p.bodyWidth*.18;
 // Each smoothstep leg has zero speed at both ends, including both reversals.
 const sweep=t<2?0:t<2.35?-halfSpan*ease((t-2)/.35):t<3?-halfSpan+2*halfSpan*ease((t-2.35)/.65):halfSpan*(1-ease((t-3)/.4));
 const targetX=p.cx+sweep,targetY=p.bodyY;
 const enter=ease((t-1.65)/.3),exit=ease((t-3.4)/.3);
 const nx=targetX-flowEnd[0]*ws,ny=targetY-flowEnd[1]*ws-(1-enter)*58-exit*58;
 const x=nx-(nozzle[0]-anchor[0])*ss,y=ny-(nozzle[1]-anchor[1])*ss;
 return {x,y,nx,ny,width,waterWidth,alpha:enter*(1-exit),sweep,halfSpan,impactX:targetX,impactY:targetY};
}
// One six-frame silhouette is being reviewed. Two extra silhouettes are deferred.
// Chest and belly clumps have different peak amounts, with irregular open gaps.
const foamPatches=[[-23,-7,32,2],[8,-11,28,1],[27,2,25,2],[-14,13,28,1],[12,16,30,2]],foamSchedules=new Map();
function contactTime(start,end,predicate,exposure){
 const dt=1/240;let dose=0;
 for(let t=start;t<end;t+=dt){if(predicate(t))dose+=dt;if(dose>=exposure)return t;}
 return Infinity;
}
function foamSchedule(p,motions){
 const key=[p.cx,p.bodyWidth,p.ratio,p.bottom].join('/');
 if(!motions&&foamSchedules.has(key))return foamSchedules.get(key);
 const sponge=motions?.sponge||spongePose,shower=motions?.shower||showerPose;
 const clumps=foamPatches.map(([dx,dy,width,peak])=>{
  const x=p.cx+dx*p.ratio,y=p.bodyY+dy*p.ratio,w=width*p.ratio;
  const touched=contactTime(.3,1.6,t=>{
   const s=sponge(t,p);return s.active&&Math.pow((s.x-x)/(24+w*.14),2)+Math.pow((s.y-y)/18,2)<=1;
  },.025);
  const born=touched+.1;
  const rinseAt=contactTime(2,3.4,t=>{
   const s=shower(t,p);return t>=born&&s.alpha>0&&y>s.ny&&y<p.bottom&&Math.abs(s.impactX-x)<14+w*.1;
  },.04+Math.max(0,y-(p.bodyY-12*p.ratio))*.003);
  return {x,y,width:w,peak,born,rinseAt};
 });
 if(!motions)foamSchedules.set(key,clumps);
 return clumps;
}
function foamState(t,clump){
 const age=t-clump.born;if(age<0)return null;
 const wet=t-clump.rinseAt;
 if(wet>=.49)return null;
 if(wet>=0){
  const index=wet<.16&&clump.peak===2?3:wet<.3?4:5;
  return {index,alpha:1-ease((wet-.43)/.06),slide:1.5*ease(wet/.49)};
 }
 return {index:Math.min(clump.peak,age<.11?0:age<.25?1:2),alpha:ease(age/.055),slide:0};
}
function foamAt(ctx,t,p,reduced=false){
 for(const clump of foamSchedule(p)){
  const state=reduced?{index:clump.peak,alpha:1,slide:0}:foamState(t,clump);
  if(state)sprite(ctx,'foam',state.index,clump.x,clump.y+state.slide,clump.width,state.alpha);
 }
}
function draw(ctx,t,{species='bird',stage='adult',reduced=false,at}={}){
 if(!window.CleaningAnimationData||t<0||t>=duration)return;
 const p=pose(species,stage,at);ctx.save();ctx.imageSmoothingEnabled=false;
 if(reduced){foamAt(ctx,t,p,true);sprite(ctx,'sponge',0,p.cx+25,p.bodyY-4,60);ctx.restore();return;}
 foamAt(ctx,t,p);
 if(t<1.95){
  const sponge=spongePose(t,p);
  const d=definition('sponge'),times=d?.timesMs||[140,120,140],loop=times.reduce((a,b)=>a+b,0);let ms=Math.max(0,t-.3)*1000%loop,index=0;
  while(index<times.length-1&&ms>=times[index])ms-=times[index++];
  sprite(ctx,'sponge',sponge.active?index:0,sponge.x,sponge.y,sponge.width,sponge.alpha);
 }
 const shower=showerPose(t,p);
 if(shower.alpha>0){
  if(t>=2&&t<3.4){const fi=t<2.12?0:t>=3.22?5:1+Math.floor((t-2.12)/.085)%4;sprite(ctx,'water',fi,shower.nx,shower.ny,shower.waterWidth,shower.alpha);}
  sprite(ctx,'shower',0,shower.x,shower.y,shower.width,shower.alpha);
 }
 for(let i=0;i<6;i++){
  const born=2+i*.19,age=t-born,life=.62;if(age<0||age>=life)continue;
  const emission=showerPose(born,p),q=age/life,x=emission.impactX+[-13,6,-4,13][i%4]*p.ratio,impact=p.bottom-3;
  const startY=Math.max(emission.ny+12,p.bodyY-28);
  if(q<.72)sprite(ctx,'drops',q<.3?0:1,x,startY+(impact-startY)*Math.pow(q/.72,1.5),8);
  else sprite(ctx,'drops',Math.min(4,2+Math.floor((q-.72)/.28*3)),x,impact,18,1-clamp((q-.9)/.1));
 }
 // Let the final drops fall and the head leave before the clean sparkle.
 for(const [offset,x,y] of [[0,p.cx-57,p.bodyY-28],[.08,p.cx+57,p.bodyY-41]]){
  const q=(t-3.72-offset)/.4;if(q>=0&&q<1)sprite(ctx,'sparkle',Math.min(4,Math.floor(q*5)),x,y,22);
 }
 ctx.restore();
}
window.CleaningAnimationView={duration,ready,sprite,phaseAt,pose,spongePose,showerPose,foamSchedule,foamState,draw};
})();
