const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
function boot(){
 const elements=[],events=new Map(),calls=[];
 const element=()=>({hidden:false,value:'auto',children:[],setAttribute(){},appendChild(e){this.children.push(e);elements.push(e);}});
 const controls=element();
 class Image{constructor(){this.complete=true;}set src(value){this._src=value;const png=fs.readFileSync(path.join(__dirname,value));this.naturalWidth=png.readUInt32BE(16);this.naturalHeight=png.readUInt32BE(20);}get src(){return this._src;}}
 const c={Image,performance:{now:()=>100},document:{querySelector:()=>controls,createElement:element,addEventListener:(k,f)=>events.set(k,f)},PetRules:{updateSad:p=>{p.sadReasons.mood=p.mood<=40;return p.sadReasons.mood;}},PetSadView:{active:()=>false}};
 c.window=c;vm.createContext(c);vm.runInContext(fs.readFileSync(__dirname+'/wyvern-adult-view.js','utf8'),c);
 const ctx={save(){},restore(){},drawImage(...args){calls.push(args);}};
 return {c,view:c.WyvernAdultView,ctx,calls,events,select:elements.find(e=>e.id==='wyvern-art-state')};
}
const pet=()=>({petId:'one',stage:'adult',healthState:'healthy',sleepMode:'awake',mood:55,sadReasons:{mood:false,satiety:false,cleanliness:false}});
test('all illustrated states draw real, complete PNG sources within the scene',()=>{
 const {view,ctx,calls}=boot();
 for(const state of ['idle','sad','sick','happy','sleep','dead','flight']){
  assert.equal(view.paint(ctx,state,0,false).state,state);
  const [im,sx,sy,sw,sh,dx,dy,dw,dh]=calls.at(-1);
  assert(sx>=0&&sy>=0&&sx+sw<=im.naturalWidth&&sy+sh<=im.naturalHeight,state+' source crop');
  assert(Math.abs(dw/dh-sw/sh)<1e-10,state+' proportions preserved');
  assert(dx>=0&&dy>=0&&dx+dw<=384&&dy+dh<=256,state+' not clipped');
 }
});
test('idle and sleep timing uses stable scale/anchors and reduced motion is static',()=>{
 const {view,ctx,calls}=boot();
 for(const [state,times] of [['idle',[0,220,380,600]],['sleep',[0,1100]]]){
  const frames=times.map(t=>{const r=view.paint(ctx,state,t,false);return {index:r.index,geometry:calls.at(-1).slice(5)};});
  assert.equal(new Set(frames.map(f=>f.index)).size,times.length);
  for(const f of frames)assert.deepEqual(f.geometry,frames[0].geometry);
  assert.equal(view.paint(ctx,state,1500,true).index,0);
 }
});
test('state priority and visual previews do not mutate domain values',()=>{
 const {view,ctx,select}=boot(),p=pet(),before=JSON.stringify(p);
 assert.equal(view.resolve(p,0,null),'idle');assert.equal(JSON.stringify(p),before);
 assert.equal(view.resolve({...p,mood:100},0,null),'idle');
 assert.equal(view.resolve({...p,webExpression:'happy'},0,null),'happy');
 assert.equal(view.resolve({...p,mood:20},0,null),'sad');
 assert.equal(view.resolve({...p,mood:100,healthState:'sick'},0,null),'sick');
 assert.equal(view.resolve({...p,sleepMode:'normal',healthState:'sick'},0,null),'sleep');
 assert.equal(view.resolve({...p,healthState:'dead'},0,null),'dead');
 for(const state of ['sad','sick','happy','sleep','dead','flight']){select.value=state;select.onchange();assert.equal(view.resolve(p,0,null),state);assert.equal(JSON.stringify(p),before);}
 view.syncControls(p,false);assert.equal(view.resolve(p,0,null),'idle');
 assert.equal(view.draw(ctx,{...p,stage:'baby'},0,null,0,false),false);
});
test('pet reaction expires, stays with its pet, and cannot override illness/death',()=>{
 const {c,view,events}=boot(),p=pet();c.petDataManager={read:()=>({pet:p})};
 events.get('pet-interaction-complete')({detail:{kind:'pet'}});
 assert.equal(view.resolve(p,100,null),'happy');
 assert.equal(view.resolve({...p,healthState:'sick'},100,null),'sick');
 assert.equal(view.resolve({...p,healthState:'dead'},100,null),'dead');
 assert.equal(view.resolve(p,1300,null),'idle');
 events.get('pet-interaction-complete')({detail:{kind:'pet'}});
 assert.equal(view.resolve({...p,petId:'two'},100,null),'idle');
});
test('unavailable art falls back without breaking rendering',()=>{
 const {view,ctx,calls}=boot();view.paint(ctx,'dead',0,true);calls.at(-1)[0].complete=false;
 assert.equal(view.paint(ctx,'dead',0,true),false);
});
