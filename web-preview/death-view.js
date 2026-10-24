/* Visual-only death sequence; never changes the pet, its memorial record or the save. */
(()=>{'use strict';
const W=384,H=256,GROUND=184,STONE_X=190;
// Timeline (ms): the scene dims while the pet lies down 0-1200; the body melts into rising light motes
// from the head down 1200-3600 (the main moment) while a pale copy of the pet floats out of it at the
// same time 1200-3000, shrinks into a ball of light 3000-3600 and flies off the top 3600-4800;
// a gravestone rises 4400-5600, flowers sprout 5400-6200 and from 6200 the grave rests with drifting
// petals (day) or fireflies (night).
const T={dim:1200,dissolve:[1200,3600],emerge:[1200,1800],float:[1800,3000],condense:[3000,3600],ascend:[3600,4800],stone:[4400,5600],flowers:[5400,6200],name:[5600,6100],rest:6200};
// Soul colour is shared by every species; the shape always comes from the pet's own idle art.
const SOUL={wash:'rgba(232,244,255,.8)',halo:'rgba(170,215,255,.95)',core:'255,255,255',rim:'190,226,255'};
const FALLBACK={x:150,y:100,w:80,h:80,points:[]};
const clamp=v=>Math.max(0,Math.min(1,v)),span=(t,[a,b])=>clamp((t-a)/(b-a));
const ease=u=>u*u*(3-2*u),easeIn=u=>u*u,easeOut=u=>1-(1-u)*(1-u);
const hash=i=>{const s=Math.sin(i*12.9898+78.233)*43758.5453;return s-Math.floor(s);};
function canvas(){return typeof document==='undefined'?null:Object.assign(document.createElement('canvas'),{width:W,height:H});}
// Returns false while the illustrated pose is still loading and a stand-in was drawn instead.
function pose(ctx,species,stage,state){
 if(species===2){if(window.WyvernAdultView?.paint(ctx,state,0,true,{stage}))return true;if(window.WyvernView)WyvernView.paint(ctx,WyvernFrames[stage==='baby'?'kBabyWyvernIdleFrames':'kWyvernIdleFrames'][0]);return !window.WyvernAdultView;}
 if(species===1)window.BirdView?.still(ctx,stage,state==='dead',190,135,3);
 return true;
}
// Opaque bounds plus a 4 px grid of body pixels for the light motes. Null until the art has loaded.
function scan(g){
 let d;try{d=g.getImageData(0,0,W,H)?.data;}catch{return null;}if(!d)return null;
 let x0=W,y0=H,x1=-1,y1=-1;const points=[];
 for(let y=0;y<H;y++)for(let x=0;x<W;x++)if(d[(y*W+x)*4+3]>140){if(x<x0)x0=x;if(x>x1)x1=x;if(y<y0)y0=y;if(y>y1)y1=y;if(x%4===0&&y%4===0)points.push([x,y]);}
 return x1<0?null:{x:x0,y:y0,w:x1-x0+1,h:y1-y0+1,points};
}
const layers=new Map();
// One cached offscreen copy per pose; the soul copy is washed pale, the body shows to just past the
// middle, then it fades out so the feet are fully transparent.
// Only finished art is cached: measuring a stand-in would fix the soul and halo to the wrong outline.
function layer(species,stage,state,soul=false){
 const key=[species,stage,state,soul].join(':');if(layers.has(key))return layers.get(key);
 const c=canvas();if(!c)return null;const g=c.getContext('2d');g.imageSmoothingEnabled=false;const ready=pose(g,species,stage,state);
 const box=scan(g);
 if(soul&&g.createLinearGradient){const b=box||FALLBACK;g.save();g.globalCompositeOperation='source-atop';g.fillStyle=SOUL.wash;g.fillRect(0,0,W,H);
  g.globalCompositeOperation='destination-out';const tail=g.createLinearGradient(0,b.y+b.h*.55,0,b.y+b.h*.88);tail.addColorStop(0,'rgba(0,0,0,0)');tail.addColorStop(.6,'rgba(0,0,0,.7)');tail.addColorStop(1,'rgba(0,0,0,1)');g.fillStyle=tail;g.fillRect(0,0,W,H);g.restore();}
 const result={canvas:c,box:box||FALLBACK};if(ready&&box)layers.set(key,result);return result;
}
let work=null;
// The body brightens and is erased from the top down behind a soft edge.
function body(ctx,L,t){
 const u=span(t,T.dissolve);if(u>=1)return;
 if(u<=0||!L.canvas.getContext){ctx.drawImage(L.canvas,0,0);return;}
 work=work||canvas();const g=work.getContext('2d'),b=L.box,edge=b.y-12+u*(b.h+24);
 g.save();g.clearRect(0,0,W,H);g.drawImage(L.canvas,0,0);
 g.globalCompositeOperation='source-atop';g.fillStyle=`rgba(${SOUL.core},${.25+.5*u})`;g.fillRect(0,0,W,H);
 g.globalCompositeOperation='destination-out';const m=g.createLinearGradient(0,edge-12,0,edge+12);m.addColorStop(0,'rgba(0,0,0,1)');m.addColorStop(1,'rgba(0,0,0,0)');g.fillStyle=m;g.fillRect(0,0,W,edge+12);
 g.restore();ctx.drawImage(work,0,0);
}
// Each sampled body pixel leaves as a mote when the erase edge passes it.
function motes(ctx,L,t){
 const b=L.box,pts=b.points;if(!pts.length)return;
 const step=Math.max(1,Math.ceil(pts.length/170)),[d0,d1]=T.dissolve,life=1900;
 ctx.save();ctx.shadowColor=SOUL.halo;ctx.shadowBlur=4;
 for(let i=0;i<pts.length;i+=step){const [x,y]=pts[i],h=hash(i),born=d0+(y-b.y)/b.h*(d1-d0-400)+h*300,age=(t-born)/life;if(age<0||age>=1)continue;
  ctx.globalAlpha=(1-age)*.9;ctx.fillStyle=h<.4?'#ffffff':h<.75?'#dff1ff':'#fff4d6';
  const s=h>.82?3:2;ctx.fillRect(Math.round(x+Math.sin(age*5+i)*4*age+(h-.5)*18*age),Math.round(y-easeOut(age)*(36+h*44)),s,s);}
 ctx.restore();
}
// One steady, slow rise from the body until the soul has shrunk into the ball of light.
function soulPlace(t,b){
 const cx=b.x+b.w/2,cy=b.y+b.h/2,u=span(t,[T.emerge[0],T.condense[1]]),sway=Math.sin((t-T.emerge[0])/520)*5*Math.min(1,(t-T.emerge[0])/800);
 return {x:cx+sway,y:cy-60*u+Math.sin(t/380)*1.5};
}
// Halo floating just above the head: centred on the pixels in the top quarter of the silhouette, so horns or
// crests of any species push it up rather than off to one side.
function halo(ctx,b){
 const top=b.points.filter(([,y])=>y<b.y+b.h*.25),hx=top.length?top.reduce((a,[x])=>a+x,0)/top.length:b.x+b.w/2;
 const rx=Math.max(11,Math.min(24,b.w*.2)),ry=rx*.32,hy=b.y-ry-3;
 ctx.save();ctx.shadowColor=SOUL.halo;ctx.shadowBlur=12;ctx.lineWidth=5;ctx.strokeStyle='#e4f2ff';
 ctx.beginPath();ctx.ellipse?.(hx,hy,rx,ry,0,0,Math.PI*2);ctx.stroke();
 ctx.shadowBlur=0;ctx.lineWidth=2;ctx.strokeStyle='#ffffff';ctx.beginPath();ctx.ellipse?.(hx,hy-1,rx-1,ry-1,0,Math.PI*1.1,Math.PI*1.9);ctx.stroke();ctx.restore();
}
function soul(ctx,species,stage,t,base){
 if(t<T.emerge[0]||t>=T.condense[1])return;
 const S=layer(species,stage,'idle',true);if(!S)return;
 const p=soulPlace(t,base),c=easeIn(span(t,T.condense)),scale=(.55+.27*ease(span(t,[T.emerge[0],T.float[0]+600])))*(1-.7*c);
 const sb=S.box,sx=sb.x+sb.w/2,sy=sb.y+sb.h/2;
 ctx.save();ctx.globalAlpha=.75*ease(span(t,T.emerge))*(1-c);ctx.shadowColor=SOUL.halo;ctx.shadowBlur=14;
 ctx.translate(p.x,p.y);ctx.scale(scale,scale);ctx.translate(-sx,-sy);ctx.drawImage(S.canvas,0,0);halo(ctx,sb);ctx.restore();
}
function glow(ctx,x,y,r,alpha){
 if(!ctx.createRadialGradient||alpha<=0)return;
 ctx.save();for(const [R,stops] of [[r*2.4,[[0,.5],[.45,.2],[1,0]]],[r,[[0,1],[.6,.9],[1,0]]]]){
  const g=ctx.createRadialGradient(x,y,0,x,y,R);for(const [at,a] of stops)g.addColorStop(at,`rgba(${at<.5?SOUL.core:SOUL.rim},${a*alpha})`);
  ctx.fillStyle=g;ctx.beginPath();ctx.arc(x,y,R,0,Math.PI*2);ctx.fill();}
 ctx.restore();
}
function sparkle(ctx,x,y,size,alpha){ctx.save();ctx.globalAlpha=alpha;ctx.fillStyle='#ffffff';ctx.fillRect(Math.round(x-size),Math.round(y)-1,size*2,2);ctx.fillRect(Math.round(x)-1,Math.round(y-size),2,size*2);ctx.restore();}
function orbAt(t,base){
 const start=soulPlace(T.condense[1],base),u=easeIn(span(t,T.ascend));
 return {x:start.x+Math.sin(t/260)*4*u,y:start.y+(-40-start.y)*u};
}
function orb(ctx,t,base){
 if(t<T.condense[0]||t>=T.ascend[1])return;
 const grow=ease(span(t,T.condense)),fade=1-clamp((t-(T.ascend[1]-400))/400),p=t<T.ascend[0]?soulPlace(t,base):orbAt(t,base);
 // Trail: earlier positions on the same path, fading behind the ball.
 if(t>=T.ascend[0])for(let i=1;i<=6;i++){const q=orbAt(t-i*45,base);ctx.save();ctx.globalAlpha=(1-i/7)*.7*fade;ctx.fillStyle=i%2?'#ffffff':'#dff1ff';ctx.fillRect(Math.round(q.x)-1,Math.round(q.y+i*2)-1,3,3);ctx.restore();}
 glow(ctx,p.x,p.y,(4+9*grow)*(1+.08*Math.sin(t/90)),grow*fade);
 const tw=(t-T.ascend[0])/500;if(tw>=0&&tw<1)sparkle(ctx,p.x,p.y,Math.round(4+10*Math.sin(Math.PI*tw)),1-tw);
}
// Placeholder pixel-art stone (2 px cells) until painted art replaces it; the face stays blank for text.
const STONE={w:30,h:34,cell:2,base:2};
function stoneShape(ctx,x,y){
 const {w,h,cell:c}=STONE,r=w/2;
 for(let row=0;row<h;row++){const dy=r-row-.5,hw=row<r?Math.round(Math.sqrt(Math.max(0,r*r-dy*dy))):r,left=x+(r-hw)*c,width=hw*2*c,top=y+row*c;
  if(hw<=0)continue;ctx.fillStyle='#4f4650';ctx.fillRect(left,top,width,c);
  if(row===0||hw<2)continue;ctx.fillStyle='#bdb6ae';ctx.fillRect(left+c,top,width-2*c,c);
  ctx.fillStyle='#d9d3ca';ctx.fillRect(left+c,top,Math.min(3,hw-1)*c,c);
  ctx.fillStyle='#958d85';ctx.fillRect(left+width-Math.min(4,hw)*c,top,Math.min(3,hw-1)*c,c);}
 ctx.fillStyle='#4f4650';ctx.fillRect(x+c,y+(h-1)*c,(w-2)*c,c);
 // Base slab and moss.
 ctx.fillStyle='#4f4650';ctx.fillRect(x-2*c,y+h*c,(w+4)*c,STONE.base*c+2);ctx.fillStyle='#8f877f';ctx.fillRect(x-c,y+h*c+c,(w+2)*c,STONE.base*c-c);ctx.fillStyle='#a69e95';ctx.fillRect(x-c,y+h*c+c,(w+2)*c,c);
 ctx.fillStyle='#7aa257';for(const [cx,cy,cw] of [[1,h-3,3],[2,h-2,4],[w-5,h-2,4],[w-4,h-3,2],[4,7,2],[5,6,1]])ctx.fillRect(x+cx*c,y+cy*c,cw*c,c);
 ctx.fillStyle='#5e8a45';for(const [cx,cy,cw] of [[1,h-2,2],[w-4,h-1,3]])ctx.fillRect(x+cx*c,y+cy*c,cw*c,c);
}
function engrave(ctx,text,x,y,font,max){
 ctx.save();ctx.font=font;ctx.textAlign='center';ctx.textBaseline='middle';const w=ctx.measureText?.(text)?.width||0;
 if(w>max)ctx.font=font.replace(/\d+px/,m=>Math.max(8,Math.floor(parseInt(m)*max/w))+'px');
 ctx.fillStyle='#ece7df';ctx.fillText(text,x+1,y+1);ctx.fillStyle='#5d544d';ctx.fillText(text,x,y);ctx.restore();
}
// The stone is painted once at 1:1 and drawn as one image: row-by-row rectangles on the home page's
// non-integer device-pixel scale left faint seams between rows.
let stoneCanvas=null;
function stoneSprite(ctx,x,y){
 const {w,h,cell:c,base}=STONE,pad=2*c,sw=(w+4)*c,sh=(h+base)*c+2;
 if(!stoneCanvas&&typeof document!=='undefined'){stoneCanvas=Object.assign(document.createElement('canvas'),{width:sw,height:sh});stoneShape(stoneCanvas.getContext('2d'),pad,0);}
 if(!stoneCanvas){stoneShape(ctx,x,y);return;}
 ctx.save();ctx.imageSmoothingEnabled=false;ctx.drawImage(stoneCanvas,x-pad,y);ctx.restore();
}
function stone(ctx,t,name,shake=true){
 const u=easeOut(span(t,T.stone));if(u<=0)return;
 const {w,h,cell:c,base}=STONE,fullH=(h+base)*c+2,x=STONE_X-w*c/2,y=GROUND-fullH,jitter=shake&&u<1?Math.round(Math.sin(t/30))*1:0;
 ctx.save();ctx.globalAlpha=.25*u;ctx.fillStyle='#2d2a20';ctx.beginPath();ctx.ellipse?.(STONE_X+3,GROUND,w*c/2+12,5,0,0,Math.PI*2);ctx.fill();ctx.restore();
 ctx.save();ctx.beginPath();ctx.rect(0,0,W,GROUND+1);ctx.clip();ctx.translate(jitter,Math.round((1-u)*fullH));
 stoneSprite(ctx,x,y);
 const n=span(t,T.name);if(n>0){ctx.globalAlpha=n;const face=x+w*c/2;
  engrave(ctx,'R.I.P',face,y+20,'700 9px "Courier New",monospace',w*c-16);
  ctx.fillStyle='#8a8279';ctx.fillRect(face-14,y+28,28,1);
  engrave(ctx,name||'',face,y+40,'900 12px "Microsoft JhengHei","PingFang TC","Noto Sans TC",sans-serif',w*c-12);}
 ctx.restore();
}
function dust(ctx,t){
 const u=(t-T.stone[0])/1000;if(u<0||u>=1)return;
 ctx.save();for(let i=0;i<8;i++){const side=i%2?1:-1,h=hash(i+40),r=3+6*easeOut(u)*(.6+h*.6);
  ctx.globalAlpha=(1-u)*.55;ctx.fillStyle='#d8c9a6';ctx.beginPath();ctx.arc(STONE_X+side*(STONE.w+6+30*easeOut(u)*(.4+h)),GROUND-2-12*u*h,r,0,Math.PI*2);ctx.fill();}
 ctx.restore();
}
// [offset from stone centre, petal colour, stem height in cells]; 3 px cells so they read beside the stone.
const FLOWERS=[[-50,'#f6a6c1',4],[-40,'#ffe07a',6],[-31,'#ffffff',3],[33,'#c9a7f0',5],[43,'#ffffff',3],[52,'#f6a6c1',6]];
function flowers(ctx,t,now,still){
 const c=3;
 FLOWERS.forEach(([dx,color,cells],i)=>{const u=span(t,[T.flowers[0]+i*110,T.flowers[0]+i*110+380]);if(u<=0)return;
  const sway=still?0:Math.round(Math.sin(now/700+i*1.7)*.8),x=STONE_X+dx,stem=Math.round(cells*c*easeOut(Math.min(1,u*1.4))),top=GROUND-stem;
  ctx.fillStyle='#3f7a33';ctx.fillRect(x,top,c-1,stem);
  if(u>.3){ctx.fillStyle='#69a84a';ctx.fillRect(x+(i%2?c-1:-c),GROUND-2*c,c,c);ctx.fillRect(x+(i%2?2*c-1:-2*c),GROUND-3*c,c,c);}
  if(u<.65)return;const b=x+sway-1;
  if(u<.85){ctx.fillStyle=color;ctx.fillRect(b-1,top-c-1,c+2,c+1);return;}
  ctx.fillStyle=color;ctx.fillRect(b-c,top-c,c,c);ctx.fillRect(b+c,top-c,c,c);ctx.fillRect(b,top-2*c,c,c);ctx.fillRect(b,top,c,c);
  ctx.fillStyle=color==='#ffe07a'?'#e58a3a':'#ffc93d';ctx.fillRect(b,top-c,c,c);});
}
// Gentle life around the resting grave; nothing here moves with reduced motion.
function ambient(ctx,t,now,night){
 const fade=clamp((t-T.rest)/1200);if(fade<=0)return;ctx.save();
 if(night){ctx.shadowColor='rgba(230,255,150,.9)';ctx.shadowBlur=6;
  for(let i=0;i<5;i++){const a=.5+.5*Math.sin(now/600+i*2.1);if(a<.15)continue;ctx.globalAlpha=fade*a;ctx.fillStyle='#f2ffb0';
   ctx.fillRect(Math.round(STONE_X+Math.cos(now/1900+i*1.3)*(48+i*6)),Math.round(GROUND-40+Math.sin(now/1400+i*2)*24-i*4),2,2);}}
 else for(let i=0;i<4;i++){const cycle=7000,q=((now+i*cycle/4)%cycle)/cycle,x=STONE_X-90+q*180+Math.sin(q*12+i)*8,y=GROUND-110+q*95+Math.sin(q*9+i*2)*6;
  ctx.globalAlpha=fade*Math.sin(Math.PI*q)*.9;ctx.fillStyle=i%2?'#f6a6c1':'#ffd1dc';ctx.fillRect(Math.round(x),Math.round(y),3,2);ctx.fillRect(Math.round(x)+1,Math.round(y)+2,1,1);}
 ctx.restore();
}
// Cool dusk wash over the whole garden; lifts a little once the grave is at rest.
function dim(ctx,t,night){
 const peak=night?.18:.24,rest=night?.08:.1;
 const a=t<T.dim?peak*ease(t/T.dim):t<T.stone[0]?peak:peak+(rest-peak)*ease(span(t,[T.stone[0],T.rest]));
 ctx.save();ctx.fillStyle=`rgba(${night?'10,14,40':'34,40,78'},${a})`;ctx.fillRect(0,0,W,H);ctx.restore();
}
function phase(t){return t<T.dissolve[0]?'fall':t<T.ascend[1]?'soul':t<T.rest?'grave':'rest';}
// Returns {phase, rest}; rest is true once the afterlife menu may show.
function draw(ctx,{species=1,stage='adult',name='',elapsed=0,now=elapsed,reduced=false,night=false}={}){
 const t=reduced?Math.max(elapsed,T.rest):Math.max(0,elapsed);
 ctx.save();ctx.imageSmoothingEnabled=false;dim(ctx,t,night);
 const L=layer(species,stage,'dead'),base=(L||{box:FALLBACK}).box;
 if(t<T.dissolve[1]){if(L)body(ctx,L,t);else pose(ctx,species,stage,'dead');}
 if(t<T.ascend[1]){motes(ctx,{box:base},t);soul(ctx,species,stage,t,base);orb(ctx,t,base);}
 dust(ctx,t);stone(ctx,t,name,!reduced);flowers(ctx,t,now,reduced);if(!reduced)ambient(ctx,t,now,night);
 ctx.restore();return {phase:phase(t),rest:t>=T.rest};
}
window.PetDeathView={T,draw,phase};
})();
