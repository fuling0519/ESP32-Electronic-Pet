/* Garden level-up celebration: happy hops, a gold halo, twinkles and a level badge. Visual only; never changes EXP or saves. */
(()=>{'use strict';
const LEVEL_UP_MS=2800,MAX_WAIT_MS=15000;
// Timeline (ms): a small dip 0-300, two hops 300-750 and 800-1200, halo 150-2500, badge pops at 700 and its
// number turns over at 1150, banner from 900, everything fades out over the last 300 ms.
const T={dip:300,hops:[[300,750,22],[800,1200,12]],halo:[150,2500],badge:700,flip:[1150,1350],banner:900,fade:LEVEL_UP_MS-300};
// Feet / head-top anchors per species and stage, matching where the garden normally stands each pet.
const POSE={
 1:{adult:{cx:170,cy:137,top:72},baby:{cx:175,cy:145,top:96}},
 2:{adult:{cx:190,cy:177,top:54},baby:{cx:190,cy:177,top:88}}};
const TWINKLES=[[400,-84,-46,26],[520,78,-58,30],[640,-96,10,24],[760,92,4,28],[900,-58,-86,22],[1040,52,-92,26],[1200,-72,40,22],[1360,84,44,24],[1520,-20,-104,22],[1700,22,-100,24]];
let pending=null,playing=null,shown=new Map();
const clamp=t=>Math.max(0,Math.min(1,t));
// request(): one pending celebration per pet; repeat requests (care result + BLE event, several levels) merge into it.
// `level` is the new level when known; `gained` alone resolves against the pet's level when the animation starts.
function request({petId,level=null,gained=null,from=null}={},now=0){
 if(!petId)return false;
 if(level!==null&&level<=(shown.get(petId)??0))return false;
 if(playing&&playing.petId===petId){if(level!==null&&level>playing.to)playing.to=level;return false;}
 if(pending&&pending.petId===petId){
  if(level!==null)pending.to=Math.max(pending.to??0,level);
  if(from!==null)pending.from=pending.from===null?from:Math.min(pending.from,from);
  if(gained!==null&&level===null)pending.gained=Math.max(pending.gained??0,gained);
  return true;}
 pending={petId,to:level,from,gained,at:now};return true;
}
// Pet gone, dead, departed or replaced: drop everything for it.
function observe(p){
 const live=p&&['baby','adult'].includes(p.stage)&&!p.isDeparted&&p.healthState!=='dead'&&p.health!=='dead';
 if(!live||pending&&pending.petId!==p.petId){pending=null;}
 if(!live||playing&&playing.petId!==p.petId){playing=null;}
}
// Called every frame. `blocked` is true while care, its thank-you bubble, growth, a game or a touch reaction plays.
// Returns 'start' on the frame it begins, true while playing, false otherwise.
function step(now,p,blocked){
 if(playing){if(now-playing.start<LEVEL_UP_MS)return true;shown.set(playing.petId,playing.to);playing=null;}
 if(!pending)return false;
 if(now-pending.at>MAX_WAIT_MS||!p||p.petId!==pending.petId||p.sleepMode==='normal'){pending=null;return false;}
 if(blocked)return false;
 const to=pending.to??(Number.isInteger(p.level)?p.level:null),gained=pending.gained??1;
 if(to!==null&&to<=(shown.get(p.petId)??0)){pending=null;return false;}
 let from=pending.from??(to!==null?Math.max(1,to-gained):null);if(from!==null&&to!==null&&from>=to)from=to-1;
 playing={petId:p.petId,to,from,start:now};pending=null;return 'start';
}
function draw(ctx,now,{species,stage,reduced=false}={}){
 if(!playing)return false;const t=now-playing.start;if(t<0||t>=LEVEL_UP_MS)return false;
 const pose=POSE[species]?.[stage]||POSE[1].adult,fade=t<T.fade?1:clamp((LEVEL_UP_MS-t)/300);
 ctx.save();ctx.imageSmoothingEnabled=false;
 if(!reduced)halo(ctx,t,pose,fade);
 pet(ctx,species,stage,pose,reduced?0:lift(t),reduced?0:t);
 if(!reduced)twinkles(ctx,t,pose,fade);
 badge(ctx,reduced?LEVEL_UP_MS/2:t,pose,playing,reduced?1:fade,reduced);
 if(reduced||t>=T.banner)banner(ctx,'升級了！',(reduced?1:clamp((t-T.banner)/300))*fade);
 ctx.restore();return true;
}
// Vertical offset (negative = up): a 3 px dip before take-off and after each landing, then two arcs.
function lift(t){
 if(t<T.dip)return 3*Math.sin(Math.PI*t/T.dip/2);
 for(const [a,b,h] of T.hops)if(t>=a&&t<b)return -h*Math.sin(Math.PI*(t-a)/(b-a));
 const land=T.hops.find(([,b])=>t>=b&&t<b+120);return land?2*Math.sin(Math.PI*(t-land[1])/120):0;
}
function pet(ctx,species,stage,pose,dy,t){
 ctx.save();ctx.translate(0,Math.round(dy));
 if(species===2){if(!window.WyvernAdultView?.paint(ctx,'happy',t,false,{stage})&&window.WyvernView&&window.WyvernFrames)window.WyvernView.paint(ctx,window.WyvernFrames[stage==='baby'?'kBabyWyvernIdleFrames':'kWyvernIdleFrames'][0]);}
 else if(window.BirdDemo){const p={petId:'level-up',stage,healthState:'healthy',sleepMode:'awake',mood:100,satiety:100,cleanliness:100};window.BirdDemo.paint(ctx,p,'happy',0,pose.cx,pose.cy,3,true,false);}
 else window.BirdView?.still(ctx,stage,false,pose.cx,pose.cy,3);
 ctx.restore();
}
// Warm gold ring behind the pet: grows out from the body, pulses once, then thins away.
function halo(ctx,t,pose,fade){
 const [a,b]=T.halo;if(t<a||t>=b||!ctx.createRadialGradient)return;
 const u=(t-a)/(b-a),cy=(pose.top+pose.cy+(pose.cy-pose.top)*.2)/2,r=(stageR(pose))*(.55+.45*Math.min(1,u*2.4))+Math.sin(t/120)*2,alpha=Math.min(1,u*5)*(u<.75?1:(1-u)/.25)*fade;
 ctx.save();
 const g=ctx.createRadialGradient(pose.cx,cy,r*.35,pose.cx,cy,r);
 g.addColorStop(0,'rgba(255,248,214,0)');g.addColorStop(.5,`rgba(255,236,150,${.7*alpha})`);g.addColorStop(.8,`rgba(255,200,70,${.9*alpha})`);g.addColorStop(1,'rgba(255,190,60,0)');
 ctx.fillStyle=g;ctx.beginPath();ctx.arc(pose.cx,cy,r,0,Math.PI*2);ctx.fill();ctx.restore();
 // Rising motes in front of the halo.
 ctx.save();for(let i=0;i<8;i++){const born=a+i*170,q=(t-born)/1100;if(q<0||q>=1)continue;
  const x=Math.round(pose.cx+((i*53)%120-60)),y=Math.round(pose.cy-6-q*90);ctx.globalAlpha=(q<.7?1:(1-q)/.3)*fade;
  ctx.fillStyle='#8a5a12';ctx.fillRect(x-1,y-1,4,4);ctx.fillStyle=i%2?'#fff1a6':'#ffd45c';ctx.fillRect(x,y,2,2);}
 ctx.restore();
}
const stageR=pose=>Math.max(60,(pose.cy-pose.top)*.95);
// Cleaning sparkle sprites when loaded; otherwise small pixel crosses.
function twinkles(ctx,t,pose,fade){
 const sheet=window.CleaningAnimationView?.ready?.(),cy=(pose.top+pose.cy)/2;
 ctx.save();ctx.globalAlpha=fade;
 for(const [delay,dx,dy,w] of TWINKLES){const q=(t-delay)/400;if(q<0||q>=1)continue;const x=pose.cx+dx,y=cy+dy;
  if(sheet)window.CleaningAnimationView.sprite(ctx,'sparkle',Math.min(4,Math.floor(q*5)),x,y,w);
  else{const s=Math.round(4+4*Math.sin(Math.PI*q));ctx.fillStyle='#fff1a6';ctx.fillRect(Math.round(x-s),Math.round(y),s*2+2,2);ctx.fillRect(Math.round(x),Math.round(y-s),2,s*2+2);}}
 ctx.restore();
}
// Pixel badge above the head: a star and "Lv 3", whose number turns over into "Lv 4" at T.flip.
const STAR=['....#....','...#y#...','###yyy###','#yyywyyy#','.#yyyyy#.','..#yyy#..','.#yy#yy#.','#yy#.#yy#','###...###'];
function badge(ctx,t,pose,{from,to},alpha,reduced){
 if(t<T.badge||to===null)return;
 const u=t-T.badge,scale=reduced?1:u<160?Math.max(.2,1.25*u/160):u<260?1.25-.25*(u-160)/100:1;
 const cx=pose.cx,cy=Math.max(26,pose.top-26),w=84,h=30,L=-w/2,top=-h/2;
 ctx.save();ctx.globalAlpha*=alpha;ctx.translate(Math.round(cx),Math.round(cy));ctx.scale(scale,scale);
 // Rounded pixel plate: dark outline, gold face, light top band, darker bottom lip.
 ctx.fillStyle='#4a3020';ctx.fillRect(L+2,top-2,w-4,h+4);ctx.fillRect(L,top,w,h);
 ctx.fillStyle='#ffc93c';ctx.fillRect(L+2,top+2,w-4,h-4);
 ctx.fillStyle='#ffe58a';ctx.fillRect(L+4,top+2,w-8,6);ctx.fillStyle='#d9921e';ctx.fillRect(L+4,h/2-6,w-8,4);
 ctx.fillStyle='#fffbe0';ctx.fillRect(L+5,top+3,5,2);
 // Star on the left, 2 px per cell.
 STAR.forEach((row,y)=>[...row].forEach((c,x)=>{if(c==='.')return;ctx.fillStyle=c==='#'?'#4a3020':c==='w'?'#fffbe0':'#fff1a6';ctx.fillRect(L+6+x*2,top+6+y*2,2,2);}));
 ctx.font='900 17px "Microsoft JhengHei","PingFang TC","Noto Sans TC",sans-serif';ctx.textBaseline='middle';ctx.fillStyle='#4a3020';
 ctx.textAlign='left';ctx.fillText('Lv',L+28,1);
 const [f0,f1]=T.flip,roll=reduced||from===null?1:clamp((t-f0)/(f1-f0));
 number(ctx,String(from??to),String(to),L+60,roll,h);
 ctx.restore();
}
// Old value slides up and out while the new one slides up from below, clipped to the plate.
function number(ctx,oldText,newText,x,roll,h){
 ctx.save();ctx.beginPath();ctx.rect(x-12,-h/2+3,24,h-6);ctx.clip();ctx.textAlign='center';
 const shift=Math.round(roll*h);
 if(roll<1)ctx.fillText(oldText,x,1-shift);
 if(roll>0){ctx.fillStyle=roll<1?'#4a3020':'#8a2a0c';ctx.fillText(newText,x,1+h-shift);}
 ctx.restore();
}
// Same cream-outlined banner as hatching and growing up.
function banner(ctx,text,alpha){
 if(alpha<=0)return;
 ctx.save();ctx.globalAlpha=alpha;ctx.font='900 22px "Microsoft JhengHei","PingFang TC","Noto Sans TC",sans-serif';ctx.textAlign='center';ctx.textBaseline='middle';ctx.lineJoin='round';
 ctx.lineWidth=6;ctx.strokeStyle='#fff8e6';ctx.strokeText(text,190,224);ctx.fillStyle='#4a3020';ctx.fillText(text,190,224);ctx.restore();
}
function reset(){pending=null;playing=null;shown=new Map();}
window.PetLevelUp={request,observe,step,draw,reset,LEVEL_UP_MS,MAX_WAIT_MS,get active(){return !!playing;},get pending(){return pending&&{...pending};},get playing(){return playing&&{...playing};}};
})();
