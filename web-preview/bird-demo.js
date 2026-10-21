/* Presentation-only bird demonstrations; no adapter calls or domain writes. */
(()=>{'use strict';
const baseDraw=BirdView.draw;
const states=[['auto','跟隨寵物'],['idle','待機（走動＋喝水）'],['sad','傷心'],['sick','生病'],['sleep','睡眠'],['happy','開心／摸頭'],['startled','驚嚇／戳碰'],['feed','吃飯'],['dead','長眠外觀']];
let preview='auto',enabled=false,reaction=null;
const controls=document.querySelector('.preview-controls');let label,select,note;
if(controls){label=document.createElement('label');label.textContent='小鳥展示 ';label.hidden=true;select=document.createElement('select');select.id='bird-art-state';select.setAttribute('aria-label','小鳥外觀展示');for(const [value,text] of states){const o=document.createElement('option');o.value=value;o.textContent=text;select.appendChild(o);}select.onchange=()=>preview=select.value;label.appendChild(select);controls.appendChild(label);note=document.createElement('small');note.hidden=true;note.textContent='僅展示外觀，不改數值；開心／驚嚇沿用現有角色搭配效果，長眠為閉眼示意。';controls.appendChild(note);}
function syncControls(p,allow){enabled=!!allow;if(label)label.hidden=!enabled;if(note)note.hidden=!enabled;if(!enabled){preview='auto';reaction=null;if(select)select.value='auto';}}
document.addEventListener('pet-interaction-complete',e=>{if(enabled&&e.detail){const p=window.petDataManager?.read().pet;if(p)reaction={kind:e.detail.kind,petId:p.petId,start:performance.now()};}});
function paint(ctx,p,state,now,cx=190,cy=126,scale=3,reduced=false){
 const model={...p,healthState:'healthy',sleepMode:'awake',mood:80,satiety:80,cleanliness:100,sadReasons:{mood:false,satiety:false,cleanliness:false}};
 let action=null,elapsed=0,sad=false,time=reduced?0:now;
 if(state==='sad')sad=true;
 if(state==='sick')model.healthState='sick';
 if(state==='sleep'||state==='dead')model.sleepMode='normal';
 if(state==='feed'){action='feed';elapsed=(time%3000)/1000;}
 if(state==='happy')time=reduced?1800:1600+time%1000;
 const dy=state==='startled'&&!reduced?Math.abs(Math.sin(now/90))*3:0;
 const drawn=state==='idle'||state==='water'?idle(ctx,model,time,cx,cy,scale,reduced,state==='water'):baseDraw(ctx,model,time,action,elapsed,sad,cx,cy-dy,scale);
 if(!drawn)return false;
 if(state==='feed')window.WhiteBirdFeed?.bowl(ctx,p,elapsed);
 ctx.fillStyle=state==='happy'?'#df819d':'#574039';ctx.font='bold 20px sans-serif';
 if(state==='happy')ctx.fillText('♥',cx+48,cy-50);
 if(state==='startled')ctx.fillText('!',cx+48,cy-50);
 if(state==='sleep'){ctx.font='14px sans-serif';ctx.fillText('Zzz',cx+48,cy-50);}
 return true;
}
function waterPose(now,cx,cy,onlyWater=false){
 const t=onlyWater?12+(now%5000)/1000:(now%24000)/1000;
 if(t<8)return null;
 // Grass bank beside the lower stream, to the right of the bridge deck.
 const startX=cx-12,targetX=125,targetY=170;
 let progress=t<12?(t-8)/4:t<17?1:t<21?1-(t-17)/4:0;
 progress=progress*progress*(3-2*progress);
 return {x:startX+(targetX-startX)*progress,y:cy+(targetY-cy)*progress,splash:t>=12&&t<17,walking:t<12||t>=17&&t<21};
}
function drinkPose(now){
 const t=(now%2000)/2000;
 const phase=t<.30?t/.30:t<.48?1:t<.78?1-(t-.48)/.30:0;
 const dip=phase*phase*(3-2*phase);
 return {dip,angle:-.28*dip,sipping:t>=.30&&t<.48,swallowing:t>=.78};
}
function idle(ctx,p,now,cx,cy,scale,reduced,onlyWater=false){
 if(window.WhiteBirdView){
  if(window.WhiteBirdActivity?.draw(ctx,p,now,cx,cy,scale,reduced))return true;
  return baseDraw(ctx,p,reduced?0:now,null,0,false,cx,cy,scale);
 }
 const water=!reduced&&waterPose(now,cx,cy,onlyWater);
 if(water){
  const drink=drinkPose(now),bob=water.walking?-Math.round(Math.abs(Math.sin(now/160))*2):0;
  const result=baseDraw(ctx,p,now,water.splash&&drink.swallowing?'feed':null,(water.splash&&drink.swallowing)?0.3:0,false,water.x,water.y+bob,scale,water.splash?drink.dip:0);
  if(result&&water.splash&&ctx.fillRect){
   const beakX=water.x-17,waterY=water.y+18;
   ctx.save();ctx.fillStyle='#b7edf2';
   if(drink.dip>.7){ctx.fillRect(beakX-9,waterY,18,2);ctx.fillRect(beakX-13,waterY+3,26,1);}
   if(drink.sipping){ctx.fillStyle='#ecffff';ctx.fillRect(beakX-4,waterY-2,8,2);}
   else if(drink.swallowing){ctx.fillStyle='#78c9df';ctx.fillRect(water.x-9,water.y+5,3,4);ctx.fillStyle='#efffff';ctx.fillRect(water.x-9,water.y+5,1,2);}
   ctx.restore();
  }return result;
 }
 // Short walks with pauses; keep the original wings and sprite proportions.
 const t=(now%8000)/1000,walking=t<3||(t>=4&&t<7);
 const offset=reduced?0:t<3?t*8:t<4?24:t<7?24-(t-4)*8:0;
 const step=reduced||!walking?0:-Math.round(Math.abs(Math.sin(now/160))*2);
 return baseDraw(ctx,p,reduced?0:now,null,0,false,cx+offset-(!reduced?12:0),cy+step,scale);
}
function idleActive(p){return enabled&&['auto','idle'].includes(preview)&&p?.healthState==='healthy'&&p.sleepMode==='awake'&&!p.isDeparted&&!PetRules.updateSad({...p,sadReasons:{...p.sadReasons}});}
BirdView.draw=function(ctx,p,now,action,elapsed,sad,cx=190,cy=126,scale=3){
 if(!enabled||action||p.isDeparted||p.healthState==='dead'||!['baby','adult'].includes(p.stage))return baseDraw(ctx,p,now,action,elapsed,sad,cx,cy,scale);
 let state=preview;
 if(reaction&&(reaction.petId!==p.petId||now-reaction.start>=1200||p.healthState!=='healthy'||p.sleepMode!=='awake'))reaction=null;
 if(state==='auto'&&reaction)state=reaction.kind==='pet'?'happy':'startled';
 if(state==='auto')return !sad&&idleActive(p)?idle(ctx,p,now,cx,cy,scale,window.matchMedia?.('(prefers-reduced-motion: reduce)').matches||false):baseDraw(ctx,p,now,action,elapsed,sad,cx,cy,scale);
 return paint(ctx,p,state,now,cx,cy,scale,window.matchMedia?.('(prefers-reduced-motion: reduce)').matches||false);
};
window.BirdDemo={paint,syncControls,states,idleActive,waterPose,drinkPose,previewing:()=>enabled&&preview!=='auto'};
})();
