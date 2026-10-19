(()=>{'use strict';
function phase(age){return age>=300?'hatched':age>=270?'large':age>=240?'small':'incubating';}
function label(age){return {incubating:'孵化中 · 等待小生命',small:'孵化中 · 出現小裂痕',large:'孵化中 · 裂痕變大，即將出生',hatched:'孵化完成'}[phase(age)];}
function draw(ctx,age,now,reduced=false,fastDemo=false){const state=phase(age);if(state==='hatched')return false;
 const cracked=state!=='incubating',index=cracked?(state==='large'?1:0):(reduced?0:Math.floor(now/650)%2);
 SharedPetView.paint(ctx,cracked?'kCrackedEggFrames':'kEggFrames',index,94,45,3,'#fff0cf');
 ctx.fillStyle='#57483a';ctx.font='14px sans-serif';ctx.fillText(fastDemo&&state==='incubating'?'蛋 · 準備孵化':label(age),95,217);return true;}
window.PetEggView={phase,label,draw};
})();
