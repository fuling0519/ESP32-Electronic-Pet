/* Mock-only gestures. No raw adapter access or BLE writes. */
(()=>{'use strict';
const scene=document.querySelector('.scene'),canvas=document.querySelector('#bird');
const panel=document.createElement('section');panel.className='pet-interactions';panel.setAttribute('aria-label','寵物觸碰互動');
panel.innerHTML='<strong>觸碰互動</strong><button type="button" id="tool-pet" aria-pressed="true">✋ 摸頭／順毛</button><button type="button" id="tool-poke" aria-pressed="false">╱ 樹枝戳戳</button><button type="button" id="touch-once">互動一下</button><p id="touch-help">選工具後輕點角色，或在頭上滑一下順毛。每次心情＋3／－3，間隔3秒，不給EXP。</p><p id="touch-result" role="status" aria-live="polite"></p>';
document.querySelector('.care').after(panel);
const overlay=document.createElement('canvas');overlay.width=384;overlay.height=256;overlay.className='touch-effects';overlay.setAttribute('aria-hidden','true');scene.append(overlay);const pen=overlay.getContext('2d');
let tool='pet',down=null,effect=null,hover=null;
const result=document.querySelector('#touch-result'),petButton=document.querySelector('#tool-pet'),pokeButton=document.querySelector('#tool-poke'),once=document.querySelector('#touch-once');
function select(value){tool=value;petButton.setAttribute('aria-pressed',String(tool==='pet'));pokeButton.setAttribute('aria-pressed',String(tool==='poke'));canvas.style.cursor=tool==='pet'?'grab':'crosshair';}
petButton.onclick=()=>select('pet');pokeButton.onclick=()=>select('poke');select('pet');
function allowed(){const s=petDataManager.read(),p=s.pet;return s.metadata.source==='mock'&&p&&['baby','adult'].includes(p.stage)&&p.healthState==='healthy'&&p.sleepMode==='awake'&&!p.isDeparted&&!document.querySelector('[data-action="feed"]').disabled&&!document.querySelector('#game-dialog').open&&!document.querySelector('#farewell-dialog').open;}
function hit(p){return ((p.x-190)/100)**2+((p.y-130)/90)**2<=1;}
function point(e){const r=canvas.getBoundingClientRect();return {x:(e.clientX-r.left)*384/r.width,y:(e.clientY-r.top)*256/r.height};}
function interact(at={x:190,y:110}){if(!allowed()){result.textContent='目前不能觸碰：請在展示模式、清醒健康且動作結束後互動。';return;}
 try{const r=petDataManager.command(tool);effect={kind:tool,start:performance.now(),...at,petId:petDataManager.read().pet.petId};result.textContent=(tool==='pet'?'牠開心地接受摸摸，喜歡你的陪伴！':'牠不高興了，別再戳牠啦！')+' 心情 '+(r.actual>=0?'+':'')+r.actual+'。';document.dispatchEvent(new Event('pet-interaction-complete'));}catch(e){result.textContent=e.message;}}
once.onclick=()=>interact();
canvas.addEventListener('pointerdown',e=>{if(e.button!==0||!allowed())return;const p=point(e);if(!hit(p))return;down={...p,id:e.pointerId};canvas.setPointerCapture(e.pointerId);e.preventDefault();});
canvas.addEventListener('pointermove',e=>{hover=point(e);});
canvas.addEventListener('pointerup',e=>{if(!down||e.pointerId!==down.id)return;const p=point(e);down=null;if(hit(p))interact(p);});
canvas.addEventListener('pointercancel',()=>{down=null;});canvas.addEventListener('pointerleave',()=>{hover=null;});
function draw(now){pen.clearRect(0,0,384,256);once.disabled=!allowed();
 if(hover&&tool==='poke'&&allowed()){pen.strokeStyle='#795033';pen.lineWidth=3;pen.beginPath();pen.moveTo(hover.x+3,hover.y+3);pen.lineTo(hover.x+28,hover.y+32);pen.stroke();}
 if(effect){const p=petDataManager.read().pet,t=(now-effect.start)/1200;if(t>=1||!p||p.petId!==effect.petId||p.healthState==='dead'||p.isDeparted)effect=null;else{pen.globalAlpha=1-t;
 if(effect.kind==='pet'){pen.fillStyle='#ef9fb2';for(let i=0;i<3;i++){const x=Math.round(effect.x-20+i*20),y=Math.round(effect.y-20-t*28);for(const [dx,dy,w,h] of [[0,0,4,4],[6,0,4,4],[0,4,10,4],[2,8,6,3],[4,11,2,2]])pen.fillRect(x+dx,y+dy,w,h);}}
 else{pen.fillStyle='#c75348';const x=235,y=70;for(const [dx,dy,w,h] of [[0,0,3,10],[0,10,10,3],[17,0,3,10],[10,10,10,3],[0,18,3,10],[0,15,10,3],[17,18,3,10],[10,15,10,3]])pen.fillRect(x+dx,y+dy,w,h);pen.fillStyle='#664035';pen.fillRect(170,109,10,3);pen.fillRect(204,109,10,3);}
 pen.globalAlpha=1;}}
 requestAnimationFrame(draw);}
requestAnimationFrame(draw);
})();
