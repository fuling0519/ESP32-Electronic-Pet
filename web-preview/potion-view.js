/* Web healing potion. The pixels are computed from simple shapes each frame, so the tilt, the draining
   liquid and the lighting stay consistent at every angle without hand-drawing each frame. */
(()=>{'use strict';
const SIZE=76,PIVOT=38,PX=1.5,R=12.5,DURATION=1400,GLOW=34;
const C={line:[61,47,74],liquid:['#1f5a4a','#2b765f','#3f9878','#60bb91','#97ddb7','#d8f8e6'],glass:['#a9c4cf','#cfe3e9','#eef8fa'],
 cork:['#56372a','#7b5038','#a37150','#c99a6c'],gold:['#8a5d1b','#c9922f','#f3d47e'],heart:['#b8486a','#ef7f9c','#ffc4d2'],shine:'#ffffff'};
const hex=h=>[parseInt(h.slice(1,3),16),parseInt(h.slice(3,5),16),parseInt(h.slice(5,7),16)];
const RGB={};for(const k of ['liquid','glass','cork','gold','heart'])RGB[k]=C[k].map(hex);RGB.shine=hex(C.shine);
const LIGHT=(()=>{const v=[-.5,-.62,.6],m=Math.hypot(...v);return v.map(x=>x/m);})();
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v)),span=(t,a,b)=>clamp((t-a)/(b-a),0,1),smooth=u=>u*u*(3-2*u);

// Bottle shape in local units (native pixels, origin at the body centre, neck pointing up).
function glassAt(x,y){
 if(x*x+y*y<=R*R)return 'body';
 if(Math.abs(x)<=3.5&&y>=-18.5&&y<-9)return 'neck';
 if(Math.abs(x)<=4.5&&y>=-20.5&&y<-18.5)return 'lip';
 return null;
}
const ringAt=(x,y)=>Math.abs(x)<=4.5&&y>=-12.5&&y<-10.5;
const corkAt=(x,y)=>y>=-25&&y<-20.5&&Math.abs(x)<=(y<-23.5?3.6:3);
// Small heart emblem on the glass, in local pixels relative to the body centre.
const HEART=['.#.#.','#####','#####','.###.','..#..'];
function heartAt(x,y){const c=Math.floor(x+2.5),r=Math.floor(y-.5);return HEART[r]?.[c]==='#'?r:null;}
function insideAt(x,y){
 if(x*x+y*y<=(R-1.4)**2)return true;
 return Math.abs(x)<=2.2&&y>=-20.5&&y<-9;
}

// timeline (ms): fade in and cork pop while upright, tip over, pour, then the empty bottle fades out.
function pose(ms,reduced){
 const tip=reduced?0:smooth(span(ms,420,700));
 return {
  angle:-112*tip*Math.PI/180,
  drop:reduced?0:(1-smooth(span(ms,0,180)))*-6,
  alpha:span(ms,0,140)*(1-span(ms,1220,1400)),
  volume:.8*(1-span(ms,reduced?300:640,1200)),
  cork:span(ms,220,520),
  glow:reduced?.5:1-span(ms,600,900)
 };
}

let buffer=null,image=null;
function render(ms,reduced){
 const p=pose(ms,reduced),c=Math.cos(p.angle),s=Math.sin(p.angle);
 buffer=buffer||Object.assign(document.createElement('canvas'),{width:SIZE,height:SIZE});
 const g=buffer.getContext('2d');image=image||g.createImageData(SIZE,SIZE);const d=image.data;d.fill(0);
 const kind=new Array(SIZE*SIZE).fill(null),local=new Array(SIZE*SIZE),interior=[];
 const corkShift=p.cork>0?{x:10*p.cork,y:-3-18*p.cork+22*p.cork*p.cork}:null;
 for(let j=0;j<SIZE;j++)for(let i=0;i<SIZE;i++){
  const wx=i+.5-PIVOT,wy=j+.5-PIVOT,lx=c*wx+s*wy,ly=-s*wx+c*wy,k=j*SIZE+i;local[k]=[lx,ly];
  if(!p.cork&&corkAt(lx,ly))kind[k]='cork';
  else if(ringAt(lx,ly))kind[k]='gold';
  else if(glassAt(lx,ly))kind[k]=insideAt(lx,ly)?'inside':'wall';
  else if(corkShift&&p.cork<1&&corkAt(wx-corkShift.x,wy-corkShift.y))kind[k]='flying';
  if(kind[k]==='inside')interior.push(k);
 }
 // Liquid settles at the bottom in screen space, whatever the bottle's angle.
 interior.sort((a,b)=>Math.floor(b/SIZE)-Math.floor(a/SIZE)||(b%SIZE)-(a%SIZE));
 const liquid=new Set(interior.slice(0,Math.round(interior.length*p.volume)));
 const solid=k=>kind[k]&&kind[k]!=='flying';
 const put=(k,rgb,a=255)=>{d[k*4]=rgb[0];d[k*4+1]=rgb[1];d[k*4+2]=rgb[2];d[k*4+3]=a;};
 for(let k=0;k<SIZE*SIZE;k++){
  const t=kind[k],i=k%SIZE,j=Math.floor(k/SIZE);
  // A second outline ring just outside the shape doubles the stroke without eating into the glass.
  if(!t){if([[1,0],[-1,0],[0,1],[0,-1]].some(([a,b])=>{const x=i+a,y=j+b;return x>=0&&y>=0&&x<SIZE&&y<SIZE&&solid(y*SIZE+x);}))put(k,C.line);continue;}
  const [lx,ly]=local[k];
  if(t==='flying'){const fx=i+.5-PIVOT-corkShift.x,rim=[[1,0],[-1,0],[0,1],[0,-1]].some(([a,b])=>kind[(j+b)*SIZE+i+a]!=='flying');put(k,rim?C.line:RGB.cork[fx<-1?3:fx<1.5?2:1],Math.round(255*(1-p.cork)));continue;}
  const edge=[[1,0],[-1,0],[0,1],[0,-1]].some(([a,b])=>{const x=i+a,y=j+b;return x<0||y<0||x>=SIZE||y>=SIZE||!solid(y*SIZE+x);});
  if(edge){put(k,C.line);continue;}
  // Sphere lighting for the body, a cylinder for the neck; light from the upper left of the screen.
  const wx=i+.5-PIVOT,wy=j+.5-PIVOT,body=lx*lx+ly*ly<=R*R;
  let nx,ny,nz;if(body){nx=wx/R;ny=wy/R;nz=Math.sqrt(Math.max(0,1-nx*nx-ny*ny));}else{const n=clamp(lx/3.5,-1,1);nx=n*c;ny=n*s;nz=Math.sqrt(1-n*n);}
  const lit=nx*LIGHT[0]+ny*LIGHT[1]+nz*LIGHT[2];
  if(t==='cork'){put(k,RGB.cork[ly<-22.5?3:lx<-1.5?3:lx<1?2:lx<2.5?1:0]);continue;}
  if(t==='gold'){put(k,RGB.gold[lit>.55?2:lit>.1?1:0]);continue;}
  const heart=body?heartAt(lx,ly):null;
  if(heart!==null){put(k,RGB.heart[heart===0&&lx<0?2:heart>=3?0:1]);continue;}
  const angle=Math.atan2(ly,lx)*180/Math.PI,r=Math.hypot(lx,ly);
  if(body&&r>=8.2&&r<=10&&angle>=-168&&angle<=-112||body&&Math.abs(lx+3.6)<.8&&Math.abs(ly+5.8)<.8||!body&&Math.abs(lx+1.6)<.6&&ly>-18&&ly<-13){put(k,RGB.shine,t==='wall'?200:235);continue;}
  if(t==='wall'){put(k,RGB.glass[lit>.5?2:lit>0?1:0],215);continue;}
  if(liquid.has(k)){
   const surface=kind[k-SIZE]==='inside'&&!liquid.has(k-SIZE);let tone=lit>.75?4:lit>.45?3:lit>.15?2:lit>-.2?1:0;
   if(body&&r>=9.6&&angle>=20&&angle<=70)tone=Math.max(tone,3); // light bouncing back through the glass
   put(k,RGB.liquid[surface?Math.min(5,tone+2):tone]);continue;
  }
  put(k,RGB.glass[lit>.5?2:1],lit>.5?70:45);
 }
 // A few bubbles rise while the bottle is upright and full.
 if(p.volume>.2&&!reduced)for(let b=0;b<3;b++){
  const life=(ms/1000*(.9+b*.25)+b*.37)%1,i=PIVOT-5+b*5,j=Math.round(PIVOT+9-life*15),k=j*SIZE+i;
  if(liquid.has(k)&&liquid.has(k-SIZE))put(k,RGB.liquid[5]);
 }
 g.putImageData(image,0,0);
 return p;
}

// Falling drops, aligned to the same pixel grid as the bottle.
function drops(ctx,ms,x,y,p){
 const mx=Math.sin(p.angle)*20.5,my=-Math.cos(p.angle)*20.5;
 ctx.save();
 for(let born=640;born<1180;born+=50){
  const age=(ms-born)/420;if(age<0||age>=1)continue;
  const h=Math.sin(born*12.9898)*43758.5453%1,fx=mx-1-age*6-Math.abs(h)*3,fy=my+age*age*58+age*6;
  const gx=x+Math.round(fx)*PX,gy=y+Math.round(fy)*PX;
  ctx.globalAlpha=1-Math.max(0,age-.75)*4;
  ctx.fillStyle=C.liquid[3];ctx.fillRect(gx,gy,PX*2,PX*3);ctx.fillStyle=C.liquid[5];ctx.fillRect(gx,gy,PX,PX);
 }
 ctx.restore();
}

function sparkle(ctx,x,y,size,alpha){
 ctx.globalAlpha=alpha;ctx.fillStyle=C.gold[2];
 ctx.fillRect(x-PX/2,y-size,PX,size*2);ctx.fillRect(x-size,y-PX/2,size*2,PX);ctx.fillStyle='#ffffff';ctx.fillRect(x-PX/2,y-PX/2,PX,PX);
}

// cx, cy: screen position of the body centre while upright.
// fps: the pets animate at 5-7 frames per second, so time is held in steps to match them; 0 = every display frame.
function draw(ctx,elapsedSeconds,cx=264,cy=74,reduced=false,fps=7){
 const raw=elapsedSeconds*1000;if(raw<0||raw>=DURATION||typeof document==='undefined')return false;
 const ms=fps>0?Math.floor(raw*fps/1000)*1000/fps:raw;
 const p=render(ms,reduced),x=cx-PIVOT*PX,y=Math.round(cy+p.drop*PX)-PIVOT*PX;
 ctx.save();ctx.globalAlpha=p.alpha;
 if(p.glow>0){const r=ctx.createRadialGradient(cx,cy,3,cx,cy,GLOW);r.addColorStop(0,'rgba(170,245,205,.55)');r.addColorStop(1,'rgba(170,245,205,0)');ctx.globalAlpha=p.alpha*p.glow;ctx.fillStyle=r;ctx.fillRect(cx-GLOW,cy-GLOW,GLOW*2,GLOW*2);
  for(const [dx,dy,ph] of [[-23,-17,0],[21,-23,.35],[23,10,.7]]){const tw=(Math.sin(ms/160+ph*6)+1)/2;sparkle(ctx,cx+dx,cy+dy,PX*(1+Math.round(tw*2)),p.alpha*p.glow*(.4+.6*tw));}
  ctx.globalAlpha=p.alpha;}
 ctx.imageSmoothingEnabled=false;ctx.drawImage(buffer,x,y,SIZE*PX,SIZE*PX);
 if(!reduced)drops(ctx,ms,cx,cy,p);
 ctx.restore();return true;
}
window.PotionView={draw,duration:DURATION};
})();
