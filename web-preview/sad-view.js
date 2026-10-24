/* Shared presentation-only sad preview; normal mode follows domain thresholds. */
(()=>{'use strict';
function active(p){return p.sleepMode==='awake'&&!p.isDeparted&&p.healthState!=='dead'&&PetRules.updateSad({...p});}
function bird(ctx,p,now,treat=false){
 const frames=BirdSadFrames[p.stage==='baby'?'kBabyBirdSadFrames':'kBirdSadFrames'];
 const bits=frames[treat||matchMedia('(prefers-reduced-motion: reduce)').matches?0:Math.floor(now/1000)%2];
 ctx.fillStyle='#fffaf0';for(let r=0;r<44;r++)for(let c=0;c<64;c++)if(!(bits[r*8+(c>>3)]&(128>>(c%8))))ctx.fillRect(94+c*3,45+r*3,3,3);
}
window.PetSadView={active,bird};
})();
