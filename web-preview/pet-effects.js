/* Code-drawn pixel effects (hearts, Zzz, exclamation) and the sleeping breath. No image assets.
   Glyphs are bitmaps with a dark outline so they stay readable on grass by
   day and night. The garden uses the 'fine' style; 'pixel' is the earlier
   3px-cell version kept for the preview comparison. */
(()=>{'use strict';
// 1 = fill, 2 = highlight.
const G={
 heartBig:['.11.11.','1211111','1111111','.11111.','..111..','...1...'],
 heartSmall:['11.11','12111','.111.','..1..'],
 zBig:['111111','....11','...11.','..11..','.11...','111111'],
 zMid:['11111','...1.','..1..','.1...','11111'],
 zSmall:['1111','..1.','.1..','1111'],
 bang:['.11.','1211','1211','1211','1111','.11.','.11.','....','.11.','.11.'],
 dashUp:['1','1','1'],
 dashLeft:['1..','.1.','..1'],
 dashRight:['..1','.1.','1..']
};
// Fine style: the same shapes rasterised at 1 canvas px per cell (about 3x the
// detail) with a 2px outline, close to the pets' own pixel grain and line weight.
// A cell is filled when 4x4 supersampling finds it at least half covered.
// Cell codes: o outline, 1 fill, 2 highlight, 4 mid shade, 3 deep shade.
const PAD=2,SS=4;
function build(w,h,inside,highlight=()=>false,shade=()=>null){
 const W=w+PAD*2,H=h+PAD*2,HW=W*SS,HH=H*SS,mask=new Uint8Array(HW*HH),grown=new Uint8Array(HW*HH),r=PAD*SS;
 for(let j=0;j<HH;j++)for(let i=0;i<HW;i++){
  const x=(i+.5)/SS-PAD,y=(j+.5)/SS-PAD;
  mask[j*HW+i]=x>=0&&y>=0&&x<=w&&y<=h&&inside(x,y)?1:0;
 }
 for(let j=0;j<HH;j++)for(let i=0;i<HW;i++){
  if(!mask[j*HW+i])continue;
  for(let dj=-r;dj<=r;dj++)for(let di=-r;di<=r;di++){
   const ii=i+di,jj=j+dj;
   if(di*di+dj*dj<=r*r&&ii>=0&&jj>=0&&ii<HW&&jj<HH)grown[jj*HW+ii]=1;
  }
 }
 const rows=[];
 for(let y=0;y<H;y++){
  let row='';
  for(let x=0;x<W;x++){
   let c1=0,c2=0;
   for(let j=0;j<SS;j++)for(let i=0;i<SS;i++){const k=(y*SS+j)*HW+x*SS+i;c1+=mask[k];c2+=grown[k];}
   c1/=SS*SS;c2/=SS*SS;
   const hi=highlight(x+.5-PAD,y+.5-PAD);
   row+=c1>=.5?(hi?'2':shade(x+.5-PAD,y+.5-PAD)||'1'):c2>=.5?'o':'.';
  }
  rows.push(row);
 }
 return rows;
}
const heartTest=(w,h)=>(x,y)=>{const nx=(x/w-.5)*2.3,ny=(1-y/h)*2.3-1.05,q=nx*nx+ny*ny-1;return q*q*q-nx*nx*ny*ny*ny<=0;};
// Coloured-pixel-art shading: cells near the lower-right edge (light from the
// upper left) step down to a mid then a deep red.
const heartShade=(w,h)=>{const inside=heartTest(w,h);return (x,y)=>!inside(x+1.6,y+1.6)?'3':!inside(x+3.2,y+3.2)?'4':null;};
const heartHi=(w,h)=>(x,y)=>{const nx=(x/w-.5)*2.3,ny=(1-y/h)*2.3-1.05;return (nx+.52)**2*1.6+(ny-.6)**2<.045;};
function zTest(w,h,t){
 const ax=w-t*.3,ay=t,bx=t*.3,by=h-t,len=Math.hypot(bx-ax,by-ay);
 return (x,y)=>y<t||y>h-t||Math.abs((by-ay)*x-(bx-ax)*y+bx*ay-by*ax)/len<=t*.42;
}
function bangTest(w,h){
 const stem=h*.64,r=w*.32,dotY=h-r;
 return (x,y)=>y<stem?Math.abs(x-w/2)<=w/2-y/stem*w*.2-(y<1.5?1:0):Math.hypot(x-w/2,y-dotY)<=r;
}
const bangHi=(w,h)=>(x,y)=>x>w*.22&&x<w*.4&&y>2.5&&y<h*.64*.6;
const SHAPES={
 heartBig:[21,18,heartTest(21,18),heartHi(21,18),heartShade(21,18)],heartSmall:[15,13,heartTest(15,13),heartHi(15,13),heartShade(15,13)],
 zBig:[18,18,zTest(18,18,4)],zMid:[15,15,zTest(15,15,3)],zSmall:[12,12,zTest(12,12,3)],
 bang:[11,28,bangTest(11,28),bangHi(11,28)],
 dashUp:[2,8,()=>true],
 dashLeft:[7,7,(x,y)=>Math.abs(x-y)<=1.1],
 dashRight:[7,7,(x,y)=>Math.abs(x+y-7)<=1.1]
};
const F={};
for(const [name,[w,h,inside,hi,shade]] of Object.entries(SHAPES))F[name]=build(w,h,inside,hi,shade);

const outlines=new Map();
function ring(rows){
 if(outlines.has(rows))return outlines.get(rows);
 const h=rows.length,w=rows[0].length,on=(x,y)=>y>=0&&y<h&&x>=0&&x<w&&rows[y][x]!=='.',cells=[];
 for(let y=-1;y<=h;y++)for(let x=-1;x<=w;x++){
  if(on(x,y))continue;
  for(let dy=-1;dy<=1;dy++)for(let dx=-1;dx<=1;dx++)if(on(x+dx,y+dy)){cells.push([x,y]);dy=dx=2;}
 }
 outlines.set(rows,cells);return cells;
}
const PALETTE={
 heart:{line:'#5b2147',fill:'#ff5f8f',mid:'#ec4679',shade:'#c92f62',hi:'#ffe3ec'},
 z:{line:'#2f2b5c',fill:'#eef0ff',hi:'#ffffff'},
 bang:{line:'#4e2c08',fill:'#ffc928',hi:'#fff6c8'}
};
// Draws `rows` centered on (cx,cy) with optional pop scale and opacity.
function glyph(ctx,rows,cx,cy,u,colors,{scale=1,alpha=1}={}){
 if(alpha<=0||scale<=0)return;
 const h=rows.length,w=rows[0].length;
 ctx.save();ctx.globalAlpha*=Math.min(1,alpha);
 // Whole-pixel offset: odd widths would put every cell edge on a half pixel.
 ctx.translate(Math.round(cx),Math.round(cy));ctx.scale(scale,scale);ctx.translate(-Math.round(w*u/2),-Math.round(h*u/2));
 // One path per colour so neighbouring cells never leave antialiased seams.
 const layer=(color,cells)=>{ctx.beginPath();for(const [x,y] of cells)ctx.rect(x*u,y*u,u,u);ctx.fillStyle=color;ctx.fill();};
 const cells={o:[],1:[],3:[],4:[],2:[]};
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){const c=rows[y][x];if(cells[c])cells[c].push([x,y]);}
 // Hand-written pixel glyphs get a computed 1-cell ring; fine glyphs carry their own outline.
 layer(colors.line,cells.o.length?cells.o:ring(rows));
 layer(colors.fill,cells[1]);
 if(cells[4].length)layer(colors.mid,cells[4]);if(cells[3].length)layer(colors.shade,cells[3]);
 if(cells[2].length)layer(colors.hi,cells[2]);
 ctx.restore();
}
// Draws a named effect glyph: 'fine' (default) or the earlier 'pixel' 3px cells.
function shape(ctx,name,cx,cy,colors,{scale=1,alpha=1,style='fine'}={}){
 if(style==='pixel')glyph(ctx,G[name],cx,cy,name.startsWith('dash')?2:3,colors,{scale,alpha});
 else glyph(ctx,F[name],cx,cy,1,colors,{scale,alpha});
}
const clamp=t=>Math.max(0,Math.min(1,t));
// Overshoot pop: 0 -> 1.25 -> 1 over `ms`.
function pop(t,ms=160){t=clamp(t/ms);return t<.6?1.25*(t/.6):1.25-.25*((t-.6)/.4);}

/* Hearts: like the firmware, one at a time, alternating left/right of the head,
   big/small mixed, each popping in, swaying upward and fading. `held` keeps
   spawning; otherwise only hearts spawned before `until` ms are drawn. */
const HEART_GAP=280,HEART_LIFE=1150;
function hearts(ctx,ms,{x,y,spread=34,until=Infinity,reduced=false,style}={}){
 if(reduced){
  shape(ctx,'heartBig',x-spread,y-6,PALETTE.heart,{style});shape(ctx,'heartSmall',x+spread,y-18,PALETTE.heart,{style});return;
 }
 const last=Math.floor(Math.min(ms,until)/HEART_GAP);
 for(let i=Math.max(0,last-5);i<=last;i++){
  const born=i*HEART_GAP;if(born>until)break;
  const age=ms-born;if(age<0||age>=HEART_LIFE)continue;
  const side=i%2?1:-1,big=i%3!==1,t=age/HEART_LIFE;
  const hx=x+side*(spread+(i%4)*3)+Math.sin(age/170+i)*4,hy=y-4-t*40;
  shape(ctx,big?'heartBig':'heartSmall',hx,hy,PALETTE.heart,{scale:pop(age),alpha:t<.6?1:1-(t-.6)/.4,style});
 }
}

/* Zzz: three Z's growing small -> big, drifting up-right from the head in turn. */
const Z_CYCLE=2100,Z_LIFE=1800;
function zzz(ctx,ms,{x,y,reduced=false,style}={}){
 const zs=['zSmall','zMid','zBig'];
 if(reduced){zs.forEach((g,i)=>shape(ctx,g,x+i*14,y-i*16,PALETTE.z,{style}));return;}
 for(let k=-1;k<=0;k++)for(let i=0;i<3;i++){
  const age=ms-(Math.floor(ms/Z_CYCLE)+k)*Z_CYCLE-i*500;if(age<0||age>=Z_LIFE)continue;
  const t=age/Z_LIFE;
  shape(ctx,zs[i],x+i*12+t*16+Math.sin(age/260+i)*3,y-i*12-t*26,PALETTE.z,
   {scale:pop(age,220),alpha:t<.7?1:1-(t-.7)/.3,style});
 }
}

/* Exclamation for a poke (1.2 s): pops in, shakes, with three burst dashes. */
const SHOCK_MS=1200;
function shock(ctx,ms,{x,y,reduced=false,style}={}){
 if(ms<0||ms>=SHOCK_MS)return;
 const fade=reduced?1:ms<SHOCK_MS-200?1:(SHOCK_MS-ms)/200;
 const shake=reduced||ms<160||ms>620?0:Math.round(Math.sin(ms/28)*2*(1-(ms-160)/460));
 shape(ctx,'bang',x+shake,y,PALETTE.bang,{scale:reduced?1:pop(ms,200),alpha:fade,style});
 // Burst dashes radiate from the mark and fade by 650 ms.
 const burst=reduced?.5:clamp(ms/220),gone=reduced?1:clamp((650-ms)/250);
 if(gone<=0)return;
 for(const [a,g] of [[-2.36,'dashLeft'],[-1.57,'dashUp'],[-.79,'dashRight']]){
  const r=21+burst*6;
  shape(ctx,g,x+Math.cos(a)*r,y-9+Math.sin(a)*r,PALETTE.bang,{alpha:gone*fade,style});
 }
}
// Sleeping breath shared by every species: a 3% vertical swell around the feet every 3.2 s (about 3 canvas px).
function breath(now,reduced=false){return reduced?1:1+.03*(1-Math.cos(Math.max(0,now)%3200/3200*Math.PI*2))/2;}
window.PetEffects={hearts,zzz,shock,breath,glyph,shape,glyphs:G,fineGlyphs:F,HEART_GAP,HEART_LIFE,SHOCK_MS};
})();
