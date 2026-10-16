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
 const ink=(c,r)=>c>=0&&c<64&&r>=0&&r<44&&!(bits[r*8+(c>>3)]&(128>>(c%8)));
 // Keep the original animation masks; add a crisp one-pixel outline outside them.
 ctx.fillStyle='#211b2e';
 for(let r=-1;r<=44;r++)for(let c=-1;c<=64;c++)if(!ink(c,r)&&(ink(c-1,r)||ink(c+1,r)||ink(c,r-1)||ink(c,r+1)))ctx.fillRect(x+c*scale,y+r*scale,scale,scale);
 for(let r=0;r<44;r++)for(let c=0;c<64;c++)if(ink(c,r)){
  // Limited pastel palette, with discrete pixel bands rather than gradients.
  ctx.fillStyle=r<9?'#f7df9d':r>28&&c>23&&c<40?'#fff1cf':r>18&&(c<19||c>45)?'#e9cd86':r>32?'#a282d0':r<20?'#cbb0f4':'#b99ae9';
  ctx.fillRect(x+c*scale,y+r*scale,scale,scale);
 }
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
