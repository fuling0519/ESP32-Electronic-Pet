const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const c={};c.window=c;vm.createContext(c);vm.runInContext(fs.readFileSync(__dirname+'/dirt-view.js','utf8'),c);const D=c.PetDirt;
// Records every painted 1 px cell (path rects and fillRects) with the composite mode and alpha it was drawn with.
// No document here, so the pet's dullness takes its plain source-atop path.
function recorder(){const cells=[],stack=[];let path=[];const ctx={canvas:{width:384,height:256},globalCompositeOperation:'source-over',globalAlpha:1,fillStyle:'',
 save(){stack.push([this.globalCompositeOperation,this.globalAlpha]);},restore(){[this.globalCompositeOperation,this.globalAlpha]=stack.pop();},
 beginPath(){path=[];},rect(x,y,w,h){path.push({x,y,w,h});},
 fill(){for(const r of path)cells.push({...r,mode:this.globalCompositeOperation,alpha:this.globalAlpha,color:this.fillStyle});},
 fillRect(x,y,w,h){cells.push({x,y,w,h,mode:this.globalCompositeOperation,alpha:this.globalAlpha,color:this.fillStyle});}};return {ctx,cells};}
const box={cx:163,top:80,bottom:194,w:84};
const onPet=cells=>cells.filter(f=>f.mode==='source-atop'),around=cells=>cells.filter(f=>f.mode!=='source-atop');
const [W,H]=D.GRID,alphaAt=(px,x,y)=>px[(y*W+x)*4+3];
test('cleanliness maps to the firmware bands: >75 clean, then three dirt levels',()=>{
 for(const [v,l] of [[100,0],[76,0],[75,1],[51,1],[50,2],[26,2],[25,3],[0,3]])assert.equal(D.level(v),l,String(v));
 assert.equal(D.level(undefined),0);
});
test('the pet only dulls over its own pixels, more with each level, with no marks on it',()=>{
 const dull=[0,1,2,3].map(level=>{const {ctx,cells}=recorder();D.draw(ctx,{level,box,now:0,smell:false});
  assert.ok(cells.every(f=>f.mode==='source-atop'&&f.w===384),'one whole-pet wash, no spots');return cells[0]?.alpha||0;});
 assert.equal(dull[0],0);assert.ok(dull[1]>0&&dull[2]>dull[1]&&dull[3]>dull[2]);
});
test('screen grime creeps in from the edges as the level rises and leaves the middle clear',()=>{
 const cover=l=>{const px=D.grimePixels(l);let n=0;for(let i=3;i<px.length;i+=4)if(px[i]>60)n++;return n/(W*H);};
 assert.equal(cover(0),0);assert.ok(cover(1)>0&&cover(2)>cover(1)&&cover(3)>cover(2));assert.ok(cover(3)<.5,'never covers the garden');
 for(const l of [1,2,3]){const px=D.grimePixels(l);assert.equal(alphaAt(px,W>>1,H>>1),0,'centre clear');}
 const px=D.grimePixels(3);for(const [x,y] of [[1,1],[W-2,1],[1,H-2],[W-2,H-2]])assert.ok(alphaAt(px,x,y)>80,'every corner grimed');
});
test('stink wisps and flies only at the dirtiest level, and not while being cleaned',()=>{
 const outside=opts=>{const {ctx,cells}=recorder();D.draw(ctx,{box,now:400,...opts});return around(cells).length;};
 assert.equal(outside({level:2}),0);assert.ok(outside({level:3})>0);assert.equal(outside({level:3,smell:false}),0);
});
test('washing fades the dullness down to the level that remains, and the composite mode is restored',()=>{
 const wash=(alpha,keep)=>{const {ctx,cells}=recorder();D.draw(ctx,{level:3,keep,box,now:0,alpha,smell:false});return onPet(cells)[0]?.alpha||0;};
 const two=recorder();D.draw(two.ctx,{level:2,box,now:0});
 assert.ok(Math.abs(wash(0,2)-two.cells[0].alpha)<1e-9,'fully washed equals level 2');assert.ok(wash(.5,2)>wash(0,2)&&wash(.5,2)<wash(1,2));
 const {ctx}=recorder();D.draw(ctx,{level:3,box,now:0});assert.equal(ctx.globalCompositeOperation,'source-over');assert.equal(ctx.globalAlpha,1);
 const none=recorder();D.draw(none.ctx,{level:3,box,alpha:0});assert.equal(none.cells.length,0);
});
test('a partial clean drops the smell with the washed level',()=>{
 const {ctx,cells}=recorder();D.draw(ctx,{level:3,keep:2,box,now:400,alpha:0});assert.equal(around(cells).length,0);
});
test('no stray specks: every grimed cell joins the edge grime, none float in the garden',()=>{
 const px=D.grimePixels(3);let stray=0;
 for(let y=1;y<H-1;y++)for(let x=1;x<W-1;x++){if(!alphaAt(px,x,y))continue;let n=0;for(const [dx,dy] of [[1,0],[-1,0],[0,1],[0,-1]])if(alphaAt(px,x+dx,y+dy))n++;if(!n)stray++;}
 assert.equal(stray,0);
});
