/* Visual-only transition; never advances age, grants EXP or saves a pet. */
(()=>{'use strict';let previous=null,pending=null,start=null;
const GROW_MS=3600,HATCH_MS=4200;
// Hatch timeline (ms): egg breaks 0-1850 (PetEggView), flash 1600-2300, baby from 1800, banner from 2000.
const FLASH=[1600,1800,2300],BABY_AT=1800,BANNER_AT=2000,TWINKLE_AT=2000;
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
// After hatching, the same twinkling stars as a finished clean, popping one by one around the baby.
const TWINKLES=[[0,136,96,26],[150,248,84,30],[300,118,150,24],[450,264,146,28],[600,170,66,24],[750,226,172,26],[900,144,128,22],[1050,240,112,28],[1200,126,182,22],[1350,212,62,26]];
// Asking ready() starts loading the sheets, so they are in place long before a hatch reaches them.
window.CleaningAnimationView?.ready();
// The adult is bigger, so its stars sit wider and come a little faster.
const GROW_TWINKLES=[[0,122,80,28],[100,262,70,30],[200,100,150,24],[300,284,150,28],[400,160,46,24],[500,234,180,26],[600,136,190,22],[700,250,110,26]];
function twinkles(ctx,t,list=TWINKLES,at=TWINKLE_AT){
 if(!window.CleaningAnimationView)return false;
 if(!CleaningAnimationView.ready())return true;
 for(const [delay,x,y,w] of list){const q=(t-at-delay)/400;if(q>=0&&q<1)CleaningAnimationView.sprite(ctx,'sparkle',Math.min(4,Math.floor(q*5)),x,y,w);}
 return true;
}
// Grow timeline (ms): the baby glows brighter 0-1000, turns into a ball of light 1000-1800, the ball
// swells into a flash 1800-2400, the adult appears at 2100 and loses its glow by 2700.
const GROW={glow:1000,orb:1800,flash:[1950,2100,2500],adult:2100,unglow:2700,twinkle:2300,banner:2400},ORB={x:190,y:132};
let glowCanvas=null;
// Draws the pet with a warm white wash over its own pixels (1 = a plain light silhouette) and a soft halo.
function glowing(ctx,stage,species,wash,halo){
 if(typeof document==='undefined'){pet(ctx,stage,species);return;}
 glowCanvas=glowCanvas||Object.assign(document.createElement('canvas'),{width:384,height:256});
 const g=glowCanvas.getContext('2d');g.clearRect(0,0,384,256);g.save();g.imageSmoothingEnabled=false;pet(g,stage,species);
 g.globalCompositeOperation='source-atop';g.fillStyle=`rgba(255,250,228,${wash})`;g.fillRect(0,0,384,256);g.restore();
 ctx.save();ctx.shadowColor='rgba(255,236,160,.95)';ctx.shadowBlur=halo;ctx.drawImage(glowCanvas,0,0);ctx.restore();
}
// A bright core with a wide warm halo, so the ball still reads on pale grass.
function orb(ctx,radius,alpha){
 if(!ctx.createRadialGradient||alpha<=0)return;
 ctx.save();
 for(const [r,stops] of [[radius*2,[[0,.55],[.5,.25],[1,0]]],[radius,[[0,1],[.55,.95],[1,0]]]]){
  const g=ctx.createRadialGradient(ORB.x,ORB.y,0,ORB.x,ORB.y,r);
  for(const [at,a] of stops)g.addColorStop(at,at<.5?`rgba(255,255,244,${a*alpha})`:`rgba(255,226,140,${a*alpha})`);
  ctx.fillStyle=g;ctx.beginPath();ctx.arc(ORB.x,ORB.y,r,0,Math.PI*2);ctx.fill();}
 ctx.restore();
}
function drawGrow(ctx,t,reduced){
 const {from,to,species}=pending;
 if(reduced){pet(ctx,to,species);banner(ctx,'長大了！',1);return;}
 if(t<GROW.glow){const u=t/GROW.glow,pulse=.5+.5*Math.sin(u*u*Math.PI*10);glowing(ctx,from,species,.15+.6*u*(.6+.4*pulse),6+10*u);}
 else if(t<GROW.orb){const u=(t-GROW.glow)/(GROW.orb-GROW.glow);
  ctx.save();ctx.globalAlpha=Math.max(0,1-u*1.6);glowing(ctx,from,species,1,16);ctx.restore();
  orb(ctx,(34+22*(1-u))*(1+.07*Math.sin(t/70)),Math.min(1,u*1.8));}
 else if(t<GROW.adult)orb(ctx,34+90*((t-GROW.orb)/(GROW.adult-GROW.orb))**2,1);
 if(t>=GROW.adult){const u=Math.min(1,(t-GROW.adult)/(GROW.unglow-GROW.adult));if(u<1)glowing(ctx,to,species,1-u,16*(1-u));else pet(ctx,to,species);
  if(!twinkles(ctx,t,GROW_TWINKLES,GROW.twinkle))sparkles(ctx,t);}
 const [f0,f1,f2]=GROW.flash;
 if(t>=f0&&t<f2){const a=t<f1?(t-f0)/(f1-f0):1-(t-f1)/(f2-f1);ctx.save();ctx.globalAlpha=Math.max(0,a)*.95;ctx.fillStyle='#fffbe8';ctx.fillRect(0,0,384,256);ctx.restore();}
 if(t>=GROW.banner)banner(ctx,'長大了！',Math.min(1,(t-GROW.banner)/300));
}
function oldEgg(ctx){ctx.fillStyle='#fff0cf';ctx.fillRect(167,98,42,60);ctx.fillRect(173,86,30,12);ctx.fillStyle='#876646';for(let i=0;i<4;i++)ctx.fillRect(181+i%2*5,110+i*7,5,7);}
// Small shell chips burst out of the crack under the flash and fall away.
function chips(ctx,t,shell){
 const egg=window.PetEggView?.EGG||{cx:190,bottom:200,height:84},s=(t-BABY_AT)/1000;if(s<0||s>1)return;
 ctx.save();ctx.globalAlpha=Math.min(1,(1-s)*2);
 for(let i=0;i<10;i++){const a=-Math.PI*(.08+.84*i/9),v=70+(i*37)%40,x=Math.round(egg.cx+Math.cos(a)*v*s),y=Math.round(egg.bottom-egg.height*.55+Math.sin(a)*v*s+120*s*s);
  ctx.fillStyle='#4a2618';ctx.fillRect(x-2,y-2,5,4);ctx.fillStyle=shell;ctx.fillRect(x-1,y-1,3,2);}
 ctx.restore();
}
// Dark text with a thick cream outline so it stays readable on grass, water and the flash.
function banner(ctx,text,alpha){
 ctx.save();ctx.globalAlpha=alpha;ctx.font='900 22px "Microsoft JhengHei","PingFang TC","Noto Sans TC",sans-serif';ctx.textAlign='center';ctx.textBaseline='middle';ctx.lineJoin='round';
 ctx.lineWidth=6;ctx.strokeStyle='#fff8e6';ctx.strokeText(text,190,224);ctx.fillStyle='#4a3020';ctx.fillText(text,190,224);ctx.restore();
}
function drawHatch(ctx,elapsed,reduced){
 const t=elapsed;
 if(t<BABY_AT||reduced&&t<HATCH_MS/2){if(!window.PetEggView?.drawHatch(ctx,pending.petId,reduced?0:t,reduced))oldEgg(ctx);}
 if(t>=BABY_AT&&!(reduced&&t<HATCH_MS/2)){const hop=t<BABY_AT+400&&!reduced?-6*Math.sin(Math.PI*(t-BABY_AT)/400):0;
  // Chips fly out behind the baby so none of them lands on its face.
  if(!reduced)chips(ctx,t,SHELL[window.PetEggView?.design(pending.petId)??0]);
  pet(ctx,'baby',pending.species,hop);
  if(!reduced){if(!twinkles(ctx,t))sparkles(ctx,t,t>HATCH_MS-600?(HATCH_MS-t)/600:1);}}
 if(!reduced&&t>=FLASH[0]&&t<FLASH[2]){const a=t<FLASH[1]?(t-FLASH[0])/(FLASH[1]-FLASH[0]):1-(t-FLASH[1])/(FLASH[2]-FLASH[1]);ctx.save();ctx.globalAlpha=Math.max(0,a)*.95;ctx.fillStyle='#fffbe8';ctx.fillRect(0,0,384,256);ctx.restore();}
 if(t>=BANNER_AT||reduced&&t>=HATCH_MS/2)banner(ctx,(pending.name?pending.name+' ':'')+'孵化了！',reduced?1:Math.min(1,(t-BANNER_AT)/300));
}
function draw(ctx,now,reduced=false){
 if(!pending)return false;if(start===null)start=now;
 const hatching=pending.from==='egg',elapsed=now-start;
 // Reduced motion: both transitions swap straight to the new stage with their banner, no glow, flash or chips.
 if(elapsed>=(hatching?HATCH_MS:GROW_MS)){pending=null;start=null;return false;}
 ctx.save();ctx.imageSmoothingEnabled=false;
 if(hatching)drawHatch(ctx,elapsed,reduced);
 else drawGrow(ctx,elapsed,reduced);
 ctx.restore();return true;
}
window.PetGrowthView={observe,draw,get active(){return !!pending;}};
})();
