const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
function setup(reduced=false){const c={Image:class{constructor(){this.complete=true;this.naturalWidth=1254;this.naturalHeight=1254;}},PetRules:{updateSad:()=>false},matchMedia:()=>({matches:reduced})};c.window=c;vm.createContext(c);
 for(const f of ['pet-effects.js','assets/pets/bird/white-idle-data-v4.js','assets/pets/bird/white-sleep-data-v1.js','assets/pets/bird/white-startled-data-v1.js','assets/pets/bird/white-dead-data-v1.js','white-bird-view.js'])vm.runInContext(fs.readFileSync(__dirname+'/'+f,'utf8'),c);return c;}
// Records every draw with the transform in force, so a pose's toes can be mapped back to the canvas.
function recorder(){const calls=[];let t={x:0,y:0,sy:1};const stack=[];
 return {calls,ctx:{save(){stack.push({...t});},restore(){t=stack.pop();},fillRect(){},translate(x,y){t.x+=x;t.y+=y*1;},scale(_,sy){t.sy*=sy;},drawImage(im,...a){calls.push({src:im.src,a,t:{...t}});}}};}
function toes(call,d,stage){const h=stage==='adult'?132:106,fit=h/d.source[1]*(d.scale||1),[x,y]=call.a;return [call.t.x+x+d.feet[0]*fit,call.t.y+(y+d.feet[1]*fit)*call.t.sy];}
test('sleep uses the roosting still with its toes where the base image stands, for both stages',()=>{const c=setup(true);
 for(const stage of ['adult','baby']){const p={stage,healthState:'healthy',sleepMode:'normal'},before=JSON.stringify(p),r=recorder();
  assert.equal(c.WhiteBirdView.draw(r.ctx,p,0,null,0,false,190,126,3),true);const [call]=r.calls,d=c.WhiteBirdSleepData,h=stage==='adult'?132:106;
  assert.ok(call.src.endsWith('white-sleep-v1.png'));assert.equal(r.calls.length,1,'no sweat or sad marks while asleep');
  const [x,y]=toes(call,d,stage),base=r=>c.WhiteBirdView.draw(r.ctx,{stage,healthState:'healthy',sleepMode:'awake'},0,'clean',0,false,190,126,3);
  const b=recorder();base(b);const bx=b.calls[0].a[0]+d.baseFeet[0]*h/1254+(-.053+.12)*h;
  assert.ok(Math.abs(x-bx)<1e-9,'toes under the base image toes, shifted to the idle stance');assert.ok(Math.abs(y-(126+h*.47))<1e-9,'on the feet line');
  assert.equal(JSON.stringify(p),before);}});
test('sleep breathes slowly around the feet; reduced motion stays still',()=>{const c=setup();const b=c.PetEffects.breath;
 assert.equal(b(0),1);assert.ok(Math.abs(b(1600)-1.03)<1e-12);assert.equal(b(1600,true),1);
 const r=recorder();c.WhiteBirdView.draw(r.ctx,{stage:'adult',healthState:'healthy',sleepMode:'normal'},1600,null,0,false);
 const [x,y]=toes(r.calls[0],c.WhiteBirdSleepData,'adult');assert.ok(Math.abs(y-(126+132*.47))<1e-9,'feet stay planted while breathing');assert.ok(r.calls[0].t.sy>1);});
test('sick sleep also roosts; death and actions keep their own art',()=>{const c=setup(true);let r=recorder();
 c.WhiteBirdView.draw(r.ctx,{stage:'adult',healthState:'sick',sleepMode:'normal'},0,null,0,false);assert.ok(r.calls[0].src.endsWith('white-sleep-v1.png'));
 r=recorder();c.WhiteBirdView.draw(r.ctx,{stage:'adult',healthState:'dead',sleepMode:'normal'},0,null,0,false);assert.ok(r.calls[0].src.endsWith('white-dead-v1.png'));
 r=recorder();c.WhiteBirdView.draw(r.ctx,{stage:'adult',healthState:'healthy',sleepMode:'awake'},0,null,0,false);assert.ok(!r.calls[0].src.endsWith('white-sleep-v1.png'));});
test('a poke shows the startled still planted at the idle stance, same as the sleep toes',()=>{const c=setup(true);
 for(const stage of ['adult','baby']){const r=recorder(),s=recorder(),h=stage==='adult'?132:106;
  assert.equal(c.WhiteBirdView.draw(r.ctx,{stage,healthState:'healthy',sleepMode:'awake'},0,'startled',0,false),true);
  c.WhiteBirdView.draw(s.ctx,{stage,healthState:'healthy',sleepMode:'normal'},0,null,0,false);
  assert.ok(r.calls[0].src.endsWith('white-startled-v1.png'));assert.equal(r.calls.length,1);
  const a=toes(r.calls[0],c.WhiteBirdStartledData,stage),b=toes(s.calls[0],c.WhiteBirdSleepData,stage);
  assert.ok(Math.abs(a[0]-b[0])<1e-9&&Math.abs(a[1]-b[1])<1e-9);assert.ok(Math.abs(a[1]-(126+h*.47))<1e-9);}});
test('missing pose data falls back to the base image instead of drawing nothing',()=>{const c=setup(true);c.WhiteBirdSleepData=c.WhiteBirdStartledData=undefined;
 let r=recorder();assert.equal(c.WhiteBirdView.draw(r.ctx,{stage:'adult',healthState:'healthy',sleepMode:'normal'},0,null,0,false),true);assert.ok(r.calls[0].src.endsWith('white-idle-v2.png'));
 r=recorder();assert.equal(c.WhiteBirdView.draw(r.ctx,{stage:'adult',healthState:'healthy',sleepMode:'awake'},0,'startled',0,false),true);assert.ok(r.calls[0].src.endsWith('white-idle-v2.png'));});
test('both stills are enlarged around their toes: startled to the idle body height, sleep to the idle feather area',()=>{const c=setup(true);
 assert.ok(c.WhiteBirdStartledData.scale>1.05&&c.WhiteBirdStartledData.scale<1.2,'ChatGPT drew it about 10% small');assert.ok(c.WhiteBirdSleepData.scale>1.1&&c.WhiteBirdSleepData.scale<1.25,'roosting still keeps the same amount of bird');
 const r=recorder();c.WhiteBirdView.draw(r.ctx,{stage:'adult',healthState:'healthy',sleepMode:'awake'},0,'startled',0,false);const d=c.WhiteBirdStartledData;
 assert.ok(Math.abs(r.calls[0].a[3]-d.crop.height*132/1254*d.scale)<1e-9);});
