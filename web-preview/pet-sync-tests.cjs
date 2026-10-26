/* Run: node --test web-preview/pet-sync-tests.cjs. 養成同步 phase 1 (PROTOCOL.md): status additions, device_info.rules,
 * new activity values and pet_event. No radio, browser permissions or network. */
const {test}=require('node:test');
const assert=require('node:assert/strict');
const vm=require('node:vm'),fs=require('node:fs'),crypto=require('node:crypto');
function boot(storage=new Map()){
 const elements=new Map();const element=()=>({textContent:'',value:'',children:[],replaceChildren(){this.children=[];},append(...items){this.children.push(...items);},appendChild(item){this.children.push(item);return item;},showModal(){},after(){},before(){},focus(){},setAttribute(){},prepend(...items){this.children.unshift(...items);},addEventListener(){}});
 const context=vm.createContext({console,crypto:crypto.webcrypto,TextEncoder,TextDecoder,Uint8Array,Uint32Array,DataView,Date,BigInt,Map,Set,JSON,Number,Error,setTimeout,clearTimeout,
 document:{querySelector:s=>{if(!elements.has(s))elements.set(s,element());return elements.get(s);},createElement:element},
 localStorage:{getItem:k=>storage.get(k)??null,setItem:(k,v)=>storage.set(k,v),removeItem:k=>storage.delete(k)},
 location:{protocol:'https:'},isSecureContext:true,navigator:{}});context.window=context;
 for(const file of ['local-store.js','pet-rules.js','pet-games.js','wyvern-sprites.js','wyvern-view.js','data-sources.js','ble-source.js','memorials.js','data-manager.js','device-session.js'])vm.runInContext(fs.readFileSync(__dirname+'/'+file,'utf8'),context,{filename:file});
 return {c:context,m:context.petDataManager,ble:context.PetSources.ble,storage,elements};
}
const PET_ID='18446744073709551615';
const status=(pet={},top={})=>({v:1,type:'status',device_id:'esp32-AABBCCDDEEFF',activity:'idle',...top,pet:{id:PET_ID,name:'Tamama',life_stage:'baby',satiety:80,mood:70,cleanliness:95,age_seconds:'3600',health:'healthy',is_dead:false,sleep:'awake',...pet}});
const info=(extra={})=>({v:1,type:'device_info',id:1,protocol_version:1,device_id:'esp32-AABBCCDDEEFF',firmware_version:'test',max_message_bytes:1024,features:['web_game','pet_events'],...extra});
const plain=x=>JSON.parse(JSON.stringify(x));
// A connected device that has sent its info and one full snapshot.
function connected(infoExtra={},pet={},top={}){const b=boot();b.ble.connection='waiting';b.ble.messageReceived(info(infoExtra));b.ble.messageReceived(status(pet,top));b.m.select('ble');return b;}
const full={sad_reasons:['satiety','sick'],danger_seconds:0,sick_seconds:120,health:'sick',satiety:20};

test('old firmware snapshot without the sync fields maps exactly as before',()=>{
 const {c}=boot(),p=plain(c.PetBle.mapStatus(status()));
 assert.equal(p.sadReasons,undefined,'no firmware sad reasons, so the page keeps computing them');
 assert.equal(p.dangerSeconds,null);assert.equal(p.sickSeconds,null);assert.equal(p.deathCause,null);
 assert.equal(p.petId,PET_ID);assert.equal(p.stage,'baby');assert.equal(p.speciesId,1);
 const {ble,m}=connected({features:['web_game']});
 assert.equal(ble.read().petEvents,false);assert.equal(ble.read().rules,null);assert.equal(ble.read().memorials,null);
 assert.equal(m.view('ble').petEvents,false);assert.equal(m.view('ble').rules,null);
});
test('new status fields are accepted and mapped to domain fields',()=>{
 const {c}=boot(),p=plain(c.PetBle.mapStatus(status(full,{memorial_count:3,memorial_hash:'5c1e09ab'})));
 assert.deepEqual(p.sadReasons,{device:true,mood:false,satiety:true,cleanliness:false,sick:true});
 assert.equal(p.dangerSeconds,0);assert.equal(p.sickSeconds,120);assert.equal(p.deathCause,null);
 assert.deepEqual(plain(c.PetBle.mapStatus(status({sad_reasons:[]}))).sadReasons,{device:true,mood:false,satiety:false,cleanliness:false,sick:false});
 assert.equal(c.PetBle.mapStatus(status({danger_seconds:4294967295})).dangerSeconds,4294967295,'the whole uint32 range');
 const dead=status({health:'dead',is_dead:true,sad_reasons:[],sick_seconds:86400,death_cause:'untreated_sickness'});
 assert.equal(c.PetBle.mapStatus(dead).deathCause,'untreated_sickness');
 assert.equal(c.PetBle.mapStatus(status({health:'dead',is_dead:true,death_cause:'meteor'})).deathCause,'unknown','an unknown cause is shown as unknown, not rejected');
 const {ble,m}=connected({},full,{memorial_count:3,memorial_hash:'5c1e09ab'});
 assert.deepEqual(plain(ble.read().memorials),{count:3,hash:'5c1e09ab'});assert.deepEqual(plain(m.view('ble').memorials),{count:3,hash:'5c1e09ab'});
 assert.equal(m.view('ble').pet.sickSeconds,120);
});
test('every new status field rejects the whole snapshot when present but malformed',()=>{
 const {c}=boot(),bad=[
  [{sad_reasons:'mood'}],[{sad_reasons:['hungry']}],[{sad_reasons:['mood','mood']}],[{sad_reasons:[1]}],[{sad_reasons:null}],
  [{life_stage:'egg',sad_reasons:['mood']}],[{health:'dead',is_dead:true,sad_reasons:['sick']}],[{is_departed:true,sad_reasons:['mood']}],
  [{danger_seconds:-1}],[{danger_seconds:1.5}],[{danger_seconds:'5'}],[{danger_seconds:4294967296}],[{danger_seconds:null}],
  [{sick_seconds:-1}],[{health:'sick',sick_seconds:2.5}],[{health:'sick',sick_seconds:'9'}],[{health:'sick',sick_seconds:4294967296}],[{sick_seconds:null}],
  [{sick_seconds:5}],
  [{death_cause:'untreated_sickness'}],[{health:'dead',is_dead:true,death_cause:3}],[{health:'dead',is_dead:true,death_cause:null}],
  [{},{memorial_count:3}],[{},{memorial_hash:'5c1e09ab'}],[{},{memorial_count:3,memorial_hash:'5C1E09AB'}],[{},{memorial_count:3,memorial_hash:'5c1e09a'}],
  [{},{memorial_count:256,memorial_hash:'5c1e09ab'}],[{},{memorial_count:-1,memorial_hash:'5c1e09ab'}],[{},{memorial_count:1.5,memorial_hash:'5c1e09ab'}],[{},{memorial_count:'3',memorial_hash:'5c1e09ab'}],[{},{memorial_count:null,memorial_hash:null}]];
 for(const [pet,top] of bad)assert.throws(()=>c.PetBle.mapStatus(status(pet,top)),undefined,JSON.stringify([pet,top]));
 // A rejected snapshot leaves the last good one in place.
 const {ble}=connected();const before=plain(ble.read());
 assert.throws(()=>ble.messageReceived(status({sad_reasons:['nope']})));assert.deepEqual(plain(ble.read()),before);
});
test('device_info.rules accepts positive integers and rejects anything else',()=>{
 const {c,ble}=boot(),r=c.PetBle.mapRules;
 assert.deepEqual(plain(r({sick_after_seconds:21600,death_after_sick_seconds:86400,memorial_limit:32,extra:'x'})),{sick_after_seconds:21600,death_after_sick_seconds:86400,memorial_limit:32});
 assert.equal(r(undefined),null);assert.deepEqual(plain(r({memorial_limit:32})),{memorial_limit:32});
 for(const bad of [null,[],'x',{sick_after_seconds:0},{sick_after_seconds:-5},{death_after_sick_seconds:1.5},{memorial_limit:'32'},{memorial_limit:null},{sick_after_seconds:4294967296}])assert.throws(()=>r(bad),undefined,JSON.stringify(bad));
 ble.connection='waiting';assert.throws(()=>ble.messageReceived(info({rules:{sick_after_seconds:0}})),/規則/);assert.equal(ble.info,undefined,'a rejected device_info is not stored');
 const ok=connected({rules:{sick_after_seconds:21600,death_after_sick_seconds:86400,memorial_limit:32}});
 assert.deepEqual(plain(ok.m.view('ble').rules),{sick_after_seconds:21600,death_after_sick_seconds:86400,memorial_limit:32});assert.equal(ok.m.view('ble').petEvents,true);
 ok.ble.disconnect();assert.equal(ok.ble.read().rules,null);assert.equal(ok.ble.read().petEvents,false);
});
test('firmware sad_reasons override the local computation; without them the local one still runs',()=>{
 const {c}=boot(),R=c.PetRules,map=pet=>plain(c.PetDomain.normalize(c.PetBle.mapStatus(status(pet))));
 // Low mood but the firmware (still inside its hysteresis) says not sad: the page agrees with the firmware.
 let p=map({mood:10,sad_reasons:[]});assert.equal(R.updateSad({...p}),false);assert.deepEqual(plain(R.sadnessReasons(p)),[]);
 // Mood 44 recovering after sadness: the firmware still counts it; local thresholds alone would not.
 p=map({mood:44,sad_reasons:['mood']});assert.equal(R.updateSad({...p}),true);assert.match(R.sadnessReasons(p)[0],/心情 44/);
 assert.equal(R.updateSad({...map({mood:44})}),false,'local fallback without hysteresis memory');
 p=map({health:'sick',sick_seconds:10,sad_reasons:['sick']});assert.match(R.sadnessReasons(p).at(-1),/治療/);
 assert.equal(R.updateSad({...map({mood:10})}),true,'old firmware: local thresholds');
 assert.equal(c.PetSadView,undefined);vm.runInContext(fs.readFileSync(__dirname+'/sad-view.js','utf8'),c);
 assert.equal(c.PetSadView.active(map({mood:10,sad_reasons:[]})),false);assert.equal(c.PetSadView.active(map({mood:90,sad_reasons:['cleanliness']})),true);
 // Views that force a happy model replace sadReasons, which drops the device marker with it.
 assert.equal(R.updateSad({...map({sad_reasons:['mood']}),mood:100,sadReasons:{mood:false,satiety:false,cleanliness:false}}),false);
 // Mock / demo mode is unchanged.
 const {m}=boot();m.demoNeed('mood',10);assert.equal(m.read().pet.sadReasons.device,undefined);assert.equal(R.updateSad({...m.read().pet}),true);
});
test('sickness countdown uses the snapshot seconds against device rules, else nothing',()=>{
 const {c}=boot(),R=c.PetRules,rules={sick_after_seconds:21600,death_after_sick_seconds:86400};
 const sick={stage:'adult',healthState:'sick',sickSeconds:3600,satiety:20,cleanliness:50};
 assert.equal(R.countdown(sick,rules),'若不治療，約 23 小時後會離世');
 assert.equal(R.countdown(sick,rules,60),'若不治療，約 22 小時 59 分後會離世');
 assert.equal(R.countdown({...sick,sickSeconds:90000},rules),'若不治療，約 不到 1 分鐘後會離世');
 const danger={stage:'baby',healthState:'healthy',dangerSeconds:21000,satiety:0,cleanliness:40};
 assert.equal(R.countdown(danger,rules),'飽食度歸零，約 10 分鐘後會生病');
 assert.equal(R.countdown({...danger,satiety:0,cleanliness:0},rules),'飽食度與清潔度歸零，約 10 分鐘後會生病');
 assert.equal(R.countdown({...danger,dangerSeconds:0},rules),'');assert.equal(R.countdown(danger,null),'','no rules from the firmware');
 assert.equal(R.countdown({...sick,sickSeconds:null},rules),'','old firmware without seconds');
 assert.equal(R.countdown({...sick,healthState:'dead'},rules),'');assert.equal(R.countdown({...sick,isDeparted:true},rules),'');
});
test('menu, sleeping and ended are not idle: web play stays unavailable with a hint',()=>{
 for(const [activity,hint] of [['menu',/主畫面/],['sleeping',/睡覺/],['ended',/不能陪玩/],['busy',/主畫面/],['someday',/主畫面/]]){
  const {c}=connected({},{},{activity});assert.match(c.PetDeviceSession.blocker('toy'),hint,activity);}
 assert.equal(connected().c.PetDeviceSession.blocker('toy'),'');
});
test('pet_event reaches listeners only when ready, for the current pet, from the device, and never changes state',()=>{
 const {ble,m,storage}=connected({},{},{memorial_count:1,memorial_hash:'0000abcd'}),seen=[];let emitted=0;
 m.onPetEvent(e=>seen.push(e));ble.subscribe(()=>emitted++);
 const before=plain(m.read('ble')),saved=storage.get('aviary.device-snapshot');
 const ev=(event,extra={})=>ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event,source:'device',...extra});
 ev('fed');ev('level_up',{level:4});ev('memorial_deleted',{memorial_pet_id:'3'});ev('got_sick',{source:undefined});
 ev('cleaned',{pet_id:'41'});ev('treated',{source:'web'});ev('dance');ev('fed',{pet_id:'x1'});ev('adopted',{pet_id:'43'});
 assert.deepEqual(plain(seen),[{event:'fed',petId:PET_ID,source:'device'},{event:'level_up',petId:PET_ID,source:'device',level:4},{event:'memorial_deleted',petId:PET_ID,source:'device',memorialPetId:'3'},{event:'got_sick',petId:PET_ID,source:null},{event:'adopted',petId:'43',source:'device'}]);
 assert.deepEqual(plain(m.read('ble')),before,'events never change the pet or metadata');assert.equal(storage.get('aviary.device-snapshot'),saved);assert.equal(emitted,0,'no snapshot listener runs');
 // Even an event claiming the pet died leaves the snapshot alone; the next status decides.
 ev('died');assert.equal(m.read('ble').pet.healthState,'healthy');
 // Not ready (still waiting for the snapshot, or disconnected) and firmware without the feature: ignored.
 seen.length=0;ble.connection='waiting';ev('fed');ble.connection='ready';
 const old=connected({features:['web_game']}),oldSeen=[];old.m.onPetEvent(e=>oldSeen.push(e));old.ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'fed',source:'device'});
 ble.disconnect();ev('fed');assert.deepEqual(seen,[]);assert.deepEqual(oldSeen,[]);
});

// The real page: a device pet_event only replays the matching animation, and never sends a care command.
test('device-mode page plays the feed animation for a device fed event and skips events while one is playing',()=>{
 const {c,m,ble,elements}=boot();const get=s=>c.document.querySelector(s);const buttons=['feed','clean','play','treat'].map(kind=>({dataset:{action:kind},addEventListener(type,fn){this.click=fn;}}));
 const drawing=new Proxy({drawImage(){},createRadialGradient:()=>({addColorStop(){}}),createLinearGradient:()=>({addColorStop(){}})},{get:(o,k)=>o[k]||(()=>{})});
 const decorate=el=>Object.assign(el,{dataset:{},style:{},classList:{add(){},remove(){},toggle(){},contains(){return false;}},setAttribute(){},removeAttribute(){},focus(){},close(){},getContext:()=>drawing,clientWidth:384});
 for(const id of fs.readFileSync(__dirname+'/index.html','utf8').matchAll(/id="([^"]+)"/g))decorate(get('#'+id[1]));const h1=decorate({});get('#aviary').querySelector=()=>h1;
 get('#stage').options=['egg','baby','adult'].map(value=>({value}));c.document.querySelectorAll=()=>buttons;c.document.addEventListener=()=>{};
 let T=0;c.Image=class{constructor(){this.complete=true;this.naturalWidth=2172;this.naturalHeight=724;}};c.ResizeObserver=class{observe(){}};c.matchMedia=()=>({matches:false});c.devicePixelRatio=1;c.performance={now:()=>T};
 c.setInterval=()=>0;c.clearInterval=()=>{};c.document.dispatchEvent=()=>{};let frame;c.requestAnimationFrame=fn=>{frame=fn;};c.checkBleReadiness=()=>{};const createElement=c.document.createElement;c.document.createElement=()=>decorate(createElement());
 for(const file of ['wyvern-adult-view.js','shared-sprites.js','shared-view.js','assets/pets/bird/white-idle-data-v4.js','assets/pets/bird/white-sad-data-v1.js','assets/pets/bird/white-sick-data-v1.js','white-bird-view.js','bird-view.js','assets/pets/bird/white-activity-data-v1.js','white-bird-activity.js','bird-demo.js','assets/items/blue-seed-bowl-data-v1.js','blue-seed-bowl.js','white-bird-feed.js','growth-view.js','death-view.js','farewell-view.js','egg-view.js','bird-sad-sprites.js','sad-view.js','game-ui.js','app.js'])vm.runInContext(fs.readFileSync(__dirname+'/'+file,'utf8'),c,{filename:file});
 const commands=[];m.command=kind=>{commands.push(kind);throw new Error('care must not be sent');};
 assert.equal(get('#mode').value,'ble');
 ble.connection='waiting';ble.messageReceived(info({rules:{sick_after_seconds:21600,death_after_sick_seconds:86400}}));ble.messageReceived(status({satiety:0,danger_seconds:21000,sad_reasons:['satiety']}));
 assert.match(get('#sad-reason').textContent,/飽食度 0.*飽食度歸零，約 10 分鐘後會生病/);assert.equal(get('#sad-reason').hidden,false);
 const play=buttons[2],step=ms=>{T+=ms;frame(T);};step(16);assert.equal(play.disabled,false,'idle device: play is offered');
 const before=plain(m.read('ble'));
 ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'fed',source:'device'});step(16);
 assert.match(get('#status').textContent,/吃飯/);assert.equal(play.disabled,true,'play waits for the animation');
 ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'cleaned',source:'device'});assert.match(get('#status').textContent,/吃飯/,'a second animation is skipped while one plays');
 ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'treated',source:'web'});
 for(let i=0;i<220;i++)step(16);assert.equal(play.disabled,false,'the animation ends by itself');
 assert.deepEqual(plain(m.read('ble')),before);assert.deepEqual(commands,[]);
 // Treat animation plays through its settle point without asking the data layer to treat.
 ble.messageReceived(status({health:'sick',sick_seconds:30,sad_reasons:['sick']}));step(16);
 assert.match(get('#sad-reason').textContent,/若不治療，約 23 小時 59 分後會離世/);
 ble.messageReceived({v:1,type:'pet_event',pet_id:PET_ID,event:'treated',source:'device'});step(16);assert.match(get('#status').textContent,/治療/);assert.equal(play.disabled,true);for(let i=0;i<240;i++)step(16);assert.equal(play.disabled,false,'the treat animation ends by itself');
 assert.deepEqual(commands,[]);assert.equal(m.read('ble').pet.healthState,'sick','only the next snapshot cures it');
 // A dead pet with an unknown cause says so.
 ble.messageReceived(status({health:'dead',is_dead:true,death_cause:'meteor'}));assert.match(get('#pet-description').textContent,/已離世（原因未知）/);
 ble.messageReceived(status({health:'dead',is_dead:true,death_cause:'untreated_sickness'}));assert.match(get('#pet-description').textContent,/生病未治療/);
});
