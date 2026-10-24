const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
function setup(){const c={Image:class{constructor(){this.complete=true;this.naturalWidth=1254;}},PetRules:{updateSad:()=>false},matchMedia:()=>({matches:false})};c.window=c;vm.createContext(c);
 for(const f of ['assets/pets/bird/white-dead-data-v1.js','white-bird-view.js','bird-view.js'])vm.runInContext(fs.readFileSync(__dirname+'/'+f,'utf8'),c);return c;}
function recorder(){const calls=[];return {calls,ctx:{save(){},restore(){},fillRect(){},drawImage(im,...a){calls.push([im.src,...a]);}}};}
test('dead bird lies down on the feet line, sized to the standing bird, for both stages',()=>{const c=setup(),d=c.WhiteBirdDeadData;
 for(const [stage,height] of [['adult',132],['baby',106]]){const r=recorder();assert.equal(c.WhiteBirdView.draw(r.ctx,{stage,healthState:'dead',sleepMode:'normal'},0,null,0,false,190,135,3),true);
  const [src,x,y,w,h]=r.calls[0],fit=height/d.source[1]*c.WhiteBirdView.DEAD_SCALE;assert.ok(src.endsWith('white-dead-v1.png'));assert.equal(r.calls.length,1,'no sad or sick effects');
  assert.ok(Math.abs(w-d.crop.width*fit)<1e-9&&Math.abs(h-d.crop.height*fit)<1e-9);assert.ok(Math.abs(y+h-(135+height*.47))<1e-9,'rests on the feet line');assert.ok(Math.abs(x+d.anchor[0]*fit-190)<1e-9,'body centred');}});
test('closed BirdView.still is the resting look; open still and sleep keep the standing art',()=>{const c=setup();
 let r=recorder();c.BirdView.still(r.ctx,'adult',true);assert.ok(r.calls[0][0].endsWith('white-dead-v1.png'));
 r=recorder();c.BirdView.still(r.ctx,'adult',false);assert.ok(!r.calls[0][0].endsWith('white-dead-v1.png'));
 r=recorder();c.WhiteBirdView.draw(r.ctx,{stage:'adult',healthState:'healthy',sleepMode:'normal'},0,null,0,false);assert.ok(!r.calls[0][0].endsWith('white-dead-v1.png'),'sleep is not the dead pose');});
test('missing lying-down data reports not drawn instead of standing up',()=>{const c=setup();c.WhiteBirdDeadData=undefined;const r=recorder();
 assert.equal(c.WhiteBirdView.draw(r.ctx,{stage:'adult',healthState:'dead',sleepMode:'normal'},0,null,0,false),false);assert.equal(r.calls.length,0);});
