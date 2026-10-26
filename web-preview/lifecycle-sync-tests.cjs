/* Run: node --test web-preview/lifecycle-sync-tests.cjs. 養成同步 phase 3 (PROTOCOL.md): depart / adopt with the
 * lifecycle feature, the web confirm dialog (D3), the snapshot-driven farewell and egg, over a fake BLE writer with fake
 * timers. Harness copied from care-sync-tests.cjs. No radio or real ESP32. */
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
 for(const file of ['local-store.js','pet-rules.js','pet-games.js','wyvern-sprites.js','wyvern-view.js','data-sources.js','ble-source.js','memorials.js','data-manager.js','device-session.js','device-care.js','device-lifecycle.js'])vm.runInContext(fs.readFileSync(__dirname+'/'+file,'utf8'),context,{filename:file});
 const b={c:context,m:context.petDataManager,ble:context.PetSources.ble,elements,sent:[]};
 // Fake GATT writer: reassembles the 16-byte fragments into the JSON commands the page sent.
 const parts=new Map();b.ble.writer={async writeValueWithResponse(d){const id=d[0]|d[1]<<8,list=parts.get(id)||[];list[d[2]]=Array.from(d.slice(4));parts.set(id,list);if(list.filter(Boolean).length===d[3]){parts.delete(id);b.sent.push(JSON.parse(Buffer.from(list.flat()).toString('utf8')));}}};
 return b;
}
const PET_ID='42';
const status=(pet={},top={})=>({v:1,type:'status',device_id:'esp32-AABBCCDDEEFF',activity:'idle',...top,pet:{id:PET_ID,name:'Tamama',life_stage:'baby',satiety:60,mood:70,cleanliness:50,age_seconds:'3600',health:'healthy',is_dead:false,sleep:'awake',...pet}});
const info=(features=['web_game','pet_events','care','lifecycle'])=>({v:1,type:'device_info',id:1,protocol_version:1,device_id:'esp32-AABBCCDDEEFF',firmware_version:'test',max_message_bytes:1024,features});
// Every test runs on fake timers (and Date), so pending waits never hold the process open. Per test, not as file
// hooks, because --test-isolation=none shares one root with the other test files.
const test=(name,fn)=>plainTest(name,async()=>{mock.timers.enable({apis:['setTimeout','Date'],now:0});try{await fn();}finally{mock.timers.reset();}});
const plain=x=>JSON.parse(JSON.stringify(x));
const flush=async()=>{for(let i=0;i<5;i++)await new Promise(r=>setImmediate(r));};
function connected(features,pet={},top={}){const b=boot();b.ble.connection='waiting';b.ble.messageReceived(info(features));b.ble.messageReceived(status(pet,top));b.m.select('ble');return b;}
const cares=b=>b.sent.filter(x=>x.cmd==='care');
const sentOf=(b,cmd)=>b.sent.filter(x=>x.cmd===cmd);
const ack=(b,x,extra={})=>b.ble.messageReceived({v:1,type:'command_result',id:x.id,ok:true,...extra});
const result=(b,x,extra={})=>b.ble.messageReceived({v:1,type:'action_result',id:x.id,op:x.op,action:x.action,outcome:'done',changes:{satiety:20,exp:3,levels_gained:0},...extra});
// Settles a promise into a plain record so tests can check it without unhandled rejections.
const watch=p=>{const r={state:'pending'};p.then(v=>{r.state='done';r.value=v;},e=>{r.state='error';r.error=e;});return r;};

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
 for(const file of ['wyvern-adult-view.js','shared-sprites.js','shared-view.js','assets/pets/bird/white-idle-data-v4.js','assets/pets/bird/white-sad-data-v1.js','assets/pets/bird/white-sick-data-v1.js','white-bird-view.js','bird-view.js','assets/pets/bird/white-activity-data-v1.js','white-bird-activity.js','bird-demo.js','assets/items/blue-seed-bowl-data-v1.js','blue-seed-bowl.js','white-bird-feed.js','growth-view.js','level-up-view.js','death-view.js','farewell-view.js','egg-view.js','bird-sad-sprites.js','sad-view.js','game-ui.js','app.js'])vm.runInContext(fs.readFileSync(__dirname+'/'+file,'utf8'),c,{filename:file});
 b.ble.connection='waiting';b.ble.messageReceived(info(features));b.ble.messageReceived(status());
 const [feed,clean,play,rest,treat]=buttons;return {...b,get,feed,clean,play,rest,treat,step:ms=>{T+=ms;frame(T);}};
}
const L=b=>b.c.PetDeviceLifecycle;
const FULL='電子雞的紀念冊已滿，請先在 ESP32 上刪除一筆紀念紀錄再試。';
const departed={is_departed:true},dead={health:'dead',is_dead:true};

test('depart / adopt need the lifecycle feature; buttons follow the pet and activity from the snapshot',()=>{
 const old=connected(['web_game','pet_events','care']);assert.equal(old.m.view('ble').lifecycle,false);
 assert.equal(L(old).shown('depart'),false);assert.match(L(old).blocker('depart'),/還不支援/);
 assert.throws(()=>old.ble.lifecycle('depart'),/不能從網頁送別/);assert.throws(()=>old.m.farewell(),/請在裝置送別/);assert.throws(()=>old.m.adopt(),/請在裝置領養/);
 const cases=[
  [{},{},{depart:''},{adopt:false}],
  [{life_stage:'adult'},{activity:'menu'},{depart:''},{adopt:false}],
  [{health:'sick',sick_seconds:9},{},{depart:''},{adopt:false}],
  [{sleep:'normal'},{activity:'sleeping'},{depart:/睡覺/},{adopt:false}],
  [{},{activity:'device_game'},{depart:/陪你玩/},{adopt:false}],
  [{},{activity:'busy'},{depart:/主畫面/},{adopt:false}],
  [{},{activity:'web_game'},{depart:/主畫面/},{adopt:false}],
  [{},{activity:'ended'},{depart:/主畫面/},{adopt:false}],
  [{life_stage:'egg'},{},{depart:false},{adopt:false}],
  [dead,{activity:'ended'},{depart:false},{adopt:''}],
  [departed,{activity:'ended'},{depart:false},{adopt:''}],
  [departed,{activity:'busy'},{depart:false},{adopt:/結束畫面/}],
  [dead,{activity:'menu'},{depart:false},{adopt:/結束畫面/}]];
 for(const [pet,top,...wants] of cases){const b=connected(undefined,pet,top);
  for(const want of wants)for(const [kind,hint] of Object.entries(want)){const label=JSON.stringify([pet,top,kind]);
   if(hint===false){assert.equal(L(b).shown(kind),false,label);continue;}
   assert.equal(L(b).shown(kind),true,label);if(hint==='')assert.equal(L(b).blocker(kind),'',label);else assert.match(L(b).blocker(kind),hint,label);}}
 const b=connected();b.ble.disconnect();assert.equal(b.m.view('ble').lifecycle,false);assert.throws(()=>b.ble.lifecycle('depart'),/不能從網頁送別/);
});
test('depart reuses the care op path: same op on resend, matched by op, one write at a time',async()=>{
 const b=connected(),r=watch(b.m.farewell());await flush();
 const [x]=sentOf(b,'depart');assert.deepEqual(Object.keys(x).sort(),['cmd','id','op','pet_id','v']);assert.equal(x.pet_id,PET_ID);assert.match(x.op,/^[0-9a-f]{16}$/);
 assert.equal(b.m.view('ble').careSending,true);assert.match(L(b).blocker('depart'),/上一個/);assert.match(b.c.PetDeviceCare.blocker('feed'),/上一個/);
 assert.throws(()=>b.ble.care('feed'),/上一個/);
 ack(b,x);mock.timers.tick(10000);await flush();const y=sentOf(b,'depart')[1];assert.equal(y.op,x.op);assert.notEqual(y.id,x.id);
 result(b,y,{action:'depart',changes:{memorial_saved:true,bogus:1}});await flush();
 assert.equal(r.state,'done');assert.deepEqual(plain(r.value),{action:'depart',changes:{memorial_saved:true}});assert.equal(b.m.view('ble').careSending,false);
 mock.timers.tick(30000);await flush();assert.equal(sentOf(b,'depart').length,2,'no resend after the result');
});
test('adopt targets the ended pet and names the new egg from changes; 30 s without a result is unknown',async()=>{
 {const b=connected(undefined,departed,{activity:'ended'}),r=watch(L(b).run('adopt'));await flush();
  const [x]=sentOf(b,'adopt');assert.equal(x.pet_id,PET_ID);
  result(b,x,{action:'adopt',changes:{new_pet_id:'43',name:'Kororo',species_id:2}});await flush();
  assert.equal(r.state,'done');assert.deepEqual(plain(r.value.changes),{new_pet_id:'43',name:'Kororo',species_id:2});
  assert.equal(L(b).doneText(r.value),'領養了新蛋：Kororo。等牠在電子雞上孵化吧。');
  assert.equal(L(b).doneText({action:'adopt',changes:{name:'Pipi',species_id:9}}),'領養了新蛋：Pipi。等牠在電子雞上孵化吧。');}
 {const b=connected(undefined,dead,{activity:'ended'}),r=watch(L(b).run('adopt'));await flush();
  for(let i=0;i<3;i++){mock.timers.tick(10000);await flush();}assert.equal(r.state,'error');assert.equal(r.error.code,'timeout');assert.match(r.error.message,/領養結果/);
  const ops=sentOf(b,'adopt').map(x=>x.op);assert.equal(ops.length,3);assert.equal(new Set(ops).size,1);assert.equal(b.sent.at(-1).cmd,'get_status');}
});
test('refusals and failures map to Traditional Chinese, memorial_full points to deleting on the ESP32',async()=>{
 for(const [kind,pet,top] of [['depart',{},{}],['adopt',departed,{activity:'ended'}]])
  for(const code of ['memorial_full','busy','not_allowed','pet_changed','op_conflict','invalid_value']){
   const b=connected(undefined,pet,top),r=watch(L(b).run(kind));await flush();ack(b,sentOf(b,kind)[0],{ok:false,code});await flush();
   assert.equal(r.state,'error',kind+code);assert.equal(r.error.code,code);if(code==='memorial_full')assert.equal(r.error.message,FULL);else assert.doesNotMatch(r.error.message,/[a-z_]{6,}/,kind+code);
   assert.equal(b.m.view('ble').careSending,false);}
 const b=connected(undefined,departed,{activity:'ended'}),r=watch(L(b).run('adopt'));await flush();
 result(b,sentOf(b,'adopt')[0],{outcome:'failed',code:'memorial_full',changes:undefined});await flush();assert.equal(r.error.message,FULL);
 assert.match(L(b).errorText('depart',{code:'busy'}),/主畫面/);assert.match(L(b).errorText('adopt',{code:'busy'}),/結束畫面/);
 assert.match(L(b).errorText('depart',{code:'save_failed'}),/沒能保存/);
 assert.equal(L(b).doneText({action:'depart',changes:{memorial_saved:false}}),'牠已踏上旅程。電子雞還在保存紀念紀錄，稍後會自動完成。');
 assert.equal(L(b).doneText({action:'depart',changes:{memorial_saved:true}}),'牠已踏上旅程，紀念紀錄保存在電子雞上。');
});
test('pet_event departed / adopted from this page (source web) never reaches the page; device ones do',()=>{
 const b=connected(),seen=[];b.m.onPetEvent(e=>seen.push(e.event+':'+e.source));
 b.ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'departed',source:'web'});
 b.ble.messageReceived({v:1,type:'pet_event',pet_id:'43',event:'adopted',source:'web'});
 b.ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'departed',source:'device'});
 b.ble.messageReceived({v:1,type:'pet_event',pet_id:'43',event:'adopted',source:'device'});
 assert.deepEqual(seen,['departed:device','adopted:device']);
});

// The real page: the confirm dialog, then the snapshot-driven farewell, its photo, and adoption.
function lifecyclePage(){
 const p=page();const {c,get}=p;
 for(const file of ['device-lifecycle.js'])vm.runInContext(fs.readFileSync(__dirname+'/'+file,'utf8'),c,{filename:file});
 const ce=c.document.createElement;c.document.createElement=(...a)=>Object.assign(ce(...a),{toDataURL:()=>'data:image/png;base64,QQ=='});
 p.opened=[];for(const id of ['#farewell-dialog','#adopt-dialog']){const d=get(id);d.open=false;d.showModal=()=>{d.open=true;p.opened.push(id);};d.close=()=>{d.open=false;};}
 p.focused=[];get('#farewell-dialog form button').focus=()=>p.focused.push('留下');
 p.draws=[];const draw=c.PetFarewellView.draw;c.PetFarewellView.draw=(ctx,o)=>{p.draws.push(o.elapsed);return draw(ctx,o);};
 p.ble.messageReceived(status());p.step(16);return p;
}
test('device page: 送別 opens the confirm dialog first, defaulting to 留下, and sends only after 確認送別',async()=>{
 const html=fs.readFileSync(__dirname+'/index.html','utf8');
 assert.match(html,/<dialog id="farewell-dialog">.*送別後無法召回.*<button autofocus>留下<\/button>.*id="confirm-farewell"/,'留下 is the default choice');
 const p=lifecyclePage(),{get}=p;
 assert.equal(get('#device-lifecycle').hidden,false);assert.equal(get('#device-farewell').title,'');
 p.ble.messageReceived(status({},{activity:'busy'}));assert.match(get('#device-farewell').title,/主畫面/);
 get('#device-farewell').onclick();assert.match(get('#status').textContent,/主畫面/);assert.deepEqual(p.opened,[],'a blocked click only explains');
 p.ble.messageReceived(status({age_seconds:'3725'},{activity:'menu'}));get('#device-farewell').onclick();
 assert.deepEqual(p.opened,['#farewell-dialog']);assert.deepEqual(p.focused,['留下']);
 assert.equal(get('#farewell-description').textContent,'Tamama · 陪伴 1 小時 2 分 5 秒');
 await flush();assert.equal(sentOf(p,'depart').length,0,'nothing sent while the dialog is open');
 get('#farewell-dialog').close();await flush();assert.equal(sentOf(p,'depart').length,0,'留下 sends nothing');
 // Without the feature the device row stays hidden.
 const old=page(['web_game','pet_events','care']);vm.runInContext(fs.readFileSync(__dirname+'/device-lifecycle.js','utf8'),old.c);old.ble.messageReceived(status());assert.equal(old.get('#device-lifecycle').hidden,true);
});
test('device page: depart done, then the departed snapshot plays the farewell once and saves the photo on the device record',async()=>{
 const p=lifecyclePage(),{get,c}=p;get('#device-farewell').onclick();get('#confirm-farewell').onclick();await flush();
 const [x]=sentOf(p,'depart');assert.equal(x.pet_id,PET_ID);assert.equal(get('#farewell-dialog').open,false);assert.equal(get('#status').textContent,'正在請電子雞送牠出發……');
 ack(p,x);result(p,x,{action:'depart',changes:{memorial_saved:false}});await flush();
 assert.equal(get('#status').textContent,'牠已踏上旅程。電子雞還在保存紀念紀錄，稍後會自動完成。');assert.equal(p.draws.length,0,'done alone plays nothing');
 p.ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'departed',source:'web'});
 p.ble.messageReceived(status(departed,{activity:'busy'}));assert.equal(get('#device-lifecycle').hidden,true);
 assert.equal(get('#status').textContent,'牠已踏上旅程。電子雞還在保存紀念紀錄，稍後會自動完成。','the snapshot keeps the result readable');
 p.step(16);assert.ok(p.draws[0]>=0&&p.draws[0]<100,'the sequence starts from the viewfinder');
 for(let i=0;i<300;i++)p.step(16);
 // Later snapshots of the same departed pet (the device going to ended) never restart it.
 p.ble.messageReceived(status(departed,{activity:'ended'}));p.ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'departed',source:'web'});
 for(let i=0;i<200;i++)p.step(16);
 assert.ok(p.draws.every((t,i)=>!i||t>=p.draws[i-1]),'elapsed only grows: played once');assert.ok(p.draws.at(-1)>7000,'reached the postcard');
 const recs=c.PetMemorials.records().filter(r=>r.source==='ble'&&r.petId===PET_ID&&r.departed);
 assert.equal(recs.length,1);assert.equal(recs[0].deviceId,'esp32-AABBCCDDEEFF');assert.equal(recs[0].photo,'data:image/png;base64,QQ==');assert.equal(recs[0].photoKind,'scene');
 assert.equal(c.PetMemorials.savePhoto({petId:PET_ID,deviceId:'esp32-000000000000'},'data:image/png;base64,QQ=='),false,'another device keeps its own records');
});
test('device page: a device-side departure (source device) also plays from the snapshot, once',()=>{
 const p=lifecyclePage();p.ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'departed',source:'device'});p.step(16);assert.equal(p.draws.length,0,'the event alone plays nothing');
 p.ble.messageReceived(status(departed,{activity:'busy'}));for(let i=0;i<50;i++)p.step(16);p.ble.messageReceived(status(departed,{activity:'ended'}));for(let i=0;i<50;i++)p.step(16);
 assert.ok(p.draws[0]<100);assert.ok(p.draws.every((t,i)=>!i||t>=p.draws[i-1]));
 assert.equal(p.get('#status').textContent,'牠已踏上旅程。','the departed state replaces the previous line');
});
test('device page: an adoption on the ESP32 replaces the farewell line with the egg, keeping its species a surprise',()=>{
 const p=lifecyclePage(),{get}=p;p.ble.messageReceived(status(departed,{activity:'ended'}));for(let i=0;i<50;i++)p.step(16);assert.equal(get('#status').textContent,'牠已踏上旅程。');
 p.ble.messageReceived({v:1,type:'pet_event',pet_id:'43',event:'adopted',source:'device'});
 p.ble.messageReceived(status({id:'43',name:'Kororo',life_stage:'egg',species_id:2,age_seconds:'0'},{activity:'idle'}));p.step(16);
 assert.equal(get('#status').textContent,'Kororo 還在蛋裡，等牠在電子雞上孵化吧。');assert.equal(get('#pet-species').textContent,'種類：孵化後揭曉');
 assert.doesNotMatch(get('#status').textContent,/小飛龍|小鳥/);
});
test('device page: adopt confirms with device wording, shows the new egg from changes, then the egg snapshot takes over',async()=>{
 const p=lifecyclePage(),{get}=p;p.ble.messageReceived(status(departed,{activity:'busy'}));for(let i=0;i<500;i++)p.step(16);
 assert.equal(get('#afterlife-menu').hidden,false);assert.equal(get('#adopt-egg').disabled,false);assert.match(get('#adopt-egg').title,/結束畫面/);
 get('#adopt-egg').onclick();assert.deepEqual(p.opened,[],'blocked until the device shows its end screen');assert.match(get('#status').textContent,/結束畫面/);
 p.ble.messageReceived(status(departed,{activity:'ended'}));assert.equal(get('#adopt-egg').title,'');assert.match(get('#adoption-note').textContent,/由電子雞決定/);
 get('#adopt-egg').onclick();assert.deepEqual(p.opened,['#adopt-dialog']);assert.equal(get('#adopt-dialog h2').textContent,'領養新蛋？');assert.match(get('#adopt-dialog p').textContent,/Tamama.*由電子雞決定/);
 await flush();assert.equal(sentOf(p,'adopt').length,0,'nothing sent before 領養新蛋 is confirmed');
 get('#confirm-adopt').onclick();await flush();const [x]=sentOf(p,'adopt');assert.equal(x.pet_id,PET_ID);
 result(p,x,{action:'adopt',changes:{new_pet_id:'43',name:'Kororo',species_id:2}});await flush();
 assert.equal(get('#status').textContent,'領養了新蛋：Kororo。等牠在電子雞上孵化吧。');
 p.ble.messageReceived({v:1,type:'pet_event',pet_id:'43',event:'adopted',source:'web'});
 p.ble.messageReceived({...status({id:'43',name:'Kororo',life_stage:'egg',species_id:2,age_seconds:'0'},{activity:'idle'})});p.step(16);
 assert.equal(p.m.view('ble').pet.stage,'egg');assert.equal(get('#afterlife-menu').hidden,true);assert.equal(get('#device-lifecycle').hidden,true,'no farewell for an egg');
 assert.equal(get('#status').textContent,'領養了新蛋：Kororo。等牠在電子雞上孵化吧。');
 // memorial_full from the device.
 const q=lifecyclePage();q.ble.messageReceived(status(dead,{activity:'ended'}));q.step(16);q.get('#adopt-egg').onclick();q.get('#confirm-adopt').onclick();await flush();
 ack(q,sentOf(q,'adopt')[0],{ok:false,code:'memorial_full'});await flush();assert.equal(q.get('#status').textContent,FULL);
});
test('demo mode keeps its own farewell and adopt dialogs, never the device commands',()=>{
 const p=lifecyclePage(),{get}=p;get('#mode').value='mock';get('#mode-toggle').onclick?.();
 assert.equal(get('#device-lifecycle').hidden,true);
 const before=p.sent.length;get('#farewell').onclick();get('#confirm-farewell').onclick();assert.equal(p.sent.length,before);assert.equal(p.m.view('mock').pet.isDeparted,true);
});
