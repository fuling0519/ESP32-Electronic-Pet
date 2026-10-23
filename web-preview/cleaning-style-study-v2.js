/* Static lifted-soap and foam-quantity study; no care actions or persistence. */
(()=>{'use strict';
const base='assets/shared/cleaning/style-study-v2/',d=CleaningStyleStudyV2Data,settings=window.CleaningStyleStudySettings||{};
const toolData=settings.tool||d.soap,defaultWidth=settings.defaultWidth||40;
const soap=new Image(),foam=new Image();
soap.src=settings.file||base+'soap-sample-v2.png';foam.src=base+'foam-sheet-v2.png';
function soapAt(ctx,x,bottom,width){
 if(!soap.complete||!soap.naturalWidth)return;
 ctx.drawImage(soap,Math.round(x-width/2),Math.round(bottom-width*toolData.height/toolData.width),width,Math.round(width*toolData.height/toolData.width));
}
function foamAt(ctx,index,x,y,width){
 if(!foam.complete||!foam.naturalWidth)return;
 const i=Math.max(0,Math.min(2,index)),size=d.foam.cell*width/d.foam.highVisibleWidth;
 ctx.drawImage(foam,i*d.foam.cell,0,d.foam.cell,d.foam.cell,Math.round(x-size/2),Math.round(y-size/2),size,size);
}
function effects(ctx,{stage='adult',species='bird',bottom=177,soapWidth=defaultWidth,foamWidth=64,foamStage=1,visible=true}={}){
 if(!visible)return;
 const ratio=stage==='baby'?.82:1,bodyY=bottom-(species==='bird'?34:30)*ratio;
 foamAt(ctx,foamStage,190,bodyY,foamWidth*ratio);
 soapAt(ctx,settings.toolX??205,bodyY+(settings.bottomOffset??5),Math.round(soapWidth*(settings.shrinkTool===false?1:ratio)));
}
function scene(ctx,species,stage,options={}){
 const p={stage,healthState:'healthy',sleepMode:'awake',satiety:100,mood:80,cleanliness:100,speciesId:species==='bird'?1:2};
 ctx.save();ctx.imageSmoothingEnabled=false;
 if(species==='bird')WhiteBirdView.draw(ctx,p,0,'clean',0,false,190,126,3);
 else if(WyvernAdultView.paint(ctx,'idle',0,true,{stage}));
 else WyvernView.draw(ctx,p,0,'clean',0,true);
 const bottom=species==='bird'?126+(stage==='adult'?132:106)*.47:177;
 effects(ctx,{...options,species,stage,bottom});ctx.restore();
}
function samples(ctx){
 ctx.save();ctx.imageSmoothingEnabled=false;ctx.fillStyle='#403e48';ctx.font='14px sans-serif';
 const xs=settings.file?[130,350,570,850]:[100,310,550,830];
 for(let i=0;i<4;i++){
  const width=i?Math.round(64*d.foam.frames[i-1].visibleWidth/d.foam.highVisibleWidth):defaultWidth;
  const name=i?['Low','Medium','High'][i-1]:(settings.label||'Soap');
  ctx.fillText(name+' / '+width+' px',xs[i]-45,24);
  if(!i){soapAt(ctx,xs[i],80,defaultWidth);soapAt(ctx,xs[i],settings.sampleBottom??310,defaultWidth*4);}
  else{foamAt(ctx,i-1,xs[i],60,64);foamAt(ctx,i-1,xs[i],(settings.sampleBottom??310)-95,256);}
 }
 ctx.fillText('Actual scene pixels above; 4x detail below',12,120);ctx.restore();
}
window.CleaningStyleStudy={scene,effects,samples};
})();
