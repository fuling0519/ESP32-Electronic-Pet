(()=>{'use strict';
function paint(ctx,name,index,x,y,scale=3,color='#fffaf0'){
 const s=SharedPetSprites[name],bits=s.frames[Math.max(0,Math.min(s.frames.length-1,index))],stride=Math.ceil(s.w/8);ctx.fillStyle=color;
 for(let r=0;r<s.h;r++)for(let c=0;c<s.w;c++)if(!(bits[r*stride+(c>>3)]&(128>>(c%8))))ctx.fillRect(x+c*scale,y+r*scale,scale,scale);
}
function clean(ctx,elapsed){
 const frame=Math.min(2,Math.floor(elapsed/.4));
 paint(ctx,'kCleaningFrames',frame,66,30,3,'#bce1e8');
 // Small pixel bubbles travel with the water; no extra retreat phase.
 const top=177-[36,72,108][frame];
 for(let i=0;i<7;i++){
  const x=84+i*29,y=top+8+((i*13-Math.floor(elapsed*18))%28+28)%28;
  ctx.fillStyle='#eaf9f8';ctx.fillRect(x,y,6,2);ctx.fillRect(x-2,y+2,2,4);ctx.fillRect(x+6,y+2,2,4);ctx.fillRect(x,y+6,6,2);
 }
}
function potionFrame(seconds){const ms=seconds*1000;return ms<250?0:ms<500?1:ms<850?2:ms<1200?3:4;}
function recovery(ctx,seconds){
 const step=Math.min(15,Math.max(0,Math.floor((seconds-1.4)*1000/125)));
 const left=[0,1,2,3,2,1,0,1,2,3,2,1,0,1,2,0],right=[0,0,1,2,3,2,1,0,0,1,2,3,2,1,0,0];
 for(const [x,y,light] of [[99,(step>=7&&step<=11?39:23)*3,left[step]],[282,(step>=9?25:37)*3,right[step]]]){
  if(!light)continue;ctx.fillStyle='#fff1a6';ctx.fillRect(x,y,3,3);const n=light===3?6:light===2?3:0;if(n){ctx.fillRect(x-n,y,n*2+3,3);ctx.fillRect(x,y-n,3,n*2+3);}
 }
}
window.SharedPetView={paint,clean,potionFrame,recovery};
})();
