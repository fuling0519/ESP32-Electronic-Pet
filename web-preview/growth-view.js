/* Visual-only transition; never advances age, grants EXP or saves a pet. */
(()=>{'use strict';let previous=null,pending=null,start=null;
const GROW_MS=2400,HATCH_MS=4200;
// Hatch timeline (ms): egg breaks 0-1850 (PetEggView), flash 1600-2300, baby from 1800, banner from 2000.
const FLASH=[1600,1800,2300],BABY_AT=1800,BANNER_AT=2000;
const SHELL=['#f6e2c6','#c9f3e3','#f8f1cc'];
function observe(p,species){
 if(!p||p.healthState==='dead'||p.isDeparted){previous=null;pending=null;start=null;return;}
 if(previous&&previous.petId===p.petId&&previous.stage!==p.stage&&((previous.stage==='egg'&&p.stage==='baby')||(previous.stage==='baby'&&p.stage==='adult'))){pending={from:previous.stage,to:p.stage,species,petId:p.petId,name:p.name||''};start=null;}
 previous={petId:p.petId,stage:p.stage};
}
function pet(ctx,stage,species,dy=0){
 ctx.save();ctx.translate(0,dy);
 if(species===2){if(!window.WyvernAdultView?.paint(ctx,"idle",0,true,{stage}))WyvernView.paint(ctx,WyvernFrames[stage==='baby'?'kBabyWyvernIdleFrames':'kWyvernIdleFrames'][0]);}
 else if(species===1){BirdView.still(ctx,stage,false,190,128,stage==='baby'?2.5:3);}
 ctx.restore();
}
function sparkles(ctx,elapsed,alpha=1){ctx.save();ctx.globalAlpha=alpha;ctx.fillStyle='#fff1a6';const pulse=Math.floor(elapsed/125)%16;for(let i=0;i<6;i++){const a=(i/6+pulse/16)*Math.PI*2,x=Math.round(190+Math.cos(a)*85),y=Math.round(125+Math.sin(a)*65);ctx.fillRect(x-4,y,10,2);ctx.fillRect(x,y-4,2,10);}ctx.restore();}
function oldEgg(ctx){ctx.fillStyle='#fff0cf';ctx.fillRect(167,98,42,60);ctx.fillRect(173,86,30,12);ctx.fillStyle='#876646';for(let i=0;i<4;i++)ctx.fillRect(181+i%2*5,110+i*7,5,7);}
// Small shell chips burst out of the crack under the flash and fall away.
function chips(ctx,t,shell){
 const egg=window.PetEggView?.EGG||{cx:190,bottom:200,height:84},s=(t-BABY_AT)/1000;if(s<0||s>1)return;
 ctx.save();ctx.globalAlpha=Math.min(1,(1-s)*2);
 for(let i=0;i<10;i++){const a=-Math.PI*(.08+.84*i/9),v=70+(i*37)%40,x=Math.round(egg.cx+Math.cos(a)*v*s),y=Math.round(egg.bottom-egg.height*.55+Math.sin(a)*v*s+120*s*s);
  ctx.fillStyle='#4a2618';ctx.fillRect(x-2,y-2,5,4);ctx.fillStyle=shell;ctx.fillRect(x-1,y-1,3,2);}
 ctx.restore();
}
function drawHatch(ctx,elapsed,reduced){
 const t=elapsed;
 if(t<BABY_AT||reduced&&t<HATCH_MS/2){if(!window.PetEggView?.drawHatch(ctx,pending.petId,reduced?0:t,reduced))oldEgg(ctx);}
 if(t>=BABY_AT&&!(reduced&&t<HATCH_MS/2)){const hop=t<BABY_AT+400&&!reduced?-6*Math.sin(Math.PI*(t-BABY_AT)/400):0;pet(ctx,'baby',pending.species,hop);
  if(!reduced){chips(ctx,t,SHELL[window.PetEggView?.design(pending.petId)??0]);sparkles(ctx,t,t>HATCH_MS-600?(HATCH_MS-t)/600:1);}}
 if(!reduced&&t>=FLASH[0]&&t<FLASH[2]){const a=t<FLASH[1]?(t-FLASH[0])/(FLASH[1]-FLASH[0]):1-(t-FLASH[1])/(FLASH[2]-FLASH[1]);ctx.save();ctx.globalAlpha=Math.max(0,a)*.95;ctx.fillStyle='#fffbe8';ctx.fillRect(0,0,384,256);ctx.restore();}
 if(t>=BANNER_AT||reduced&&t>=HATCH_MS/2){ctx.save();ctx.globalAlpha=reduced?1:Math.min(1,(t-BANNER_AT)/300);ctx.fillStyle='#57483a';ctx.font='bold 16px sans-serif';ctx.textAlign='center';ctx.fillText((pending.name?pending.name+' ':'')+'孵化了！',190,226);ctx.restore();}
}
function draw(ctx,now,reduced=false){
 if(!pending)return false;if(start===null)start=now;
 const hatching=pending.from==='egg',elapsed=now-start;
 // Reduced motion: hatching swaps egg for baby without shaking, flash or chips; growing is skipped as before.
 if(elapsed>=(hatching?HATCH_MS:GROW_MS)||reduced&&!hatching){pending=null;start=null;return false;}
 ctx.save();ctx.imageSmoothingEnabled=false;
 if(hatching)drawHatch(ctx,elapsed,reduced);
 else{const stage=elapsed/GROW_MS<.5?pending.from:pending.to;pet(ctx,stage,pending.species);sparkles(ctx,elapsed);ctx.fillStyle='#57483a';ctx.font='bold 16px sans-serif';ctx.fillText('長大了！',157,220);}
 ctx.restore();return true;
}
window.PetGrowthView={observe,draw,get active(){return !!pending;}};
})();
