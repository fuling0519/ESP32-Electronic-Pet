const test=require('node:test'),assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs');
function boot(){
 const c={window:{},Image:class{constructor(){this.complete=true;this.naturalWidth=1;}},
  EggHatchData:{cell:[443,582],anchor:[221,556],rows:['spotted','dots','zigzag'],crackLine:{spotted:[[20,296],[420,341]],dots:[[20,296],[420,341]],zigzag:[[20,298],[420,342]]}}};
 c.window.EggHatchData=c.EggHatchData;vm.createContext(c);vm.runInContext(fs.readFileSync(__dirname+'/egg-view.js','utf8'),c);return c.window.PetEggView;}
function recorder(){const calls=[];return {calls,ctx:new Proxy({},{get:(t,k)=>k in t?t[k]:(k==='drawImage'?(img,sx,sy)=>calls.push([img.src,sx,sy]):()=>{}),set:(t,k,v)=>(t[k]=v,true)})};}
test('egg cracks at 240 and 270 seconds, hatches at 300',()=>{const f=boot().phase;assert.equal(f(239),'incubating');assert.equal(f(240),'small');assert.equal(f(269),'small');assert.equal(f(270),'large');assert.equal(f(299),'large');assert.equal(f(300),'hatched');});
test('each pet keeps one of the three designs',()=>{const e=boot(),ids=Array.from({length:30},(_,i)=>'pet-'+i);
 for(const id of ids)assert.equal(e.design(id),e.design(id));assert.deepEqual([...new Set(ids.map(e.design))].sort(),[0,1,2]);});
test('draw picks the design row and the crack column by age',()=>{const e=boot(),row=e.design('abc');
 for(const [age,src,col] of [[0,'egg-designs-v1.png',null],[240,'egg-hatch-v1.png',0],[270,'egg-hatch-v1.png',1]]){
  const r=recorder();assert.equal(e.draw(r.ctx,age,0,true,false,'abc'),true);const [s,sx,sy]=r.calls[0];assert.ok(s.endsWith(src));
  if(col===null){assert.equal(sx,row*443);assert.equal(sy,0);}else{assert.equal(sx,col*443);assert.equal(sy,row*582);}}
 assert.equal(e.draw(recorder().ctx,300,0,true,false,'abc'),false);});
test('hatch break shows the large crack, then the lifted cap over the bottom shell',()=>{const e=boot(),row=e.design('abc');
 let r=recorder();e.drawHatch(r.ctx,'abc',500);assert.deepEqual(r.calls.map(c=>c[1]),[443]);
 r=recorder();e.drawHatch(r.ctx,'abc',1400);assert.deepEqual(r.calls.map(c=>[c[1],c[2]]),[[3*443,row*582],[2*443,row*582]]);});
