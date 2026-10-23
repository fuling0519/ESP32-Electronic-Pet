const {test}=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs'),vm=require('node:vm');
// Drives pet-interactions.js with a stub DOM and a manual clock/timer queue.
function boot(){
 let now=0;const timers=[],events=[],commands=[],handlers={};
 const el=(extra={})=>Object.assign({style:{},disabled:false,textContent:'',setAttribute(){},append(){},appendChild(){},querySelector:()=>null,getContext:()=>new Proxy({},{get:()=>()=>{}}),showModal(){},open:false},extra);
 const canvas=el({addEventListener:(k,f)=>handlers[k]=f,getBoundingClientRect:()=>({left:0,top:0,width:384,height:256}),setPointerCapture(){}});
 const nodes={'.scene':el(),'#bird':canvas,'.care':null,'#status':el(),'#touch-pet':el(),'#touch-poke':el(),'#open-status':el(),'#game-dialog':el(),'#farewell-dialog':el(),'[data-action="feed"]':el()};
 const pet={petId:'p1',stage:'adult',speciesId:2,healthState:'healthy',sleepMode:'awake',isDeparted:false};
 const c={performance:{now:()=>now},matchMedia:()=>({matches:false}),requestAnimationFrame(){},location:{search:''},
  setTimeout:(f,ms)=>{timers.push({f,at:now+ms});return timers.length;},clearTimeout:id=>{if(timers[id-1])timers[id-1].f=null;},
  CustomEvent:class{constructor(type,init){this.type=type;this.detail=init?.detail;}},
  PetEffects:{HEART_LIFE:500,SHOCK_MS:900,hearts(){},shock(){}},
  petDataManager:{read:()=>({pet,metadata:{source:'mock'}}),command:kind=>{commands.push(kind);return {actual:kind==='pet'?3:-3};}},
  document:{querySelector:s=>s in nodes?nodes[s]:null,createElement:()=>el(),body:el(),dispatchEvent:e=>events.push(e)}};
 c.window=c;vm.createContext(c);vm.runInContext(fs.readFileSync(__dirname+'/pet-interactions.js','utf8'),c);
 const advance=ms=>{now+=ms;for(const t of timers)if(t.f&&t.at<=now){const f=t.f;t.f=null;f();}};
 const press=(x=190,y=130)=>handlers.pointerdown({button:0,clientX:x,clientY:y,pointerId:1,preventDefault(){}});
 const release=(x=190,y=130)=>handlers.pointerup({clientX:x,clientY:y,pointerId:1});
 return {c,advance,press,release,commands,events};
}
test('a short press on the pet pokes immediately; there is no double-tap wait',()=>{
 const t=boot();t.press();t.advance(150);t.release();assert.deepEqual(t.commands,['poke']);
 t.advance(5000);assert.deepEqual(t.commands,['poke'],'no delayed second action');
});
test('holding past the firmware threshold pets once while held, release ends the hold',()=>{
 const t=boot(),{PETTING_MS}=t.c.PetTouch;assert.equal(PETTING_MS,700);
 t.press();t.advance(699);assert.deepEqual(t.commands,[]);t.advance(1);assert.deepEqual(t.commands,['pet']);
 assert.ok(t.events.some(e=>e.type==='pet-touch-hold'&&e.detail.holding===true));
 t.advance(2000);t.release();assert.deepEqual(t.commands,['pet'],'release after petting does not poke');
 assert.equal(t.events.at(-1).type,'pet-touch-hold');assert.equal(t.events.at(-1).detail.holding,false);
});
test('presses outside the pet and the classifier boundary',()=>{
 const t=boot();t.press(10,10);t.advance(800);t.release(10,10);assert.deepEqual(t.commands,[]);
 assert.equal(t.c.PetTouch.classify(699),'poke');assert.equal(t.c.PetTouch.classify(700),'pet');
});
