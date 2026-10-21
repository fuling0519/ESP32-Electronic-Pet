const sharp=require('sharp'),fs=require('node:fs'),path=require('node:path');
const dir=path.resolve(__dirname,'../web-preview/assets/items');
(async()=>{
 const source=path.join(dir,'blue-seed-bowl-source-v1.png');
 const {data,info}=await sharp(source).ensureAlpha().raw().toBuffer({resolveWithObject:true});
 const frames=[];
 for(let i=0;i<4;i++){
  const sx=Math.floor(i%2*info.width/2),sy=i<2?0:520,ex=Math.floor((i%2+1)*info.width/2),ey=i<2?520:info.height;
  let x0=ex,y0=ey,x1=-1,y1=-1;
  for(let y=sy;y<ey;y++)for(let x=sx;x<ex;x++)if(data[(y*info.width+x)*4+3]>=160){x0=Math.min(x0,x);x1=Math.max(x1,x);y0=Math.min(y0,y);y1=Math.max(y1,y);}
  if(x1<0)throw Error('Empty bowl');frames.push({crop:{left:x0,top:y0,width:x1-x0+1,height:y1-y0+1}});
 }
 const width=Math.max(...frames.map(f=>f.crop.width))+48,height=Math.max(...frames.map(f=>f.crop.height))+48,anchor=[Math.floor(width/2),height-24],layers=[];
 for(let i=0;i<4;i++){const f=frames[i];f.offset={left:anchor[0]-Math.round(f.crop.width/2),top:anchor[1]-f.crop.height};layers.push({input:await sharp(source).extract(f.crop).png().toBuffer(),left:i%2*width+f.offset.left,top:Math.floor(i/2)*height+f.offset.top});}
 await sharp({create:{width:width*2,height:height*2,channels:4,background:{r:0,g:0,b:0,alpha:0}}}).composite(layers).png().toFile(path.join(dir,'blue-seed-bowl-sheet-v1.png'));
 const meta={width,height,anchor,visibleWidth:Math.max(...frames.map(f=>f.crop.width)),frames};
 fs.writeFileSync(path.join(dir,'blue-seed-bowl-data-v1.js'),'window.BlueSeedBowlData='+JSON.stringify(meta)+';\n');fs.writeFileSync(path.join(dir,'blue-seed-bowl-data-v1.json'),JSON.stringify(meta,null,2)+'\n');console.log(JSON.stringify(meta));
})();
