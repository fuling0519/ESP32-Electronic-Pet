/* Run: node --test web-preview/memorial-sync-tests.cjs. 養成同步 phase 4 (PROTOCOL.md 紀念冊, plan §8.3, D3/D4):
 * get_memorials paging, delete_memorial on the shared op path, the two-way reconciliation, the pending-delete queue and
 * the album UI, over a fake ESP32 album behind a fake GATT writer, fake timers and an in-memory LocalStorage. Harness
 * adapted from lifecycle-sync-tests.cjs. No radio or real ESP32. */
const {test:plainTest,mock}=require('node:test');
const assert=require('node:assert/strict');
const vm=require('node:vm'),fs=require('node:fs'),crypto=require('node:crypto');
const DEVICE='esp32-AABBCCDDEEFF',OTHER='esp32-000000000000',PET_ID='42';
const FILES=['local-store.js','pet-rules.js','pet-games.js','wyvern-sprites.js','wyvern-view.js','data-sources.js','ble-source.js','memorials.js','data-manager.js','memorial-sync.js'];
const wire=(petId,name,extra={})=>({pet_id:petId,name,species_id:1,age_seconds:'3600',kind:'departed',stage:'adult',protected:false,...extra});
const hashOf=items=>crypto.createHash('md5').update(JSON.stringify(items)).digest('hex').slice(0,8);
function boot(storage=new Map()){
 const elements=new Map();const element=()=>({textContent:'',value:'',children:[],replaceChildren(){this.children=[];},append(...items){this.children.push(...items);},appendChild(item){this.children.push(item);return item;},showModal(){this.open=true;this.shown=(this.shown||0)+1;},close(){this.open=false;},after(){},before(){},focus(){},setAttribute(){},prepend(...items){this.children.unshift(...items);},addEventListener(){}});
 const context=vm.createContext({console:{...console,info(){}},crypto:crypto.webcrypto,TextEncoder,TextDecoder,Uint8Array,Uint32Array,DataView,BigInt,Map,Set,JSON,Number,Error,
 setTimeout:(...a)=>setTimeout(...a),clearTimeout:t=>clearTimeout(t),
 document:{querySelector:s=>{if(!elements.has(s))elements.set(s,element());return elements.get(s);},createElement:element},
 localStorage:{getItem:k=>storage.get(k)??null,setItem:(k,v)=>storage.set(k,v),removeItem:k=>storage.delete(k)},
 location:{protocol:'https:'},isSecureContext:true,navigator:{}});context.window=context;
 Object.defineProperty(context,'Date',{get:()=>Date});
 for(const file of FILES)vm.runInContext(fs.readFileSync(__dirname+'/'+file,'utf8'),context,{filename:file});
 const b={c:context,m:context.petDataManager,ble:context.PetSources.ble,S:context.PetMemorialSync,elements,storage,sent:[],get:s=>context.document.querySelector(s)};
 // Fake ESP32 album: answers get_memorials pages and delete_memorial (per pet id: ok, not_found, not_allowed, busy, silent).
 b.device={items:[],deleteMode:{},onPage:null,pet:{},top:{}};
 b.hash=()=>hashOf(b.device.items);
 b.status=(pet=b.device.pet,top=b.device.top)=>({v:1,type:'status',device_id:DEVICE,activity:'idle',memorial_count:b.device.items.length,memorial_hash:b.hash(),...top,pet:{id:PET_ID,name:'Tamama',life_stage:'baby',satiety:60,mood:70,cleanliness:50,age_seconds:'3600',health:'healthy',is_dead:false,sleep:'awake',...pet}});
 const reply=msg=>setImmediate(()=>{if(b.ble.connection==='ready')b.ble.messageReceived({v:1,...msg});});
 const respond=x=>{
  if(x.cmd==='get_memorials'){b.device.onPage?.(x.offset);const items=b.device.items;reply({type:'command_result',id:x.id,ok:true,code:'ok'});
   reply({type:'memorials',id:x.id,hash:b.hash(),total:items.length,offset:x.offset,items:items.slice(x.offset,x.offset+5)});}
  if(x.cmd==='delete_memorial'){const mode=b.device.deleteMode[x.memorial_pet_id]||'ok';if(mode==='silent')return;
   // not_found: the record is already gone on the ESP32 (deleted there in the meantime).
   if(mode==='not_found'){b.device.items=b.device.items.filter(i=>i.pet_id!==x.memorial_pet_id);reply({type:'command_result',id:x.id,ok:false,code:mode});reply(b.status());return;}
   if(mode!=='ok'){reply({type:'command_result',id:x.id,ok:false,code:mode});return;}
   reply({type:'command_result',id:x.id,ok:true,code:'ok'});
   setImmediate(()=>{b.device.items=b.device.items.filter(i=>i.pet_id!==x.memorial_pet_id);reply({type:'action_result',id:x.id,op:x.op,action:'delete_memorial',outcome:'done',changes:{}});reply(b.status());});}
 };
 const parts=new Map();b.writer={async writeValueWithResponse(d){const id=d[0]|d[1]<<8,list=parts.get(id)||[];list[d[2]]=Array.from(d.slice(4));parts.set(id,list);if(list.filter(Boolean).length===d[3]){parts.delete(id);const x=JSON.parse(Buffer.from(list.flat()).toString('utf8'));b.sent.push(x);respond(x);}}};
 b.connect=(features=['pet_events','care','lifecycle','memorials'])=>{b.ble.writer=b.writer;b.ble.connection='waiting';
  b.ble.messageReceived({v:1,type:'device_info',id:1,protocol_version:1,device_id:DEVICE,firmware_version:'test',max_message_bytes:1024,features});b.ble.messageReceived(b.status());};
 return b;
}
const test=(name,fn)=>plainTest(name,async()=>{mock.timers.enable({apis:['setTimeout','Date'],now:Date.parse('2026-10-26T08:00:00Z')});try{await fn();}finally{mock.timers.reset();}});
const plain=x=>JSON.parse(JSON.stringify(x));
const flush=async(n=40)=>{for(let i=0;i<n;i++)await new Promise(r=>setImmediate(r));};
const sentOf=(b,cmd)=>b.sent.filter(x=>x.cmd===cmd);
const watch=p=>{const r={state:'pending'};p.then(v=>{r.state='done';r.value=v;},e=>{r.state='error';r.error=e;});return r;};
const records=b=>JSON.parse(b.storage.get('aviary.memorials')||'{"data":[]}').data;
const deviceRecords=(b,id=DEVICE)=>records(b).filter(r=>r.source==='ble'&&r.deviceId===id);
const syncState=b=>JSON.parse(b.storage.get('aviary.memorial-sync')).data;
const seed=(storage,list)=>storage.set('aviary.memorials',JSON.stringify({version:1,data:list}));
const webRecord=(petId,name,extra={})=>({key:'ble:'+DEVICE+':s'+Math.random().toString(36).slice(2)+':'+petId,source:'ble',deviceId:DEVICE,name,petId,ageSeconds:'100',stage:'adult',dead:false,departed:true,kind:'departed',speciesId:1,provisional:true,receivedAt:'2026-10-20T10:00:00.000Z',firstSeenAt:'2026-10-18T10:00:00.000Z',...extra});
const PHOTO='data:image/png;base64,QQ==';
// The album with its device garden open, so the UI re-renders like the real dialog.
function album(b,category='all'){b.get('#memorial-category').value=category;b.c.PetMemorials.open('ble:'+DEVICE);b.get('#cemetery-dialog').open=true;}
const cards=b=>b.get('#memorial-list').children.filter(c=>c.className?.startsWith?.('album-card'));
const cardOf=(b,name)=>cards(b).find(c=>c.children[1].children[0].textContent.startsWith(name+' '));
const texts=c=>c.children[1].children.map(x=>x.textContent);
const buttonOf=c=>c.children.find(x=>x.className==='album-delete');

test('get_memorials reads every page, restarts from offset 0 when the hash changes, and needs the memorials feature',async()=>{
 const b=boot();b.device.items=Array.from({length:7},(_,i)=>wire(String(i+1),'P'+(i+1),{kind:i%2?'resting':'departed',stage:i===6?'unknown':'baby',species_id:i===2?2:undefined}));b.connect();await flush();
 assert.equal(b.m.view('ble').memorialSync,true);
 assert.deepEqual(sentOf(b,'get_memorials').map(x=>x.offset),[0,5],'the first connection reads both pages');
 assert.deepEqual(Object.keys(sentOf(b,'get_memorials')[0]).sort(),['cmd','id','offset','v'],'read-only, no op');
 // Direct read: the album changes between page 1 and page 2, so the read starts over.
 b.sent.length=0;let changed=false;b.device.onPage=o=>{if(o===5&&!changed){changed=true;b.device.items.push(wire('8','P8'));}};
 const r=watch(b.m.deviceMemorials());await flush();
 assert.deepEqual(sentOf(b,'get_memorials').map(x=>x.offset),[0,5,0,5]);assert.equal(r.state,'done');
 assert.equal(r.value.hash,b.hash());assert.equal(r.value.items.length,8);
 assert.deepEqual(plain(r.value.items[2]),{petId:'3',name:'P3',speciesId:2,ageSeconds:'3600',kind:'departed',stage:'baby',protected:false});
 assert.equal(r.value.items[6].stage,'unknown');assert.equal(r.value.items[0].speciesId,1);
 // Malformed pages reject the whole read.
 const page=b.c.PetBle.mapMemorialPage;
 for(const bad of [{hash:'XYZ'},{total:-1},{offset:3},{items:[1,2,3,4,5,6].map(i=>wire(String(i),'x'))},{items:[wire('a','x')]},{items:[wire('1','')]},{items:[wire('1','x',{kind:'dead'})]},{items:[wire('1','x',{stage:'teen'})]},{items:[wire('1','x',{protected:'no'})]},{items:[wire('1','x',{species_id:0})]}])
  assert.throws(()=>page({type:'memorials',hash:'0000abcd',total:1,offset:0,items:[wire('1','x')],...bad},0),/格式不相容/,JSON.stringify(bad));
 const old=boot();old.connect(['pet_events','care','lifecycle']);await flush();
 assert.equal(old.m.view('ble').memorialSync,false);await assert.rejects(old.ble.getMemorials(),/不能同步紀念冊/);
 assert.throws(()=>old.ble.deleteMemorial('3'),/不能從網頁刪除紀念/);assert.equal(sentOf(old,'get_memorials').length,0,'old firmware is never asked');
});
test('delete_memorial reuses the write op path: memorial_pet_id without pet_id, the same op on resend',async()=>{
 const b=boot();b.device.items=[wire('3','Tamama')];b.connect();await flush();b.device.deleteMode['3']='silent';
 const r=watch(b.m.deleteDeviceMemorial('3'));await flush();
 const [x]=sentOf(b,'delete_memorial');assert.deepEqual(Object.keys(x).sort(),['cmd','id','memorial_pet_id','op','v']);assert.match(x.op,/^[0-9a-f]{16}$/);
 assert.equal(b.m.view('ble').careSending,true,'one write at a time, care waits');assert.throws(()=>b.ble.care('feed'),/上一個/);
 mock.timers.tick(10000);await flush();const y=sentOf(b,'delete_memorial')[1];assert.equal(y.op,x.op);assert.notEqual(y.id,x.id);
 b.ble.messageReceived({v:1,type:'action_result',id:y.id,op:x.op,action:'delete_memorial',outcome:'done',changes:{}});await flush();
 assert.equal(r.state,'done');assert.equal(r.value.action,'delete_memorial');
 b.device.deleteMode['3']='not_found';const nf=watch(b.m.deleteDeviceMemorial('3'));await flush();assert.equal(nf.state,'error');assert.equal(nf.error.code,'not_found');
 assert.throws(()=>b.ble.deleteMemorial('x3'),/格式錯誤/);
});
test('first sync: add device-only, update keeping photo and level, merge per-connection duplicates, ask before removing',async()=>{
 const storage=new Map();
 const mockRecord={key:'mock:展示花園:m1',source:'mock',deviceId:'展示花園',name:'Demo',petId:'m1',ageSeconds:'5',stage:'baby',dead:false,departed:true,kind:'departed',speciesId:1,receivedAt:'2026-10-01T00:00:00.000Z'};
 seed(storage,[mockRecord,
  webRecord('3','Tamama',{photo:PHOTO,photoKind:'scene',level:5}),webRecord('3','Tamama',{receivedAt:'2026-10-21T10:00:00.000Z',firstSeenAt:'2026-10-15T10:00:00.000Z'}),
  webRecord('4','Kiki'),webRecord('6','Old'),webRecord(PET_ID,'Tamama',{departed:false,kind:'observed'}),
  webRecord('9','Elsewhere',{deviceId:OTHER,key:'ble:'+OTHER+':x:9'})]);
 const b=boot(storage);b.device.items=[wire('3','Tamama',{age_seconds:'7200',stage:'baby'}),wire('5','Pipi',{kind:'resting',stage:'unknown',species_id:2}),wire('6','New')];
 b.connect();await flush();
 const mine=deviceRecords(b),by=name=>mine.filter(r=>r.name===name);
 // Both sides: one record, device fields from the ESP32, photo / first date / level kept.
 assert.equal(by('Tamama').filter(r=>r.departed).length,1,'duplicates merged');const t=by('Tamama').find(r=>r.departed);
 assert.equal(t.ageSeconds,'7200');assert.equal(t.stage,'baby');assert.equal(t.synced,true);assert.equal(t.photo,PHOTO);assert.equal(t.level,5);
 assert.equal(t.firstSeenAt,'2026-10-15T10:00:00.000Z');assert.equal(t.receivedAt,'2026-10-20T10:00:00.000Z','the earliest farewell this browser saw');
 // Device only: device fields, no Web extras.
 const p=by('Pipi')[0];assert.equal(p.dead,true);assert.equal(p.departed,false);assert.equal(p.speciesId,2);assert.equal(p.stage,'unknown');
 for(const k of ['photo','receivedAt','firstSeenAt','level'])assert.equal(p[k],undefined,k);
 // Reused id 6 with another name is another pet: New is added, Old waits for the summary with Kiki.
 assert.equal(by('New')[0].synced,true);assert.equal(by('Old')[0].synced,undefined);assert.equal(by('Kiki').length,1);
 const d=b.get('#memorial-sync-dialog');assert.equal(d.shown,1);assert.equal(b.get('#memorial-sync-title').textContent,'同步後會移除 2 筆');assert.match(b.get('#memorial-sync-text').textContent,/Kiki、Old/);
 assert.equal(syncState(b).devices[DEVICE].removalOk,false);assert.equal(syncState(b).devices[DEVICE].hash,b.hash());
 // The alive pet's first-seen record, the other device and the demo stay untouched.
 assert.equal(by('Tamama').filter(r=>r.kind==='observed').length,1);assert.equal(deviceRecords(b,OTHER).length,1);assert.deepEqual(records(b).find(r=>r.source==='mock'),mockRecord);
 b.get('#memorial-sync-confirm').onclick();
 assert.deepEqual(deviceRecords(b).filter(r=>r.dead||r.departed).map(r=>r.name).sort(),['New','Pipi','Tamama']);assert.equal(syncState(b).devices[DEVICE].removalOk,true);
 assert.deepEqual(records(b).find(r=>r.source==='mock'),mockRecord,'demo records never take part');
 // Album: device save order, unknown stage text, no date for the device-only one.
 album(b);assert.deepEqual(cards(b).map(c=>c.children[1].children[0].textContent.split(' ')[0]),['New','Pipi','Tamama'],'newest (last saved) first');
 assert.ok(texts(cardOf(b,'Pipi')).some(x=>/電子雞沒有記下成長階段/.test(x)));assert.ok(texts(cardOf(b,'Pipi')).includes('從電子雞同步，告別日期不明'));
 assert.match(b.get('#album-device-note').textContent,/雙向同步/);
});
test('先保留 keeps the records and asks again on the next connection; later removals need no question',async()=>{
 const storage=new Map();seed(storage,[webRecord('4','Kiki'),webRecord('3','Tamama')]);
 const b=boot(storage);b.device.items=[wire('3','Tamama')];b.connect();await flush();
 assert.equal(b.get('#memorial-sync-dialog').shown,1);b.get('#memorial-sync-keep').onclick();
 assert.equal(deviceRecords(b).filter(r=>r.name==='Kiki').length,1,'kept for now');
 album(b);assert.ok(buttonOf(cardOf(b,'Kiki')),'a kept web-only record can still be deleted locally');
 b.ble.disconnect();b.sent.length=0;b.connect();await flush();
 assert.equal(sentOf(b,'get_memorials').length,1,'the same hash is read again while the summary is unanswered');assert.equal(b.get('#memorial-sync-dialog').shown,2);
 b.get('#memorial-sync-confirm').onclick();assert.equal(deviceRecords(b).filter(r=>r.name==='Kiki').length,0);
 // Two-way from now on: deleted on the ESP32 -> deleted here without a question.
 b.device.items=[];b.sent.length=0;b.ble.messageReceived(b.status());await flush();
 assert.equal(sentOf(b,'get_memorials').length,1);assert.equal(deviceRecords(b).filter(r=>r.dead||r.departed).length,0);assert.equal(b.get('#memorial-sync-dialog').shown,2);
 album(b);assert.match(b.get('#album-device-note').textContent,/移除電子雞上已刪除的 1 筆（Tamama）/);
 // Same hash on the next connection: nothing is read.
 b.ble.disconnect();b.sent.length=0;b.connect();await flush();assert.equal(sentOf(b,'get_memorials').length,0);
});
test('the current ended pet is kept while the ESP32 has not saved its record yet, and cannot be deleted',async()=>{
 const storage=new Map();seed(storage,[webRecord('3','Tamama')]);const b=boot(storage);b.device.items=[wire('3','Tamama')];b.connect();await flush();
 b.device.pet={is_departed:true};b.device.top={activity:'busy'};b.ble.messageReceived(b.status());await flush();
 // app.js calls remember() for every ready snapshot; this harness has no app.js.
 const seen=()=>b.c.PetMemorials.remember(b.m.view('ble').pet,'ble','session-'+b.ble.session);seen();
 const cur=deviceRecords(b).find(r=>r.petId===PET_ID&&r.departed);assert.ok(cur,'remember() made the farewell record');
 b.device.items=[];b.ble.messageReceived(b.status());await flush();
 assert.ok(deviceRecords(b).find(r=>r.petId===PET_ID&&r.departed),'not removed: the ESP32 may still be saving it');
 album(b);const c=cardOf(b,'Tamama');assert.equal(buttonOf(c),undefined);assert.ok(texts(c).some(x=>/還沒把這筆存進紀念冊/.test(x)));
 // Saved (protected) on the ESP32: merged into one synced record that stays undeletable.
 b.device.items=[wire(PET_ID,'Tamama',{protected:true})];b.ble.messageReceived(b.status());await flush();
 const list=deviceRecords(b).filter(r=>r.petId===PET_ID&&r.departed);assert.equal(list.length,1);assert.equal(list[0].protected,true);
 // Reconnecting adds no per-connection copy of the synced record.
 b.ble.disconnect();b.connect();await flush();seen();assert.equal(deviceRecords(b).filter(r=>r.petId===PET_ID&&r.departed).length,1);
 album(b);const p=cardOf(b,'Tamama');assert.equal(buttonOf(p),undefined);assert.ok(texts(p).some(x=>/領養新蛋後才能刪除/.test(x)));
 assert.equal(b.S.request([list[0]]).connected,true);assert.equal(b.S.pending(DEVICE).length,0,'protected never queued');
});
function synced(items=[wire('3','Tamama'),wire('4','Kiki',{kind:'resting'}),wire('5','Pipi')]){const b=boot();b.device.items=items;b.connect();return b;}
const del=(b,name)=>{buttonOf(cardOf(b,name)).onclick();b.get('#album-delete-confirm').onclick();};
test('connected: delete confirms, waits the undo window, sends delete_memorial and then offers no undo',async()=>{
 const b=synced();await flush();album(b);
 buttonOf(cardOf(b,'Kiki')).onclick();assert.equal(b.get('#album-delete-text').textContent,'要刪除 Kiki 的紀念嗎？');assert.match(b.get('#album-delete-note').textContent,/電子雞上的這筆紀念也會一起刪除/);
 assert.equal(b.S.pending(DEVICE).length,0,'asking alone queues nothing');b.get('#album-delete-confirm').onclick();
 assert.match(b.get('#album-toast-text').textContent,/送出前可以復原/);assert.equal(b.get('#album-undo').hidden,false);
 // Undo inside the window: nothing is sent.
 b.get('#album-undo').onclick();assert.equal(b.S.pending(DEVICE).length,0);mock.timers.tick(7000);await flush();assert.equal(sentOf(b,'delete_memorial').length,0);
 assert.match(b.get('#album-toast-text').textContent,/已取消刪除 Kiki/);
 del(b,'Kiki');assert.equal(cardOf(b,'Kiki').className,'album-card dead pending');
 mock.timers.tick(6100);await flush();
 assert.deepEqual(sentOf(b,'delete_memorial').map(x=>x.memorial_pet_id),['4']);
 assert.equal(deviceRecords(b).filter(r=>r.name==='Kiki').length,0);assert.equal(b.S.pending(DEVICE).length,0);
 assert.equal(b.get('#album-toast-text').textContent,'已從電子雞刪除 Kiki 的紀念，無法復原。');assert.equal(b.get('#album-undo').hidden,true);
 assert.equal(sentOf(b,'get_memorials').length,2,'the list is read again after the deletion');assert.equal(syncState(b).devices[DEVICE].hash,b.hash());
 // The device's own pet_event for this deletion has source web and does nothing; one from the ESP32 re-reads.
 b.sent.length=0;b.ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'memorial_deleted',memorial_pet_id:'4',source:'web'});await flush();assert.equal(b.sent.length,0);
 b.ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'memorial_added',source:'device'});await flush();assert.equal(sentOf(b,'get_memorials').length,1);
});
test('disconnected: deletion is queued as 待同步, left out of counts, and sent after reconnecting',async()=>{
 const b=synced();await flush();b.ble.disconnect();album(b);
 assert.deepEqual(b.get('#album-tabs').children.map(x=>x.textContent),['全部','遠行 2','長眠 1']);
 buttonOf(cardOf(b,'Tamama')).onclick();assert.match(b.get('#album-delete-note').textContent,/待同步/);b.get('#album-delete-confirm').onclick();
 assert.match(b.get('#album-toast-text').textContent,/已標成待同步/);
 const c=cardOf(b,'Tamama');assert.equal(c.className,'album-card departed pending');assert.ok(texts(c).includes('待同步：連上電子雞後會自動刪除。'));assert.equal(buttonOf(c).textContent,'取消刪除');
 assert.deepEqual(b.get('#album-tabs').children.map(x=>x.textContent),['全部','遠行 1','長眠 1'],'待同步 is not counted');
 assert.match(b.get('#album-device-note').textContent,/1 筆刪除待同步/);
 // Survives a reload (LocalStorage, per device).
 const again=boot(b.storage);assert.deepEqual(again.S.pending(DEVICE).map(q=>[q.petId,q.name]),[['3','Tamama']]);
 again.device.items=b.device.items;again.connect();await flush();
 assert.deepEqual(sentOf(again,'delete_memorial').map(x=>x.memorial_pet_id),['3'],'sent automatically after reconnecting');
 assert.equal(again.S.pending(DEVICE).length,0);assert.equal(deviceRecords(again).filter(r=>r.name==='Tamama'&&r.departed).length,0);assert.equal(again.device.items.length,2);
});
test('queued deletions: not_found counts as done, protected is shown again, busy and disconnects keep the item',async()=>{
 const b=synced([wire('3','Tamama'),wire('4','Kiki'),wire('5','Pipi'),wire('7','Momo')]);await flush();b.ble.disconnect();album(b);
 for(const n of ['Tamama','Kiki','Pipi'])del(b,n);assert.equal(b.S.pending(DEVICE).length,3);
 // Queued for another device: never sent to this one.
 b.c.PetLocalStore.write('memorial-sync',{...syncState(b),queue:[...syncState(b).queue,{deviceId:OTHER,petId:'7',name:'Momo',speciesId:1,key:'x'}]});
 const r=boot(b.storage);r.device.items=b.device.items;Object.assign(r.device.deleteMode,{3:'not_found',4:'not_allowed',5:'busy'});r.connect();await flush();
 assert.deepEqual(sentOf(r,'delete_memorial').map(x=>x.memorial_pet_id),['3','4','5'],'busy stops the round');
 assert.equal(deviceRecords(r).filter(x=>x.name==='Tamama').length,0,'not_found = deleted');
 assert.deepEqual(r.S.pending(DEVICE).map(q=>q.name),['Pipi']);assert.deepEqual(r.S.pending(OTHER).map(q=>q.name),['Momo']);
 album(r);assert.equal(cardOf(r,'Kiki').className,'album-card departed','protected one is shown again');
 assert.match(r.get('#album-device-note').textContent,/電子雞正在忙/);
 // Busy is retried after the pause once the ESP32 can take it.
 r.device.deleteMode[5]='ok';mock.timers.tick(10100);await flush();
 assert.equal(r.S.pending(DEVICE).length,0);assert.equal(deviceRecords(r).filter(x=>x.name==='Pipi').length,0);
 // Activity that cannot delete: nothing is sent, the item waits.
 r.device.top={activity:'device_game'};r.ble.messageReceived(r.status());await flush();album(r);del(r,'Momo');mock.timers.tick(7000);await flush();
 assert.equal(sentOf(r,'delete_memorial').filter(x=>x.memorial_pet_id==='7').length,0);assert.match(r.get('#album-device-note').textContent,/回到主畫面後送出/);
 // A disconnect while sending keeps it too.
 r.device.top={};r.device.deleteMode[7]='silent';mock.timers.tick(10000);r.ble.messageReceived(r.status());await flush();
 assert.equal(sentOf(r,'delete_memorial').filter(x=>x.memorial_pet_id==='7').length,1);album(r);assert.equal(buttonOf(cardOf(r,'Momo')),undefined,'sending: no undo, no delete');
 r.get('#album-undo').onclick?.();r.ble.disconnect();mock.timers.tick(31000);await flush();
 assert.deepEqual(r.S.pending(DEVICE).map(q=>q.name),['Momo']);assert.equal(r.S.cancel([r.S.pending(DEVICE)[0].key]).cancelled.length,1,'not sending any more: can be cancelled');
});
test('a queued id that now names another pet (NVS rebuilt) is settled, never sent',async()=>{
 const b=synced([wire('3','Tamama')]);await flush();b.ble.disconnect();album(b);del(b,'Tamama');
 const r=boot(b.storage);r.device.items=[wire('3','Other')];r.connect();await flush();
 assert.equal(sentOf(r,'delete_memorial').length,0);assert.equal(r.S.pending(DEVICE).length,0);
 assert.deepEqual(deviceRecords(r).filter(x=>x.departed).map(x=>x.name),['Other']);
});
test('reconcile is pure and leaves demo and other devices alone',()=>{
 const b=boot(),R=b.c.PetMemorialSync.reconcile;
 const recs=[{key:'mock:展示花園:1',source:'mock',deviceId:'展示花園',name:'Tamama',petId:'3',ageSeconds:'1',stage:'adult',dead:true,departed:false,kind:'dead',speciesId:1},webRecord('3','Tamama'),webRecord('3','Tamama',{deviceId:OTHER,key:'o'})];
 const before=plain(recs),res=R(recs,DEVICE,[],null,true);
 assert.deepEqual(plain(recs),before);assert.deepEqual(plain(res.next.map(r=>r.key)),['mock:展示花園:1','o']);assert.deepEqual(plain(res.removed.map(r=>r.name)),['Tamama']);
 const dup=R([webRecord('3','Tamama'),webRecord('3','Tamama'),webRecord('3','Tamama',{speciesId:2})],DEVICE,[{petId:'3',name:'Tamama',speciesId:1,ageSeconds:'9',kind:'departed',stage:'adult',protected:false}],null,false);
 assert.deepEqual(plain(dup.merged),['Tamama']);assert.equal(dup.next.filter(r=>r.synced).length,1);assert.equal(dup.candidates.length,1,'other species, same id and name: another pet');
});
