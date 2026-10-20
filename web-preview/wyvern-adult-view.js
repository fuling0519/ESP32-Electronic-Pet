/* Web-only illustrated adult art. Domain state and firmware masks stay separate. */
(()=>{'use strict';
const base='assets/pets/wyvern/adult/';
const images=new Map();
function image(name){
 if(!images.has(name)){const im=new Image();im.src=base+name;images.set(name,im);}
 return images.get(name);
}
// Shared scale/anchor within each animation: never fit each frame independently.
const poses={
 idle:{file:'idle-subtle-sheet-v2.png',cell:777,columns:2,frames:[0,1,2,3],times:[220,160,220,320],anchor:[388,659],scale:126/522},
 sad:{file:'emotions-sheet-v1.png',cell:470,columns:2,frames:[0,1],times:[2200,240],anchor:[234,444],scale:126/423},
 sick:{file:'emotions-sheet-v1.png',cell:470,columns:2,frames:[2,3],times:[1400,900],anchor:[234,444],scale:120/399},
 happy:{file:'emotions-sheet-v1.png',cell:470,columns:2,frames:[4,5],times:[1800,650],anchor:[234,444],scale:126/368},
 sleep:{files:['sleep-00.png','sleep-01.png'],times:[1100,900],rect:[0,0,887,640],anchor:[456,588],scale:180/832},
 dead:{file:'dead-still-v1.png',times:[1],rect:[0,0,1684,934],anchor:[848,856],scale:200/1592},
 flight:{file:'flight-three-quarter-preview-v1.png',times:[1],rect:[0,0,1470,1070],anchor:[774,977],scale:172/1260}
};
for(const pose of Object.values(poses))for(const file of pose.files||[pose.file])image(file);
let preview='auto',reaction=null,lastState='',stateStart=0;
const controls=document.querySelector('.preview-controls');
let label,select,note;
if(controls){
 label=document.createElement('label');label.textContent='小龍素材 ';label.hidden=true;
 select=document.createElement('select');select.id='wyvern-art-state';select.setAttribute('aria-label','小龍素材預覽');
 for(const [value,text] of [['auto','跟隨寵物'],['idle','待機'],['sad','傷心'],['sick','生病'],['happy','開心'],['sleep','蜷縮睡眠'],['dead','死亡靜態'],['flight','飛行造型（靜態）']]){
  const option=document.createElement('option');option.value=value;option.textContent=text;select.appendChild(option);
 }
 select.onchange=()=>{preview=select.value;lastState='';};label.appendChild(select);controls.appendChild(label);
 note=document.createElement('small');note.hidden=true;note.textContent='只切換外觀，不改養成數值。飛行為靜態造型；吃飯與驚嚇專用圖待補。';controls.appendChild(note);
}
function eligible(p){return p&&p.stage==='adult'&&!p.isDeparted&&p.healthState!=='dead';}
function syncControls(p,enabled){
 if(label)label.hidden=!enabled;if(note)note.hidden=!enabled;
 if(!enabled){preview='auto';if(select)select.value='auto';reaction=null;}
}
document.addEventListener('pet-interaction-complete',e=>{
 const p=window.petDataManager?.read().pet;
 if(eligible(p)&&e.detail)reaction={kind:e.detail.kind,petId:p.petId,start:performance.now()};
});
function resolve(p,now,action){
 if(p.healthState==='dead')return 'dead';
 if(preview!=='auto'&&!action)return preview;
 if(p.sleepMode==='normal')return 'sleep';
 if(p.healthState==='sick'||action==='treat')return 'sick';
 if(reaction&&(reaction.petId!==p.petId||now-reaction.start>=1200))reaction=null;
 if(reaction&&!action)return reaction.kind==='pet'?'happy':'idle';
 if(action==='feed'||action==='clean')return 'idle';
 if(p.webExpression==='happy')return 'happy';
 if(window.PetSadView?.active(p)||PetRules.updateSad({...p,sadReasons:{...p.sadReasons}}))return 'sad';
 return 'idle';
}
function paint(ctx,state,elapsed=0,reduced=false,{x=190,y=177,size=1}={}){
 const pose=poses[state];if(!pose)return false;
 const cycle=pose.times.reduce((a,b)=>a+b,0);let phase=Math.max(0,elapsed)%cycle,index=0;
 if(!reduced)while(index<pose.times.length-1&&phase>=pose.times[index])phase-=pose.times[index++];
 const im=image(pose.files?pose.files[index]:pose.file);
 if(!im.complete||!im.naturalWidth)return false;
 let rect=pose.rect;
 if(pose.cell){const frame=pose.frames[index];rect=[frame%pose.columns*pose.cell,Math.floor(frame/pose.columns)*pose.cell,pose.cell,pose.cell];}
 const [sx,sy,w,h]=rect,scale=pose.scale*size;
 // Crop only at draw time; source files remain intact. Preserve each pose's proportions.
 ctx.save();ctx.imageSmoothingEnabled=false;
 ctx.drawImage(im,sx,sy,w,h,x-pose.anchor[0]*scale,y-pose.anchor[1]*scale,w*scale,h*scale);ctx.restore();
 return {state,index};
}
function draw(ctx,p,now,action,elapsed,reduced){
 if(p.stage!=='adult')return false;
 const state=resolve(p,now,action),key=p.petId+':'+state;
 if(lastState!==key){lastState=key;stateStart=now;}
 const flight=state==='flight',startled=reaction?.kind==='poke'&&!action&&state==='idle'&&!reduced;
 const offset=startled?Math.max(0,1-(now-reaction.start)/1200)*Math.abs(Math.sin((now-reaction.start)/85))*5:0;
 return paint(ctx,state,now-stateStart,reduced,{x:190,y:flight?143:177-offset});
}
window.WyvernAdultView={paint,draw,resolve,syncControls};
})();
