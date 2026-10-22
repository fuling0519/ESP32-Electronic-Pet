/* Approved white bird base art and dedicated sleep/startle expression sprite sheets. */
(()=>{'use strict';
const image=new Image();image.src='assets/pets/bird/white-idle-v2.png';
const idleImage=new Image();idleImage.src='assets/pets/bird/white-idle-sheet-v4.png';
const sadImage=new Image();sadImage.src='assets/pets/bird/white-sad-sheet-v1.png';
const sickImage=new Image();sickImage.src='assets/pets/bird/white-sick-sheet-v1.png';
const sleepImage=new Image();sleepImage.src='assets/pets/bird/white-sleep-sheet-v3.png?v=sleep-tail-uncropped-20261022';
const startleImage=new Image();startleImage.src='assets/pets/bird/white-startle-sheet-v2.png';
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
// Sleep and startle are presentation-only sprite sequences based on the unchanged bird art.
function sleepFrame(now,reduced=false){
 const d=window.WhiteBirdSleepData;
 if(!d)return 4;
 if(reduced)return d.times.length-1;
 let t=Math.max(0,now)%d.times.reduce((a,b)=>a+b,0),i=0;
 while(i<d.times.length-1&&t>=d.times[i])t-=d.times[i++];
 return i;
}
function startleFrame(t){
 const d=window.WhiteBirdStartleData;
 if(!d)return 0;
 const elapsed=Math.max(0,t)*1000;
 let total=0;
 for(let i=0;i<d.times.length;i++){total+=d.times[i];if(elapsed<total)return i;}
 return d.times.length-1;
}
function drawStartled(ctx,p,now,started,cx=190,cy=126,scale=3,reduced=false){
 const base=(p.stage==='adult'?132:106)*scale/3,height=base*.9,feet=cy+base*.47;
 const t=reduced?0:Math.max(0,(now-started)/1000),d=window.WhiteBirdStartleData;
 if(!startleImage.complete||!startleImage.naturalWidth||!d)return draw(ctx,p,now,null,0,false,cx,cy,scale);
 if(t>=d.times.reduce((a,b)=>a+b,0)/1000)return draw(ctx,p,now,null,0,false,cx,cy,scale);
 const frame=reduced?2:startleFrame(t),fit=height*.94/d.visibleHeight;
 ctx.save();ctx.imageSmoothingEnabled=false;
 const left=cx-d.anchor[0]*fit,top=feet-d.anchor[1]*fit;ctx.drawImage(startleImage,frame*d.cell,0,d.cell,d.cell,left,top,d.cell*fit,d.cell*fit);
 if(!reduced&&frame===3){ctx.fillStyle='rgba(255,244,235,.28)';for(const [x,y] of [[134,180],[272,256]]){ctx.beginPath();ctx.ellipse(left+x*fit,top+y*fit,17*fit,13*fit,0,0,Math.PI*2);ctx.fill();}}
 ctx.restore();return true;
}
function draw(ctx,p,now,action,elapsed,sad,cx=190,cy=126,scale=3){
 if(action==='feed'&&window.WhiteBirdFeed?.draw(ctx,p,elapsed,scale,window.matchMedia?.('(prefers-reduced-motion: reduce)').matches||false))return true;
 if(!image.complete||!image.naturalWidth)return false;
 // Keep the approved art unchanged while reducing its displayed body size by 10%.
 // The reference foot line stays fixed so the bird remains grounded on the scene.
 const baseHeight=(p.stage==='adult'?132:106)*scale/3,height=baseHeight*.9;
 const feet=cy+baseHeight*.47;
 const width=height*image.naturalWidth/image.naturalHeight;
 ctx.save();ctx.imageSmoothingEnabled=false;
 const sleeping=!action&&p.sleepMode==='normal'&&p.healthState!=='dead';
 const idle=!action&&!sad&&p.sleepMode!=='normal'&&p.healthState==='healthy'&&!PetRules.updateSad({...p})&&idleImage.complete&&idleImage.naturalWidth&&window.WhiteBirdIdleData;
 const crying=!action&&p.sleepMode!=='normal'&&p.healthState==='healthy'&&(sad||PetRules.updateSad({...p}))&&sadImage.complete&&sadImage.naturalWidth&&window.WhiteBirdSadData;
 const ill=p.healthState==='sick'&&p.sleepMode!=='normal'&&(!action||action==='treat'&&elapsed<1.4)&&sickImage.complete&&sickImage.naturalWidth&&window.WhiteBirdSickData;
 if(ill){
  const reduced=window.matchMedia?.('(prefers-reduced-motion: reduce)').matches||false;
  const d=WhiteBirdSickData,motion=sickMotion(now,reduced||!!action),fit=height*.94/d.visibleHeight;
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
  ctx.drawImage(sadImage,index*d.cell,0,d.cell,d.cell,cx-d.anchor[0]*fit,feet-d.anchor[1]*fit,d.cell*fit,d.cell*fit);
 }else if(sleeping&&sleepImage.complete&&sleepImage.naturalWidth&&window.WhiteBirdSleepData){
  const reduced=window.matchMedia?.('(prefers-reduced-motion: reduce)').matches||false,d=WhiteBirdSleepData,index=sleepFrame(now,reduced),fit=height*.94/d.visibleHeight;
  ctx.drawImage(sleepImage,index*d.cell,0,d.cell,d.cell,cx-d.anchor[0]*fit,feet-d.anchor[1]*fit,d.cell*fit,d.cell*fit);
 }else if(idle){
  const d=WhiteBirdIdleData,index=idleFrame(now,window.matchMedia?.('(prefers-reduced-motion: reduce)').matches||false);
  const fit=height*.94/d.visibleHeight;
  ctx.drawImage(idleImage,index%2*d.cell,Math.floor(index/2)*d.cell,d.cell,d.cell,cx-d.anchor[0]*fit,feet-d.anchor[1]*fit,d.cell*fit,d.cell*fit);
 }else ctx.drawImage(image,cx-width/2,feet-height*.97,width,height);
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
window.WhiteBirdView={draw,idleFrame,sadFrame,sickMotion,drawStartled};
})();
