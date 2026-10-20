(()=>{'use strict';
function resolve(s,key,selected=0,repeat=false){
 if(!s||repeat)return null;
 if(s.kind==='rps'&&s.phase==='choose'){
  if(key==='ArrowLeft'||key==='ArrowRight')return {select:(selected+(key==='ArrowLeft'?2:1))%3};
  if(key==='Enter')return {answer:selected};
 }
 if(s.kind==='memory'&&s.phase==='answer'){
  const i=['ArrowUp','ArrowRight','ArrowDown','ArrowLeft'].indexOf(key);if(i>=0)return {answer:i};
 }
 return null;
}
window.PetGameKeys={resolve};
})();
