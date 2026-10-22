/* Run: node web-preview/offline-tests.cjs. No radio, browser permissions or network. */
const {test}=require('node:test');
const assert=require('node:assert/strict');
const vm=require('node:vm'),fs=require('node:fs'),crypto=require('node:crypto');
function boot(storage=new Map()){
 const elements=new Map();const element=()=>({textContent:'',value:'',children:[],replaceChildren(){this.children=[];},append(...items){this.children.push(...items);},appendChild(item){this.children.push(item);return item;},showModal(){},after(){},before(){},focus(){},setAttribute(){},prepend(...items){this.children.unshift(...items);},addEventListener(){}});
 const context=vm.createContext({console,crypto:crypto.webcrypto,TextEncoder,TextDecoder,Uint8Array,Uint32Array,DataView,Date,BigInt,Map,Set,JSON,Number,Error,setTimeout,clearTimeout,
 document:{querySelector:s=>{if(!elements.has(s))elements.set(s,element());return elements.get(s);},createElement:element},
 localStorage:{getItem:k=>storage.get(k)??null,setItem:(k,v)=>storage.set(k,v),removeItem:k=>storage.delete(k)},
 location:{protocol:'https:'},isSecureContext:true,navigator:{}});context.window=context;
 for(const file of ['local-store.js','pet-rules.js','pet-games.js','wyvern-sprites.js','wyvern-view.js','data-sources.js','ble-source.js','memorials.js','data-manager.js'])vm.runInContext(fs.readFileSync(__dirname+'/'+file,'utf8'),context,{filename:file});
 return {c:context,m:context.petDataManager,storage,elements};
}
const status=(overrides={})=>({v:1,type:'status',device_id:'esp32-AABBCCDDEEFF',pet:{id:'18446744073709551615',name:'Tamama',life_stage:'baby',satiety:80,mood:70,cleanliness:95,age_seconds:'18446744073709551615',health:'healthy',is_dead:false,sleep:'awake',...overrides}});

test('memorial postcard keeps a terminal pet photo across reload without changing identity or accepting external URLs',()=>{
 const {c,m,storage}=boot();m.preview('adult','awake');m.farewell();const pet=m.read().pet;
 const before=JSON.parse(storage.get('aviary.memorials')).data.find(r=>r.petId===pet.petId);
 assert.equal(c.PetMemorials.savePhoto(pet,'https://example.com/photo.png'),false);
 assert.equal(c.PetMemorials.savePhoto({...pet,petId:'other'},'data:image/png;base64,YQ=='),false);
 assert.equal(c.PetMemorials.savePhoto(pet,'data:image/png;base64,'+'A'.repeat(400000)),false);
 assert.equal(c.PetMemorials.savePhoto(pet,'data:image/png;base64,YQ=='),true);
 const after=JSON.parse(storage.get('aviary.memorials')).data.find(r=>r.petId===pet.petId);
 assert.deepEqual({...after,photo:undefined},{...before,photo:undefined});
 const resumed=boot(storage);resumed.elements.get('#memorial-category').value='departed';
 resumed.c.PetMemorials.open('mock:展示花園');
 const card=resumed.elements.get('#memorial-list').children[0];
 assert.equal(card.className,'memorial-card postcard-card');assert.equal(card.children[0].textContent,'遠行');
 assert.equal(card.children[1].children[0].src,'data:image/png;base64,YQ==');
 assert(!card.children.some(child=>child.className==='mini-stone'));
 card.children.at(-1).onclick();
 assert.equal(JSON.parse(storage.get('aviary.memorials')).data.find(r=>r.petId===pet.petId).hiddenFromAlbum,true);
 const hiddenReload=boot(storage);hiddenReload.elements.get('#memorial-category').value='departed';hiddenReload.c.PetMemorials.open('mock:展示花園');
 const undo=hiddenReload.elements.get('#memorial-list').children.find(child=>child.textContent.startsWith('復原明信片：'));
 assert(undo);undo.onclick();
 assert.equal(JSON.parse(storage.get('aviary.memorials')).data.find(r=>r.petId===pet.petId).hiddenFromAlbum,false);
 assert.equal(JSON.parse(storage.get('aviary.memorials')).data.find(r=>r.petId===pet.petId).photo,'data:image/png;base64,YQ==');
});
test('complete demo lifecycle reloads consistently from egg through farewell and new identity',()=>{
 const storage=new Map(),m=boot(storage).m;m.preview('adult','dead');m.adopt();const egg=m.read().pet,id=egg.petId,species=egg.speciesId;
 for(let i=0;i<6;i++)m.tick(50);assert.equal(m.read().pet.stage,'baby');assert.equal(m.read().pet.petId,id);
 let resumed=boot(storage).m;assert.equal(resumed.read().pet.stage,'baby');assert.equal(resumed.read().pet.speciesId,species);
 resumed.tick(3600);assert.equal(resumed.read().pet.stage,'adult');resumed.command('feed');resumed.command('clean');
 const before=resumed.read().pet.exp;resumed.startGame('rps',0);resumed.cancelGame();assert.equal(resumed.read().pet.exp,before);
 resumed.farewell();assert.equal(resumed.read().pet.isDeparted,true);resumed=boot(storage).m;assert.equal(resumed.read().pet.isDeparted,true);assert.throws(()=>resumed.command('feed'));
 resumed.adopt();assert.notEqual(resumed.read().pet.petId,id);assert.equal(resumed.read().pet.stage,'egg');
});

test('long sleep retains its original gravestone while travel retains its postcard',()=>{
 const {c,m,elements}=boot();m.preview('adult','dead');c.PetMemorials.remember(m.read().pet,'mock','test');
 elements.get('#memorial-category').value='dead';c.PetMemorials.open('mock:展示花園');
 const card=elements.get('#memorial-list').children.find(child=>child.className==='memorial-card');
 assert(card);assert.equal(card.children[0].className,'mini-stone');assert.equal(card.children[0].textContent,'長眠');
 assert(!card.children.some(child=>child.className==='postcard-photo'||child.className==='postcard-caption'));
});
test('pet and poke change mood only, share cooldown, persist and never write BLE',()=>{
 const {m,c,storage}=boot();let now=10000;c.Date=class extends Date{static now(){return now;}};const before=m.read().pet;
 assert.equal(m.command('pet').actual,3);assert.equal(m.read().pet.mood,before.mood+3);assert.equal(m.read().pet.exp,before.exp);assert.throws(()=>m.command('poke'),/3秒/);
 now+=3000;assert.equal(m.command('poke').actual,-3);assert.equal(boot(storage).m.read().pet.mood,before.mood);
 c.PetSources.mock.pet.mood=100;now+=3000;assert.equal(m.command('pet').actual,0);c.PetSources.mock.pet.mood=1;now+=3000;assert.equal(m.command('poke').actual,-1);assert.equal(m.read().pet.mood,0);
 m.select('ble');assert.throws(()=>m.command('pet'),/唯讀/);
});
test('touch rejects egg, sleep, sickness, games and terminal pets',()=>{
 const {m}=boot();m.startGame('rps',0);assert.throws(()=>m.command('pet'),/遊戲/);m.cancelGame();m.preview('adult','sleep');assert.throws(()=>m.command('poke'));m.preview('adult','sick');assert.throws(()=>m.command('pet'));m.preview('adult','dead');assert.throws(()=>m.command('poke'));m.adopt();assert.throws(()=>m.command('pet'));
});
test('RPS countdown exposes 3 2 1 only, memory prepares five seconds and uses 2 3 5 notes',()=>{
 const {m,c}=boot();m.startGame('rps',0);m.answerGame(0,0);for(const [time,n] of [[0,3],[349,3],[350,2],[700,1]]){const s=m.gameState(time);assert.equal(s.countdown,n);assert.equal(s.opponent,undefined);}assert.equal(m.gameState(1050).phase,'round-result');m.cancelGame();m.startGame('memory',0);assert.equal(m.gameState(4999).phase,'prepare');assert.equal(m.gameState(4999).lit,undefined);assert.equal(m.gameState(5000).phase,'show');assert.equal(c.PetSources.mock.game.sequence.length,2);const g=c.PetSources.mock.game;g.round=2;g.next(6000);assert.equal(g.sequence.length,3);g.round=3;g.next(7000);assert.equal(g.sequence.length,5);
});
test('legacy demo migrates, canonical state survives reload, reads cannot mutate adapters',()=>{
 const storage=new Map([['aviary.demo',JSON.stringify({version:1,data:{petId:'old',name:'Tamama',stage:'baby',condition:'sick',ageSeconds:300,satiety:80,mood:70,cleanliness:95}})]]);
 const {m}=boot(storage);assert.equal(m.read().pet.healthState,'sick');assert.equal(m.read().pet.condition,undefined);m.command('treat');m.tick(1);
 const saved=JSON.parse(storage.get('aviary.demo')).data;assert.equal(saved.healthState,'healthy');assert.equal(saved.sleepMode,'awake');assert.equal(saved.condition,undefined);
 const next=boot(storage).m;assert.equal(next.read().pet.petId,'old');const p=next.read().pet;p.satiety=0;assert.equal(next.read().pet.satiety,80);
});
test('care caps, treatment, death lock, memorial and adoption persist distinct identities',()=>{
 const {m,storage}=boot();m.command('feed');m.command('feed');m.command('feed');assert.equal(m.read().pet.satiety,100);m.command('clean');assert.equal(m.read().pet.cleanliness,75);assert.throws(()=>m.command('play'));assert.equal(m.read().pet.mood,55);
 m.preview('adult','sick');assert.throws(()=>m.command('play'));m.command('treat');assert.equal(m.read().pet.healthState,'healthy');assert.equal(m.read().pet.satiety,100);
 const old=m.read().pet.petId;m.preview('adult','dead');assert.throws(()=>m.command('feed'));m.tick(100);const age=m.read().pet.ageSeconds;m.tick(100);assert.equal(m.read().pet.ageSeconds,age);m.preview('adult','awake');assert.equal(m.read().pet.healthState,'dead');m.adopt();assert.notEqual(m.read().pet.petId,old);assert.match(m.read().pet.name,/^[A-Z][a-z]{5}$/);assert.equal(m.read().pet.stage,'egg');assert.equal(boot(storage).m.read().pet.petId,m.read().pet.petId);assert.ok(JSON.parse(storage.get('aviary.memorials')).data.some(p=>p.petId===old&&p.dead));
});
test('egg and baby timing, no reverse or skipped stage, sleep recovery and decay',()=>{
 const {m}=boot();m.preview('adult','dead');m.adopt();m.preview('adult','awake');assert.equal(m.read().pet.stage,'egg');m.tick(299);assert.equal(m.read().pet.stage,'egg');m.tick(1);assert.equal(m.read().pet.stage,'baby');m.tick(60);assert.equal(m.read().pet.satiety,79);m.preview('egg','awake');assert.equal(m.read().pet.stage,'baby');m.preview('baby','sleep');const before=m.read().pet;m.tick(600);const after=m.read().pet;assert.equal(after.mood,before.mood);assert.equal(after.satiety,before.satiety-10);assert.equal(after.sleepMode,'normal');
});
test('six-hour exposure and 24-hour sickness stop age exactly at death; partition invariant',()=>{
 function setup(){const b=boot();const p=b.c.PetSources.mock.pet;p.satiety=0;p.cleanliness=0;return b;}
 const a=setup(),b=setup();a.m.tick(21600);assert.equal(a.m.read().pet.healthState,'sick');a.m.tick(86410);b.m.tick(10000);b.m.tick(98010);assert.equal(a.m.read().pet.healthState,'dead');assert.equal(a.m.read().pet.ageSeconds,3600+21600+86400);assert.equal(a.m.read().pet.ageSeconds,b.m.read().pet.ageSeconds);
});
test('BLE maps canonical death and 64-bit strings; invalid complete snapshots rejected',()=>{
 const {c}=boot(),map=c.PetBle.mapStatus;const p=map(status({health:'dead',is_dead:true}));assert.equal(p.healthState,'dead');assert.equal(p.condition,undefined);assert.equal(p.petId,'18446744073709551615');assert.equal(p.deviceId,'esp32-AABBCCDDEEFF');
 for(const patch of [{health:'unknown'},{life_stage:'CHILD'},{satiety:101},{is_dead:true},{sleep:'deep'}])assert.throws(()=>map(status(patch)));
 assert.throws(()=>map({...status(),v:2}));assert.throws(()=>map({...status(),device_id:'esp32-pet-AABBCCDDEEFF'}));assert.equal(map({...status(),device_id:undefined}).deviceId,null);
});
test('BLE cache reload remains stale, new mapper ignores obsolete cached pet projection',()=>{
 const {c,m,storage}=boot();c.PetSources.ble.messageReceived(status());assert.equal(m.read('ble').metadata.stale,false);const next=boot(storage).m.read('ble');assert.equal(next.metadata.stale,true);assert.equal(next.metadata.connection,'disconnected');assert.ok(next.metadata.receivedAt);assert.equal(next.pet.healthState,'healthy');
});
test('BLE disallows care query and manager command; unsupported version halts pending queries',async()=>{
 const {c,m}=boot();m.select('ble');assert.throws(()=>m.command('feed'));await assert.rejects(c.PetSources.ble.query('feed'));const ble=c.PetSources.ble;let rejected=false;ble.pending.set(1,{timer:null,reject:()=>{rejected=true;}});assert.throws(()=>ble.messageReceived({v:2,type:'status'}));assert.equal(rejected,true);assert.equal(ble.pending.size,0);assert.equal(ble.connection,'disconnected');
});
test('corrupt demo is preserved and failed memorial persistence blocks adoption',()=>{
 const storage=new Map([['aviary.demo','broken']]);const {c,m}=boot(storage);m.command('feed');assert.equal(storage.get('aviary.demo'),'broken');m.preview('adult','dead');c.localStorage.setItem=()=>{throw new Error('quota');};const id=m.read().pet.petId;assert.throws(()=>m.adopt());assert.equal(m.read().pet.petId,id);
});
test('fragmented UTF-8 snapshot is reassembled, malformed data never replaces cache',()=>{
 const {c,m}=boot(),ble=c.PetSources.ble;const wire=status({name:'山雀'}),bytes=new TextEncoder().encode(JSON.stringify(wire)),count=Math.ceil(bytes.length/16);
 for(let i=0;i<count;i++){const fragment=new Uint8Array(4+Math.min(16,bytes.length-i*16));fragment.set([1,0,i,count]);fragment.set(bytes.slice(i*16,i*16+16),4);ble.receive(new DataView(fragment.buffer));}
 assert.equal(m.read('ble').pet.name,'山雀');ble.receive(new DataView(new Uint8Array([2,0,1,2,1]).buffer));assert.equal(m.read('ble').pet.name,'山雀');assert.equal(ble.partial,null);
});
test('main page loads the reviewed cleaning player before the application',()=>{
 const html=fs.readFileSync(__dirname+'/index.html','utf8'),app=fs.readFileSync(__dirname+'/app.js','utf8');
 const data=html.indexOf('assets/shared/cleaning/animation-v1/animation-data.js'),foam=html.indexOf('assets/shared/cleaning/foam-study-v2/foam-data.js'),view=html.indexOf('cleaning-animation-view.js'),main=html.lastIndexOf('app.js');
 assert.ok(data>=0&&data<foam&&foam<view&&view<main,'cleaning data and player load before the main app');
 assert.match(app,/window\.CleaningAnimationView/);assert.match(app,/view\.draw\(ctx,elapsed/);assert.match(app,/action==='clean'\?cleanDuration\(\):3\.4/);
});
test('UI boots and care, death/adoption, source switch run through DataManager',()=>{
 const {c,m,elements}=boot();const get=s=>c.document.querySelector(s);const buttons=['feed','clean','play','treat'].map(kind=>({dataset:{action:kind},addEventListener(type,fn){this.click=fn;}}));
 const artCalls=[];const drawing=new Proxy({drawImage(im){artCalls.push(im.src);},createRadialGradient:()=>({addColorStop(){}}),createLinearGradient:()=>({addColorStop(){}})},{get:(o,k)=>o[k]||(()=>{})});
 const decorate=el=>Object.assign(el,{dataset:{},style:{},classList:{add(){},remove(){},toggle(){}},setAttribute(){},removeAttribute(){},focus(){},close(){},getContext:()=>drawing,clientWidth:384});
 for(const id of fs.readFileSync(__dirname+'/index.html','utf8').matchAll(/id="([^"]+)"/g))decorate(get('#'+id[1]));get('#aviary').querySelector=()=>decorate({});
 get('#stage').options=['egg','baby','adult'].map(value=>({value}));c.document.querySelectorAll=()=>buttons;c.document.addEventListener=()=>{};
 c.Image=class{constructor(){this.complete=true;this.naturalWidth=2172;this.naturalHeight=724;}set src(value){this._src=value;const file=__dirname+"/"+value;if(fs.existsSync(file)){const png=fs.readFileSync(file);this.naturalWidth=png.readUInt32BE(16);this.naturalHeight=png.readUInt32BE(20);}}get src(){return this._src;}};c.ResizeObserver=class{observe(){}};c.matchMedia=()=>({matches:false});c.devicePixelRatio=1;c.performance={now:()=>0};c.__intervals=[];c.setInterval=fn=>{c.__intervals.push(fn);return c.__intervals.length;};c.clearInterval=()=>{};c.document.dispatchEvent=()=>{};let frame;c.requestAnimationFrame=fn=>{frame=fn;};c.checkBleReadiness=()=>{};const createElement=c.document.createElement;c.document.createElement=()=>decorate(createElement());for(const file of ['wyvern-adult-view.js','shared-sprites.js','shared-view.js','assets/pets/bird/white-idle-data-v4.js','assets/pets/bird/white-sad-data-v1.js','assets/pets/bird/white-sick-data-v1.js','white-bird-view.js','bird-view.js','assets/pets/bird/white-activity-data-v1.js','white-bird-activity.js','bird-demo.js','assets/items/blue-seed-bowl-data-v1.js','blue-seed-bowl.js','white-bird-feed.js','growth-view.js','egg-view.js','sad-view.js','game-keys.js'])vm.runInContext(fs.readFileSync(__dirname+'/'+file,'utf8'),c,{filename:file});
 vm.runInContext(fs.readFileSync(__dirname+'/game-ui.js','utf8'),c);vm.runInContext(fs.readFileSync(__dirname+'/app.js','utf8'),c);assert.equal(get('#satiety-value').textContent,60);get('#night').onclick();assert.ok(get('#background').src.includes('garden-detailed-night.png'));get('#day').onclick();assert.ok(get('#background').src.includes('garden-detailed-day.png'));get('#species-preview').value='2';get('#species-preview').onchange();frame(0);assert.ok(artCalls.some(src=>src.endsWith("idle-subtle-sheet-v2.png")),"adult uses approved illustrated idle");assert.equal(m.read().pet.speciesId,1);assert.ok(get('#pet-species').textContent.includes('小飛龍'));buttons[0].click();frame(3000);assert.equal(m.read().pet.satiety,80);assert.equal(get('#satiety-value').textContent,80);c.openPetGames();get('#start-rps').onclick();assert.equal(get('#game-inputs').children.length,3);m.cancelGame();
 get('#condition').value='sick';get('#condition').onchange();buttons[3].click();frame(1199);assert.ok(artCalls.at(-1).endsWith("emotions-sheet-v1.png")||artCalls.some(src=>src.endsWith("emotions-sheet-v1.png")));assert.equal(m.read().pet.healthState,'sick');frame(1200);assert.equal(m.read().pet.healthState,'healthy');const exp=m.read().pet.exp;frame(3500);assert.equal(m.read().pet.exp,exp);assert.equal(exp,8);
 get('#species-preview').value='1';get('#species-preview').onchange();get('#condition').value='awake';get('#condition').onchange();buttons[0].click();artCalls.length=0;frame(360);assert.ok(artCalls.some(src=>src.endsWith('white-activity-sheet-v1.png')),'bird feeds with whole bowing pose');assert.ok(artCalls.some(src=>src.endsWith('blue-seed-bowl-sheet-v1.png')),'bird feeds with blue seed bowl');frame(3000);assert.equal(m.read().pet.satiety,100);get('#species-preview').value='2';get('#species-preview').onchange();get('#condition').value='dead';get('#condition').onchange();frame(4000);assert.ok(artCalls.some(src=>src.endsWith("dead-still-v1.png")),"death uses side-lying illustration");frame(6000);assert.equal(m.read().pet.healthState,'dead');get('#confirm-adopt').onclick();assert.equal(m.read().pet.stage,'egg');get('#mode').value='ble';get('#mode').onchange();assert.equal(get('#satiety-value').textContent,'—');assert.ok(buttons.every(b=>b.disabled));get('#mode').value='mock';get('#mode').onchange();assert.equal(m.read().pet.stage,'egg');m.preview('baby','awake');m.preview('adult','awake');get('#stage').value='adult';get('#condition').value='awake';get('#condition').onchange();get('#farewell').onclick();get('#confirm-farewell').onclick();assert.equal(get('#health-label').textContent,'已遠行');frame(7000);assert.equal(m.read().pet.isDeparted,true);m.adopt();c.performance.now=()=>10000;c.__intervals[0]();assert.equal(m.read().pet.stage,'baby');assert.equal(m.read().pet.ageSeconds,300);frame(10000);
});
test('sleep never cures sickness, delayed growth requires treatment, reload preserves both states',()=>{
 const {m,storage,c}=boot();m.preview('adult','dead');m.adopt();m.tick(300);m.preview('baby','sick');m.preview('baby','sleep');m.tick(3600);assert.equal(m.read().pet.stage,'baby');assert.equal(m.read().pet.healthState,'sick');assert.equal(m.read().pet.sleepMode,'normal');assert.throws(()=>m.command('treat'));
 const next=boot(storage).m;assert.equal(next.read().pet.healthState,'sick');assert.equal(next.read().pet.sleepMode,'normal');next.preview('baby','awake');assert.equal(next.read().pet.healthState,'sick');next.command('treat');assert.equal(next.read().pet.stage,'adult');assert.equal(next.read().pet.healthState,'healthy');
 c.PetSources.mock.pet.timers.sick=86399;m.tick(10);assert.equal(m.read().pet.healthState,'dead');assert.equal(m.read().pet.ageSeconds,3901);
});
test('newly zero need starts exposure on following second; recovery clears continuous danger immediately',()=>{
 const {m,c}=boot();const p=c.PetSources.mock.pet;p.satiety=1;p.cleanliness=100;m.tick(600);assert.equal(m.read().pet.satiety,0);assert.equal(m.read().pet.timers.danger,0);m.tick(1);assert.equal(m.read().pet.timers.danger,1);m.command('feed');assert.equal(m.read().pet.timers.danger,0);
});
test('actual transport serializes only allowed queries; mismatched device IDs cannot replace state',async()=>{
 const {c}=boot(),ble=c.PetSources.ble,outgoing=[];let fragments=[];
 ble.writer={async writeValueWithResponse(data){assert.ok(data.length<=20);fragments.push(...data.slice(4));if(data[2]===data[3]-1){const command=JSON.parse(new TextDecoder().decode(new Uint8Array(fragments)));fragments=[];outgoing.push(command.cmd);ble.messageReceived(command.cmd==='get_device_info'?{v:1,type:'device_info',protocol_version:1,device_id:'esp32-AABBCCDDEEFF',firmware_version:'test',max_message_bytes:1024}:status());}}};
 await ble.query('get_device_info');await ble.query('get_status');assert.deepEqual(outgoing,['get_device_info','get_status']);await assert.rejects(ble.query('reset'));assert.equal(outgoing.length,2);
 assert.throws(()=>ble.messageReceived({...status(),device_id:'esp32-001122334455'}));assert.equal(ble.read().pet.deviceId,'esp32-AABBCCDDEEFF');
});
test('invalid canonical fields and corrupt BLE cache are preserved without fallback guesses',()=>{
 const storage=new Map([['aviary.demo',JSON.stringify({version:1,data:{petId:'old',name:'Tamama',stage:'baby',healthState:'unknown',sleepMode:'awake',ageSeconds:300,satiety:80,mood:70,cleanliness:95}})],['aviary.device-snapshot','corrupt']]);const original=storage.get('aviary.demo');const {m}=boot(storage);m.tick(1);assert.equal(storage.get('aviary.demo'),original);assert.equal(m.read('ble').pet,null);assert.equal(storage.get('aviary.device-snapshot'),'corrupt');
});
test('10/9 species defaults, unknown IDs, departed state and optional EXP contract',()=>{
 const {c}=boot(),map=c.PetBle.mapStatus;
 assert.equal(map(status()).speciesId,1);assert.equal(map(status({species_id:2})).speciesId,2);assert.equal(map(status({species_id:3})).speciesId,3);
 for(const species_id of [null,'2',-1,2.5])assert.throws(()=>map(status({species_id})));
 assert.equal(map(status({species_id:2,is_departed:true})).isDeparted,true);
 for(const patch of [{is_departed:'true'},{is_departed:true,is_dead:true,health:'dead'},{is_departed:true,life_stage:'egg'},{is_departed:true,sleep:'normal'},{level:1},{level:1,exp:50,exp_to_next_level:50},{level:20,exp:1,exp_to_next_level:0}])assert.throws(()=>map(status(patch)));
 assert.equal(map(status({level:20,exp:0,exp_to_next_level:0})).level,20);
});
test('adoption preserves the same pending UUID/name/species through storage failure and reload',()=>{
 const {c,m,storage}=boot();m.preview('adult','dead');let randomCalls=0;c.crypto={randomUUID:crypto.randomUUID,getRandomValues:a=>{randomCalls++;a.fill(1);return a;}};
 const write=c.localStorage.setItem;let fail=true;c.localStorage.setItem=(k,v)=>{if(k==='aviary.demo'&&fail)throw new Error('quota');write(k,v);};
 assert.throws(()=>m.adopt());const pending=JSON.stringify(c.PetSources.mock.pendingAdoption),calls=randomCalls;fail=false;m.adopt();assert.equal(JSON.stringify(c.PetSources.mock.pet),pending);assert.equal(randomCalls,calls);assert.equal(m.read().pet.speciesId,2);assert.equal(boot(storage).m.read().pet.speciesId,2);
 m.tick(300);m.preview('baby','sleep');m.tick(1);assert.equal(m.read().pet.speciesId,2);
});
test('both species share care EXP, level carry and cap, farewell lock and own memorial species',()=>{
 for(const speciesId of [1,2]){
  const {c,m,storage}=boot(),p=c.PetSources.mock.pet;p.speciesId=speciesId;p.exp=49;m.command('feed');assert.equal(m.read().pet.level,2);assert.equal(m.read().pet.exp,2);p.level=20;p.exp=0;p.satiety=100;m.command('feed');assert.equal(p.exp,0);
  m.farewell();assert.equal(m.read().pet.isDeparted,true);const age=p.ageSeconds;m.tick(100);assert.equal(m.read().pet.ageSeconds,age);assert.throws(()=>m.command('clean'));assert.throws(()=>m.startGame('rps',0));
  const record=JSON.parse(storage.get('aviary.memorials')).data.find(r=>r.petId===p.petId);assert.equal(record.speciesId,speciesId);assert.equal(record.departed,true);assert.equal(boot(storage).m.read().pet.isDeparted,true);m.adopt();assert.notEqual(m.read().pet.petId,p.petId);
 }
});
test('10/9 mood multipliers, weighted sleep and same-second ordering',()=>{
 for(const [satiety,cleanliness,loss] of [[100,100,1],[100,40,2],[40,40,3]]){const {c,m}=boot();Object.assign(c.PetSources.mock.pet,{satiety,cleanliness,mood:80});m.tick(1200);assert.equal(m.read().pet.mood,80-loss);}
 for(const [satiety,cleanliness,seconds,gain] of [[100,100,1200,1],[100,40,2400,1],[40,40,2400,0]]){const {c,m}=boot();Object.assign(c.PetSources.mock.pet,{satiety,cleanliness,mood:50});m.preview('adult','sleep');m.tick(seconds);assert.equal(m.read().pet.mood,50+gain);}
 const {c,m}=boot();Object.assign(c.PetSources.mock.pet,{satiety:51,cleanliness:100,mood:80,timers:{satiety:599,cleanliness:0,mood:1199,recovery:0,danger:0,sick:0}});m.tick(1);assert.equal(m.read().pet.mood,78);
});
test('wyvern uses all 19 original 64x44 masks and exact FPS boundaries',()=>{
 const {c}=boot(),frames=Object.values(c.WyvernFrames).flat();assert.equal(frames.length,19);assert.equal(frames.reduce((n,f)=>n+f.length,0),6688);assert.ok(frames.every(f=>f.length===352));
 for(const bits of frames){const pixels=[],ctx={fillStyle:'',fillRect:(...args)=>pixels.push({args,color:ctx.fillStyle})};c.WyvernView.paint(ctx,bits,0,0,1);const foreground=pixels.filter(p=>{const [x,y]=p.args;return x>=0&&x<64&&y>=0&&y<44&&!(bits[y*8+(x>>3)]&(128>>(x%8)));});let count=0;for(let row=0;row<44;row++)for(let col=0;col<64;col++)if(!(bits[row*8+(col>>3)]&(128>>(col%8)))){assert.deepEqual(foreground[count++].args,[col,row,1,1]);}assert.equal(foreground.length,count);assert.ok(pixels.some(p=>p.color==='#211b2e'));assert.ok(pixels.every(p=>p.args[2]===1&&p.args[3]===1));}
 const p={stage:'baby',healthState:'healthy',sleepMode:'awake',satiety:80,mood:80,cleanliness:100};const frame=(now,action,elapsed)=>c.WyvernView.frameFor({...p},now,action,elapsed);
 assert.equal(frame(166).index,0);assert.equal(frame(167).index,1);p.sleepMode='normal';assert.equal(frame(999).index,0);assert.equal(frame(1000).index,1);p.stage='adult';assert.equal(frame(1000).index,0);p.sleepMode='awake';p.healthState='sick';assert.equal(frame(1000).name,'kWyvernSadFrames');assert.equal(frame(2000,'treat',.5).index,0);assert.equal(frame(0,'feed',.374).index,0);assert.equal(frame(0,'feed',.375).index,1);assert.equal(frame(0,'feed',1.5).index,0);
});
test('both games settle once, cancelled games give nothing, new species does not change rewards',()=>{
 for(const speciesId of [1,2]){
  const {c,m}=boot();c.PetSources.mock.pet.speciesId=speciesId;m.startGame('rps',0);c.PetSources.mock.game.random=()=>0;
  for(let round=0;round<3;round++){m.answerGame((c.PetSources.mock.game.opponent+1)%3,round*2000);m.gameState(round*2000+1050);if(round<2)m.nextRound(round*2000+1850);}
  assert.equal(m.read().pet.mood,65);assert.equal(m.read().pet.exp,15);m.gameState(9000);assert.equal(m.read().pet.exp,15);
  m.startGame('rps',10000);m.cancelGame();m.gameState(20000);assert.equal(m.read().pet.exp,15);
 }
 const {c,m}=boot();m.startGame('memory',0);assert.equal(m.gameState(4999).phase,'prepare');assert.equal(m.gameState(4999).lit,undefined);m.gameState(5000);let now=5000;for(let round=0;round<3;round++){const g=c.PetSources.mock.game;while(g.phase==='show'){now=g.nextAt;m.gameState(now);}for(const choice of g.sequence)m.answerGame(choice,now);m.gameState(now+179);assert.equal(m.read().pet.exp,0);now+=180;m.gameState(now);if(round<2){now+=800;m.nextRound(now);}}
 assert.equal(m.read().pet.mood,65);assert.equal(m.read().pet.exp,15);m.gameState(now+1000);assert.equal(m.read().pet.exp,15);
});
test('storage backoff keeps latest data, allows explicit retry and resets after success',()=>{
 const {c,storage}=boot();let now=0,attempts=0,fail=true;c.Date=class extends Date{static now(){return now;}};
 const set=c.localStorage.setItem;c.localStorage.setItem=(k,v)=>{attempts++;if(fail)throw new Error('quota');set(k,v);};
 assert.equal(c.PetLocalStore.write('test',{value:1}),false);now=4999;assert.equal(c.PetLocalStore.write('test',{value:2}),false);assert.equal(attempts,1);now=5000;assert.equal(c.PetLocalStore.write('test',{value:3}),false);assert.equal(attempts,2);now=14999;c.PetLocalStore.write('test',{value:4});assert.equal(attempts,2);fail=false;assert.equal(c.PetLocalStore.write('test',{value:5},true),true);assert.equal(JSON.parse(storage.get('aviary.test')).data.value,5);fail=true;c.PetLocalStore.write('test',{value:6});const n=attempts;now+=5000;c.PetLocalStore.write('test',{value:7});assert.equal(attempts,n+1);
});
test('game boundaries enforce reveal/feedback delay, hidden opponent and timeout without replay',()=>{
 const {c,m}=boot();m.startGame('rps',0);const original=c.PetSources.mock.game.opponent;assert.equal(m.gameState(0).opponent,undefined);m.answerGame(0,0);assert.equal(c.PetSources.mock.game.opponent,original);assert.equal(m.gameState(1049).phase,'countdown');assert.equal(m.gameState(1050).phase,'round-result');m.nextRound(1849);assert.equal(m.gameState(1849).round,1);m.nextRound(1850);assert.equal(m.gameState(1850).round,2);
 m.startGame('memory',0);const g=c.PetSources.mock.game;m.gameState(10000);assert.equal(g.phase,'show');assert.equal(g.noteIndex,0);let now=10000;while(g.phase==='show'){now=g.nextAt;m.gameState(now);}const deadline=g.until;m.answerGame(g.sequence[0],deadline);assert.equal(m.gameState(deadline).phase,'round-result');assert.equal(g.correct,false);assert.equal(m.read().pet.exp,0);
});

test('feeding and cleaning at 100 still play like the firmware, without extra EXP',()=>{
 const app=fs.readFileSync(__dirname+'/app.js','utf8');
 assert.doesNotMatch(app,/已達 100/);assert.doesNotMatch(app,/吃飽囉/);
 const {c,m}=boot(),p=c.PetSources.mock.pet;p.satiety=100;p.cleanliness=100;const exp=p.exp;
 assert.equal(m.command('feed').actual,0);assert.equal(m.command('clean').actual,0);
 assert.equal(p.satiety,100);assert.equal(p.cleanliness,100);assert.equal(p.exp,exp);
});
