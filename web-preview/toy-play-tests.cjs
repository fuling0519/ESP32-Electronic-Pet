const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
function load(...files){const c={};c.window=c;vm.createContext(c);for(const f of files)vm.runInContext(fs.readFileSync(__dirname+'/'+f,'utf8'),c);return c;}
const {ToyPhysics:P}=load('toy-physics.js');
test('a fling lands in the garden in its direction; a slow drag is not a throw',()=>{
 assert.equal(P.landing({x:205,y:234},{x:0,y:-100}),null);
 const up=P.landing({x:205,y:234},{x:0,y:-600});assert.equal(up.x,205);assert.ok(up.y<234&&up.y>=P.GROUND.top);
 const right=P.landing({x:205,y:200},{x:900,y:-10});assert.equal(right.x,P.GROUND.right,'clamped at the right edge');
 const pond=P.clampGround({x:20,y:200});assert.ok(pond.x>=P.GROUND.pondRight,'never lands in the pond');
 const far=P.landing({x:205,y:234},{x:0,y:-5000}),near=P.landing({x:205,y:234},{x:0,y:-200});
 assert.ok(234-far.y>234-near.y,'faster flings go further');
});
test('the ball arcs to the landing point, bounces on, rolls and rests inside the garden',()=>{
 const f=P.flight({x:205,y:234},{x:205,y:150});
 assert.equal(f.at(0).z,0);assert.equal(f.at(0).phase,'air');
 const mid=f.at(f.air/2);assert.ok(mid.z>30);assert.ok(mid.y<234&&mid.y>150);
 assert.equal(f.at(f.air).phase,'bounce');assert.equal(Math.round(f.at(f.air).y),150);
 // Just past the end: the phase sums are not exact in floating point.
 assert.equal(f.at(f.duration+1e-6).phase,'rest');assert.equal(f.at(f.duration+1e-6).z,0);
 const rest=f.at(f.duration+5);assert.deepEqual({x:rest.x,y:rest.y},{...f.rest});
 assert.ok(rest.y>=P.GROUND.top&&rest.y<=P.GROUND.bottom);
});
test('chasing moves at a fixed speed and stops exactly at the target',()=>{
 // Spread: objects made inside the vm context have that context's prototype.
 assert.deepEqual({...P.step({x:0,y:0},{x:100,y:0},50,1)},{x:50,y:0,arrived:false});
 assert.deepEqual({...P.step({x:90,y:0},{x:100,y:0},50,1)},{x:100,y:0,arrived:true});
});
test('catching needs the ball near the mouth and low enough; the adult wyvern reaches higher',()=>{
 const pet={x:200,y:180};
 assert.equal(P.reaches(pet,{x:210,y:184,z:10},P.moves(1,'adult').reach),true);
 assert.equal(P.reaches(pet,{x:210,y:184,z:30},P.moves(1,'adult').reach),false);
 assert.equal(P.reaches(pet,{x:210,y:184,z:30},P.moves(2,'adult').reach),true);
 assert.equal(P.reaches(pet,{x:230,y:180,z:0},Infinity),false,'too far to the side');
 assert.ok(P.moves(2,'adult').flies&&!P.moves(1,'baby').flies);
 assert.ok(P.depthScale(150,190)<1&&P.depthScale(210,190)>1);
});
test('a toy session counts five throws, completes once and gives a fixed reward',()=>{
 const {PetToyGame}=load('pet-games.js'),g=new PetToyGame({petId:'p',stage:'adult'},0);
 assert.equal(g.kind,'toy');assert.equal(g.phase,'play');
 g.answer(2,0);assert.equal(g.throws,0,'only caught 1 or fetched 0 count');
 for(const caught of [1,0,1,1])g.answer(caught,0);assert.equal(g.phase,'play');assert.equal(g.round,5);
 g.answer(0,0);assert.equal(g.phase,'complete');assert.equal(g.catches,3);
 g.answer(1,0);assert.equal(g.throws,5,'no throws after completion');
 assert.deepEqual({...g.rewards()},{mood:6,exp:10});
});
test('a chorus session counts three rounds, completes once and pays by rounds sung right',()=>{
 const {PetChorusGame}=load('pet-games.js'),make=()=>new PetChorusGame({petId:'p',stage:'baby'},0);
 const g=make();assert.equal(g.kind,'chorus');g.answer(5,0);assert.equal(g.played,0,'only 1 or 0 counts');
 g.answer(1,0);g.answer(0,0);assert.equal(g.phase,'play');assert.equal(g.round,3);g.answer(1,0);assert.equal(g.phase,'complete');
 g.answer(1,0);assert.equal(g.played,3,'nothing after completion');assert.deepEqual({...g.rewards()},{mood:6,exp:10});
 const all=make();for(let i=0;i<3;i++)all.answer(1,0);assert.deepEqual({...all.rewards()},{mood:10,exp:15});
 const one=make();one.answer(1,0);one.answer(0,0);one.answer(0,0);assert.deepEqual({...one.rewards()},{mood:3,exp:5});
});
