const test=require('node:test'),assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs');
// Fake 2D context: records text and pose painting; getImageData reports a solid 40x60 body.
function context(log){
 const data=new Uint8ClampedArray(384*256*4);for(let y=120;y<180;y++)for(let x=170;x<210;x++)data[(y*384+x)*4+3]=255;
 return new Proxy({},{get:(o,k)=>k in o?o[k]:k==='fillText'?t=>log.text.push(t):k==='getImageData'?()=>({data}):k==='measureText'?()=>({width:40}):k.startsWith('create')?()=>({addColorStop(){}}):()=>{},set:(o,k,v)=>(o[k]=v,true)});
}
function boot(){
 const log={poses:[],text:[]},c={window:{},document:{createElement:()=>({width:0,height:0,getContext:()=>context(log)})}};
 c.window.WyvernAdultView={paint(ctx,state){log.poses.push(state);return true;}};
 vm.createContext(c);vm.runInContext(fs.readFileSync(__dirname+'/death-view.js','utf8'),c);return {view:c.window.PetDeathView,win:c.window,log,ctx:()=>context(log)};
}
test('death timeline runs fall, soul, grave, then rest',()=>{const {view}=boot();
 for(const [t,p] of [[0,'fall'],[1199,'fall'],[1200,'soul'],[4799,'soul'],[4800,'grave'],[6199,'grave'],[6200,'rest'],[20000,'rest']])assert.equal(view.phase(t),p,'at '+t);});
test('afterlife menu waits for the grave to settle',()=>{const {view,ctx}=boot();
 assert.equal(view.draw(ctx(),{species:2,elapsed:6199}).rest,false);assert.equal(view.draw(ctx(),{species:2,elapsed:6200}).rest,true);});
test('soul is the pet idle pose, only while it is in the air',()=>{
 for(const [t,soul] of [[1100,false],[1300,true],[3500,true],[3700,false]]){const {view,log,ctx}=boot();view.draw(ctx(),{species:2,stage:'adult',elapsed:t});assert.equal(log.poses.includes('idle'),soul,'at '+t);}});
test('body pose is painted once and reused from the cache',()=>{const {view,log,ctx}=boot();
 view.draw(ctx(),{species:2,elapsed:0});view.draw(ctx(),{species:2,elapsed:500});assert.deepEqual(log.poses,['dead']);});
test('name is engraved only after the stone has risen',()=>{
 for(const [t,named] of [[5000,false],[6200,true]]){const {view,log,ctx}=boot();view.draw(ctx(),{species:1,name:'Tamama',elapsed:t});assert.equal(log.text.includes('Tamama'),named,'at '+t);}});
test('a stand-in drawn while the art loads is measured again once the art arrives',()=>{const {view,win,log,ctx}=boot();
 let loaded=false;win.WyvernAdultView.paint=(c,state)=>{log.poses.push(state);return loaded;};
 view.draw(ctx(),{species:2,elapsed:1500});view.draw(ctx(),{species:2,elapsed:1600});
 loaded=true;view.draw(ctx(),{species:2,elapsed:1700});const before=log.poses.length;view.draw(ctx(),{species:2,elapsed:1800});
 assert.equal(log.poses.length,before,'finished art is cached');assert.ok(before>=6,'stand-ins were not cached');});
test('reduced motion goes straight to the resting grave',()=>{const {view,log,ctx}=boot();
 const r=view.draw(ctx(),{species:2,name:'Tamama',elapsed:0,reduced:true});assert.equal(r.rest,true);assert.ok(log.text.includes('Tamama'));assert.ok(!log.poses.includes('idle'),'no soul');});
