/* Package six generated full-body poses; crop/place only, one scale throughout. */
const sharp=require('sharp'),fs=require('node:fs'),path=require('node:path');
const dir=path.resolve(__dirname,'../web-preview/assets/pets/bird');
(async()=>{
 const source=path.join(dir,'white-activity-source-v1.png');
 const {data,info}=await sharp(source).ensureAlpha().raw().toBuffer({resolveWithObject:true});
 const frames=[];
 for(let i=0;i<6;i++){
  const left=Math.floor(i%2*info.width/2),top=Math.floor(Math.floor(i/2)*info.height/3),right=Math.floor((i%2+1)*info.width/2),bottom=Math.floor((Math.floor(i/2)+1)*info.height/3);
  let x0=right,y0=bottom,x1=-1,y1=-1;
  for(let y=top;y<bottom;y++)for(let x=left;x<right;x++)if(data[(y*info.width+x)*4+3]>=160){x0=Math.min(x0,x);y0=Math.min(y0,y);x1=Math.max(x1,x);y1=Math.max(y1,y);}
  if(x1<0)throw Error('Missing pose '+i);
  frames.push({crop:{left:x0,top:y0,width:x1-x0+1,height:y1-y0+1}});
 }
 const cell=Math.max(...frames.map(f=>Math.max(f.crop.width,f.crop.height)))+48,anchor=[Math.floor(cell/2),cell-24],layers=[];
 for(let i=0;i<6;i++){
  const f=frames[i];f.offset={left:anchor[0]-Math.round(f.crop.width/2),top:anchor[1]-f.crop.height};
  layers.push({input:await sharp(source).extract(f.crop).png().toBuffer(),left:i%2*cell+f.offset.left,top:Math.floor(i/2)*cell+f.offset.top});
 }
 await sharp({create:{width:cell*2,height:cell*3,channels:4,background:{r:0,g:0,b:0,alpha:0}}}).composite(layers).png().toFile(path.join(dir,'white-activity-sheet-v1.png'));
 const meta={cell,anchor,standingHeight:frames[0].crop.height,frames};
 fs.writeFileSync(path.join(dir,'white-activity-data-v1.js'),'window.WhiteBirdActivityData='+JSON.stringify(meta)+';\n');
 fs.writeFileSync(path.join(dir,'white-activity-data-v1.json'),JSON.stringify(meta,null,2)+'\n');
 console.log(JSON.stringify(meta));
})();
