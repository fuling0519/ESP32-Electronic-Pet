/* Shared Web food bowl art; animation does not award or consume domain values. */
(()=>{'use strict';
const image=new Image();image.src='assets/items/blue-seed-bowl-sheet-v1.png';
function frame(seconds){return Math.max(0,Math.min(3,Math.floor(Math.max(0,seconds)/.75)));}
function draw(ctx,seconds,cx=255,bottom=215,width=64){
 if(!image.complete||!image.naturalWidth||!window.BlueSeedBowlData)return false;
 const d=BlueSeedBowlData,index=frame(seconds),fit=width/d.visibleWidth;
 ctx.save();ctx.imageSmoothingEnabled=false;
 ctx.drawImage(image,index%2*d.width,Math.floor(index/2)*d.height,d.width,d.height,cx-d.anchor[0]*fit,bottom-d.anchor[1]*fit,d.width*fit,d.height*fit);
 ctx.restore();return true;
}
window.BlueSeedBowl={frame,draw};
})();
