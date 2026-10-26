const test=require('node:test'),assert=require('node:assert/strict'),fs=require('fs'),vm=require('vm');
function boot(extra={}){const c={window:{},...extra};vm.createContext(c);vm.runInContext(fs.readFileSync(__dirname+'/level-up-view.js','utf8'),c);return c.window.PetLevelUp;}
const pet=(o={})=>({petId:'a',stage:'adult',healthState:'healthy',sleepMode:'awake',isDeparted:false,level:4,...o});
function ctx(log=[]){return new Proxy({log},{get(t,k){if(k==='log')return t.log;if(k==='createRadialGradient')return()=>({addColorStop(){}});return(...a)=>{if(k==='fillText')t.log.push(a[0]);};},set(){return true;}});}

test('celebration waits while blocked, then lasts 2800 ms and shows the badge from old to new level',()=>{
 const L=boot();L.request({petId:'a',level:4,from:3},0);
 assert.equal(L.step(100,pet(),true),false,'care, thank-you bubble, growth or a game blocks the start');
 assert.equal(L.active,false);assert.equal(L.step(200,pet(),false),'start');
 assert.deepEqual({...L.playing,start:undefined},{petId:'a',to:4,from:3,start:undefined});
 const c=ctx();assert.equal(L.draw(c,200+1500,{species:2,stage:'adult'}),true);
 assert.ok(c.log.includes('4')&&c.log.includes('升級了！'),'badge number and banner are drawn');
 assert.equal(L.step(200+2799,pet(),false),true);assert.equal(L.step(200+2800,pet(),false),false);assert.equal(L.active,false);
});
test('several sources for the same level (care result + BLE event) play once; a later level plays again',()=>{
 const L=boot();L.request({petId:'a',gained:1},0);L.request({petId:'a',level:5},10);
 assert.equal(L.step(20,pet({level:4}),false),'start');assert.equal(L.playing.to,5);assert.equal(L.playing.from,4);
 assert.equal(L.request({petId:'a',level:5},30),false,'a duplicate arriving mid-play is absorbed');
 L.step(3000,pet(),false);
 assert.equal(L.request({petId:'a',level:5},3100),false,'already celebrated level is ignored');
 assert.equal(L.request({petId:'a',level:6},3200),true);assert.equal(L.step(3300,pet({level:6}),false),'start');
});
test('levels gained without a level resolve against the pet when it starts; multiple levels collapse into one',()=>{
 const L=boot();L.request({petId:'a',gained:2},0);assert.equal(L.step(10,pet({level:7}),false),'start');
 assert.equal(L.playing.from,5);assert.equal(L.playing.to,7);
 const M=boot();M.request({petId:'a',level:3,from:2},0);M.request({petId:'a',level:5,from:4},5);M.step(10,pet(),false);
 assert.equal(M.playing.from,2);assert.equal(M.playing.to,5);
});
test('death, farewell, a new pet, sleep or waiting too long drop the celebration',()=>{
 for(const p of [pet({healthState:'dead'}),pet({health:'dead'}),pet({isDeparted:true}),pet({petId:'b'}),pet({stage:'egg'}),null]){
  const L=boot();L.request({petId:'a',level:5},0);L.observe(p);assert.equal(L.pending,null);}
 const S=boot();S.request({petId:'a',level:5},0);assert.equal(S.step(10,pet({sleepMode:'normal'}),false),false);assert.equal(S.pending,null);
 const W=boot();W.request({petId:'a',level:5},0);assert.equal(W.step(W.MAX_WAIT_MS-1,pet(),true),false);assert.ok(W.pending);
 assert.equal(W.step(W.MAX_WAIT_MS+1,pet(),false),false);assert.equal(W.pending,null);
 const D=boot();D.request({petId:'a',level:5},0);D.step(0,pet(),false);D.observe(pet({healthState:'dead'}));assert.equal(D.active,false);
});
test('both species and stages draw their happy pose; reduced motion keeps the badge and banner',()=>{
 const calls=[],c=ctx();
 const win={WyvernAdultView:{paint:(g,state,t,r,o)=>{calls.push(['wyvern',state,o.stage]);return true;}},BirdDemo:{paint:(g,p,state,now,cx,cy)=>{calls.push(['bird',state,p.stage,cx,cy]);return true;}}};
 const B=boot({window:win});B.request({petId:'a',level:2,from:1},0);B.step(0,pet(),false);
 for(const [species,stage] of [[1,'baby'],[1,'adult'],[2,'baby'],[2,'adult']])assert.equal(B.draw(c,1000,{species,stage}),true);
 assert.deepEqual(calls.map(x=>x.slice(0,3)),[['bird','happy','baby'],['bird','happy','adult'],['wyvern','happy','baby'],['wyvern','happy','adult']]);
 const r=ctx();assert.equal(B.draw(r,10,{species:1,stage:'adult',reduced:true}),true);assert.ok(r.log.includes('2')&&r.log.includes('升級了！'));
});
test('app wires all three sources into the queue and drops the old LEVEL UP text',()=>{
 const app=fs.readFileSync(__dirname+'/app.js','utf8'),html=fs.readFileSync(__dirname+'/index.html','utf8');
 assert.ok(!app.includes('LEVEL UP!')&&!app.includes('celebrationUntil'));
 assert.match(app,/levelsGained\)celebrateMock\(result\.levelsGained\)/,'showcase treat');
 assert.match(app,/if\(levelsGained\)celebrateMock\(levelsGained\)/,'showcase feed, clean and play');
 assert.match(app,/pet-game-complete',e=>\{const gained=e\.detail\?\.levelsGained/,'web games, showcase and device');
 assert.match(app,/e\.event==='level_up'.*celebrate\(Number\.isInteger\(e\.level\)\?\{level:e\.level\}/,'BLE pet_event');
 assert.match(app,/now<thanksUntil\|\|!!window\.PetGrowthView\?\.active\|\|!!window\.PetToyPlay\?\.active\(\)\|\|!!window\.PetChorus\?\.active\(\)/,'queues behind care, thanks, growth and games');
 for(const f of ['toy-play.js','chorus-play.js'])assert.match(fs.readFileSync(__dirname+'/'+f,'utf8'),/CustomEvent\('pet-game-complete',\{detail:\{levelsGained:r\.levels_gained/,f+' passes device levels_gained');
 assert.ok(html.indexOf('level-up-view.js')>0&&html.indexOf('level-up-view.js')<html.indexOf('app.js'));
});
