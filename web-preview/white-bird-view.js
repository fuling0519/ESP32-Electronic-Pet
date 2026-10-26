/* Approved white bird base art plus idle, sad, sick, sleeping, startled and lying-down (dead) art. */
(()=>{'use strict';
const image=new Image();image.src='assets/pets/bird/white-idle-v2.png';
const idleImage=new Image();idleImage.src='assets/pets/bird/white-idle-sheet-v4.png';
const sadImage=new Image();sadImage.src='assets/pets/bird/white-sad-sheet-v1.png';
const sickImage=new Image();sickImage.src='assets/pets/bird/white-sick-sheet-v1.png';
const deadImage=new Image();deadImage.src='assets/pets/bird/white-dead-v1.png';
// Closed smiling eyes drawn onto the idle image (tools/draw_white_bird_happy.py); shown while being petted.
const happyImage=new Image();happyImage.src='assets/pets/bird/white-happy-v1.png?v=left-eye-tilt-20261025';
// Roosting asleep and poked-startled stills (tools/prepare_white_bird_pose.py), same 1254 px canvas as the base image.
const sleepImage=new Image();sleepImage.src='assets/pets/bird/white-sleep-v1.png';
const startledImage=new Image();startledImage.src='assets/pets/bird/white-startled-v1.png';
const DEAD_SCALE=1.35;

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
// Torso centre and eye line as fractions of the drawn height from (cx,cy), measured from the white body
// pixels with the tail excluded. The base image (used during care) sits further left than the idle sheets.
const ANCHORS={base:{body:-.12,headX:-.13,headY:-.2},idle:{body:-.053,headX:-.085,headY:-.23}};
function anchors(stage,cx=190,cy=126,scale=3,pose='idle'){
 const h=(stage==='adult'?132:106)*scale/3,a=ANCHORS[pose]||ANCHORS.idle;
 return {x:cx+a.body*h,y:cy,headX:cx+a.headX*h,headY:cy+a.headY*h,feet:cy+h*.47,height:h};
}
// Plants a pose still at the base image's pixel scale (times its own size fix), its toes where the idle sheets stand.
function pose(ctx,img,d,stage,cx,cy,scale,stretch=1){
 if(!d||!img.complete||!img.naturalWidth)return false;
 const h=(stage==='adult'?132:106)*scale/3,fit=h/d.source[1]*(d.scale||1),feetX=cx-h/2+(ANCHORS.idle.body-ANCHORS.base.body)*h+d.baseFeet[0]*h/d.source[1],feetY=cy+h*.47;
 ctx.save();ctx.imageSmoothingEnabled=false;ctx.translate(feetX,feetY);ctx.scale(1,stretch);
 ctx.drawImage(img,-d.feet[0]*fit,-d.feet[1]*fit,d.crop.width*fit,d.crop.height*fit);ctx.restore();return true;
}
function draw(ctx,p,now,action,elapsed,sad,cx=190,cy=126,scale=3){
 if(action==='feed'&&window.WhiteBirdFeed?.draw(ctx,p,elapsed,scale,window.matchMedia?.('(prefers-reduced-motion: reduce)').matches||false))return true;
 if(!image.complete||!image.naturalWidth)return false;
 if(action==='happy'&&happyImage.complete&&happyImage.naturalWidth){
  // Same canvas as the base image, shifted to where the idle sheets stand so a pat does not jump sideways;
  // a small hop every 250 ms (seamless over the demo's 1 s loop).
  const h=(p.stage==='adult'?132:106)*scale/3,lift=Math.round(Math.abs(Math.sin(now*Math.PI/250))*2*scale/3);
  ctx.save();ctx.imageSmoothingEnabled=false;ctx.drawImage(happyImage,cx-h/2+(ANCHORS.idle.body-ANCHORS.base.body)*h,cy-h/2-lift,h,h);ctx.restore();return true;
 }
 if(action==='startled'&&pose(ctx,startledImage,window.WhiteBirdStartledData,p.stage,cx,cy,scale))return true;
 if(!action&&p.sleepMode==='normal'&&p.healthState!=='dead'&&pose(ctx,sleepImage,window.WhiteBirdSleepData,p.stage,cx,cy,scale,
  window.PetEffects?.breath?.(now,window.matchMedia?.('(prefers-reduced-motion: reduce)').matches||false)??1))return true;
 // Comparable visual size to the adult wyvern's 126px body width; uniform scaling only.
 const height=(p.stage==='adult'?132:106)*scale/3;
 // Lying down: the source draws the bird smaller than the idle image, so DEAD_SCALE matches its
 // head-to-rump length to the standing body height; body centred on the feet line.
 if(p.healthState==='dead'){
  const d=window.WhiteBirdDeadData;if(!d||!deadImage.complete||!deadImage.naturalWidth)return false;
  const fit=height/d.source[1]*DEAD_SCALE;ctx.save();ctx.imageSmoothingEnabled=false;
  ctx.drawImage(deadImage,cx-d.anchor[0]*fit,cy+height*.47-d.anchor[1]*fit,d.crop.width*fit,d.crop.height*fit);ctx.restore();return true;
 }
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
 // Keep shared state effects separate from the single approved character image. Sadness has no extra mark: the face
 // and the emote bubble already show it.
 if(!ill&&!action&&p.sleepMode!=='normal'&&p.healthState==='sick'){
  ctx.fillStyle='#438dac';ctx.fillRect(cx+width*.36,cy-height*.2,4,8);ctx.fillRect(cx+width*.36-3,cy-height*.2+7,10,8);
  ctx.fillStyle='#effbff';ctx.fillRect(cx+width*.36,cy-height*.2+8,2,4);
 }
 ctx.restore();return true;
}
window.WhiteBirdView={draw,anchors,idleFrame,sadFrame,sickMotion,DEAD_SCALE};
})();
