/* Reuse complete bowing/raising poses for pecking; no water effects or domain writes. */
(()=>{'use strict';
const image=new Image();image.src='assets/pets/bird/white-activity-sheet-v1.png';
function sample(seconds,reduced=false){
 const t=Math.max(0,Math.min(3,seconds));
 if(reduced)return {frame:5,peck:false};
 if(t>=2.25)return {frame:4,peck:false};
 const q=t%.75;
 return {frame:q<.12?5:q<.30?2:q<.48?3:q<.61?2:4,peck:q>=.30&&q<.48};
}
function layout(stage,scale=3){
 const height=(stage==='adult'?132:106)*scale/3*.9,cx=220,feet=201;
 const fit=window.WhiteBirdActivityData?height*.94/WhiteBirdActivityData.standingHeight:0;
 return {cx,cy:feet-height*.47,feet,fit,bowlX:cx-138*fit,bowlBottom:feet+21*scale/3,bowlWidth:(stage==='adult'?64:52)*scale/3};
}
function ready(){return image.complete&&!!image.naturalWidth&&!!window.WhiteBirdActivityData;}
function draw(ctx,p,seconds,scale=3,reduced=false){
 if(!ready())return false;
 const d=WhiteBirdActivityData,q=sample(seconds,reduced),l=layout(p.stage,scale);
 ctx.save();ctx.imageSmoothingEnabled=false;
 ctx.drawImage(image,q.frame%2*d.cell,Math.floor(q.frame/2)*d.cell,d.cell,d.cell,l.cx-d.anchor[0]*l.fit,l.feet-d.anchor[1]*l.fit,d.cell*l.fit,d.cell*l.fit);
 ctx.restore();return true;
}
function bowl(ctx,p,seconds){
 if(!ready()||!window.BlueSeedBowl)return false;
 const l=layout(p.stage);return BlueSeedBowl.draw(ctx,seconds,l.bowlX,l.bowlBottom,l.bowlWidth);
}
window.WhiteBirdFeed={sample,layout,ready,draw,bowl};
})();
