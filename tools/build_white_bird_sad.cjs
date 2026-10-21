const sharp=require('sharp'),fs=require('node:fs'),path=require('node:path');
const kind=process.argv[2]||'sad';
if(!['sad','sick'].includes(kind))throw Error('Invalid expression');
const dir=path.resolve(__dirname,'../web-preview/assets/pets/bird');
(async()=>{
 const source=path.join(dir,'white-'+kind+'-source-v1.png');
 const {data,info}=await sharp(source).ensureAlpha().raw().toBuffer({resolveWithObject:true});
 const frames=[];
 for(let i=0;i<2;i++){
  const start=Math.floor(i*info.width/2),end=Math.floor((i+1)*info.width/2);
  let x0=end,y0=info.height,x1=-1,y1=-1;
  for(let y=0;y<info.height;y++)for(let x=start;x<end;x++)if(data[(y*info.width+x)*4+3]>=160){x0=Math.min(x0,x);x1=Math.max(x1,x);y0=Math.min(y0,y);y1=Math.max(y1,y);}
  if(x1<0)throw Error('Missing expression');
  let hx0=end,hx1=0;
  for(let y=y0;y<y0+(y1-y0)*.15;y++)for(let x=x0;x<=x1;x++)if(data[(y*info.width+x)*4+3]>=160){hx0=Math.min(hx0,x);hx1=Math.max(hx1,x);}
  frames.push({crop:{left:x0,top:y0,width:x1-x0+1,height:y1-y0+1},head:Math.round((hx0+hx1)/2)-x0});
 }
 const ax=Math.max(...frames.map(f=>f.head))+24,bottom=Math.max(...frames.map(f=>f.crop.height))+24;
 const cell=Math.max(bottom+24,ax+Math.max(...frames.map(f=>f.crop.width-f.head))+24),layers=[];
 for(let i=0;i<2;i++){const f=frames[i];f.offset={left:ax-f.head,top:bottom-f.crop.height};layers.push({input:await sharp(source).extract(f.crop).png().toBuffer(),left:i*cell+f.offset.left,top:f.offset.top});}
 await sharp({create:{width:cell*2,height:cell,channels:4,background:{r:0,g:0,b:0,alpha:0}}}).composite(layers).png().toFile(path.join(dir,'white-'+kind+'-sheet-v1.png'));
 const meta={cell,anchor:[ax,bottom],visibleHeight:Math.max(...frames.map(f=>f.crop.height)),times:kind==='sad'?[2200,240]:[1400,900],frames};
 fs.writeFileSync(path.join(dir,'white-'+kind+'-data-v1.js'),'window.WhiteBird'+(kind==='sad'?'Sad':'Sick')+'Data='+JSON.stringify(meta)+';\n');
 fs.writeFileSync(path.join(dir,'white-'+kind+'-data-v1.json'),JSON.stringify(meta,null,2)+'\n');console.log(JSON.stringify(meta));
})();
