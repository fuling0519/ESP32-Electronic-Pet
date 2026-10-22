// Render the same static palette study and pet players as the browser preview.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const {createCanvas,loadImage}=require('@napi-rs/canvas');
const version=process.argv[2]||'v1';if(!['v1','v2','v3','v4'].includes(version))throw Error('Use v1, v2, v3 or v4');
const root=path.resolve(__dirname,'../web-preview'),dest=path.join(root,'assets/shared/cleaning/style-study-'+version);
(async()=>{
 const pending=[];
 class AssetImage{
  constructor(){this.complete=false;}
  set src(file){this.file=file;pending.push(loadImage(path.join(root,file)).then(im=>{this.native=im;this.naturalWidth=im.width;this.naturalHeight=im.height;this.complete=true;}));}
 }
 const context={Image:AssetImage,document:{querySelector:()=>null,addEventListener(){}},PetRules:{updateSad:()=>false},matchMedia:()=>({matches:true})};context.window=context;vm.createContext(context);
 const study=version!=='v1'?['assets/shared/cleaning/style-study-v2/study-data-v2.js',...(['v3','v4'].includes(version)?['assets/shared/cleaning/style-study-'+version+'/study-settings-'+version+'.js']:[]),'cleaning-style-study-v2.js']:['cleaning-style-study.js'];
 for(const file of ['assets/pets/bird/white-idle-data-v4.js','assets/pets/bird/white-sad-data-v1.js','assets/pets/bird/white-sick-data-v1.js','white-bird-view.js','wyvern-sprites.js','wyvern-adult-view.js','wyvern-view.js',...study])vm.runInContext(fs.readFileSync(path.join(root,file),'utf8'),context);
 await Promise.all(pending);
 const out=createCanvas(1536,612),ctx=out.getContext('2d');
 const wrap=()=>new Proxy(ctx,{get(target,key){if(key==='drawImage')return (im,...args)=>target.drawImage(im.native||im,...args);const value=target[key];return typeof value==='function'?value.bind(target):value;},set(target,key,value){target[key]=value;return true;}});
 const titles=[['bird','baby','Baby bird'],['bird','adult','Adult bird'],['wyvern','baby','Baby wyvern (current mask)'],['wyvern','adult','Adult wyvern']];
 for(let row=0;row<2;row++){
  const bg=await loadImage(path.join(root,'assets/garden-detailed-'+(row?'night':'day')+'.png'));
  for(let i=0;i<titles.length;i++){
   const [species,stage,label]=titles[i],x=i*384,y=row*306;
   ctx.save();ctx.translate(x,y);ctx.fillStyle=row?'#353744':'#f7f5ed';ctx.fillRect(0,0,384,306);ctx.fillStyle=row?'#f7f5ed':'#403e48';ctx.font='16px sans-serif';ctx.fillText(label+' / '+(row?'night':'day'),12,24);
   ctx.translate(0,40);ctx.drawImage(bg,0,0,384,256);context.CleaningStyleStudy.scene(wrap(),species,stage);ctx.restore();
  }
 }
 fs.writeFileSync(path.join(dest,'scene-contact-'+version+'.png'),out.toBuffer('image/png'));
 const detail=createCanvas(version!=='v1'?1000:550,version==='v4'?450:version!=='v1'?350:128),dctx=detail.getContext('2d');
 for(let y=0;y<detail.height;y+=8)for(let x=0;x<detail.width;x+=8){dctx.fillStyle=((x+y)/8)%2?'#eee':'#fff';dctx.fillRect(x,y,8,8);}
 const pctx=new Proxy(dctx,{get(t,k){if(k==='drawImage')return (im,...a)=>t.drawImage(im.native||im,...a);const v=t[k];return typeof v==='function'?v.bind(t):v;},set(t,k,v){t[k]=v;return true;}});
 context.CleaningStyleStudy.samples(pctx);fs.writeFileSync(path.join(dest,'size-contact-'+version+'.png'),detail.toBuffer('image/png'));
 if(version!=='v1'){
  ctx.clearRect(0,0,out.width,out.height);const bg=await loadImage(path.join(root,'assets/garden-detailed-day.png'));
  for(let row=0;row<2;row++)for(let stage=0;stage<3;stage++){
   ctx.save();ctx.translate(stage*512,row*306);ctx.fillStyle='#f7f5ed';ctx.fillRect(0,0,512,306);ctx.fillStyle='#403e48';ctx.font='16px sans-serif';ctx.fillText((row?'Adult wyvern':'Adult bird')+' / '+['Low','Medium','High'][stage]+' foam',16,25);
   ctx.translate(64,40);ctx.drawImage(bg,0,0,384,256);context.CleaningStyleStudy.scene(wrap(),row?'wyvern':'bird','adult',{foamStage:stage});ctx.restore();
  }
  fs.writeFileSync(path.join(dest,'quantity-scenes-'+version+'.png'),out.toBuffer('image/png'));
 }
 console.log('Rendered '+version+' with existing pet players; static study only.');
})().catch(error=>{console.error(error);process.exitCode=1;});
