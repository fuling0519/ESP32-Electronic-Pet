const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
function boot(){
 const elements=[],events=new Map(),calls=[];
 const element=()=>({hidden:false,value:'auto',children:[],setAttribute(){},appendChild(e){this.children.push(e);elements.push(e);}});
 const controls=element();
 class Image{constructor(){this.complete=true;}set src(value){this._src=value;const png=fs.readFileSync(path.join(__dirname,value));this.naturalWidth=png.readUInt32BE(16);this.naturalHeight=png.readUInt32BE(20);}get src(){return this._src;}}
 const c={Image,performance:{now:()=>100},document:{hidden:false,querySelector:s=>s==='.preview-controls'?controls:s==='dialog[open]'&&c.blocked?{}:null,createElement:element,addEventListener:(k,f)=>events.set(k,f)},PetRules:{updateSad:p=>{p.sadReasons.mood=p.mood<=40;return p.sadReasons.mood;}},PetSadView:{active:()=>false}};
 c.window=c;vm.createContext(c);vm.runInContext(fs.readFileSync(__dirname+'/wyvern-adult-view.js','utf8'),c);
 const stack=[],bounds=[];let transform={x:0,y:0,sx:1,sy:1};
 const ctx={globalAlpha:1,save(){stack.push({...transform,alpha:this.globalAlpha});},restore(){const t=stack.pop();this.globalAlpha=t.alpha;transform=t;},translate(x,y){transform.x+=x*transform.sx;transform.y+=y*transform.sy;},scale(x,y){transform.sx*=x;transform.sy*=y;},drawImage(...args){calls.push(args);const [,,,,,x,y,w,h]=args,a=transform.x+x*transform.sx,b=transform.x+(x+w)*transform.sx,top=transform.y+y*transform.sy;bounds.push({x:Math.min(a,b),y:top,w:Math.abs(b-a),h:h*transform.sy,facing:transform.sx,alpha:this.globalAlpha});}};
 return {c,view:c.WyvernAdultView,ctx,calls,bounds,events};
}
const pet=()=>({petId:'one',stage:'adult',healthState:'healthy',sleepMode:'awake',mood:55,sadReasons:{mood:false,satiety:false,cleanliness:false}});
test('all illustrated states draw real, complete PNG sources within the scene',()=>{
 const {view,ctx,calls}=boot();
 for(const state of ['idle','sad','sick','happy','shock','sleep','dead','flight']){
  assert.equal(view.paint(ctx,state,0,false).state,state);
  const [im,sx,sy,sw,sh,dx,dy,dw,dh]=calls.at(-1);
  assert(sx>=0&&sy>=0&&sx+sw<=im.naturalWidth&&sy+sh<=im.naturalHeight,state+' source crop');
  // Happy is the one deliberate x-squeeze, matching idle's eye spacing.
  assert(Math.abs(dw/dh-sw/sh*(state==='happy'?0.86:1))<1e-10,state+' proportions preserved');
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
 const {view,ctx}=boot(),p=pet(),before=JSON.stringify(p);
 assert.equal(view.resolve(p,0,null),'idle');assert.equal(JSON.stringify(p),before);
 assert.equal(view.resolve({...p,mood:100},0,null),'idle');
 assert.equal(view.resolve({...p,webExpression:'happy'},0,null),'happy');
 assert.equal(view.resolve({...p,mood:20},0,null),'sad');
 assert.equal(view.resolve({...p,mood:100,healthState:'sick'},0,null),'sick');
 assert.equal(view.resolve({...p,sleepMode:'normal',healthState:'sick'},0,null),'sleep');
 assert.equal(view.resolve({...p,healthState:'dead'},0,null),'dead');
 for(const state of ['sad','sick','happy','shock','sleep','dead','flight']){view.setPreview(state);assert.equal(view.resolve(p,0,null),state);assert.equal(JSON.stringify(p),before);}
 view.syncControls(p,false);assert.equal(view.resolve(p,0,null),'idle');
 assert.equal(view.draw(ctx,{...p,stage:'egg'},0,null,0,false),false);
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
test('head pat smiles from pointer-down and stays happy through release',()=>{
 const {c,view,ctx,events,calls}=boot(),p=pet();c.petDataManager={read:()=>({pet:p})};
 events.get('pet-touch-hold')({detail:{holding:true}});
 assert.equal(view.resolve(p,5000,null),'happy');
 assert.equal(view.draw(ctx,p,5000,null,0,false).index,1);
 assert.equal(calls.at(-1)[0].src.endsWith('emotions-sheet-v1.png'),true);
 c.performance.now=()=>6000;events.get('pet-touch-hold')({detail:{holding:false}});
 assert.equal(view.resolve(p,7000,null),'happy');
 assert.equal(view.resolve(p,7300,null),'idle');
 events.get('pet-touch-hold')({detail:{holding:true}});
 assert.equal(view.resolve({...p,healthState:'sick'},6100,null),'sick');
});
test('poke shows the startled still with a short bounce, then returns to idle',()=>{
 const {c,view,ctx,events,calls}=boot(),p=pet(),before=JSON.stringify(p);c.petDataManager={read:()=>({pet:p})};
 events.get('pet-interaction-complete')({detail:{kind:'poke'}});
 assert.equal(view.resolve(p,100,null),'shock');
 assert.equal(view.resolve({...p,healthState:'sick'},100,null),'sick');
 assert.equal(view.resolve(p,100,'feed'),'eat');
 assert.equal(view.draw(ctx,p,100,null,0,false).state,'shock');
 assert.match(calls.at(-1)[0].src,/shock-still-v1\.png$/);
 const rest=calls.at(-1)[6];view.draw(ctx,p,230,null,0,false);
 assert(calls.at(-1)[6]<rest,'bounces upward');
 view.draw(ctx,p,240,null,0,true);assert.equal(calls.at(-1)[6],rest,'reduced motion stays still');
 assert.equal(view.draw(ctx,p,1300,null,0,false).state,'idle');
 assert.equal(JSON.stringify(p),before);
});
test('unavailable art falls back without breaking rendering',()=>{
 const {view,ctx,calls}=boot();view.paint(ctx,'dead',0,true);calls.at(-1)[0].complete=false;
 assert.equal(view.paint(ctx,'dead',0,true),false);
});

test('flight uses all six rectangular cells with exact 900ms timing and fixed body pivot',()=>{
 const {view,ctx,calls,bounds}=boot(),times=[0,170,300,430,600,730];
 const manifest=JSON.parse(fs.readFileSync(__dirname+'/assets/pets/wyvern/adult/flight-sheet-v1.json','utf8'));
 for(const [i,t] of times.entries()){
  assert.equal(view.paint(ctx,'flight',t,false,{x:190,y:120,facing:1}).index,i);
  const args=calls.at(-1),right=bounds.at(-1);
  assert.equal(args[0].src,'assets/pets/wyvern/adult/flight-sheet-v1.png');
  assert.deepEqual(args.slice(1,5),[i%3*560,Math.floor(i/3)*448,560,448]);
  assert.deepEqual(args.slice(3,5),manifest.cellSize);
  assert(Math.abs(right.w-manifest.cellSize[0]*manifest.renderScale)<1e-10);
  assert(Math.abs(right.x-(190-manifest.renderAnchor[0]*manifest.renderScale))<1e-10);
  view.paint(ctx,'flight',t,false,{x:190,y:120,facing:-1});
  const left=bounds.at(-1);
  assert(Math.abs(right.x-left.x)<1e-10,'mirror has no pivot displacement');
  assert.equal(left.facing,-1);assert.equal(left.w,right.w);
 }
 assert.equal(view.paint(ctx,'flight',899,false).index,5);
 assert.equal(view.paint(ctx,'flight',900,false).index,0);
 assert.equal(view.paint(ctx,'flight',730,true).index,0);
});

test('complete travel path rests, takes off, reverses at a stop, returns and lands without clipping',()=>{
 const {view,ctx,bounds}=boot();
 assert.equal(view.travelAt(23999).active,false);
 assert.equal(view.travelAt(24000).active,true);
 assert.equal(view.travelAt(30479).active,true);
 assert.equal(view.travelAt(30480).active,false);
 assert.equal(view.travelAt(0,false).lift,0);
 const firstTurn=600+1200;
 assert.equal(view.travelAt(firstTurn+119,false).facing,1);
 assert.equal(view.travelAt(firstTurn+120,false).facing,-1);
 assert.equal(view.travelAt(firstTurn+119,false).x,view.travelAt(firstTurn+120,false).x);
 const faces=new Set();let grounded=0,total=0;
 for(let t=0;t<30480;t+=40){
  if(!view.travelAt(t).active)grounded++;total++;
  view.paintTravel(ctx,t,false,{withRest:true});
  const r=bounds.at(-1);faces.add(r.facing);
  assert(r.x>=0&&r.y>=0&&r.x+r.w<=384&&r.y+r.h<=256,JSON.stringify({t,r}));
 }
 assert.deepEqual([...faces].sort(),[-1,1]);
 assert(grounded/total>.78,'at least 78% of each automatic cycle is grounded');
 assert.equal(view.paintTravel(ctx,9000,true).state,'idle');
 assert.equal(view.travelAt(6479,false).x,190);
 assert(view.travelAt(6479,false).lift<.001);
});

function advance(view,ctx,p,from,to){let result;for(let now=from;now<=to;now+=100)result=view.draw(ctx,p,now,null,0,false);return result;}

test('automatic flight is presentation-only and touch bounds follow the moving pet',()=>{
 const {view,ctx}=boot(),p=pet(),before=JSON.stringify(p);
 assert.equal(advance(view,ctx,p,0,23900).state,'idle');
 const r=advance(view,ctx,p,24000,25700),target=view.interactionTarget(p);
 assert.equal(r.state,'flight');assert(target.x>250);assert(target.y<130);
 assert.equal(JSON.stringify(p),before);assert.equal(view.interactionTarget({...p,petId:'other'}),null);
});

test('care, reactions, illness, sleep and modal dialogs interrupt flight and restart with a rest',()=>{
 for(const kind of ['feed','clean','pet','poke','sick','sleep','sad','dialog','growth']){
  const {c,view,ctx,events}=boot(),p=pet();
  advance(view,ctx,p,0,25700);let action=null,next=p;
  if(['feed','clean'].includes(kind))action=kind;
  if(['pet','poke'].includes(kind)){c.performance.now=()=>25800;c.petDataManager={read:()=>({pet:p})};events.get('pet-interaction-complete')({detail:{kind}});}
  if(kind==='sick')next={...p,healthState:'sick'};
  if(kind==='sleep')next={...p,sleepMode:'normal'};
  if(kind==='sad')next={...p,mood:20};
  if(kind==='dialog')c.blocked=true;
  if(kind==='growth')c.PetGrowthView={active:true};
  assert.notEqual(view.draw(ctx,next,25800,action,0,false).state,'flight',kind);
  assert.notEqual(view.draw(ctx,next,26100,action,0,false).state,'flight',kind);
  assert.equal(view.interactionTarget(p).x,190,kind+' settled');
  c.blocked=false;c.PetGrowthView={active:false};
  advance(view,ctx,p,26200,27200);
  assert.equal(view.draw(ctx,p,27300,null,0,false).state,'idle',kind+' restarts on ground');
 }
});

test('reduced motion, new pets, hidden tabs and long rendering gaps discard old travel',()=>{
 for(const kind of ['reduce','pet','hidden','gap','baby','controls']){
  const {c,view,ctx,events}=boot(),p=pet();advance(view,ctx,p,0,25700);
  if(kind==='reduce')assert.equal(view.draw(ctx,p,25800,null,0,true).state,'idle');
  if(kind==='pet')assert.equal(view.draw(ctx,{...p,petId:'two'},25800,null,0,false).state,'idle');
  if(kind==='hidden'){c.document.hidden=true;events.get('visibilitychange')();assert.equal(view.draw(ctx,p,25800,null,0,false).state,'idle');c.document.hidden=false;}
  if(kind==='gap')assert.equal(view.draw(ctx,p,42000,null,0,false).state,'idle');
  if(kind==='baby'){assert.equal(view.draw(ctx,{...p,stage:'baby'},25800,null,0,false).state,'idle');assert.equal(view.draw(ctx,p,25900,null,0,false).state,'idle');}
  if(kind==='controls'){view.syncControls(p,true);view.syncControls(p,false);assert.equal(view.draw(ctx,p,25800,null,0,false).state,'idle');}
 }
});

test('manual flight still respects care and health, reduced motion and unloaded images',()=>{
 const {view,ctx,calls}=boot(),p=pet();view.setPreview('flight');
 assert.equal(view.resolve({...p,sleepMode:'normal'},0,null),'sleep');
 assert.equal(view.resolve({...p,healthState:'sick'},0,null),'sick');
 assert.equal(view.resolve({...p,mood:20},0,null),'sad');
 assert.equal(view.resolve(p,0,'feed'),'eat');
 assert.equal(view.draw(ctx,p,0,null,0,true).state,'idle');
 view.paint(ctx,'flight',0,false);calls.at(-1)[0].complete=false;
 assert.equal(view.paintTravel(ctx,2000).state,'idle');
});

test('late flight images begin with a fresh grounded wait instead of joining a route midair',()=>{
 const {view,ctx,calls}=boot(),p=pet();view.paint(ctx,'flight');const im=calls.at(-1)[0];im.complete=false;
 assert.equal(advance(view,ctx,p,0,9000).state,'idle');
 im.complete=true;
 assert.equal(advance(view,ctx,p,9100,33000).state,'idle');
 assert.equal(advance(view,ctx,p,33100,34100).state,'flight');
});

test('eating repeats bite and chew three times, empties the bowl one step per bite, then smiles',()=>{
 const {view}=boot(),at=ms=>({...view.eatSample(ms)});
 assert.deepEqual([0,449,450,849,850,1699,1700,2549,2550,2999].map(at),[
  {frame:1,bowl:0},{frame:1,bowl:0},{frame:2,bowl:1},{frame:2,bowl:1},{frame:1,bowl:1},
  {frame:2,bowl:2},{frame:1,bowl:2},{frame:2,bowl:3},{frame:3,bowl:3},{frame:3,bowl:3}]);
 // VM-realm objects are spread for deep comparison. Reduced motion keeps the chewing pose, but the smile still waits for the empty bowl.
 assert.deepEqual({...view.eatSample(0,true)},{frame:2,bowl:0});
 assert.deepEqual({...view.eatSample(2600,true)},{frame:3,bowl:3});
});

const baby=()=>({...pet(),stage:'baby'});
test('baby states draw from the baby sheets at one character size, within the scene',()=>{
 const {view,ctx,calls,bounds}=boot(),sizes={};
 for(const state of ['idle','sad','sick','happy','shock','eat','walk','sleep','dead']){
  assert.equal(view.paint(ctx,state,0,false,{stage:'baby'}).state,state);
  // Use on-screen bounds: mirrored poses draw relative to a translated origin.
  const [im,sx,sy,sw,sh,,,dw,dh]=calls.at(-1),{x:dx,y:dy}=bounds.at(-1);
  assert.match(im.src,/^assets\/pets\/wyvern\/baby\/(sheet-(a-v2|b-v2|c-v1)|sleep-dead-v5)\.png$/);
  assert(sx>=0&&sy>=0&&sx+sw<=im.naturalWidth&&sy+sh<=im.naturalHeight,state+' source crop');
  assert(Math.abs(dw/dh-sw/sh)<1e-10,state+' proportions preserved');
  // Square cells may carry transparent padding above the scene; the art itself must fit.
  const sheet=JSON.parse(fs.readFileSync(path.join(__dirname,im.src.replace(/\.png$/,'.json')),'utf8'));
  const artTop=dy+(sheet.anchor[1]-Math.max(...sheet.heights))*dh/sheet.cell;
  assert.equal(sw,sheet.cell);
  assert(dx>=0&&artTop>=0&&dx+dw<=384&&dy+dh<=256,state+' not clipped');
  assert(Math.abs(dy+dh*sheet.anchor[1]/sheet.cell-177)<1e-9,state+' feet on the ground line');
  sizes[im.src]=dw/sw;
 }
 // Sheet B is drawn larger to match sheet A's head width; baby stays ~73% of adult.
 assert(Math.abs(sizes['assets/pets/wyvern/baby/sheet-b-v2.png']/sizes['assets/pets/wyvern/baby/sheet-a-v2.png']-231/219)<1e-10);
 // Sleep is drawn mirrored around its anchor, so it stays centred on the same spot.
 view.paint(ctx,'sleep',0,false,{stage:'baby'});assert.equal(bounds.at(-1).facing,-1);
 assert(Math.abs(bounds.at(-1).x+bounds.at(-1).w/2-190)<.5);
 // Walk's third frame borrows sheet C's other-foot step at C's own scale.
 view.paint(ctx,'walk',0,false,{stage:'baby',frame:2});
 assert.match(calls.at(-1)[0].src,/sheet-c-v1\.png$/);assert.deepEqual(calls.at(-1).slice(1,5),[0,0,816,816]);
 assert(Math.abs(calls.at(-1)[7]/816-92/296*231/219*220/295)<1e-10);
});
test('baby resolves the same states, sleeps and rests in its own art, and eats with the smile frame',()=>{
 const {view,ctx,calls}=boot(),p=baby(),before=JSON.stringify(p);
 assert.equal(view.draw(ctx,p,0,null,0,false).state,'idle');
 assert.equal(view.draw(ctx,{...p,mood:20},100,null,0,false).state,'sad');
 assert.equal(view.draw(ctx,{...p,healthState:'sick'},200,null,0,false).state,'sick');
 assert.equal(view.draw(ctx,{...p,sleepMode:'normal'},300,null,0,false).state,'sleep');
 assert.equal(view.draw(ctx,{...p,healthState:'dead'},400,null,0,false).state,'dead');
 const cell=i=>[i%4*334,Math.floor(i/4)*334];
 for(const [sec,i] of [[0,10],[.5,11],[2.9,9]]){view.draw(ctx,p,500,'feed',sec,false);assert.deepEqual(calls.at(-1).slice(1,3),cell(i));}
 assert.equal(JSON.stringify(p),before);
 const t=view.interactionTarget(p);assert(t.rx<100&&t.ry<90,'smaller touch area');
});
test('baby walks on the ground left and right, rests mostly, and turns on a standing frame',()=>{
 const {view,ctx,bounds}=boot();
 assert.equal(view.walkAt(23999).active,false);
 assert.equal(view.walkAt(24000).active,true);
 assert.equal(view.walkAt(33839).active,true);
 assert.equal(view.walkAt(33840).active,false);
 assert(Math.abs(view.walkAt(9839,false).x-190)<.01,"walk returns to centre");
 assert.equal(view.walkAt(2400+119,false).facing,1);
 assert.equal(view.walkAt(2400+120,false).facing,-1);
 const faces=new Set();
 for(let t=0;t<9840;t+=40){
  const r=view.paintTravel(ctx,t,false,{stage:'baby'}),b=bounds.at(-1);faces.add(b.facing);
  assert(b.x>=0&&b.x+b.w<=384&&b.y+b.h<=256,JSON.stringify({t,b}));
  if(view.walkAt(t,false).still)assert.equal(r.index,1,'standing frame while turning');
 }
 assert.deepEqual([...faces].sort(),[-1,1]);
 assert.equal(view.paintTravel(ctx,3000,true,{stage:'baby'}).state,'idle');
 const p=baby();assert.equal(advance(view,ctx,p,0,23900).state,'idle');
 assert.equal(advance(view,ctx,p,24000,25500).state,'flight');
 assert(view.interactionTarget(p).x>190,'touch area follows the walk');
});
test('device mode keeps touch reactions while demo previews stay off',()=>{
 const {c,view,events}=boot(),p=pet();c.petDataManager={read:()=>({pet:p})};
 view.setPreview('sad');view.syncControls(p,false,true);assert.equal(view.resolve(p,100,null),'idle');
 events.get('pet-interaction-complete')({detail:{kind:'poke'}});view.syncControls(p,false,true);
 assert.equal(view.resolve(p,100,null),'shock');
 view.syncControls(p,false,false);assert.equal(view.resolve(p,100,null),'idle','touch off drops the reaction');
});
