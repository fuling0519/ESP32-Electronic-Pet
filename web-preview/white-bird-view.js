/* Approved white bird base art. Dedicated expression sheets are still pending. */
(()=>{'use strict';
const image=new Image();image.src='assets/pets/bird/white-idle-v2.png';
const idleImage=new Image();idleImage.src='assets/pets/bird/white-idle-sheet-v4.png';
const sadImage=new Image();sadImage.src='assets/pets/bird/white-sad-sheet-v1.png';
const sickImage=new Image();sickImage.src='assets/pets/bird/white-sick-sheet-v1.png';
function sickMotion(now,reduced=false){
 const t=Math.max(0,now)%10000,u=(t-3000)/4000;
 return {frame:reduced||Math.max(0,now)%2300<1400?0:1,
  angle:reduced||u<=0||u>=1?0:Math.sin(u*Math.PI*2)*Math.sin(u*Math.PI)**2*.075};
}
function sadFrame(now,reduced=false){return reduced||Math.max(0,now)%2440<2200?0:1;}
function idleFrame(now,reduced=false){
 let t=Math.max(0,now)%920,index=0;
 if(!reduced)while(index<3&&t>=WhiteBirdIdleData.times[index])t-=WhiteBirdIdleData.times[index++];
 return index;
}
function draw(ctx,p,now,action,elapsed,sad,cx=190,cy=126,scale=3){
 if(action==='feed'&&window.WhiteBirdFeed?.draw(ctx,p,elapsed,scale,window.matchMedia?.('(prefers-reduced-motion: reduce)').matches||false))return true;
 if(!image.complete||!image.naturalWidth)return false;
 // Comparable visual size to the adult wyvern's 126px body width; uniform scaling only.
 const height=(p.stage==='adult'?132:106)*scale/3;
 const width=height*image.naturalWidth/image.naturalHeight;
 ctx.save();ctx.imageSmoothingEnabled=false;
 const idle=!action&&!sad&&p.sleepMode!=='normal'&&p.healthState==='healthy'&&!PetRules.updateSad({...p})&&idleImage.complete&&idleImage.naturalWidth&&window.WhiteBirdIdleData;
 const crying=!action&&p.sleepMode!=='normal'&&p.healthState==='healthy'&&(sad||PetRules.updateSad({...p}))&&sadImage.complete&&sadImage.naturalWidth&&window.WhiteBirdSadData;
 const ill=p.healthState==='sick'&&p.sleepMode!=='normal'&&(!action||action==='treat'&&elapsed<1.4)&&sickImage.complete&&sickImage.naturalWidth&&window.WhiteBirdSickData;
 if(ill){
  const reduced=window.matchMedia?.('(prefers-reduced-motion: reduce)').matches||false;
  const d=WhiteBirdSickData,motion=sickMotion(now,reduced||!!action),fit=height*.94/d.visibleHeight,feet=cy+height*.47;
  ctx.save();
  // Rotate the complete pose around its ground support, never separate body parts.
  if(motion.angle){ctx.translate(cx,feet);ctx.rotate(motion.angle);ctx.translate(-cx,-feet);}
  ctx.drawImage(sickImage,motion.frame*d.cell,0,d.cell,d.cell,cx-d.anchor[0]*fit,feet-d.anchor[1]*fit,d.cell*fit,d.cell*fit);
  if(!action){
   const x=Math.round(cx+height*.25),y=Math.round(cy-height*.28),u=Math.max(1,Math.round(height/80));
   ctx.fillStyle='#65b3cf';ctx.fillRect(x,y,u,u*3);ctx.fillRect(x-u,y+u*2,u*3,u*3);
   ctx.fillStyle='#d8f5fc';ctx.fillRect(x,y+u*2,u,u*2);
  }
  ctx.restore();
 }else if(crying){
  const d=WhiteBirdSadData,index=sadFrame(now,window.matchMedia?.('(prefers-reduced-motion: reduce)').matches||false),fit=height*.94/d.visibleHeight;
  ctx.drawImage(sadImage,index*d.cell,0,d.cell,d.cell,cx-d.anchor[0]*fit,cy+height*.47-d.anchor[1]*fit,d.cell*fit,d.cell*fit);
 }else if(idle){
  const d=WhiteBirdIdleData,index=idleFrame(now,window.matchMedia?.('(prefers-reduced-motion: reduce)').matches||false);
  const fit=height*.94/d.visibleHeight;
  ctx.drawImage(idleImage,index%2*d.cell,Math.floor(index/2)*d.cell,d.cell,d.cell,cx-d.anchor[0]*fit,cy+height*.47-d.anchor[1]*fit,d.cell*fit,d.cell*fit);
 }else ctx.drawImage(image,cx-width/2,cy-height/2,width,height);
 // Keep shared state effects separate from the single approved character image.
 if(!action&&p.sleepMode!=='normal'&&(sad||PetRules.updateSad({...p}))){
  ctx.fillStyle='#302535';for(let i=0;i<3;i++)ctx.fillRect(cx+width*.35+i*5,cy-height*.34,2,i===2?12:10);
 }
 if(!ill&&!action&&p.sleepMode!=='normal'&&p.healthState==='sick'){
  ctx.fillStyle='#438dac';ctx.fillRect(cx+width*.36,cy-height*.2,4,8);ctx.fillRect(cx+width*.36-3,cy-height*.2+7,10,8);
  ctx.fillStyle='#effbff';ctx.fillRect(cx+width*.36,cy-height*.2+8,2,4);
 }
 ctx.restore();return true;
}
window.WhiteBirdView={draw,idleFrame,sadFrame,sickMotion};
})();
