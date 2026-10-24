const test=require('node:test'),assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs');
// Fake 2D context: records text and pose painting; measureText counts 12 px per character.
function context(log){
 return new Proxy({},{get:(o,k)=>k in o?o[k]:k==='fillText'?t=>log.text.push(t):k==='measureText'?t=>({width:[...t].length*12}):k.startsWith('create')?()=>({addColorStop(){}}):k==='getImageData'?()=>({data:new Uint8ClampedArray(384*256*4)}):()=>{},set:(o,k,v)=>(o[k]=v,true)});
}
function boot(){
 const log={poses:[],text:[],urls:0},c={window:{},document:{createElement:()=>({width:0,height:0,getContext:()=>context(log),toDataURL:()=>(log.urls++,'data:image/png;base64,QQ==')})}};
 c.window.WyvernAdultView={paint(ctx,state){log.poses.push(state);return true;}};
 c.window.PetEffects={hearts(ctx,ms,o){log.hearts=(log.hearts||[]).concat([[ms,o.until]]);}};
 vm.createContext(c);vm.runInContext(fs.readFileSync(__dirname+'/farewell-view.js','utf8'),c);return {view:c.window.PetFarewellView,log,ctx:()=>context(log)};
}
const pet={species:2,stage:'adult',name:'Tamama',petId:'p-1',age:'1 小時',date:'2026.10.24'};
test('farewell runs viewfinder, shutter, departure, postcard, then rest',()=>{const {view}=boot();
 for(const [t,p] of [[0,'finder'],[1999,'finder'],[2000,'shoot'],[2399,'shoot'],[2400,'leave'],[5799,'leave'],[5800,'card'],[6999,'card'],[7000,'rest']])assert.equal(view.phase(t),p,'at '+t);});
test('menu waits for the postcard to land',()=>{const {view,ctx}=boot();
 assert.equal(view.draw(ctx(),{...pet,elapsed:6999}).rest,false);assert.equal(view.draw(ctx(),{...pet,elapsed:7000}).rest,true);});
test('no photo before the shutter; one shot per pet after it',()=>{const {view,log,ctx}=boot();
 view.draw(ctx(),{...pet,elapsed:1500});assert.equal(view.photo(),null);
 view.draw(ctx(),{...pet,elapsed:2100});assert.equal(view.photo(),'data:image/png;base64,QQ==');view.photo();assert.equal(log.urls,1,'encoded once');});
test('pet poses: happy for the photo and hops, flight for an adult wyvern leaving, walk for a baby',()=>{
 for(const [stage,t,state] of [['adult',1000,'happy'],['adult',2700,'happy'],['adult',4000,'flight'],['baby',4000,'walk']]){const {view,log,ctx}=boot();view.draw(ctx(),{...pet,stage,elapsed:t});assert.ok(log.poses.includes(state),stage+' at '+t);}});
test('postcard carries name, date and one fixed message per pet',()=>{const {view,log,ctx}=boot();
 view.draw(ctx(),{...pet,elapsed:8000});const text=log.text.join('|');
 assert.ok(text.includes('Tamama · 遠行'));assert.ok(text.includes('2026.10.24'));assert.ok(text.includes('— Tamama'));
 assert.equal(view.message('p-1'),view.message('p-1'));assert.ok(view.MESSAGES.includes(view.message('p-1')));
 assert.ok(new Set(Array.from({length:40},(_,i)=>view.message('pet-'+i))).size>=4,'messages vary between pets');});
test('closing punctuation never starts a wrapped line',()=>{const {view,ctx}=boot();
 const lines=view.wrap(ctx(),'別擔心，我會好好吃飯、好好睡覺的。',96);assert.ok(lines.every(l=>!'，。、'.includes(l[0])),lines.join('/'));});
test('reduced motion goes straight to the postcard, with a photo',()=>{const {view,log,ctx}=boot();
 const r=view.draw(ctx(),{...pet,elapsed:0,reduced:true});assert.equal(r.rest,true);assert.ok(view.photo());assert.ok(log.text.includes('— Tamama'));});
test('hops send up the head-pat pixel hearts, which float on briefly after it leaves',()=>{
 for(const [t,shown] of [[2300,false],[2500,true],[3500,true],[4300,false]]){const {view,log,ctx}=boot();view.draw(ctx(),{...pet,elapsed:t});assert.equal(!!log.hearts,shown,'at '+t);if(shown)assert.equal(log.hearts[0][1],600,'spawning stops with the hops');}});
