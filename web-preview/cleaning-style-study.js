/* Static palette/size study only. Shared by the HTML and contact-sheet renderer. */
(()=>{'use strict';
const base='assets/shared/cleaning/style-study-v1/';
const soap=new Image(),foam=new Image();
soap.src=base+'soap-sample-v1.png';foam.src=base+'foam-sample-v1.png';
function prop(ctx,image,x,bottom,width){
 if(!image.complete||!image.naturalWidth)return false;
 const height=Math.round(width*image.naturalHeight/image.naturalWidth);
 ctx.save();ctx.imageSmoothingEnabled=false;
 ctx.drawImage(image,Math.round(x-width/2),Math.round(bottom-height),width,height);
 ctx.restore();return true;
}
function effects(ctx,{stage='adult',bottom=177,soapWidth=32,foamWidth=28,visible=true}={}){
 if(!visible)return;
 const ratio=stage==='baby'?.82:1,sw=Math.round(soapWidth*ratio),fw=Math.round(foamWidth*ratio);
 prop(ctx,foam,165,bottom-9,fw);prop(ctx,foam,190,bottom-3,fw);prop(ctx,foam,214,bottom-8,fw);
 prop(ctx,soap,211,bottom-23,sw);
}
function scene(ctx,species,stage,options={}){
 const p={stage,healthState:'healthy',sleepMode:'awake',satiety:100,mood:80,cleanliness:100,speciesId:species==='bird'?1:2};
 ctx.save();ctx.imageSmoothingEnabled=false;
 if(species==='bird')WhiteBirdView.draw(ctx,p,0,'clean',0,false,190,126,3);
 else if(stage==='adult')WyvernAdultView.paint(ctx,'idle',0,true);
 else WyvernView.draw(ctx,p,0,'clean',0,true);
 const bottom=species==='bird'?126+(stage==='adult'?132:106)*.47:177;
 effects(ctx,{...options,stage,bottom});ctx.restore();
}
function samples(ctx){
 prop(ctx,soap,64,65,32);prop(ctx,foam,150,65,28);
 prop(ctx,soap,286,106,128);prop(ctx,foam,448,106,112);
}
window.CleaningStyleStudy={scene,effects,samples};
})();
