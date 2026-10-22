// Export real shared pet/effect rendering. PNG contact sheets are not browser captures.
const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),vm=require('node:vm');
const {createCanvas,loadImage}=require('@napi-rs/canvas');
const root=path.resolve(__dirname,'../web-preview'),dest=path.join(root,'assets/shared/cleaning/animation-v1');
(async()=>{
 const pending=[];
 class AssetImage{constructor(){this.complete=false;}set src(file){this.file=file;pending.push(loadImage(path.join(root,file)).then(im=>{this.native=im;this.naturalWidth=im.width;this.naturalHeight=im.height;this.complete=true;}));}}
 const c={Image:AssetImage,document:{querySelector:()=>null,addEventListener(){}},PetRules:{updateSad:()=>false},matchMedia:()=>({matches:true})};c.window=c;vm.createContext(c);
 for(const f of ['assets/pets/bird/white-idle-data-v4.js','assets/pets/bird/white-sad-data-v1.js','assets/pets/bird/white-sick-data-v1.js','white-bird-view.js','wyvern-sprites.js','wyvern-adult-view.js','wyvern-view.js','assets/shared/cleaning/animation-v1/animation-data.js','assets/shared/cleaning/foam-study-v2/foam-data.js','cleaning-animation-view.js','cleaning-animation-scene.js'])vm.runInContext(fs.readFileSync(path.join(root,f),'utf8'),c);
 c.CleaningAnimationView.ready();await Promise.all(pending);if(!c.CleaningAnimationView.ready())throw Error('An effect image did not load');
 function proxy(ctx){return new Proxy(ctx,{get(t,k){if(k==='drawImage')return (im,...args)=>t.drawImage(im.native||im,...args);const v=t[k];return typeof v==='function'?v.bind(t):v;},set(t,k,v){t[k]=v;return true;}});}
 const day=await loadImage(path.join(root,'assets/garden-detailed-day.png')),night=await loadImage(path.join(root,'assets/garden-detailed-night.png'));
 const moments=[.15,.7,1.35,2.15,2.75,3.8],captions=['Enter','Scrub','Foam','Rinse','Wash off','Finish'];
 for(const [species,stage,label] of [['bird','adult','adult-bird'],['wyvern','adult','adult-wyvern']]){
  const out=createCanvas(1152,584),ctx=out.getContext('2d');
  moments.forEach((t,i)=>{ctx.save();ctx.translate(i%3*384,Math.floor(i/3)*292);ctx.fillStyle='#f7f5ed';ctx.fillRect(0,0,384,292);ctx.fillStyle='#403e48';ctx.font='15px sans-serif';ctx.fillText(captions[i]+' / '+t+' s',12,24);ctx.translate(0,36);ctx.drawImage(day,0,0,384,256);c.CleaningAnimationScene.draw(proxy(ctx),species,stage,t);ctx.restore();});
  fs.writeFileSync(path.join(dest,label+'-storyboard.png'),out.toBuffer('image/png'));
 }
 const all=createCanvas(1536,584),actx=all.getContext('2d'),pets=[['bird','baby'],['bird','adult'],['wyvern','baby'],['wyvern','adult']];
 for(let row=0;row<2;row++)pets.forEach(([species,stage],i)=>{actx.save();actx.translate(i*384,row*292);actx.fillStyle=row?'#353744':'#f7f5ed';actx.fillRect(0,0,384,292);actx.fillStyle=row?'#fff':'#403e48';actx.font='15px sans-serif';actx.fillText(species+' '+stage+' / '+(row?'night':'day'),12,24);actx.translate(0,36);actx.drawImage(row?night:day,0,0,384,256);c.CleaningAnimationScene.draw(proxy(actx),species,stage,1.25);actx.restore();});
 fs.writeFileSync(path.join(dest,'all-pets-day-night.png'),all.toBuffer('image/png'));
 const framesDir=fs.mkdtempSync(path.join(os.tmpdir(),'pet-cleaning-animation-'));
 for(const [species,stage] of pets){const folder=path.join(framesDir,species+'-'+stage);fs.mkdirSync(folder);const out=createCanvas(384,256),ctx=out.getContext('2d');for(let i=0;i<=53;i++){ctx.clearRect(0,0,384,256);ctx.drawImage(day,0,0,384,256);c.CleaningAnimationScene.draw(proxy(ctx),species,stage,Math.min(4.2,i*.08));fs.writeFileSync(path.join(folder,String(i).padStart(3,'0')+'.png'),out.toBuffer('image/png'));}}
 console.log(JSON.stringify({framesDir,framesPerPet:54,frameMs:80,pets:pets.map(([species,stage])=>species+'-'+stage),destination:dest}));
})().catch(e=>{console.error(e);process.exitCode=1;});
