/* Visual-only transition; never advances age, grants EXP or saves a pet. */
(()=>{'use strict';let previous=null,pending=null,start=null;
function observe(p,species){
 if(!p||p.healthState==='dead'||p.isDeparted){previous=null;pending=null;start=null;return;}
 if(previous&&previous.petId===p.petId&&previous.stage!==p.stage&&((previous.stage==='egg'&&p.stage==='baby')||(previous.stage==='baby'&&p.stage==='adult'))){pending={from:previous.stage,to:p.stage,species};start=null;}
 previous={petId:p.petId,stage:p.stage};
}
function draw(ctx,now,reduced=false){
 if(!pending)return false;if(start===null)start=now;
 const elapsed=now-start;if(reduced||elapsed>=2400){pending=null;start=null;return false;}
 const t=elapsed/2400,stage=t<.5?pending.from:pending.to;
 ctx.save();ctx.imageSmoothingEnabled=false;
 if(stage==='egg'){ctx.fillStyle='#fff0cf';ctx.fillRect(167,98,42,60);ctx.fillRect(173,86,30,12);ctx.fillStyle='#876646';for(let i=0;i<4;i++)ctx.fillRect(181+i%2*5,110+i*7,5,7);}
 else if(pending.species===2){if(!(stage==="adult"&&window.WyvernAdultView?.paint(ctx,"idle",0,true)))WyvernView.paint(ctx,WyvernFrames[stage==='baby'?'kBabyWyvernIdleFrames':'kWyvernIdleFrames'][0]);}
 else if(pending.species===1){BirdView.still(ctx,stage,false,190,128,stage==='baby'?2.5:3);}
 ctx.fillStyle='#fff1a6';const pulse=Math.floor(elapsed/125)%16;for(let i=0;i<6;i++){const a=(i/6+pulse/16)*Math.PI*2,x=Math.round(190+Math.cos(a)*85),y=Math.round(125+Math.sin(a)*65);ctx.fillRect(x-4,y,10,2);ctx.fillRect(x,y-4,2,10);}
 ctx.fillStyle='#57483a';ctx.font='bold 16px sans-serif';ctx.fillText(pending.from==='egg'?'孵化了！':'長大了！',157,220);ctx.restore();return true;
}
window.PetGrowthView={observe,draw,get active(){return !!pending;}};
})();
