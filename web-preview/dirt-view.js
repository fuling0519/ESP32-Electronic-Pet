/* Dirt: the garden view grimes over from its corners and edges (a pixel overlay at the garden's own grain) and
   the pet turns dull (greyed and browned over its own shape, so it always moves with it); when very dirty, stink
   wisps rise off the pet and flies circle it. Code-drawn, no assets. Presentation only. */
(()=>{'use strict';
// Same bands as the firmware's four dirt frames: >75 clean, then three levels.
function level(cleanliness){const c=Number(cleanliness);return !Number.isFinite(c)||c>75?0:c>50?1:c>25?2:3;}
const clamp=t=>Math.max(0,Math.min(1,t));

/* ---- Screen grime: 256×171 cells over the 3:2 garden, about the background's own pixel size. ---- */
const GW=256,GH=171;
function hash(x,y,s){let h=x*374761393+y*668265263+s*982451653;h=(h^(h>>13))*1274126177;return ((h^(h>>16))>>>0)/4294967295;}
function noise(x,y,s){const xi=Math.floor(x),yi=Math.floor(y),fx=x-xi,fy=y-yi,u=fx*fx*(3-2*fx),v=fy*fy*(3-2*fy);
 const a=hash(xi,yi,s),b=hash(xi+1,yi,s),c=hash(xi,yi+1,s),d=hash(xi+1,yi+1,s);return a+(b-a)*u+(c-a)*v+(a-b-c+d)*u*v;}
const fbm=(x,y)=>noise(x,y,7)*.55+noise(x*2,y*2,8)*.3+noise(x*4,y*4,9)*.15;
// How far in from the edges the grime reaches, and how opaque it gets, per level.
const REACH=[0,.09,.15,.22],STRENGTH=[0,.6,.8,1];
const grimeCache=new Map();
// RGBA pixels of one level: dried mud creeping in from the corners and edges with a ragged rim, in three
// depth bands, lit on its upper-left edges and shaded on its lower-right ones like the garden; nothing beyond it.
function grimePixels(l){
 if(grimeCache.has(l))return grimeCache.get(l);
 const px=new Uint8ClampedArray(GW*GH*4),reach=REACH[l],k=STRENGTH[l];
 if(l){
  const field=(x,y)=>{
   const edge=Math.min(Math.min(x,GW-1-x)/GW*1.6,Math.min(y,GH-1-y)/GH*1.2);
   const corner=Math.min(Math.hypot(x/GW,y/GH),Math.hypot(1-x/GW,y/GH),Math.hypot(x/GW,1-y/GH),Math.hypot(1-x/GW,1-y/GH));
   return (reach-Math.min(edge,corner*.5))/reach+(fbm(x/12,y/12)-.5)*1.6;};
  for(let y=0;y<GH;y++)for(let x=0;x<GW;x++){
   const f=field(x,y),i=(y*GW+x)*4;
   if(f<=0)continue;
   const lit=field(x-1,y-1)<f-.1,shade=field(x+1,y+1)<f-.1,band=f>1?3:f>.5?2:1;
   let c=band===3?[96,68,40]:band===2?[122,90,56]:[150,116,78];
   if(shade&&!lit)c=c.map(v=>v*.76);else if(lit)c=c.map(v=>Math.min(255,v*1.2));
   px.set([...c.map(Math.round),Math.round((band===1?95:band===2?160:215)*k)],i);
  }
 }
 // Drop islands the noise leaves out in the garden: anything not joined to the edge grime reads as dust.
 const seen=new Uint8Array(GW*GH);
 for(let start=0;start<GW*GH;start++){
  if(seen[start]||!px[start*4+3])continue;
  const part=[start];seen[start]=1;let edge=false;
  for(let k=0;k<part.length;k++){const i=part[k],x=i%GW,y=(i-x)/GW;if(!x||!y||x===GW-1||y===GH-1)edge=true;
   for(const j of [x>0?i-1:-1,x<GW-1?i+1:-1,y>0?i-GW:-1,y<GH-1?i+GW:-1])if(j>=0&&!seen[j]&&px[j*4+3]){seen[j]=1;part.push(j);}}
  if(!edge)for(const i of part)px[i*4+3]=0;
 }
 grimeCache.set(l,px);return px;
}
let overlay=null,overlayKey='';
const sources=new Map();
function source(l){if(!sources.has(l)){const c=document.createElement('canvas');c.width=GW;c.height=GH;c.getContext('2d').putImageData(new ImageData(grimePixels(l),GW,GH),0,0);sources.set(l,c);}return sources.get(l);}
// Shows the grime for `l` over `scene`; while cleaning, `alpha` cross-fades it down to the level `keep` that remains.
function screen(scene,l,{keep=0,alpha=1}={}){
 if(!scene||typeof document==='undefined')return;
 keep=Math.min(keep,l);const fade=clamp(alpha),key=l+':'+keep+':'+Math.round(fade*24);
 if(key===overlayKey)return;overlayKey=key;
 if(!overlay){overlay=document.createElement('canvas');overlay.width=GW;overlay.height=GH;overlay.className='screen-dirt';overlay.setAttribute('aria-hidden','true');}
 if(overlay.parentNode!==scene)scene.append(overlay);
 const g=overlay.getContext('2d');g.clearRect(0,0,GW,GH);
 if(keep&&fade<1){g.globalAlpha=1-fade;g.drawImage(source(keep),0,0);}
 if(l&&fade>0){g.globalAlpha=fade;g.drawImage(source(l),0,0);}
 g.globalAlpha=1;overlay.hidden=!l;
}

/* ---- The pet: dull over its own pixels, plus stink and flies at the dirtiest level. ---- */
// Per level: how much colour is taken out, and how much warm grey is multiplied in.
const DULL=[[0,0],[.18,.12],[.34,.22],[.5,.34]];
let layer=null;
// A colour silhouette of the pet: `color` cut to the shape of what ctx holds (only the pet at this point).
function silhouette(ctx,color){
 if(!layer)layer=document.createElement('canvas');
 if(layer.width!==ctx.canvas.width||layer.height!==ctx.canvas.height){layer.width=ctx.canvas.width;layer.height=ctx.canvas.height;}
 const l=layer.getContext('2d');l.globalCompositeOperation='source-over';l.fillStyle=color;l.fillRect(0,0,layer.width,layer.height);
 l.globalCompositeOperation='destination-in';l.drawImage(ctx.canvas,0,0);return layer;
}
// box: {cx, top, bottom, w, head?} in canvas px around the pet's body; head is the top of its head (defaults to top).
// Call right after drawing the pet and before anything else, while the canvas holds only the pet.
// `alpha` fades only what is being washed away, down to the level `keep` that remains after cleaning.
function draw(ctx,{level:l,box,now=0,alpha=1,keep=0,reduced=false,smell=true}){
 keep=Math.min(keep,l);const fade=clamp(alpha);
 if(!l||alpha<=0&&!keep)return;
 const sat=DULL[keep][0]+(DULL[l][0]-DULL[keep][0])*fade,mul=DULL[keep][1]+(DULL[l][1]-DULL[keep][1])*fade,base=ctx.globalAlpha??1;
 const canLayer=typeof document!=='undefined'&&ctx.canvas&&typeof ctx.setTransform==='function';
 ctx.save();
 if(canLayer){
  ctx.setTransform(1,0,0,1,0,0);
  ctx.globalCompositeOperation='saturation';ctx.globalAlpha=base*sat;ctx.drawImage(silhouette(ctx,'#808080'),0,0);
  ctx.globalCompositeOperation='multiply';ctx.globalAlpha=base*mul;ctx.drawImage(silhouette(ctx,'#a88f72'),0,0);
 }else{ctx.globalCompositeOperation='source-atop';ctx.globalAlpha=base*mul;ctx.fillStyle='#7c6448';ctx.fillRect(0,0,ctx.canvas?.width||384,ctx.canvas?.height||256);}
 ctx.restore();
 if(l===3&&smell&&box)stink(ctx,box,now,keep===3?1:fade,reduced);
}
const SS=4;
// Rasterises inside(x,y) onto 1 px cells with 4×4 supersampling. Codes: 2 lit rim (upper left), 3 shaded rim
// (lower right), 1 body, o optional outline ring, g glint cells chosen by `glint`.
function build(w,h,inside,{outline=false,glint=()=>false,rim=1.2}={}){
 const pad=outline?1:0,rows=[];
 const cov=(cx,cy,dx=0,dy=0)=>{let n=0;for(let j=0;j<SS;j++)for(let i=0;i<SS;i++)if(inside(cx+(i+.5)/SS+dx,cy+(j+.5)/SS+dy))n++;return n/(SS*SS);};
 for(let y=-pad;y<h+pad;y++){let row='';for(let x=-pad;x<w+pad;x++){
  if(cov(x,y)>=.5){row+=glint(x+.5,y+.5)?'g':cov(x,y,rim,rim)<.5?'3':cov(x,y,-rim,-rim)<.5?'2':'1';continue;}
  let near=false;if(outline)for(let dy=-1;dy<=1&&!near;dy++)for(let dx=-1;dx<=1;dx++)if((dx||dy)&&cov(x+dx,y+dy)>=.5){near=true;break;}
  row+=near?'o':'.';}
  rows.push(row);}
 return {rows,ox:pad,oy:pad};
}
// Paints glyph rows at (x,y) = glyph origin, one path per colour so cells never leave seams.
function paint(ctx,g,x,y,colors){
 const layers={};
 g.rows.forEach((row,j)=>{for(let i=0;i<row.length;i++){const c=row[i];if(colors[c])(layers[c]||=[]).push([i,j]);}});
 for(const c of ['o','1','3','2','g'])if(layers[c]){ctx.beginPath();for(const [i,j] of layers[c])ctx.rect(x+i-g.ox,y+j-g.oy,1,1);ctx.fillStyle=colors[c];ctx.fill();}
}
// Pale, whitish yellow-green wisps with a soft rim, kept translucent so they read as smell rather than grass.
const STINK={o:'rgba(116,128,70,.55)',1:'#e2e9b4',3:'#c3cf84',2:'#fbfde9'};
// Stink wisp: a wavy, tapering stroke with an outline and lit/shaded sides, built once.
const WISP=(()=>{const w=9,h=20;return build(w,h,(x,y)=>{const t=y/h,cx=w/2+Math.sin(t*Math.PI*2.2)*2.2,half=.7+1.1*Math.sin(Math.PI*Math.min(1,t*1.15));return Math.abs(x-cx)<half;},{outline:true,rim:.9});})();
// Fly: dark body with a sheen; the pale wings are drawn separately so they can flicker.
const FLY=build(5,4,(x,y)=>Math.hypot((x-2.5)/2.2,(y-2.3)/1.6)<1,{glint:(x,y)=>Math.hypot(x-1.6,y-1.6)<.6});
const FLY_COLORS={1:'#2b2630',3:'#17131c',2:'#4a4352',g:'#c9c3d6'};
function stink(ctx,box,now,alpha,reduced){
 if(alpha<=0)return;
 const h=box.bottom-box.top,head=box.head??box.top;
 ctx.save();
 // Three wisps rise off the head in turn, swaying, popping in and fading at the top (like the Zzz).
 [-1,0,1].forEach((k,i)=>{
  const t=reduced?.45:(now/1700+i/3)%1,a=reduced?1:t<.2?t/.2:t>.75?(1-t)/.25:1;
  ctx.globalAlpha=alpha*a*.8;
  const x=Math.round(box.cx+k*box.w*.2+(reduced?0:Math.sin(now/420+i*2)*1.5)-WISP.rows[0].length/2),y=Math.round(head-6-WISP.rows.length-(reduced?6:t*14));
  paint(ctx,WISP,x,y,STINK);
 });
 // Two flies circle the body on tilted loops; wings flicker every 60 ms.
 for(let i=0;i<2;i++){
  const a=reduced?i*Math.PI+.6:now/560*(i?-1:1)+i*Math.PI;
  const x=Math.round(box.cx+Math.cos(a)*box.w*.66),y=Math.round(box.top+h*.3+Math.sin(a*2)*h*.14+Math.sin(a)*4);
  ctx.globalAlpha=alpha;
  const up=reduced||Math.floor(now/60)%2;
  ctx.fillStyle='rgba(236,244,255,.85)';ctx.fillRect(x,y-(up?3:2),2,up?2:1);ctx.fillRect(x+3,y-(up?3:2),2,up?2:1);
  ctx.fillStyle='rgba(150,170,200,.7)';ctx.fillRect(x+1,y-1,1,1);ctx.fillRect(x+3,y-1,1,1);
  paint(ctx,FLY,x,y,FLY_COLORS);
 }
 ctx.restore();
}
window.PetDirt={level,draw,screen,grimePixels,DULL,GRID:[GW,GH]};
})();
