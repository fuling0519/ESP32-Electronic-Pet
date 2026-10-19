/* Web baby bird appearance. Domain state and rewards are never changed here. */
(()=>{'use strict';
const babyImage=new Image();babyImage.src='assets/bird-baby-states-20261019.png';
const adultImage=new Image();adultImage.src='assets/bird-adult-states-20261019.png';
const sadImage=new Image();sadImage.src='assets/bird-sad-closed-20261019.png';
const awakeImage=new Image();awakeImage.src='assets/bird-sad-open-20261019.png';
function frame(p,now,action,elapsed=0,previewSad=false){
 if(action==='treat')return elapsed<1.4?2:0;
 if(action==='feed')return Math.floor(elapsed*1000/400)%2?5:0;
 if(action==='clean')return 0;
 if(p.sleepMode==='normal')return 4;
 const blink=now%2600>=1600;
 // User-approved Web change: healthy sadness keeps the awake character art.
 return p.healthState==='sick'?(blink?3:2):(blink?1:0);
}
function draw(ctx,p,now,action,elapsed=0,previewSad=false,cx=190,cy=126,scale=3){
 scale*=p.stage==='adult'?1.45:1.2;
 const image=p.stage==='adult'?adultImage:babyImage;
 if(!image.complete||!image.naturalWidth)return false;
 const faceOnly=!action&&p.sleepMode==='awake'&&p.healthState!=='sick'&&(previewSad||PetRules.updateSad({...p}));
 if(faceOnly&&p.stage!=='adult'&&sadImage.complete&&sadImage.naturalWidth){
  const art=now%2600<1600&&awakeImage.complete&&awakeImage.naturalWidth?awakeImage:sadImage;
  const fit=Math.min(64*scale/art.naturalWidth,44*scale/art.naturalHeight),w=art.naturalWidth*fit,h=art.naturalHeight*fit;
  ctx.imageSmoothingEnabled=false;ctx.drawImage(art,cx-w/2,cy-h/2,w,h);
  ctx.fillStyle='#302535';for(let i=0;i<3;i++)ctx.fillRect(214+i*6,87,3,(i===2?6:5)*3);
  return true;
 }
 const index=faceOnly?(p.stage==='adult'?(now%2600<1600?2:3):0):frame(p,now,action,elapsed,previewSad),sw=image.naturalWidth/6,sh=image.naturalHeight;
 const fit=Math.min(64*scale/sw,44*scale/sh),w=sw*fit,h=sh*fit;
 // Generated columns can contain the preceding tail at their extreme left edge.
 // Trim only that boundary strip, preserving the frame's original placement.
 const trim=index>0?Math.round(sw*.045):0;
 ctx.imageSmoothingEnabled=false;ctx.drawImage(image,index*sw+trim,0,sw-trim,sh,cx-w/2+trim*fit,cy-h/2,w-trim*fit,h);
 if(faceOnly&&p.stage!=='adult'){
  // Keep every body pixel from the awake frame; replace only the eye patches.
  const unit=Math.max(1,Math.round(w/48));
  for(const left of [.21,.55]){
   const ex=Math.round(cx-w/2+left*w),ey=Math.round(cy-h/2+.425*h);
   // Gentle inward-raised brows preserve the original eyes and cheek pixels.
   ctx.fillStyle='#574039';
   for(let step=0;step<3;step++)ctx.fillRect(ex+step*unit,ey+(left<.4?2-step:step)*unit,unit,unit);
  }
 }
 if(action!=='treat'&&action!=='feed'&&action!=='clean'&&p.sleepMode!=='normal'&&(previewSad||PetRules.updateSad({...p}))){
  ctx.fillStyle='#302535';const ox=p.stage==='baby'?40:39,oy=p.stage==='baby'?14:9;
  for(let i=0;i<3;i++)ctx.fillRect(94+(ox+i*2)*3,45+oy*3,3,(i===2?6:5)*3);
 }
 return true;
}
function still(ctx,stage,closed=false,cx=190,cy=126,scale=3,redrawOnLoad=false){
 const p={stage,healthState:'healthy',sleepMode:closed?'normal':'awake',satiety:100,mood:80,cleanliness:100};
 const render=()=>draw(ctx,p,0,null,0,false,cx,cy,scale);
 // Animated canvases redraw themselves; a late load callback must never paint
 // an old baby frame over a newer egg/incubation frame.
 if(!render()&&redrawOnLoad)(stage==='adult'?adultImage:babyImage).addEventListener?.('load',render,{once:true});
}
window.BirdView={frame,draw,still};
})();
