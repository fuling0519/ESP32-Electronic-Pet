/* Shared presentation-only sad preview; normal mode follows domain thresholds. */
(()=>{'use strict';
let preview=false;
const controls=document.querySelector('.preview-controls');
if(controls){const label=document.createElement('label');label.textContent='傷心表情預覽 ';const input=document.createElement('input');input.type='checkbox';input.setAttribute('aria-label','傷心表情預覽');input.onchange=()=>{preview=input.checked;};label.appendChild(input);controls.appendChild(label);}
function active(p){return p.sleepMode==='awake'&&!p.isDeparted&&p.healthState!=='dead'&&(preview||PetRules.updateSad({...p}));}
function bird(ctx,p,now,treat=false){
 const frames=BirdSadFrames[p.stage==='baby'?'kBabyBirdSadFrames':'kBirdSadFrames'];
 const bits=frames[treat||matchMedia('(prefers-reduced-motion: reduce)').matches?0:Math.floor(now/1000)%2];
 ctx.fillStyle='#fffaf0';for(let r=0;r<44;r++)for(let c=0;c<64;c++)if(!(bits[r*8+(c>>3)]&(128>>(c%8))))ctx.fillRect(94+c*3,45+r*3,3,3);
 if(!treat){ctx.fillStyle='#302535';const x=p.stage==='baby'?222:246,y=p.stage==='baby'?87:63;for(let i=0;i<3;i++)ctx.fillRect(x+i*4,y+i*2,1,8);}
}
window.PetSadView={active,bird,get preview(){return preview;}};
})();
