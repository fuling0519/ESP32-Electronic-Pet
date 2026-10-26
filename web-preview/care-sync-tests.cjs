/* Run: node --test web-preview/care-sync-tests.cjs. 養成同步 phase 2 (PROTOCOL.md): the care command with its op,
 * two-step reply, action_result and resends, over a fake BLE writer with fake timers. No radio or real ESP32. */
const {test:plainTest,mock}=require('node:test');
const assert=require('node:assert/strict');
const vm=require('node:vm'),fs=require('node:fs'),crypto=require('node:crypto');
function boot(){
 const storage=new Map(),elements=new Map();const element=()=>({textContent:'',value:'',children:[],replaceChildren(){this.children=[];},append(...items){this.children.push(...items);},appendChild(item){this.children.push(item);return item;},showModal(){},after(){},before(){},focus(){},setAttribute(){},prepend(...items){this.children.unshift(...items);},addEventListener(){}});
 // Timers and Date go through the outer realm at call time, so mock.timers drives them.
 const context=vm.createContext({console,crypto:crypto.webcrypto,TextEncoder,TextDecoder,Uint8Array,Uint32Array,DataView,BigInt,Map,Set,JSON,Number,Error,
 setTimeout:(...a)=>setTimeout(...a),clearTimeout:t=>clearTimeout(t),
 document:{querySelector:s=>{if(!elements.has(s))elements.set(s,element());return elements.get(s);},createElement:element},
 localStorage:{getItem:k=>storage.get(k)??null,setItem:(k,v)=>storage.set(k,v),removeItem:k=>storage.delete(k)},
 location:{protocol:'https:'},isSecureContext:true,navigator:{}});context.window=context;
 Object.defineProperty(context,'Date',{get:()=>Date});
 for(const file of ['local-store.js','pet-rules.js','pet-games.js','wyvern-sprites.js','wyvern-view.js','data-sources.js','ble-source.js','memorials.js','data-manager.js','device-session.js','device-care.js'])vm.runInContext(fs.readFileSync(__dirname+'/'+file,'utf8'),context,{filename:file});
 const b={c:context,m:context.petDataManager,ble:context.PetSources.ble,elements,sent:[]};
 // Fake GATT writer: reassembles the 16-byte fragments into the JSON commands the page sent.
 const parts=new Map();b.ble.writer={async writeValueWithResponse(d){const id=d[0]|d[1]<<8,list=parts.get(id)||[];list[d[2]]=Array.from(d.slice(4));parts.set(id,list);if(list.filter(Boolean).length===d[3]){parts.delete(id);b.sent.push(JSON.parse(Buffer.from(list.flat()).toString('utf8')));}}};
 return b;
}
const PET_ID='42';
const status=(pet={},top={})=>({v:1,type:'status',device_id:'esp32-AABBCCDDEEFF',activity:'idle',...top,pet:{id:PET_ID,name:'Tamama',life_stage:'baby',satiety:60,mood:70,cleanliness:50,age_seconds:'3600',health:'healthy',is_dead:false,sleep:'awake',...pet}});
const info=(features=['web_game','pet_events','care'])=>({v:1,type:'device_info',id:1,protocol_version:1,device_id:'esp32-AABBCCDDEEFF',firmware_version:'test',max_message_bytes:1024,features});
// Every test runs on fake timers (and Date), so pending waits never hold the process open. Per test, not as file
// hooks, because --test-isolation=none shares one root with the other test files.
const test=(name,fn)=>plainTest(name,async()=>{mock.timers.enable({apis:['setTimeout','Date'],now:0});try{await fn();}finally{mock.timers.reset();}});
const plain=x=>JSON.parse(JSON.stringify(x));
const flush=async()=>{for(let i=0;i<5;i++)await new Promise(r=>setImmediate(r));};
function connected(features,pet={},top={}){const b=boot();b.ble.connection='waiting';b.ble.messageReceived(info(features));b.ble.messageReceived(status(pet,top));b.m.select('ble');return b;}
const cares=b=>b.sent.filter(x=>x.cmd==='care');
const ack=(b,x,extra={})=>b.ble.messageReceived({v:1,type:'command_result',id:x.id,ok:true,...extra});
const result=(b,x,extra={})=>b.ble.messageReceived({v:1,type:'action_result',id:x.id,op:x.op,action:x.action,outcome:'done',changes:{satiety:20,exp:3,levels_gained:0},...extra});
// Settles a promise into a plain record so tests can check it without unhandled rejections.
const watch=p=>{const r={state:'pending'};p.then(v=>{r.state='done';r.value=v;},e=>{r.state='error';r.error=e;});return r;};

test('care sends a fresh 16-hex op with the current pet id and resolves on the matching action_result',async()=>{
 const b=connected(),r=watch(b.m.command('feed'));await flush();
 const [x]=cares(b);assert.deepEqual(Object.keys(x).sort(),['action','cmd','id','op','pet_id','v']);
 assert.equal(x.action,'feed');assert.equal(x.pet_id,PET_ID);assert.match(x.op,/^[0-9a-f]{16}$/);
 assert.equal(b.m.view('ble').careSending,true);
 ack(b,x);await flush();assert.equal(r.state,'pending','command_result ok only accepts; it settles nothing');
 result(b,{...x,op:'0000000000000000'});await flush();assert.equal(r.state,'pending','another op is not this action');
 result(b,x);await flush();assert.equal(r.state,'done');assert.deepEqual(plain(r.value),{action:'feed',changes:{satiety:20,exp:3,levels_gained:0}});
 assert.equal(b.m.view('ble').careSending,false);
 const r2=watch(b.m.command('clean'));await flush();assert.notEqual(cares(b)[1].op,x.op,'every player action has its own op');
 result(b,cares(b)[1],{action:'clean',changes:{cleanliness:30,exp:3,levels_gained:1,bogus:5,satiety:-1}});await flush();
 assert.deepEqual(plain(r2.value.changes),{cleanliness:30,exp:3,levels_gained:1},'only known non-negative integer deltas');
});
test('no action_result in 10 s resends the same op; a replay with a new id resolves by op',async()=>{
{const b=connected(),r=watch(b.m.command('treat'));await flush();
  const first=cares(b)[0];ack(b,first);
  mock.timers.tick(9999);await flush();assert.equal(cares(b).length,1,'still waiting');
  mock.timers.tick(1);await flush();const second=cares(b)[1];
  assert.equal(second.op,first.op);assert.notEqual(second.id,first.id);assert.equal(second.action,'treat');
  // The device already finished: it replays the action_result with this command's id.
  result(b,second,{action:'treat',changes:{exp:5,levels_gained:0,grew:true}});await flush();
  assert.equal(r.state,'done');assert.deepEqual(plain(r.value.changes),{exp:5,levels_gained:0,grew:true});
  mock.timers.tick(30000);await flush();assert.equal(cares(b).length,2,'no resend after the result');
}
});
test('30 s without a result gives up as unknown and reads the status again; a lost connection skips that resend',async()=>{
{const b=connected(),r=watch(b.m.command('feed'));await flush();
  mock.timers.tick(10000);await flush();assert.equal(cares(b).length,2);
  b.ble.connection='waiting';mock.timers.tick(10000);await flush();assert.equal(cares(b).length,2,'not ready: no write this round');
  b.ble.messageReceived(status());await flush();assert.equal(r.state,'pending');
  mock.timers.tick(10000);await flush();
  assert.equal(r.state,'error');assert.equal(r.error.code,'timeout');assert.match(r.error.message,/結果不明/);
  assert.equal(cares(b).length,2,'never resent at 30 s or later');assert.ok(cares(b).every(x=>x.op===cares(b)[0].op));
  assert.equal(b.sent.at(-1).cmd,'get_status','a fresh snapshot is requested');
  assert.equal(b.c.PetDeviceCare.errorText(r.error),'沒有收到電子雞的照顧結果，結果不明，請看最新狀態。');
}
});
test('command_result refusals and outcome failed reject with the protocol code and a Traditional Chinese reason',async()=>{
 const texts={busy:/主畫面/,pet_changed:/寵物換了/,not_allowed:/不能這樣照顧/,op_conflict:/再按一次/,invalid_value:/更新韌體/};
 for(const code of Object.keys(texts)){const b=connected(),r=watch(b.c.PetDeviceCare.run('feed'));await flush();
  ack(b,cares(b)[0],{ok:false,code});await flush();assert.equal(r.state,'error',code);assert.equal(r.error.code,code);assert.match(r.error.message,texts[code]);
  assert.equal(b.m.view('ble').careSending,false);}
 for(const code of ['save_failed','not_allowed','pet_changed']){const b=connected(),r=watch(b.c.PetDeviceCare.run('rest'));await flush();
  const x=cares(b)[0];assert.equal(x.action,'sleep');ack(b,x);result(b,x,{outcome:'failed',code,changes:undefined});await flush();
  assert.equal(r.state,'error');assert.equal(r.error.code,code);}
 const b=connected(),r=watch(b.c.PetDeviceCare.run('rest'));await flush();result(b,cares(b)[0],{outcome:'failed',code:'save_failed'});await flush();assert.match(r.error.message,/沒能保存/);
});
test('care needs a ready connection, the care feature and one action at a time',async()=>{
 const old=connected(['web_game','pet_events']);assert.equal(old.m.view('ble').care,false);
 assert.throws(()=>old.m.command('feed'),/唯讀/);assert.throws(()=>old.ble.care('feed'),/不能從網頁照顧/);
 assert.match(old.c.PetDeviceCare.blocker('feed'),/還不支援網頁照顧/);
 const b=connected();assert.throws(()=>b.ble.care('play'),/不支援/);
 watch(b.m.command('feed'));await flush();assert.throws(()=>b.ble.care('clean'),/上一個/);assert.match(b.c.PetDeviceCare.blocker('clean'),/上一個/);
 b.ble.disconnect();assert.equal(b.m.view('ble').care,false);assert.throws(()=>b.ble.care('feed'),/不能從網頁照顧/);
 // Demo mode never reaches the device.
 const d=boot();d.m.select('mock');const before=d.sent.length;d.m.command('feed');assert.equal(d.sent.length,before);
});
test('care buttons follow activity and pet conditions from the snapshot',()=>{
 const cases=[
  [{},{},{feed:'',clean:'',treat:/沒有生病/,rest:''}],
  [{},{activity:'menu'},{feed:'',clean:'',rest:''}],
  [{health:'sick',sick_seconds:5},{},{treat:'',feed:''}],
  [{sleep:'normal'},{activity:'sleeping'},{rest:'',feed:/睡覺/,clean:/睡覺/}],
  [{sleep:'normal'},{activity:'menu'},{rest:/睡眠畫面/}],
  [{},{activity:'device_game'},{feed:/陪你玩/,rest:/陪你玩/}],
  [{},{activity:'busy'},{feed:/主畫面/,rest:/主畫面/}],
  [{},{activity:'someday'},{clean:/主畫面/}],
  [{health:'dead',is_dead:true},{activity:'ended'},{feed:/不能照顧/,rest:/不能照顧/}],
  [{life_stage:'egg'},{},{feed:/孵化/}]];
 for(const [pet,top,want] of cases){const {c}=connected(undefined,pet,top);
  for(const [kind,hint] of Object.entries(want)){const why=c.PetDeviceCare.blocker(kind),label=JSON.stringify([pet,top,kind]);if(hint==='')assert.equal(why,'',label);else assert.match(why,hint,label);}}
 assert.equal(connected(undefined,{sleep:'normal'},{activity:'sleeping'}).c.PetDeviceCare.actionFor('rest'),'wake');
 assert.equal(connected().c.PetDeviceCare.actionFor('rest'),'sleep');
});
test('result lines name the device deltas only',()=>{
 const {c}=boot(),t=c.PetDeviceCare.doneText;
 assert.equal(t({action:'feed',changes:{satiety:20,exp:3,levels_gained:0}}),'餵好了：飽食度 +20、EXP +3。');
 assert.equal(t({action:'clean',changes:{cleanliness:0,exp:0,levels_gained:0}}),'清潔好了（已經滿了，數值沒有增加）。');
 assert.equal(t({action:'treat',changes:{exp:5,levels_gained:1,grew:true}}),'治療好了：EXP +5，升了 1 級，還長大了。');
 assert.equal(t({action:'sleep',changes:{}}),'牠在電子雞上睡著了。');assert.equal(t({action:'wake',changes:{grew:false}}),'牠醒來了。');
});

// The real page: buttons, the request, the garden animation only after done, and meters only from snapshots.
function page(features){
 const b=boot(),{c}=b;const get=s=>c.document.querySelector(s);
 const button=kind=>({dataset:{action:kind},title:'',attrs:{},classes:new Set(),small:{textContent:''},querySelector(){return this.small;},setAttribute(k,v){this.attrs[k]=v;},classList:{},addEventListener(type,fn){this.click=fn;}});
 const buttons=['feed','clean','play','rest','treat'].map(button);for(const x of buttons)x.classList={toggle:(k,on)=>on?x.classes.add(k):x.classes.delete(k),add(){},remove(){},contains:()=>false};
 const drawing=new Proxy({drawImage(){},createRadialGradient:()=>({addColorStop(){}}),createLinearGradient:()=>({addColorStop(){}})},{get:(o,k)=>o[k]||(()=>{})});
 const decorate=el=>Object.assign(el,{dataset:{},style:{},classList:{add(){},remove(){},toggle(){},contains(){return false;}},setAttribute(){},removeAttribute(){},focus(){},close(){},getContext:()=>drawing,clientWidth:384});
 for(const id of fs.readFileSync(__dirname+'/index.html','utf8').matchAll(/id="([^"]+)"/g))decorate(get('#'+id[1]));const h1=decorate({});get('#aviary').querySelector=()=>h1;
 get('#stage').options=['egg','baby','adult'].map(value=>({value}));c.document.querySelectorAll=()=>buttons;c.document.addEventListener=()=>{};
 let T=0;c.Image=class{constructor(){this.complete=true;this.naturalWidth=2172;this.naturalHeight=724;}};c.ResizeObserver=class{observe(){}};c.matchMedia=()=>({matches:false});c.devicePixelRatio=1;c.performance={now:()=>T};
 c.setInterval=()=>0;c.clearInterval=()=>{};c.document.dispatchEvent=()=>{};let frame;c.requestAnimationFrame=fn=>{frame=fn;};c.checkBleReadiness=()=>{};const createElement=c.document.createElement;c.document.createElement=()=>decorate(createElement());
 for(const file of ['wyvern-adult-view.js','shared-sprites.js','shared-view.js','assets/pets/bird/white-idle-data-v4.js','assets/pets/bird/white-sad-data-v1.js','assets/pets/bird/white-sick-data-v1.js','white-bird-view.js','bird-view.js','assets/pets/bird/white-activity-data-v1.js','white-bird-activity.js','bird-demo.js','assets/items/blue-seed-bowl-data-v1.js','blue-seed-bowl.js','white-bird-feed.js','growth-view.js','death-view.js','farewell-view.js','egg-view.js','bird-sad-sprites.js','sad-view.js','game-ui.js','app.js'])vm.runInContext(fs.readFileSync(__dirname+'/'+file,'utf8'),c,{filename:file});
 b.ble.connection='waiting';b.ble.messageReceived(info(features));b.ble.messageReceived(status());
 const [feed,clean,play,rest,treat]=buttons;return {...b,get,feed,clean,play,rest,treat,step:ms=>{T+=ms;frame(T);}};
}
test('device page without the care feature keeps care hidden and read-only',()=>{
 const p=page(['web_game','pet_events']);p.step(16);
 for(const x of [p.feed,p.clean,p.rest])assert.equal(x.disabled,true);assert.equal(p.play.disabled,false);
 assert.match(p.get('#status').textContent,/真實照顧命令尚未開放/);
});
test('device page: care buttons follow activity; feed animates only after done and meters only follow snapshots',async()=>{
 const p=page();p.step(16);
 assert.equal(p.feed.disabled,false);assert.equal(p.feed.title,'');assert.equal(p.rest.small.textContent,'睡覺');assert.equal(p.treat.disabled,true,'treat is for a sick pet only');
 assert.doesNotMatch(p.get('#status').textContent,/尚未開放/);
 p.ble.messageReceived(status({},{activity:'device_game'}));assert.ok(p.feed.classes.has('is-blocked'));assert.match(p.feed.title,/陪你玩/);
 p.feed.click();assert.match(p.get('#status').textContent,/陪你玩/);assert.equal(cares(p).length,0,'a blocked click only explains');
 p.ble.messageReceived(status({sleep:'normal'},{activity:'sleeping'}));assert.equal(p.rest.small.textContent,'起床');assert.ok(!p.rest.classes.has('is-blocked'));assert.ok(p.feed.classes.has('is-blocked'));
 p.ble.messageReceived(status({},{activity:'menu'}));assert.ok(!p.feed.classes.has('is-blocked'));
 const meter=()=>p.get('#satiety').value;assert.equal(meter(),60);
 p.feed.click();await flush();const x=cares(p)[0];assert.equal(x.action,'feed');
 assert.equal(p.get('#status').textContent,'正在請電子雞餵牠……');assert.equal(p.feed.disabled,true);assert.equal(p.play.disabled,true,'nothing else starts while waiting');
 ack(p,x);for(let i=0;i<30;i++)p.step(16);await flush();assert.equal(p.get('#status').textContent,'正在請電子雞餵牠……','no animation on the ack');
 result(p,x);await flush();p.step(16);
 assert.equal(p.get('#status').textContent,'餵好了：飽食度 +20、EXP +3。');assert.equal(p.play.disabled,true,'the garden feed animation is playing');
 assert.equal(meter(),60,'the +20 is not added locally');
 // The device's own event for this web command is skipped, so the animation does not restart.
 p.ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'fed',source:'web'});
 p.ble.messageReceived(status({satiety:80}));assert.equal(meter(),80,'the snapshot sets the meter');assert.equal(p.feed.disabled,true,'still animating after the snapshot');
 for(let i=0;i<220;i++)p.step(16);assert.equal(p.play.disabled,false,'the animation ends once');assert.equal(p.feed.disabled,false);
 assert.equal(p.get('#status').textContent,'餵好了：飽食度 +20、EXP +3。','the result stays readable after the animation');
 p.ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'fed',source:'web'});p.step(16);assert.equal(p.play.disabled,false,'a late web event plays nothing');
 // Refusal: a clear reason, no animation, buttons back.
 p.clean.click();await flush();ack(p,cares(p)[1],{ok:false,code:'busy'});await flush();p.step(16);
 assert.equal(p.get('#status').textContent,'電子雞正在忙，請先讓它回到主畫面。');assert.equal(p.clean.disabled,false);assert.equal(p.play.disabled,false);
 // Sleep has no garden animation: only the line, and the sleeping look waits for the snapshot.
 p.rest.click();await flush();const s=cares(p)[2];assert.equal(s.action,'sleep');result(p,s,{action:'sleep',changes:{}});await flush();
 assert.equal(p.get('#status').textContent,'牠在電子雞上睡著了。');assert.equal(p.m.view('ble').pet.condition,'awake');assert.equal(p.play.disabled,false);
 p.ble.messageReceived(status({sleep:'normal'},{activity:'sleeping'}));assert.equal(p.m.view('ble').pet.condition,'sleep');assert.equal(p.rest.small.textContent,'起床');
 // Failed outcome after the ack.
 p.rest.click();await flush();const w=cares(p)[3];assert.equal(w.action,'wake');ack(p,w);result(p,w,{outcome:'failed',code:'not_allowed'});await flush();
 assert.equal(p.get('#status').textContent,'寵物現在的狀態不能這樣照顧。');
});
test('device page: treat on a sick pet plays the potion only after done and the cure waits for the snapshot',async()=>{
 const p=page();p.ble.messageReceived(status({health:'sick',sick_seconds:30}));p.step(16);
 assert.equal(p.treat.disabled,false);p.treat.click();await flush();const x=cares(p)[0];assert.equal(x.action,'treat');
 result(p,x,{action:'treat',changes:{exp:5,levels_gained:0,grew:false}});await flush();p.step(16);
 assert.equal(p.get('#status').textContent,'治療好了：EXP +5。');assert.equal(p.play.disabled,true);
 for(let i=0;i<240;i++)p.step(16);assert.equal(p.play.disabled,false);
 assert.equal(p.m.read('ble').pet.healthState,'sick','only the next snapshot cures it');
});
