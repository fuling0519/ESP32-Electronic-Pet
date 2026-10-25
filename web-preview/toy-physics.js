/* Toy play geometry in garden canvas units (384x256): ground positions (x,y) plus a height z above the ground.
 * Pure functions only, so the throw, bounce and chase can be tested without a page. */
(()=>{'use strict';
// Open grass the pet can reach: left of the barn, above the bottom fences, and off the pond in the lower left.
const GROUND={left:100,right:335,top:140,bottom:214,pondRight:122,pondTop:166};
const HAND={x:205,y:234};
function clampGround(p){
 const x=Math.max(GROUND.left,Math.min(GROUND.right,p.x)),y=Math.max(GROUND.top,Math.min(GROUND.bottom,p.y));
 return {x:y>GROUND.pondTop?Math.max(GROUND.pondRight,x):x,y};
}
// A fling shorter than MIN_SPEED (canvas units per second) is a drop, not a throw. Faster flings go further.
const MIN_SPEED=150;
function landing(release,velocity){
 const speed=Math.hypot(velocity.x,velocity.y);
 if(speed<MIN_SPEED)return null;
 const dist=Math.max(50,Math.min(230,speed*.28));
 return clampGround({x:release.x+velocity.x/speed*dist,y:release.y+velocity.y/speed*dist});
}
const lerp=(a,b,u)=>({x:a.x+(b.x-a.x)*u,y:a.y+(b.y-a.y)*u});
// One arc to the landing point, a small bounce on along the same line, then an easing roll to rest.
function flight(from,to){
 const dist=Math.hypot(to.x-from.x,to.y-from.y),ux=dist?(to.x-from.x)/dist:0,uy=dist?(to.y-from.y)/dist:-1;
 const air=.45+dist/420,height=16+dist*.2,bounce=.38,roll=.45;
 const hop=clampGround({x:to.x+ux*dist*.16,y:to.y+uy*dist*.16}),rest=clampGround({x:hop.x+ux*dist*.07,y:hop.y+uy*dist*.07});
 function at(t){
  if(t<air){const u=Math.max(0,t)/air;return {...lerp(from,to,u),z:4*height*u*(1-u),phase:'air'};}
  t-=air;
  if(t<bounce){const u=t/bounce;return {...lerp(to,hop,u),z:4*height*.28*u*(1-u),phase:'bounce'};}
  t-=bounce;
  if(t<roll){const u=t/roll;return {...lerp(hop,rest,1-(1-u)**2),z:0,phase:'roll'};}
  return {...rest,z:0,phase:'rest'};
 }
 return {from,to,rest,air,duration:air+bounce+roll,at};
}
// Move a ground position toward a target at a fixed speed; returns the new position and whether it arrived.
function step(pos,target,speed,dt){
 const dx=target.x-pos.x,dy=target.y-pos.y,dist=Math.hypot(dx,dy),move=speed*dt;
 if(dist<=move||dist<.01)return {x:target.x,y:target.y,arrived:true};
 return {x:pos.x+dx/dist*move,y:pos.y+dy/dist*move,arrived:false};
}
// How each pet chases: walking speed, and how high a ball can still be caught (an adult wyvern flies up to meet it).
const MOVES={'1:adult':{speed:110,reach:22,flies:false},'1:baby':{speed:95,reach:18,flies:false},
 '2:adult':{speed:170,reach:50,flies:true},'2:baby':{speed:95,reach:20,flies:false}};
function moves(species,stage){return MOVES[species+':'+stage]||MOVES['1:adult'];}
// The ball is within the pet's mouth: close on the ground (an ellipse, as depth is squashed) and low enough.
function reaches(pet,ball,reach){return ((ball.x-pet.x)/16)**2+((ball.y-pet.y)/10)**2<=1&&ball.z<=reach;}
// Pets further back look slightly smaller.
function depthScale(y,homeY){return Math.max(.82,Math.min(1.08,1+(y-homeY)*.0025));}
window.ToyPhysics={GROUND,HAND,MIN_SPEED,clampGround,landing,flight,step,moves,reaches,depthScale};
})();
