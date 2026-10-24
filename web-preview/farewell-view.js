/* Visual-only farewell (departure) sequence and postcard; never changes the pet, its memorial or the save. */
(()=>{'use strict';
const W=384,H=256;
// Timeline (ms): the viewfinder closes in on a happy pet 0-2000; the shutter fires at 2000 and the flash
// fades by 2400; the pet hops twice 2400-3000, then walks (or flies) off up the path, shrinking into the
// distance 3000-5600, leaving a twinkle 5500-5900; the postcard drops in 5800-7000 and rests from 7000.
// Hearts keep floating for one heart lifetime after the hops end.
const HEART_SPAN=1800;
const T={finder:2000,shot:2000,flash:[2000,2400],hop:[2400,3000],leave:[3000,5600],twinkle:[5500,5900],card:[5800,7000],rest:7000};
const MESSAGES=['謝謝你的照顧，我出發囉！','我會帶著你給的溫暖，去看更大的世界。','旅途上看到好看的風景，都會想起你。','別擔心，我會好好吃飯、好好睡覺的。','花園的午後，是我最喜歡的時光。','有一天，換我寄風景給你。','謝謝你陪我長大，我會想你的。'];
const CARD={w:300,h:192};
const HAND='"DFKai-SB","BiauKai","KaiTi","STKaiti","Klee One",serif',SANS='"Microsoft JhengHei","PingFang TC","Noto Sans TC",sans-serif';
const clamp=v=>Math.max(0,Math.min(1,v)),span=(t,[a,b])=>clamp((t-a)/(b-a));
const ease=u=>u*u*(3-2*u),easeOut=u=>1-(1-u)*(1-u),easeOutBack=u=>1+2.2*(u-1)**3+1.2*(u-1)**2;
function hashOf(text){let h=2166136261;for(const ch of String(text||''))h=Math.imul(h^ch.codePointAt(0),16777619);return h>>>0;}
function message(petId){return MESSAGES[hashOf(petId)%MESSAGES.length];}
function canvas(w=W,h=H){return typeof document==='undefined'?null:Object.assign(document.createElement('canvas'),{width:w,height:h});}
const walkImage=typeof Image==='undefined'?null:Object.assign(new Image(),{src:'assets/pets/bird/white-activity-sheet-v1.png'});
const birdModel=stage=>({stage,healthState:'healthy',sleepMode:'awake',satiety:100,mood:80,cleanliness:100,sadReasons:{mood:false,satiety:false,cleanliness:false}});
const birdHeight=stage=>stage==='adult'?132:106;
// Where each species stands at rest; bird art is centred, wyvern art is anchored at its feet.
function home(species,stage){return species===2?{x:190,feet:177}:{x:190,feet:135+birdHeight(stage)*.47};}
// state: 'idle' | 'happy' | 'walk' (wyvern adults fly instead). Bird walk frames face left.
function pose(ctx,species,stage,state,now,{x,feet,size=1,facing=-1,opacity=1}){
 ctx.save();ctx.globalAlpha*=opacity;let ok;
 if(species===2){
  const s=state==='walk'?(stage==='adult'?'flight':'walk'):state;
  ok=window.WyvernAdultView?.paint(ctx,s,now,false,{x,y:feet,size,facing:state==='walk'?facing:1,stage});
 }else if(state==='walk'&&walkImage?.complete&&walkImage.naturalWidth&&window.WhiteBirdActivityData){
  const d=WhiteBirdActivityData,fit=birdHeight(stage)*size*.94/d.standingHeight,frame=Math.floor(now/240)%2;
  ctx.imageSmoothingEnabled=false;ctx.translate(x,feet);if(facing>0)ctx.scale(-1,1);
  ctx.drawImage(walkImage,frame*d.cell,0,d.cell,d.cell,-d.anchor[0]*fit,-d.anchor[1]*fit,d.cell*fit,d.cell*fit);ok=true;
 }else{const h=birdHeight(stage)*size;ok=window.BirdView?.draw(ctx,birdModel(stage),now,null,0,false,x,feet-h*.47,3*size);}
 ctx.restore();return !!ok;
}
function sparkle(ctx,x,y,size,alpha){ctx.save();ctx.globalAlpha=alpha;ctx.fillStyle='#fffbe8';ctx.fillRect(Math.round(x-size),Math.round(y)-1,size*2,2);ctx.fillRect(Math.round(x)-1,Math.round(y-size),2,size*2);ctx.restore();}
// Departure path: walkers head up-left along the garden path into the distance; adult wyverns fly off.
function leaving(species,stage,t){
 const u=ease(span(t,T.leave)),h=home(species,stage),fly=species===2&&stage==='adult';
 const to=fly?{x:96,feet:-6}:{x:118,feet:104};
 const lift=fly?Math.sin(Math.min(1,u*1.4)*Math.PI/2)*28:0;
 return {x:h.x+(to.x-h.x)*u,feet:h.feet+(to.feet-h.feet)*u-lift,size:1-.58*u,opacity:1-clamp((u-.62)/.38)};
}
function finder(ctx,t,cx,cy){
 const u=ease(span(t,[0,1200])),a=Math.min(1,t/300),inset=8+30*u,arm=22;
 ctx.save();
 if(ctx.createRadialGradient){const g=ctx.createRadialGradient(W/2,H/2,H*.3,W/2,H/2,W*.62);g.addColorStop(0,'rgba(20,16,30,0)');g.addColorStop(1,`rgba(20,16,30,${.45*a})`);ctx.fillStyle=g;ctx.fillRect(0,0,W,H);}
 ctx.globalAlpha=a;ctx.lineCap='square';
 for(const [color,width] of [['rgba(40,30,40,.55)',5],['#fffaf0',3]]){ctx.strokeStyle=color;ctx.lineWidth=width;ctx.beginPath();
  for(const [x,y,dx,dy] of [[inset,inset,1,1],[W-inset,inset,-1,1],[inset,H-inset,1,-1],[W-inset,H-inset,-1,-1]]){ctx.moveTo(x+dx*arm,y);ctx.lineTo(x,y);ctx.lineTo(x,y+dy*arm);}
  ctx.stroke();}
 // Focus box settles on the pet, then blinks green as focus locks.
 const f=ease(span(t,[300,1400])),size=90-34*f,locked=t>1500,blink=locked&&Math.floor(t/120)%2;
 ctx.strokeStyle=locked?(blink?'#9dff9a':'#fffaf0'):'#fffaf0';ctx.lineWidth=2;ctx.beginPath();
 for(const [sx,sy] of [[-1,-1],[1,-1],[-1,1],[1,1]]){const x=cx+sx*size/2,y=cy+sy*size/2;ctx.moveTo(x,y-sy*8);ctx.lineTo(x,y);ctx.lineTo(x-sx*8,y);}
 ctx.stroke();
 if(Math.floor(t/500)%2===0){ctx.fillStyle='#ff5a5a';ctx.beginPath();ctx.arc?.(W-inset-40,inset+14,3.5,0,Math.PI*2);ctx.fill();}
 ctx.fillStyle='#fffaf0';ctx.font=`700 9px ${SANS}`;ctx.textBaseline='middle';ctx.fillText('REC',W-inset-33,inset+14);
 ctx.restore();
}
// --- Photo: the garden background plus the pet's happy pose, as the shutter saw it.
let shot=null,shotKey=null,shotUrl=null;
// Also used to rebuild a photo for older memorial records that never saved one.
function scene(species,stage,background,now=1600){
 const c=canvas();if(!c)return null;const g=c.getContext('2d');
 if(background?.complete&&background.naturalWidth){g.save();if('filter' in g)g.filter='saturate(.48)';g.drawImage(background,0,0,W,H);g.restore();}
 else{g.fillStyle='#a9c98a';g.fillRect(0,0,W,H);}
 pose(g,species,stage,'happy',now,home(species,stage));return c;
}
function capture(key,{species,stage,background,now}){const c=scene(species,stage,background,now);if(c){shot=c;shotKey=key;shotUrl=null;}return c;}
// --- Postcard, drawn in its own 300 x 192 space so the scene and memorial cards share it.
const portraits=new Map();
function portrait(species,stage){
 const key=species+':'+stage;if(portraits.has(key))return portraits.get(key);
 const c=canvas();if(!c)return null;const g=c.getContext('2d');g.imageSmoothingEnabled=false;
 if(!pose(g,species,stage,'idle',0,home(species,stage)))return null;
 let d;try{d=g.getImageData(0,0,W,H)?.data;}catch{return null;}if(!d)return null;
 let x0=W,y0=H,x1=-1,y1=-1;for(let y=0;y<H;y++)for(let x=0;x<W;x++)if(d[(y*W+x)*4+3]>140){if(x<x0)x0=x;if(x>x1)x1=x;if(y<y0)y0=y;if(y>y1)y1=y;}
 if(x1<0)return null;const r={canvas:c,box:[x0,y0,x1-x0+1,y1-y0+1]};portraits.set(key,r);return r;
}
// Character wrap for Chinese; closing punctuation never starts a line.
function wrap(ctx,text,width){const lines=[];let line='';for(const ch of text){if(line&&!'，。！？、」』）…'.includes(ch)&&(ctx.measureText?.(line+ch)?.width||0)>width){lines.push(line);line=ch;}else line+=ch;}if(line)lines.push(line);return lines;}
function stamp(ctx,x,y,species,stage){
 const w=38,h=46,r=2.2;
 ctx.save();ctx.fillStyle='#fffdf6';ctx.fillRect(x,y,w,h);
 // Perforations: paper-coloured bites around the edge.
 ctx.fillStyle='#fbf3e2';for(let i=0;i<=w;i+=5.4){ctx.beginPath();ctx.arc?.(x+i,y,r,0,Math.PI*2);ctx.arc?.(x+i,y+h,r,0,Math.PI*2);ctx.fill();}
 for(let i=0;i<=h;i+=5.4){ctx.beginPath();ctx.arc?.(x,y+i,r,0,Math.PI*2);ctx.arc?.(x+w,y+i,r,0,Math.PI*2);ctx.fill();}
 ctx.fillStyle=species===2?'#e6dcf6':'#d8ecc8';ctx.fillRect(x+5,y+5,w-10,h-10);
 const P=portrait(species,stage);
 if(P){const [bx,by,bw,bh]=P.box,s=Math.min((w-14)/bw,(h-16)/bh);ctx.imageSmoothingEnabled=false;ctx.drawImage(P.canvas,bx,by,bw,bh,x+w/2-bw*s/2,y+h-8-bh*s,bw*s,bh*s);}
 ctx.fillStyle='#7d6557';ctx.font=`700 5px ${SANS}`;ctx.textAlign='center';ctx.fillText('GARDEN POST',x+w/2,y+11);
 ctx.restore();
}
function postmark(ctx,x,y,date){
 ctx.save();ctx.strokeStyle='rgba(120,64,96,.62)';ctx.fillStyle='rgba(120,64,96,.7)';ctx.lineWidth=1.2;
 ctx.beginPath();ctx.arc?.(x,y,19,0,Math.PI*2);ctx.stroke();ctx.beginPath();ctx.arc?.(x,y,14.5,0,Math.PI*2);ctx.stroke();
 ctx.textAlign='center';ctx.textBaseline='middle';ctx.font=`700 6px ${SANS}`;ctx.fillText(date||'',x,y+1);
 ctx.font=`700 4.5px ${SANS}`;ctx.fillText('花園郵局',x,y-7);ctx.fillText('遠行',x,y+8);
 // Cancellation waves running left from the mark.
 for(let k=0;k<3;k++){ctx.beginPath();for(let i=0;i<=34;i+=2){const px=x-22-i,py=y-6+k*6+Math.sin(i/3)*1.6;i?ctx.lineTo(px,py):ctx.moveTo(px,py);}ctx.stroke();}
 ctx.restore();
}
function postcard(ctx,{photo,name='',age='',date='',petId='',species=1,stage='adult'}={}){
 const {w,h}=CARD;ctx.save();
 ctx.fillStyle='#fbf3e2';ctx.fillRect(0,0,w,h);
 // Faint paper fibres; fixed per card so the texture never shimmers between frames.
 for(let i=0;i<70;i++){const s=hashOf('fibre'+i);ctx.fillStyle=`rgba(150,120,80,${.05+(s%5)/100})`;ctx.fillRect(s%w,(s>>>8)%h,1+(s>>>16)%3,1);}
 ctx.strokeStyle='#d6c19f';ctx.lineWidth=1;ctx.strokeRect(.5,.5,w-1,h-1);
 // Photo, slightly tilted, with a white border and a strip of tape.
 ctx.save();ctx.translate(90,74);ctx.rotate(-.04);
 ctx.fillStyle='rgba(60,40,20,.18)';ctx.fillRect(-77,-51,158,108);ctx.fillStyle='#ffffff';ctx.fillRect(-79,-53,158,106);
 if(photo&&(photo.naturalWidth||photo.width)){ctx.imageSmoothingEnabled=false;ctx.drawImage(photo,-74,-48,148,96);}else{ctx.fillStyle='#cfe0b6';ctx.fillRect(-74,-48,148,96);}
 ctx.fillStyle='rgba(236,222,186,.82)';ctx.rotate(.09);ctx.fillRect(-22,-60,44,12);ctx.restore();
 ctx.fillStyle='#5d4a3e';ctx.textAlign='center';ctx.textBaseline='alphabetic';
 ctx.font=`700 13px ${HAND}`;ctx.fillText(name+' · 遠行',90,152);
 ctx.font=`9px ${SANS}`;ctx.fillStyle='#8a7262';ctx.fillText(age?'陪伴 '+age:'',90,168);
 // Divider and the writing side.
 ctx.strokeStyle='#cdb895';ctx.setLineDash?.([3,3]);ctx.beginPath();ctx.moveTo(176,18);ctx.lineTo(176,174);ctx.stroke();ctx.setLineDash?.([]);
 stamp(ctx,250,12,species,stage);postmark(ctx,236,54,date);
 ctx.textAlign='left';ctx.fillStyle='#5d4a3e';ctx.font=`11px ${HAND}`;ctx.fillText('給 照顧我的你：',186,96);
 ctx.font=`12px ${HAND}`;const lines=wrap(ctx,message(petId),100);
 lines.slice(0,3).forEach((line,i)=>{const y=116+i*18;ctx.fillText(line,186,y);ctx.fillStyle='rgba(160,130,100,.35)';ctx.fillRect(186,y+5,104,1);ctx.fillStyle='#5d4a3e';});
 ctx.textAlign='right';ctx.font=`700 12px ${HAND}`;ctx.fillText('— '+name,290,180);
 ctx.restore();
}
function phase(t){return t<T.shot?'finder':t<T.flash[1]?'shoot':t<T.card[0]?'leave':t<T.rest?'card':'rest';}
// Returns {phase, rest}; rest is true once the afterlife menu may show.
function draw(ctx,{species=1,stage='adult',name='',petId='',age='',date='',elapsed=0,now=elapsed,reduced=false,background=null}={}){
 const t=reduced?Math.max(Math.min(elapsed,1e9),T.rest):Math.max(0,Math.min(elapsed,1e9)),key=petId+':'+species+':'+stage;
 if((t>=T.shot||reduced)&&shotKey!==key)capture(key,{species,stage,background,now:1600});
 ctx.save();ctx.imageSmoothingEnabled=false;const h=home(species,stage);
 if(t<T.hop[0]){pose(ctx,species,stage,'happy',now,h);if(t<T.shot)finder(ctx,t,h.x,h.feet-(species===2?(stage==='adult'?62:46):birdHeight(stage)*.5));}
 else if(t<T.leave[0]){const u=span(t,T.hop),dy=-Math.abs(Math.sin(u*Math.PI*2))*9;pose(ctx,species,stage,'happy',now,{...h,feet:h.feet+dy});
}
 else if(t<T.leave[1])pose(ctx,species,stage,'walk',now,leaving(species,stage,t));
 // The same pixel hearts as a head pat, spawned while it hops and left to float up where it stood.
 const hm=t-T.hop[0];if(hm>=0&&hm<HEART_SPAN&&!reduced)window.PetEffects?.hearts(ctx,hm,{x:h.x,y:h.feet-(species===2?(stage==='adult'?110:80):birdHeight(stage)*.85),spread:species===2?40:34,until:T.hop[1]-T.hop[0]});
 const tw=span(t,T.twinkle);if(tw>0&&tw<1){const end=leaving(species,stage,T.leave[1]);sparkle(ctx,end.x,end.feet-14,Math.round(3+6*Math.sin(Math.PI*tw)),1-tw);}
 const [f0,f1]=T.flash;if(t>=f0&&t<f1){ctx.save();ctx.globalAlpha=.95*(1-easeOut(span(t,T.flash)));ctx.fillStyle='#fffdf4';ctx.fillRect(0,0,W,H);ctx.restore();}
 const c=span(t,T.card);
 if(c>0){
  ctx.save();ctx.fillStyle=`rgba(30,24,40,${.32*ease(c)})`;ctx.fillRect(0,0,W,H);
  const drop=easeOutBack(c),y=-CARD.h-20+(H/2-CARD.h/2+CARD.h+20)*drop,rot=(-.14+.11*ease(c));
  ctx.translate(W/2,y+CARD.h/2);ctx.rotate(rot);ctx.shadowColor='rgba(30,20,10,.35)';ctx.shadowBlur=10;ctx.shadowOffsetY=4;
  ctx.fillStyle='#fbf3e2';ctx.fillRect(-CARD.w/2,-CARD.h/2,CARD.w,CARD.h);ctx.shadowColor='transparent';
  ctx.translate(-CARD.w/2,-CARD.h/2);postcard(ctx,{photo:shot,name,age,date,petId,species,stage});ctx.restore();
 }
 ctx.restore();return {phase:phase(t),rest:t>=T.rest};
}
// PNG of the last shot for the memorial record, or null before the shutter fires.
function photo(){if(!shot)return null;try{shotUrl=shotUrl||shot.toDataURL('image/png');}catch{return null;}return shotUrl;}
window.PetFarewellView={T,CARD,MESSAGES,draw,phase,photo,postcard,message,scene,wrap};
})();
