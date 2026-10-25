const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const data=JSON.parse(fs.readFileSync(__dirname+'/assets/shared/cleaning/animation-v1/animation-data.json','utf8'));
Object.assign(data.assets,JSON.parse(fs.readFileSync(__dirname+'/assets/shared/cleaning/foam-study-v2/manifest.json','utf8')).assets);
const c={CleaningAnimationData:data,Image:class{constructor(){this.complete=true;this.naturalWidth=1;}set src(value){this.file=value;}}};c.window=c;vm.createContext(c);vm.runInContext(fs.readFileSync(__dirname+'/cleaning-animation-view.js','utf8'),c);const v=c.CleaningAnimationView;
function render(t,species='bird',stage='adult',reduced=false){const calls=[],ctx={globalAlpha:1,imageSmoothingEnabled:false,save(){},restore(){},drawImage(im,...args){calls.push({file:im.file,args});}};v.draw(ctx,t,{species,stage,reduced});return calls;}
test('rinse reverses gently, uses a smaller path for babies, and stays within each torso',()=>{
 for(const species of ['bird','wyvern']){
  const child=v.pose(species,'baby'),adult=v.pose(species,'adult');
  assert.ok(v.showerPose(2.35,child).halfSpan<v.showerPose(2.35,adult).halfSpan);
  for(const p of [child,adult]){
   assert.ok(v.showerPose(2.35,p).x<v.showerPose(3,p).x);
   for(const turn of [2,2.35,3,3.4]){const h=.0001,m=v.showerPose(turn,p);assert.ok(Math.abs((m.x-v.showerPose(turn-h,p).x)/h)<.16);assert.ok(Math.abs((v.showerPose(turn+h,p).x-m.x)/h)<.16);}
   for(let t=2;t<3.4;t+=.01){const s=v.showerPose(t,p);assert.ok(s.impactX>=190-p.bodyWidth/2&&s.impactX<=190+p.bodyWidth/2);}
  }
 }
});
test('rendered water top remains attached to rendered nozzle throughout the moving rinse',()=>{
 for(const species of ['bird','wyvern'])for(const stage of ['baby','adult'])for(const t of [2,2.12,2.35,2.65,3,3.22,3.399]){
  const calls=render(t,species,stage),head=calls.find(x=>x.file.includes('shower-sheet')),water=calls.find(x=>x.file.includes('water-sheet'));assert.ok(head&&water);
  const s=data.assets.shower,w=data.assets.water,hs=head.args[6]/s.cellWidth,ws=water.args[6]/w.cellWidth;
  const nx=head.args[4]+s.nozzle[0]*hs,ny=head.args[5]+s.nozzle[1]*hs;
  const wx=water.args[4]+w.anchor[0]*ws,wy=water.args[5]+w.anchor[1]*ws;
  assert.ok(Math.abs(nx-wx)<=1.01&&Math.abs(ny-wy)<=1.01,`${species}/${stage}/${t}: nozzle detached`);
 }
});
test('water stops before head exits, effects clear at the end, reduced mode has no shower',()=>{
 assert.ok(!render(3.4).some(x=>x.file.includes('water-sheet')));
 assert.ok(render(3.4).some(x=>x.file.includes('shower-sheet')));
 assert.ok(!render(3.71).some(x=>/water-sheet|shower-sheet/.test(x.file)));
 assert.ok(!render(3.73).some(x=>/drops-sheet|water-sheet|shower-sheet/.test(x.file)));
 assert.ok(render(3.73).some(x=>x.file.includes('sparkle-sheet')));
 assert.equal(render(4.2).length,0);
 assert.ok(!render(2.5,'bird','adult',true).some(x=>/water-sheet|shower-sheet|drops-sheet/.test(x.file)));
});
test('sponge rendering keeps the same 60px reference scale for every age and species',()=>{
 const widths=[];for(const species of ['bird','wyvern'])for(const stage of ['baby','adult']){const call=render(.7,species,stage).find(x=>x.file.includes('sponge-sheet'));assert.ok(call);widths.push(call.args[6]*data.assets.sponge.referenceWidth/data.assets.sponge.cellWidth);}
 for(const width of widths)assert.ok(Math.abs(width-60)<1e-9);
});
test('sponge wipes across both sides three times while descending through each torso',()=>{
 for(const species of ['bird','wyvern'])for(const stage of ['baby','adult']){
  const p=v.pose(species,stage),samples=Array.from({length:301},(_,i)=>v.spongePose(.3+1.3*i/300,p));
  let reversals=0,lastDirection=0;
  for(let i=1;i<samples.length;i++){
   assert.ok(samples[i].y>=samples[i-1].y-1e-9);
   assert.ok(Math.abs(samples[i].x-190)<p.bodyWidth/2);
   const direction=Math.sign(samples[i].x-samples[i-1].x);
   if(direction&&lastDirection&&direction!==lastDirection)reversals++;
   if(direction)lastDirection=direction;
  }
  assert.equal(reversals,2);
  assert.ok(Math.min(...samples.map(s=>s.x))<174.5);
  assert.ok(Math.max(...samples.map(s=>s.x))>205.5);
  assert.ok(samples.at(-1).y-samples[0].y>=18);
  assert.ok(samples[0].y<p.bodyY&&samples.at(-1).y>p.bodyY);
  assert.ok(samples.at(-1).y<p.bottom);
 }
 for(const species of ['bird','wyvern']){
  const baby=v.pose(species,'baby'),adult=v.pose(species,'adult');
  assert.ok(v.spongePose(.3,baby).x<v.spongePose(.3,adult).x);
  assert.ok(v.spongePose(1.6,baby).y-v.spongePose(.3,baby).y<v.spongePose(1.6,adult).y-v.spongePose(.3,adult).y);
 }
});
test('sponge slows at horizontal turns and joins entry and exit without jumps',()=>{
 const p=v.pose('bird','adult'),h=.00001;
 for(const turn of [.3,.3+1.3/3,.3+2*1.3/3,1.6]){
  const a=v.spongePose(turn-h,p),b=v.spongePose(turn,p),c=v.spongePose(turn+h,p);
  assert.ok(Math.abs((b.x-a.x)/h)<.04&&Math.abs((c.x-b.x)/h)<.04);
  assert.ok(Math.abs(c.y-a.y)<.001);
 }
 assert.equal(v.spongePose(0,p).alpha,0);
 assert.equal(v.spongePose(1.95,p).alpha,0);
 assert.ok(!render(1.95).some(x=>x.file.includes('sponge-sheet')));
});
test('lather requires actual sponge contact; rinse waits for water contact and clears each clump',()=>{
 for(const species of ['bird','wyvern'])for(const stage of ['baby','adult']){
  const p=v.pose(species,stage),clumps=v.foamSchedule(p);
  assert.equal(clumps.length,5);
  assert.ok(Math.max(...clumps.map(x=>x.born))-Math.min(...clumps.map(x=>x.born))>.2);
  assert.ok(Math.max(...clumps.map(x=>x.rinseAt))-Math.min(...clumps.map(x=>x.rinseAt))>.2);
  for(const clump of clumps){
   assert.ok(Number.isFinite(clump.born)&&clump.born<1.7);
   assert.equal(v.foamState(clump.born-.001,clump),null);
   assert.equal(v.foamState(clump.born+.07,clump).index,0);
   assert.equal(v.foamState(1.99,clump).index,clump.peak);
   assert.equal(v.foamState(clump.rinseAt+.35,clump).index,5);
   assert.equal(v.foamState(3.4,clump),null);
  }
  const missed=v.foamSchedule(p,{sponge:()=>({x:999,y:999,active:true})});
  assert.ok(missed.every(x=>x.born===Infinity&&v.foamState(1,x)===null));
  const dry=v.foamSchedule(p,{shower:()=>({impactX:999,ny:0,alpha:1})});
  assert.ok(dry.every(x=>x.rinseAt===Infinity&&v.foamState(3.3,x).index===x.peak));
  // Scrubbing in the opposite direction changes which side lathers first.
  const reversed=v.foamSchedule(p,{sponge:(t,p)=>{const s=v.spongePose(t,p);return {...s,x:380-s.x};}});
  assert.ok(clumps[0].born>clumps[2].born&&reversed[0].born<reversed[2].born);
 }
 assert.ok(!render(3.4).some(x=>x.file.includes('foam-sheet')));
 assert.equal(render(1,'bird','adult',true).filter(x=>x.file.includes('foam-sheet')).length,5);
});
test('a measured torso placement moves the sponge, foam, rinse and sparkles together',()=>{
 const centred=v.pose('bird','adult'),placed=v.pose('bird','adult',{x:154,bottom:199}),dx=154-190,dy=199-centred.bottom;
 assert.equal(centred.cx,190);assert.equal(placed.cx,154);assert.equal(placed.bodyY-centred.bodyY,dy);
 for(const t of [.3,.8,1.2,1.6])assert.ok(Math.abs(v.spongePose(t,placed).x-v.spongePose(t,centred).x-dx)<1e-9);
 for(const t of [2,2.5,3.2])assert.ok(Math.abs(v.showerPose(t,placed).impactX-v.showerPose(t,centred).impactX-dx)<1e-9);
 const a=v.foamSchedule(centred),b=v.foamSchedule(placed);
 a.forEach((clump,i)=>{assert.ok(Math.abs(b[i].x-clump.x-dx)<1e-9);assert.ok(Math.abs(b[i].born-clump.born)<.01);});
 const sparks=at=>{const calls=[],ctx={globalAlpha:1,imageSmoothingEnabled:false,save(){},restore(){},drawImage(im,...args){calls.push({file:im.file,args});}};v.draw(ctx,3.8,{species:'bird',stage:'adult',at});return calls.filter(x=>x.file.includes('sparkle')).map(x=>x.args[4]);};
 assert.deepEqual(sparks({x:154,bottom:centred.bottom}),sparks(undefined).map(x=>x+dx));
});
