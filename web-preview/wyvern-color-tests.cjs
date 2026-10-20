const fs=require('fs'),vm=require('vm'),assert=require('assert');
const crypto=require('crypto'),approvedAdult=require('./assets/wyvern-adult-approved-20261020.json');
const c={};c.window=c;vm.createContext(c);
for(const f of ['wyvern-sprites.js','assets/wyvern-colors-before-20261020.js','wyvern-view.js'])vm.runInContext(fs.readFileSync(__dirname+'/'+f,'utf8'),c);
function pixels(view,bits){const map=new Map(),ctx={fillStyle:'',fillRect(x,y,w,h){assert.equal(w,1);assert.equal(h,1);map.set(x+','+y,this.fillStyle);}};view.paint(ctx,bits,0,0,1);return map;}
let count=0;const preview=[];
for(const [name,frames] of Object.entries(c.WyvernFrames))for(const [index,bits] of frames.entries()){
 const before=pixels(c.WyvernBeforeView,bits),after=pixels(c.WyvernView,bits);
 if(name.startsWith('kBabyWyvern')){assert(![...after.values()].includes('#cbb0f4'),name+' uniform darker purple');assert.equal(after.get('32,40'),'#FFF0A6',name+' yellow lower belly');}
 if(name.startsWith('kWyvern'))assert.equal(crypto.createHash('sha256').update(JSON.stringify([...after].sort())).digest('hex'),approvedAdult[name+':'+index],name+' approved adult unchanged');
 assert.deepEqual([...before.keys()].sort(),[...after.keys()].sort(),name+' silhouette');
 for(const [key,color] of after){const [cx,cy]=key.split(',').map(Number);const adultBelly=name.startsWith('kWyvern')&&cy>=32&&cy<=40&&Math.abs(cx-32)<=(cy<=31?5:cy<=38?3:2);const babyBand=name.startsWith('kBabyWyvern')&&(cy===35||cy===38)&&Math.abs(cx-32)<=(cy<=37?3:2);assert.equal(color==='#211b2e',before.get(key)==='#211b2e'||babyBand,name+' facial detail/outline');assert(['#211b2e','#B58ADD','#FFF0A6','#cbb0f4','#b99ae9'].includes(color));if(color==='#FFF0A6'){const [x,y]=key.split(',').map(Number);assert(adultBelly||(y>=33&&y<=40&&Math.abs(x-32)<=(y<=37?3:2)),name+' belly only');}}
 assert([...after.values()].includes('#FFF0A6'),name+' visible belly');
 if(index===0&&name.endsWith('IdleFrames'))preview.push({name,before:[...before],after:[...after]});count++;
}
assert.equal(count,19);
if(process.argv.includes('--pixels'))process.stdout.write(JSON.stringify(preview));else console.log('PASS: all 19 frames retain silhouette and dark details; yellow restricted to belly, purple limbs.');
