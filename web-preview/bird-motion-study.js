/* Isolated motion study. Existing PNGs only; no domain, storage or BLE access. */
(()=>{'use strict';
const definitions={idle:['待機：散步＋喝水',10000],sad:['傷心',5000],sick:['生病',5000],sleep:['睡眠',8000],happy:['開心／摸頭',6000],startled:['驚嚇／輕碰',4000],feed:['吃飯：啄食三次',6000],curious:['好奇睜眼',5000]};
const images={};for(const stage of ['baby','adult']){const im=new Image();im.src='assets/bird-'+stage+'-states-20261019.png';images[stage]=im;}
const clamp=n=>Math.max(0,Math.min(1,n)),ease=n=>{n=clamp(n);return n*n*(3-2*n);};
function sample(state,ms,reduced=false){
 const duration=definitions[state]?.[1]||10000,t=(Math.max(0,ms)%duration)/1000;
 const q={state,duration,t,x:190,y:157,head:0,nod:0,arms:0,breath:0,frame:0,water:false,drop:false,food:false,heart:false,shock:false,sweat:false};
 if(reduced){q.frame=state==='sleep'?4:state==='sad'||state==='sick'?3:0;q.food=state==='feed';return q;}
 q.frame=(t*1000)%3200>3000?1:0;
 if(state==='idle'){
  const route=t<4?ease(t/4):t<7?1:1-ease((t-7)/3);
  q.x=190-65*route;q.y=157+13*route;
  if(t<4||t>7)q.y-=Math.abs(Math.sin(t*1000/150))*1.8;
  if(t>=4.5&&t<6.8){const s=(t-4.5)/2.3;q.nod=Math.sin(s*Math.PI)**2;q.head=-.22*q.nod;q.water=true;q.drop=s>.75;}
 }else if(state==='sad'){
  q.frame=t<4.35?2:3;q.nod=.15+.04*Math.sin(t*Math.PI*2/5);q.head=.08;q.arms=-.05;q.y+=2;q.breath=Math.sin(t*Math.PI*2/5)*.5;
 }else if(state==='sick'){
  q.frame=t%3<.8?2:3;q.nod=.2;q.head=Math.sin(t*Math.PI*2/5)*.035;q.arms=-.08;q.y+=3;q.sweat=true;q.breath=Math.sin(t*Math.PI*2/5)*.4;
 }else if(state==='sleep'){
  q.frame=4;q.nod=.05;q.arms=-.05;q.breath=Math.sin(t*Math.PI*2/4)*.8;
 }else if(state==='happy'){
  q.frame=1;q.heart=true;q.head=.20*(1-ease((t-4.8)/1.2));
  const up=ease((t-1.4)/1.2),down=ease((t-3.7)/1.3),lift=up*(1-down);
  q.y-=40*lift;if(t>1&&t<1.4)q.y+=3*Math.sin((t-1)/.4*Math.PI);
  if(t>=1.35&&t<5)q.arms=.22+.22*Math.sin(t*Math.PI*5);
  if(t>=5&&t<5.5)q.y+=2*Math.sin((t-5)/.5*Math.PI);
 }else if(state==='startled'){
  q.shock=t<1.3;q.frame=t>.95&&t<1.1?1:0;
  if(t<.16)q.y+=3*ease(t/.16);
  else if(t<.86){const s=(t-.16)/.7;q.y-=30*4*s*(1-s);q.arms=.42*Math.sin(s*Math.PI);}
  else if(t<1.3)q.y+=2*Math.sin((t-.86)/.44*Math.PI);
  if(t>1.3)q.head=Math.sin((t-1.3)*Math.PI*1.2)*.10*(1-ease((t-2.7)/1.3));
 }else if(state==='feed'){
  q.food=true;
  for(const start of [1,2,3])if(t>=start&&t<start+.7){q.nod=Math.sin((t-start)/.7*Math.PI)**2;q.head=-.13*q.nod;}
  q.frame=t>4&&t<4.25?5:t>4.6&&t<4.8?1:0;
 }else if(state==='curious'){
  q.head=Math.sin(t*Math.PI*2/5)*.17;q.arms=.04;q.frame=t>1.8&&t<1.98||t>4.4&&t<4.58?1:0;
 }
 return q;
}
function render(ctx,stage,state,ms,reduced=false){
 const q=sample(state,ms,reduced),im=images[stage];if(!im?.complete||!im.naturalWidth)return false;
 const sw=im.naturalWidth/6,sh=im.naturalHeight,trim=q.frame>0?Math.round(sw*.045):0;
 // A single scale across every pose; never resize individual frames to fit.
 const width=stage==='adult'?142:120,unit=width/sw,height=sh*unit,left=q.x-width/2,top=q.y-height/2;
 const original=()=>ctx.drawImage(im,q.frame*sw+trim,0,sw-trim,sh,left+trim*unit,top,width-trim*unit,height);
 const head=[[0,0],[1,0],[1,.48],[.79,.48],[.72,.50],[.60,.53],[.20,.53],[.02,.50],[0,.48]];
 const right=[[.69,.51],[.79,.51],[.86,.58],[.84,.64],[.76,.67],[.66,.63],[.62,.56]];
 const leftArm=[[.07,.54],[.17,.54],[.19,.57],[.17,.62],[.10,.64],[.04,.61],[.03,.57]];
 const chest=[[.22,.54],[.60,.54],[.63,.65],[.55,.69],[.29,.67],[.19,.60]];
 const path=points=>{ctx.moveTo(left+points[0][0]*width,top+points[0][1]*height);for(const [x,y] of points.slice(1))ctx.lineTo(left+x*width,top+y*height);ctx.closePath();};
 ctx.save();ctx.imageSmoothingEnabled=false;
 // Unarticulated torso/feet; the separate pieces use unchanged source pixels.
 ctx.save();ctx.beginPath();ctx.rect(left-2,top-2,width+4,height+4);for(const poly of [head,right,leftArm,...(q.breath?[chest]:[])])path(poly);ctx.clip('evenodd');original();ctx.restore();
 function piece(points,hx,hy,angle,dy=0){const x=left+hx*width,y=top+hy*height;ctx.save();ctx.translate(x,y+dy);ctx.rotate(angle);ctx.translate(-x,-y);ctx.beginPath();path(points);ctx.clip();original();ctx.restore();}
 piece(right,.69,.52,q.arms,q.breath);piece(leftArm,.14,.54,-q.arms,q.breath);
 if(q.breath)piece(chest,.43,.61,0,q.breath);
 piece(head,.43,.50,q.head,q.nod*13+q.breath);
 ctx.restore();
 if(q.water){ctx.fillStyle='#bbebed';ctx.fillRect(q.x-29,q.y+23,19,2);ctx.fillRect(q.x-33,q.y+26,27,1);if(q.drop){ctx.fillStyle='#70c7df';ctx.fillRect(q.x-14,q.y+13,3,4);}}
 if(q.food&&window.SharedPetView){SharedPetView.paint(ctx,'kBowlFrames',q.t<2?0:q.t<3?1:2,q.x-21,q.y+height*.24,2,'#836046');}
 ctx.font='bold 18px sans-serif';
 if(q.heart){ctx.fillStyle='#e98ba8';ctx.fillText('♥',q.x+42,q.y-55-Math.sin(q.t)*4);}
 if(q.shock){ctx.fillStyle='#b47741';ctx.fillText('!',q.x+43,q.y-50);}
 if(q.sweat){ctx.fillStyle='#408eae';ctx.fillRect(q.x+43,q.y-39,3,7);ctx.fillRect(q.x+40,q.y-32,9,6);ctx.fillStyle='#c8f1fa';ctx.fillRect(q.x+43,q.y-31,3,3);}
 if(state==='sleep'){ctx.fillStyle='#716386';ctx.font='14px sans-serif';ctx.fillText('Zzz',q.x+44,q.y-45);}
 return q;
}
window.BirdMotionStudy={definitions,sample,render};
})();
