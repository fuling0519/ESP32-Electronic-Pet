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
window.SharedPetView={paint,clean};
})();
