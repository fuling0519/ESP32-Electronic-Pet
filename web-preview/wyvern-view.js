/* Illustrated adult Web art, with original masks as the baby/loading fallback. */
(()=>{'use strict';
function frameFor(p,now,action,elapsed=0,reduced=false){
 const baby=p.stage==='baby',prefix=baby?'kBabyWyvern':'kWyvern';
 let state='Idle',frame=Math.floor(now*6/1000)%2;
 if(action==='feed'){state='Eating';frame=Math.floor(elapsed*1000/375)%4;}
 else if(action==='treat'){state='Sad';frame=0;}
 else if(p.sleepMode==='normal'){state='Sleep';frame=baby?Math.floor(now/1000)%2:0;}
 else if(PetRules.updateSad(p)){state='Sad';frame=Math.floor(now/1000)%2;}
 if(reduced)frame=0;
 return {name:prefix+state+'Frames',index:frame,bits:WyvernFrames[prefix+state+'Frames'][frame]};
}
function paint(ctx,bits,x=94,y=45,scale=3){
 const adult=Object.entries(WyvernFrames).some(([name,frames])=>name.startsWith('kWyvern')&&frames.includes(bits));
 const belly=(c,r)=>adult?r>=32&&r<=40&&Math.abs(c-32)<=(r<=31?5:r<=38?3:2):r>=33&&r<=40&&Math.abs(c-32)<=(r<=37?3:2);
 const purple=r=>adult&&r<20?'#cbb0f4':'#b99ae9';
 // Baby belly follows the approved adult's segmented bands, within its own body.
 const bodyColor=(c,r)=>belly(c,r)?(!adult&&(r===35||r===38)?'#211b2e':'#FFF0A6'):purple(r);
 const ink=(c,r)=>c>=0&&c<64&&r>=0&&r<44&&!(bits[r*8+(c>>3)]&(128>>(c%8)));
 const outside=new Set(),queue=[];
 for(let r=0;r<44;r++)for(let c=0;c<64;c++)if((r===0||r===43||c===0||c===63)&&!ink(c,r)){const n=r*64+c;if(!outside.has(n)){outside.add(n);queue.push(n);}}
 for(let i=0;i<queue.length;i++){const n=queue[i],c=n%64,r=Math.floor(n/64);for(const [a,b] of [[c-1,r],[c+1,r],[c,r-1],[c,r+1]])if(a>=0&&a<64&&b>=0&&b<44&&!ink(a,b)&&!outside.has(b*64+a)){outside.add(b*64+a);queue.push(b*64+a);}}
 // Fill enclosed body regions; small face holes remain dark eye/mouth details.
 const seen=new Set(outside);
 for(let r=0;r<44;r++)for(let c=0;c<64;c++)if(!ink(c,r)&&!seen.has(r*64+c)){
  const region=[r*64+c];seen.add(r*64+c);for(let i=0;i<region.length;i++){const n=region[i],a=n%64,b=Math.floor(n/64);for(const [u,v] of [[a-1,b],[a+1,b],[a,b-1],[a,b+1]])if(u>=0&&u<64&&v>=0&&v<44&&!ink(u,v)&&!seen.has(v*64+u)){seen.add(v*64+u);region.push(v*64+u);}}
  const cy=region.reduce((s,n)=>s+Math.floor(n/64),0)/region.length,cx=region.reduce((s,n)=>s+n%64,0)/region.length;
  const face=cy<27&&region.length<45;
  for(const n of region){const c=n%64,r=Math.floor(n/64);ctx.fillStyle=face?'#211b2e':adult?(belly(c,r)?'#FFF0A6':'#b99ae9'):bodyColor(c,r);ctx.fillRect(x+c*scale,y+r*scale,scale,scale);}
 }
 // Keep the original animation masks; add a crisp one-pixel outline outside them.
 ctx.fillStyle='#211b2e';
 for(let r=-1;r<=44;r++)for(let c=-1;c<=64;c++)if(!ink(c,r)&&(ink(c-1,r)||ink(c+1,r)||ink(c,r-1)||ink(c,r+1))){ctx.fillStyle='#211b2e';ctx.fillRect(x+c*scale,y+r*scale,scale,scale);}
 for(let r=0;r<44;r++)for(let c=0;c<64;c++)if(ink(c,r)){
  // Limited pastel palette, with discrete pixel bands rather than gradients.
  ctx.fillStyle=bodyColor(c,r);
  ctx.fillRect(x+c*scale,y+r*scale,scale,scale);
 }
}
let babyStartle=null;
document.addEventListener('pet-interaction-complete',e=>{const p=window.petDataManager?.read().pet;if(p?.stage==='baby'&&e.detail?.speciesId===2&&e.detail?.kind==='poke'&&p.healthState==='healthy'&&p.sleepMode==='awake'&&!p.isDeparted)babyStartle={petId:p.petId,start:performance.now()};});
function draw(ctx,p,now,action,elapsed,reduced){
 const illustrated=window.WyvernAdultView?.draw(ctx,p,now,action,elapsed,reduced);
 if(illustrated){if(illustrated.state==="sleep"){ctx.fillStyle="#57483a";ctx.font="bold 12px sans-serif";ctx.fillText(reduced?"Zzz":["Z","Zz","Zzz"][Math.floor(now/650)%3],248,82);}return illustrated;}
 const fallback=window.PetSadView?.active(p)?{...p,healthState:"sick"}:p;
 const startling=p.stage==='baby'&&babyStartle?.petId===p.petId&&now-babyStartle.start<1200&&p.healthState==='healthy'&&p.sleepMode==='awake'&&!action;
 if(babyStartle&&(babyStartle.petId!==p.petId||now-babyStartle.start>=1200))babyStartle=null;
 const frame=frameFor(startling?{...fallback,healthState:'healthy',sleepMode:'awake'}:{...fallback},startling?babyStartle.start+Math.min(1100,Math.max(0,now-babyStartle.start)):now,action,elapsed,reduced);
 if(startling){
  const t=now-babyStartle.start;let sx=1,sy=1,dy=0;
  if(t<150){sx=1.025;sy=.975;}else if(t<330){sx=.985;sy=1.015;dy=2;}else if(t>=630){const u=Math.min(1,(t-630)/570);sx=.985+.015*u;sy=1.015-.015*u;}
  ctx.save();ctx.translate(190,111+dy);ctx.scale(sx,sy);ctx.translate(-190,-111-dy);paint(ctx,frame.bits);ctx.restore();
 }else paint(ctx,frame.bits);
 ctx.fillStyle='#57483a';ctx.font='bold 12px sans-serif';
 if(p.sleepMode==='normal')ctx.fillText(['Z','Zz','Zzz'][Math.floor(now/650)%3],p.stage==='baby'?226:247,p.stage==='baby'?96:72);
 if(PetRules.updateSad({...p})&&p.sleepMode!=='normal'&&action!=='treat'){
  const x=p.stage==='baby'?222:246,y=p.stage==='baby'?87:63;
  for(let i=0;i<3;i++)ctx.fillRect(x+i*4,y+i*2,1,8);
 }
 return frame;
}
window.WyvernView={frameFor,paint,draw};
})();
