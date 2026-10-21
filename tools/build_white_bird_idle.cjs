/* Lossless slicing and integer placement only; never rescale source frames. */
const sharp=require('sharp'),fs=require('node:fs'),path=require('node:path');
const version=process.argv[2]||'v3';
if(!/^v[0-9]+$/.test(version))throw Error('Invalid version');
const dir=path.resolve(__dirname,'../web-preview/assets/pets/bird');
(async()=>{
 const input=path.join(dir,'white-idle-source-'+version+'.png');
 const {data,info}=await sharp(input).ensureAlpha().raw().toBuffer({resolveWithObject:true});
 const cuts=[[0,0,645,640],[645,0,info.width-645,640],[0,640,645,info.height-640],[645,640,info.width-645,info.height-640]];
 const frames=[];
 for(const [left,top,width,height] of cuts){
  let x0=width,y0=height,x1=-1,y1=-1;
  for(let y=0;y<height;y++)for(let x=0;x<width;x++)if(data[((top+y)*info.width+left+x)*4+3]>=160){x0=Math.min(x0,x);x1=Math.max(x1,x);y0=Math.min(y0,y);y1=Math.max(y1,y);}
  if(x1<0)throw Error('Empty frame');
  // Head center is measured in the top 15% of the character, excluding the tail.
  let hx0=width,hx1=0;
  for(let y=y0;y<y0+(y1-y0)*.15;y++)for(let x=x0;x<=x1;x++)if(data[((top+y)*info.width+left+x)*4+3]>=160){hx0=Math.min(hx0,x);hx1=Math.max(hx1,x);}
  frames.push({crop:{left:left+x0,top:top+y0,width:x1-x0+1,height:y1-y0+1},head:Math.round((hx0+hx1)/2)-x0});
 }
 const anchorX=Math.max(...frames.map(f=>f.head))+24;
 const bottom=Math.max(...frames.map(f=>f.crop.height))+24;
 const cell=Math.max(bottom+24,anchorX+Math.max(...frames.map(f=>f.crop.width-f.head))+24);
 const layers=[];
 for(let i=0;i<4;i++){
  const f=frames[i];f.offset={left:anchorX-f.head,top:bottom-f.crop.height};
  const input=await sharp(path.join(dir,'white-idle-source-'+version+'.png')).extract(f.crop).png().toBuffer();
  layers.push({input,left:(i%2)*cell+f.offset.left,top:Math.floor(i/2)*cell+f.offset.top});
 }
 await sharp({create:{width:cell*2,height:cell*2,channels:4,background:{r:0,g:0,b:0,alpha:0}}}).composite(layers).png().toFile(path.join(dir,'white-idle-sheet-'+version+'.png'));
 const meta={cell,anchor:[anchorX,bottom],visibleHeight:Math.max(...frames.map(f=>f.crop.height)),times:[220,160,220,320],frames};
 fs.writeFileSync(path.join(dir,'white-idle-sheet-'+version+'.json'),JSON.stringify(meta,null,2)+'\n');
 fs.writeFileSync(path.join(dir,'white-idle-data-'+version+'.js'),'window.WhiteBirdIdleData='+JSON.stringify(meta)+';\n');
 console.log(JSON.stringify(meta));
})();
