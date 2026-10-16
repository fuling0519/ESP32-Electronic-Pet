/* Web rendering of the original OLED masks; no generated replacement art. */
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
 // Draw only original foreground pixels; the garden remains visible around them.
 ctx.fillStyle='#fffaf0';
 for(let row=0;row<44;row++)for(let col=0;col<64;col++)if(!(bits[row*8+(col>>3)]&(128>>(col%8))))ctx.fillRect(x+col*scale,y+row*scale,scale,scale);
}
function draw(ctx,p,now,action,elapsed,reduced){
 const frame=frameFor({...p},now,action,elapsed,reduced);paint(ctx,frame.bits);
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
