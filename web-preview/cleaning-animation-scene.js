/* The preview and exported scenes use the existing pet renderers. */
(()=>{'use strict';
function draw(ctx,species,stage,t,reduced=false){
 const p={stage,healthState:'healthy',sleepMode:'awake',satiety:100,mood:80,cleanliness:100,speciesId:species==='bird'?1:2};
 ctx.save();ctx.imageSmoothingEnabled=false;
 if(species==='bird')WhiteBirdView.draw(ctx,p,0,'clean',0,false,190,126,3);
 else if(WyvernAdultView.paint(ctx,'idle',0,true,{stage}));
 else WyvernView.draw(ctx,p,0,'clean',0,true);
 CleaningAnimationView.draw(ctx,t,{species,stage,reduced});ctx.restore();
}
window.CleaningAnimationScene={draw};
})();
