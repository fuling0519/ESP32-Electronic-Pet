/* Whole generated poses; presentation only, no pet state writes. */
(()=>{'use strict';
const image=new Image();image.src='assets/pets/bird/white-activity-sheet-v1.png';
function sample(now,cx=190,cy=126){
 const t=Math.max(0,now)%12000,bank={x:145,y:148};
 const smooth=n=>n*n*(3-2*n);
 if(t<2000)return {x:cx,y:cy,phase:'idle',frame:0};
 if(t<5000){const q=smooth((t-2000)/3000);return {x:cx+(bank.x-cx)*q,y:cy+(bank.y-cy)*q,phase:'walk',frame:Math.floor((t-2000)/240)%2};}
 if(t<8500){
  const elapsed=t-5000,q=elapsed<2700?elapsed%900:800;
  return {...bank,phase:'drink',frame:q<160?2:q<500?3:q<650?2:4,
   ripple:q>=160&&q<500,sipping:q>=160&&q<500,
   wet:q>=650,drop:elapsed<2700?Math.min(1,(q-650)/250):(elapsed-2700)/800};
 }
 if(t<11500){const q=smooth((t-8500)/3000);return {x:bank.x+(cx-bank.x)*q,y:bank.y+(cy-bank.y)*q,phase:'walk',frame:Math.floor((t-8500)/240)%2,wet:false};}
 return {x:cx,y:cy,phase:'idle',frame:0};
}
function draw(ctx,p,now,cx,cy,scale=3,reduced=false){
 if(reduced)return false;
 if(!image.complete||!image.naturalWidth||!window.WhiteBirdActivityData)return false;
 const q=sample(now,cx,cy);if(q.phase==='idle')return false;
 const d=WhiteBirdActivityData,fit=(p.stage==='adult'?132:106)*scale/3*.94/d.standingHeight;
 const feet=q.y+(p.stage==='adult'?132:106)*scale/3*.47;
 ctx.save();ctx.imageSmoothingEnabled=false;
 ctx.drawImage(image,q.frame%2*d.cell,Math.floor(q.frame/2)*d.cell,d.cell,d.cell,q.x-d.anchor[0]*fit,feet-d.anchor[1]*fit,d.cell*fit,d.cell*fit);
 if(q.ripple){ctx.fillStyle='#bcecf4';ctx.fillRect(q.x-43,feet-2,23,2);ctx.fillStyle='#efffff';ctx.fillRect(q.x-39,feet-4,13,2);}
 // Source-pose beak anchors in the same units as the sheet, not fixed screen offsets.
 // Measured lower beak tips: sipping (-138,-4), raised pose (-122,-256)
 // relative to the packaged foot anchor. Never anchor water to the eye/cheek.
 const mouth={x:Math.round(q.x-(q.frame===3?138:122)*fit),y:Math.round(feet-(q.frame===3?4:256)*fit)};
 const unit=Math.max(1,Math.round(fit*4));
 if(q.sipping){
  const pulse=Math.floor(now/110)%2;
  ctx.fillStyle='#8ed0df';ctx.fillRect(mouth.x-unit,mouth.y+unit,unit*3,unit);
  ctx.fillStyle='#e8fbff';ctx.fillRect(mouth.x-unit*(2+pulse),mouth.y+unit*2,unit,unit);
 }
 if(q.wet&&q.frame===4){
  const slide=Math.max(0,Math.min(1,q.drop));
  // One small bead briefly clings, then drops directly below the beak and fades.
  const fall=Math.max(0,(slide-.35)/.65),dy=Math.round(fall*unit*6);
  ctx.globalAlpha=.85*(1-fall);
  ctx.fillStyle='#8ed0df';ctx.fillRect(mouth.x,mouth.y+unit+dy,unit*2,unit*2);
  ctx.fillStyle='#e8fbff';ctx.fillRect(mouth.x,mouth.y+unit+dy,unit,unit);
 }
 ctx.restore();return true;
}
window.WhiteBirdActivity={sample,draw};
})();
