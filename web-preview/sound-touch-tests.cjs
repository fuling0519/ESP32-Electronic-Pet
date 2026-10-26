/* Run: node --test web-preview/sound-touch-tests.cjs. 養成同步 phase 5 (PROTOCOL.md): the sound feature (status sound,
 * set_sound with its op) and the petted / poked pet_events, over a fake BLE writer with fake timers. No real ESP32. */
const {test:plainTest,mock}=require('node:test');
const assert=require('node:assert/strict');
const vm=require('node:vm'),fs=require('node:fs'),crypto=require('node:crypto');
function boot(){
 const storage=new Map(),elements=new Map();
 const element=()=>{const e={textContent:'',value:'',hidden:false,disabled:false,title:'',attrs:{},classes:new Set(),children:[],replaceChildren(){this.children=[];},append(...items){this.children.push(...items);},appendChild(item){this.children.push(item);return item;},showModal(){},after(){},before(){},focus(){},setAttribute(k,v){this.attrs[k]=v;},prepend(...items){this.children.unshift(...items);},addEventListener(k,f){this['on_'+k]=f;}};
  e.classList={toggle(k,on){on?e.classes.add(k):e.classes.delete(k);}};return e;};
 const context=vm.createContext({console,crypto:crypto.webcrypto,TextEncoder,TextDecoder,Uint8Array,Uint32Array,DataView,BigInt,Map,Set,JSON,Number,Error,
 setTimeout:(...a)=>setTimeout(...a),clearTimeout:t=>clearTimeout(t),
 document:{querySelector:s=>{if(!elements.has(s))elements.set(s,element());return elements.get(s);},createElement:element},
 localStorage:{getItem:k=>storage.get(k)??null,setItem:(k,v)=>storage.set(k,v),removeItem:k=>storage.delete(k)},
 location:{protocol:'https:'},isSecureContext:true,navigator:{}});context.window=context;
 Object.defineProperty(context,'Date',{get:()=>Date});
 const logs=[];context.console={...console,info:(...a)=>logs.push(a.join(' '))};
 for(const file of ['local-store.js','pet-rules.js','pet-games.js','wyvern-sprites.js','wyvern-view.js','data-sources.js','ble-source.js','memorials.js','data-manager.js','device-session.js','device-care.js'])vm.runInContext(fs.readFileSync(__dirname+'/'+file,'utf8'),context,{filename:file});
 const touches=[];context.PetTouch={remote:kind=>{touches.push(kind);return context.remoteReason||'';}};
 vm.runInContext(fs.readFileSync(__dirname+'/device-sound.js','utf8'),context,{filename:'device-sound.js'});
 const b={c:context,m:context.petDataManager,ble:context.PetSources.ble,elements,sent:[],logs,touches};
 const parts=new Map();b.ble.writer={async writeValueWithResponse(d){const id=d[0]|d[1]<<8,list=parts.get(id)||[];list[d[2]]=Array.from(d.slice(4));parts.set(id,list);if(list.filter(Boolean).length===d[3]){parts.delete(id);b.sent.push(JSON.parse(Buffer.from(list.flat()).toString('utf8')));}}};
 return b;
}
const PET_ID='42';
const status=(pet={},top={})=>({v:1,type:'status',device_id:'esp32-AABBCCDDEEFF',activity:'idle',sound:'on',...top,pet:{id:PET_ID,name:'Tamama',life_stage:'baby',satiety:60,mood:70,cleanliness:50,age_seconds:'3600',health:'healthy',is_dead:false,sleep:'awake',...pet}});
const info=(features=['web_game','pet_events','care','sound'])=>({v:1,type:'device_info',id:1,protocol_version:1,device_id:'esp32-AABBCCDDEEFF',firmware_version:'test',max_message_bytes:1024,features});
const test=(name,fn)=>plainTest(name,async()=>{mock.timers.enable({apis:['setTimeout','Date'],now:0});try{await fn();}finally{mock.timers.reset();}});
const plain=x=>JSON.parse(JSON.stringify(x));
const flush=async()=>{for(let i=0;i<5;i++)await new Promise(r=>setImmediate(r));};
function connected(features,pet={},top={}){const b=boot();b.ble.connection='waiting';b.ble.messageReceived(info(features));b.ble.messageReceived(status(pet,top));b.m.select('ble');b.c.PetDeviceSound.sync();return b;}
const sounds=b=>b.sent.filter(x=>x.cmd==='set_sound');
const ack=(b,x,extra={})=>b.ble.messageReceived({v:1,type:'command_result',id:x.id,ok:true,...extra});
const result=(b,x,extra={})=>b.ble.messageReceived({v:1,type:'action_result',id:x.id,op:x.op,action:'set_sound',outcome:'done',changes:{muted:true},...extra});
const watch=p=>{const r={state:'pending'};p.then(v=>{r.state='done';r.value=v;},e=>{r.state='error';r.error=e;});return r;};
const event=(b,name,extra={})=>b.ble.messageReceived({v:1,type:'pet_event',event:name,pet_id:PET_ID,source:'device',...extra});
const button=b=>b.elements.get('#device-sound-button'),row=b=>b.elements.get('#device-sound');

test('status sound: on / muted are kept; anything else rejects the snapshot; absent stays unchanged',()=>{
 for(const s of ['on','muted'])assert.equal(connected(undefined,{},{sound:s}).m.view('ble').sound,s);
 for(const bad of ['off','ON',true,1,null,''])assert.throws(()=>connected(undefined,{},{sound:bad}),/聲音欄位/,String(bad));
 const old=connected(['web_game','pet_events','care'],{},{sound:undefined});
 assert.equal(old.m.view('ble').soundControl,false);assert.equal(old.m.view('ble').sound,null);
 const noField=connected(undefined,{},{sound:undefined});assert.equal(noField.m.view('ble').soundControl,true);assert.equal(noField.m.view('ble').sound,null);
});
test('a disconnected view never offers the sound switch',()=>{
 const b=connected();b.ble.disconnect();const v=b.m.view('ble');assert.equal(v.soundControl,false);assert.equal(v.sound,null);
});
test('set_sound sends muted and a fresh 16-hex op without pet_id, resolves on the matching action_result',async()=>{
 const b=connected(),r=watch(b.m.setDeviceSound(true));await flush();
 const [x]=sounds(b);assert.deepEqual(Object.keys(x).sort(),['cmd','id','muted','op','v']);assert.equal(x.muted,true);assert.match(x.op,/^[0-9a-f]{16}$/);
 assert.equal(b.m.view('ble').careSending,true);
 ack(b,x);await flush();assert.equal(r.state,'pending');
 result(b,{...x,op:'0000000000000000'});await flush();assert.equal(r.state,'pending');
 result(b,x);await flush();assert.equal(r.state,'done');assert.deepEqual(plain(r.value),{action:'set_sound',changes:{muted:true}});
 assert.equal(b.m.view('ble').careSending,false);
 assert.equal(b.m.view('ble').sound,'on','the state only changes with the next snapshot');
});
test('set_sound rejects a non-boolean, a missing feature and a disconnected device',()=>{
 const b=connected();assert.throws(()=>b.m.setDeviceSound('yes'),/格式/);
 const old=connected(['web_game','pet_events','care']);assert.throws(()=>old.m.setDeviceSound(true),/不能從網頁調整聲音/);
 const dis=connected();dis.ble.disconnect();assert.throws(()=>dis.m.setDeviceSound(true),/不能從網頁調整聲音|裝置模式/);
});
test('set_sound resends the same op after 10 s and gives up after 30 s with timeout',async()=>{
 const b=connected(),r=watch(b.m.setDeviceSound(false));await flush();ack(b,sounds(b)[0]);
 mock.timers.tick(10000);await flush();mock.timers.tick(10000);await flush();
 const s=sounds(b);assert.equal(s.length,3);assert.equal(new Set(s.map(x=>x.op)).size,1);assert.ok(s.every(x=>x.muted===false));
 mock.timers.tick(10000);await flush();assert.equal(r.state,'error');assert.equal(r.error.code,'timeout');
 assert.equal(b.m.view('ble').careSending,false);
});
test('refusals and failed results carry their protocol code; the page maps them to Traditional Chinese',async()=>{
 for(const code of ['busy','missing_field','invalid_value','op_conflict']){
  const b=connected(),r=watch(b.c.PetDeviceSound.run());await flush();const [x]=sounds(b);
  ack(b,x,{ok:false,code});await flush();assert.equal(r.state,'error',code);assert.equal(r.error.code,code);assert.ok(/[一-鿿]/.test(r.error.message),code);
 }
 const b=connected(),r=watch(b.c.PetDeviceSound.run());await flush();const [x]=sounds(b);
 result(b,x,{outcome:'failed',code:'save_failed',changes:undefined});await flush();assert.equal(r.error.code,'save_failed');assert.match(r.error.message,/保存聲音設定/);
 assert.match(b.c.PetDeviceSound.errorText({code:'busy'}),/正在調整聲音設定，等它回到主畫面再試/);
});
test('a second operation waits for the first; done text follows the action_result',async()=>{
 const b=connected();b.m.setDeviceSound(true);await flush();assert.throws(()=>b.m.setDeviceSound(false),/還在等/);
 const D=b.c.PetDeviceSound;assert.equal(D.doneText({changes:{muted:true}}),'電子雞已設為靜音。');assert.equal(D.doneText({changes:{muted:false}}),'電子雞的聲音已開啟。');
});
test('button label and state come from the snapshot only and the row is device-mode only',async()=>{
 const b=connected();assert.equal(row(b).hidden,false);assert.equal(button(b).textContent,'電子雞聲音：開');assert.equal(button(b).title.includes('不是網頁聲音'),true);
 b.ble.messageReceived(status({},{sound:'muted'}));assert.equal(button(b).textContent,'電子雞聲音：靜音');
 // A click changes nothing locally: the label stays until a snapshot says otherwise.
 button(b).on_click();await flush();assert.equal(sounds(b)[0].muted,false,'muted now, so the click asks for sound on');assert.equal(button(b).textContent,'電子雞聲音：靜音');
 assert.equal(button(b).disabled,true,'disabled while the request is in flight');
 b.m.select('mock');b.c.PetDeviceSound.sync();assert.equal(row(b).hidden,true);
 const old=connected(['web_game','pet_events','care'],{},{sound:undefined});assert.equal(row(old).hidden,true);
});
test('the switch is blocked with a reason unless the activity is idle, menu, sleeping or ended',()=>{
 for(const a of ['idle','menu','sleeping','ended']){const b=connected(undefined,{},{activity:a});assert.equal(b.c.PetDeviceSound.blocker(),'',a);assert.equal(button(b).attrs['aria-disabled'],'false',a);}
 for(const a of ['device_game','web_game','feeding','petting','volume']){const b=connected(undefined,{},{activity:a});assert.notEqual(b.c.PetDeviceSound.blocker(),'',a);assert.equal(button(b).attrs['aria-disabled'],'true',a);assert.ok(button(b).title,a);}
 const none=connected(undefined,{},{sound:undefined});assert.match(none.c.PetDeviceSound.blocker(),/還沒收到/);
 const dis=connected();dis.ble.disconnect();assert.match(dis.c.PetDeviceSound.blocker(),/請先連接/);
});
test('a blocked click sends nothing and says why on the status line',async()=>{
 const b=connected(undefined,{},{activity:'feeding'});button(b).on_click();await flush();
 assert.equal(sounds(b).length,0);assert.match(b.elements.get('#status').textContent,/正在忙/);
});
test('device petted / poked events play the touch reaction once for the current pet and send nothing',()=>{
 const b=connected(),before=plain(b.m.read('ble').pet);
 event(b,'petted');event(b,'poked');assert.deepEqual(b.touches,['pet','poke']);
 assert.deepEqual(plain(b.m.read('ble').pet),before,'events never change state');assert.equal(b.sent.length,0);
 assert.ok(b.logs.some(l=>l.includes('petted')&&l.includes('accepted')));
});
test('touch events are dropped by the BLE layer for another pet, the web source, an undeclared feature and unknown names',()=>{
 const b=connected();event(b,'petted',{pet_id:'7'});event(b,'poked',{source:'web'});event(b,'tickled');assert.deepEqual(b.touches,[]);
 const noEvents=connected(['web_game','care','sound']);event(noEvents,'petted');assert.deepEqual(noEvents.touches,[]);
});
test('an ineligible touch is skipped silently with one reason line',()=>{
 const b=connected();b.c.remoteReason='asleep';const before=b.elements.get('#status')?.textContent;
 event(b,'petted');assert.deepEqual(b.touches,['pet']);
 assert.ok(b.logs.some(l=>l.includes('petted')&&l.includes('no animation: asleep')));
 assert.equal(b.elements.get('#status')?.textContent,before);
});
test('old firmware without the touch events or the sound feature is unchanged',()=>{
 const b=connected(['web_game','pet_events','care'],{},{sound:undefined});event(b,'fed');assert.deepEqual(b.touches,[]);
 assert.equal(b.m.view('ble').soundControl,false);assert.equal(row(b).hidden,true);
});

// PetTouch.remote: the eligibility the web touch uses, driven through pet-interactions.js with a stub DOM.
function touchBoot({pet={},source='ble',activity='idle',busy={}}={}){
 const events=[],el=(extra={})=>Object.assign({style:{},disabled:false,textContent:'',setAttribute(){},append(){},appendChild(){},querySelector:()=>null,getContext:()=>new Proxy({},{get:()=>()=>{}}),showModal(){},open:false},extra);
 const canvas=el({addEventListener(){},getBoundingClientRect:()=>({left:0,top:0,width:384,height:256}),setPointerCapture(){}});
 const nodes={'.scene':el(),'#bird':canvas,'.care':null,'#status':el(),'#touch-pet':el(),'#touch-poke':el(),'#open-status':el(),'#game-dialog':el(),'#farewell-dialog':el(),'[data-action="feed"]':el(),'#pet-status-card':el({addEventListener(){},matches:()=>false,hidePopover(){}}),'#status-orb-image':el({src:''}),'#open-cemetery':el({addEventListener(){}})};
 const p={petId:'p1',stage:'adult',speciesId:2,healthState:'healthy',sleepMode:'awake',isDeparted:false,...pet};
 const c={addEventListener(){},performance:{now:()=>0},matchMedia:()=>({matches:false}),requestAnimationFrame(){},location:{search:''},setTimeout,clearTimeout,
  CustomEvent:class{constructor(type,init){this.type=type;this.detail=init?.detail;}},PetEffects:{HEART_LIFE:500,SHOCK_MS:900,hearts(){},shock(){}},
  petDataManager:{read:()=>({pet:p,metadata:{source,activity}}),command:()=>{throw new Error('touch must not send commands');}},
  document:{querySelector:s=>s in nodes?nodes[s]:null,createElement:()=>el(),body:el(),dispatchEvent:e=>events.push(e)},...busy};
 c.window=c;vm.createContext(c);vm.runInContext(fs.readFileSync(__dirname+'/pet-interactions.js','utf8'),c);
 return {c,events,nodes,kinds:()=>events.filter(e=>e.type==='pet-interaction-complete').map(e=>e.detail.kind)};
}
test('PetTouch.remote plays poke and pet for an eligible device pet, with a device status line',()=>{
 const t=touchBoot();assert.equal(t.c.PetTouch.remote('poke'),'');assert.equal(t.c.PetTouch.remote('pet'),'');assert.deepEqual(t.kinds(),['poke','pet']);
 assert.match(t.nodes['#status'].textContent,/電子雞上有人摸牠/);
});
test('PetTouch.remote skips every ineligible case without a status line or reaction',()=>{
 const cases=[['not device mode',{source:'mock'}],['device ended',{activity:'ended'}],['not baby/adult',{pet:{stage:'egg'}}],['not healthy',{pet:{healthState:'sick'}}],['not healthy',{pet:{healthState:'dead'}}],['asleep',{pet:{sleepMode:'normal'}}],['departed',{pet:{isDeparted:true}}],
  ['another animation playing',{busy:{PetLevelUp:{active:true}}}],['another animation playing',{busy:{PetGrowthView:{active:true}}}],['another animation playing',{busy:{PetGardenBusy:()=>true}}],
  ['busy (game or dialog)',{busy:{PetToyPlay:{active:()=>true}}}],['busy (game or dialog)',{busy:{PetChorus:{active:()=>true}}}]];
 for(const [why,opts] of cases){const t=touchBoot(opts);assert.equal(t.c.PetTouch.remote('pet'),why,why);assert.deepEqual(t.kinds(),[],why);assert.equal(t.nodes['#status'].textContent,'',why);}
});
