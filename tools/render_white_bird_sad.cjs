const fs=require('node:fs'),vm=require('node:vm'),path=require('node:path'),{createCanvas,loadImage}=require('@napi-rs/canvas');
const kind=process.argv[2]||'sad';
if(!['sad','sick'].includes(kind))throw Error('Invalid expression');
const root=path.resolve(__dirname,'../web-preview');
(async()=>{const imgs=await Promise.all(['white-idle-v2.png','white-idle-sheet-v4.png','white-sad-sheet-v1.png','white-sick-sheet-v1.png'].map(f=>loadImage(path.join(root,'assets/pets/bird',f)))),bg=await loadImage(path.join(root,'assets/garden-detailed-day.png'));let n=0;
for(const im of imgs){Object.defineProperty(im,'src',{set(){}});Object.defineProperty(im,'naturalWidth',{value:im.width});}
const c={Image:class{constructor(){return imgs[n++];}},PetRules:{updateSad:()=>true}};c.window=c;vm.createContext(c);
for(const f of ['assets/pets/bird/white-idle-data-v4.js','assets/pets/bird/white-sad-data-v1.js','assets/pets/bird/white-sick-data-v1.js','white-bird-view.js'])vm.runInContext(fs.readFileSync(path.join(root,f),'utf8'),c);
const out=createCanvas(768,512),ctx=out.getContext('2d');
for(let i=0;i<4;i++){ctx.save();ctx.translate(i%2*384,Math.floor(i/2)*256);ctx.drawImage(bg,0,0,384,256);c.WhiteBirdView.draw(ctx,{stage:i<2?'adult':'baby',healthState:kind==='sick'?'sick':'healthy',sleepMode:'awake'},kind==='sick'?(i%2?5800:4200):(i%2?2300:0),null,0,true,190,126,3);ctx.restore();}
fs.writeFileSync(path.join(root,'assets/pets/bird/white-'+kind+'-contact-v1.png'),out.toBuffer('image/png'));
})();
