/* Web egg: three shell designs (picked per petId), crack stages and the hatch break.
   Visual only; never advances age or changes the pet. Art: assets/pets/egg/README.md. */
(()=>{'use strict';
const designs=new Image();designs.src='assets/pets/egg/egg-designs-v1.png';
const hatch=new Image();hatch.src='assets/pets/egg/egg-hatch-v1.png';
// Ground position and on-screen height of the egg; the sheet egg is 529px tall in a 443x582 cell.
const EGG={cx:190,bottom:200,height:84},SHEET_EGG_HEIGHT=529,K=EGG.height/SHEET_EGG_HEIGHT;
function phase(age){return age>=300?'hatched':age>=270?'large':age>=240?'small':'incubating';}
function label(age){return {incubating:'孵化中 · 等待小生命',small:'孵化中 · 出現小裂痕',large:'孵化中 · 裂痕變大，即將出生',hatched:'孵化完成'}[phase(age)];}
// Stable per pet: the same egg keeps its pattern across reloads and devices.
function design(petId){let h=0;for(const ch of String(petId??''))h=(h*31+ch.charCodeAt(0))>>>0;return h%3;}
function ready(){return designs.complete&&designs.naturalWidth&&hatch.complete&&hatch.naturalWidth&&window.EggHatchData;}
// Draws one sheet cell with its bottom anchor on the ground; angle tilts around that anchor.
function cell(ctx,img,col,row,{angle=0,lift=0,breathe=1}={}){
 const d=EggHatchData,[w,h]=d.cell,[ax,ay]=d.anchor;
 ctx.save();ctx.translate(EGG.cx,EGG.bottom);ctx.rotate(angle);ctx.scale(2-breathe,breathe);ctx.imageSmoothingEnabled=false;
 ctx.drawImage(img,col*w,row*h,w,h,-ax*K,-ay*K-lift*K,w*K,h*K);ctx.restore();
}
// Warm light along the main crack line (sheet coordinates in EggHatchData.crackLine).
function crackLight(ctx,row,alpha,width,{angle=0,lift=0}={}){
 const d=EggHatchData,pts=d.crackLine[d.rows[row]],[ax,ay]=d.anchor;if(!pts||alpha<=0)return;
 ctx.save();ctx.translate(EGG.cx,EGG.bottom);ctx.rotate(angle);ctx.globalCompositeOperation='lighter';
 ctx.strokeStyle=`rgba(255,222,130,${alpha})`;ctx.shadowColor='rgba(255,214,110,.9)';ctx.shadowBlur=10;ctx.lineWidth=width;ctx.lineJoin='round';
 ctx.beginPath();pts.forEach(([x,y],i)=>ctx[i?'lineTo':'moveTo']((x-ax)*K,(y-ay)*K-lift*K));ctx.stroke();ctx.restore();
}
// A short damped wobble every `every` ms; it comes more often as the egg gets closer to hatching.
function wobble(now,every,amp){const t=now%every;return t>700?0:amp*Math.sin(t/700*Math.PI*3)*(1-t/700);}
const IDLE={incubating:[5200,.045],small:[3400,.06],large:[2000,.08]};
function fallback(ctx,state,now,reduced){const cracked=state!=='incubating';SharedPetView.paint(ctx,cracked?'kCrackedEggFrames':'kEggFrames',cracked?(state==='large'?1:0):(reduced?0:Math.floor(now/650)%2),94,45,3,'#fff0cf');}
function draw(ctx,age,now,reduced=false,fastDemo=false,petId=''){
 const state=phase(age);if(state==='hatched')return false;
 if(!ready()){if(window.SharedPetView)fallback(ctx,state,now,reduced);return true;}
 const row=design(petId),[every,amp]=IDLE[state],angle=reduced?0:wobble(now,every,amp),breathe=reduced?1:1+.012*Math.sin(now/2600*Math.PI*2);
 if(state==='incubating')cell(ctx,designs,row,0,{angle,breathe});
 else{cell(ctx,hatch,state==='small'?0:1,row,{angle,breathe});if(state==='large')crackLight(ctx,row,reduced?.3:.2+.25*(.5+.5*Math.sin(now/450)),3,{angle});}
 return true;
}
// Hatch break, elapsed 0-1850ms: shaking with growing crack light, then the cap lifts a little
// along the crack and light pours out of the gap. The flash and the baby belong to PetGrowthView.
const SHAKE_MS=1000,LIFT_MS=450,LIFT=.04*SHEET_EGG_HEIGHT;
function drawHatch(ctx,petId,elapsed,reduced=false){
 if(!ready())return false;const row=design(petId);
 if(reduced||elapsed<SHAKE_MS){const u=reduced?1:elapsed/SHAKE_MS,angle=reduced?0:Math.sin(elapsed/45)*.07*u;
  cell(ctx,hatch,1,row,{angle});crackLight(ctx,row,.3+.6*u,2+3*u,{angle});return true;}
 const u=Math.min(1,(elapsed-SHAKE_MS)/LIFT_MS),ease=1-(1-u)**3,lift=LIFT*ease,tilt=-.05*ease;
 crackLight(ctx,row,1,6+10*ease,{lift:lift/2});
 cell(ctx,hatch,3,row);cell(ctx,hatch,2,row,{angle:tilt,lift});
 crackLight(ctx,row,.55,4+6*ease,{lift:lift/2});return true;
}
window.PetEggView={phase,label,design,draw,drawHatch,EGG};
})();
