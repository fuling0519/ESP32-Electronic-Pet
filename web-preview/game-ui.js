(()=>{'use strict';
const dialog=document.querySelector('#game-dialog'),message=document.querySelector('#game-message'),inputs=document.querySelector('#game-inputs'),next=document.querySelector('#game-next');
let timer=null,lastLevel=false,toneKey=null,audio=null;const activeTones=new Set();
let sound=PetLocalStore.read('sound',x=>typeof x==='boolean')??true;
const soundButton=document.querySelector('#web-sound');
function soundLabel(){soundButton.textContent='網頁聲音：'+(sound?'開':'關');soundButton.setAttribute('aria-pressed',String(sound));}
function initAudio(){try{const Audio=window.AudioContext||window.webkitAudioContext;if(Audio){audio??=new Audio();audio.resume().catch(()=>{});}}catch{audio=null;}}
function tone(freq,ms){if(!sound||!audio||document.hidden)return;try{const oscillator=audio.createOscillator(),gain=audio.createGain();oscillator.frequency.value=freq;gain.gain.value=.07;oscillator.connect(gain);gain.connect(audio.destination);activeTones.add(oscillator);oscillator.onended=()=>{activeTones.delete(oscillator);oscillator.disconnect();gain.disconnect();};oscillator.start();oscillator.stop(audio.currentTime+ms/1000);}catch{}}
soundButton.onclick=()=>{sound=!sound;if(!sound)for(const oscillator of activeTones)try{oscillator.stop();}catch{};PetLocalStore.write('sound',sound,true);initAudio();if(sound)tone(1047,120);soundLabel();};soundLabel();

function render(){
 const s=petDataManager.gameState(performance.now());if(!s)return;
 if(s.kind==='memory'&&s.phase==='show'&&s.lit!=null){const key=s.round+':'+s.demoIndex;if(key!==toneKey){toneKey=key;tone([587,659,784,523][s.lit],250);}}
 const names=s.kind==='rps'?['石頭','布','剪刀']:['↑','→','↓','←'];
 if(s.phase==='show')message.textContent='第 '+s.round+'／5 回合 · 記住箭頭順序：'+(s.lit==null?'·':names[s.lit]);
 else if(s.phase==='answer')message.textContent='第 '+s.round+'／5 回合 · 回答第 '+(s.index+1)+' 個方向（每步 5 秒）';
 else if(s.phase==='choose')message.textContent='第 '+s.round+'／3 回合 · 選擇出招';
 else if(s.phase==='countdown')message.textContent='剪刀、石頭、布……';
 else if(s.phase==='last-note')message.textContent='最後一個音符……';
 else if(s.phase==='round-result')message.textContent=s.kind==='rps'?names[s.player]+' 對 '+names[s.opponent]+' · '+{win:'贏了',draw:'平手',loss:'輸了'}[s.outcome]:s.correct?'答對！':'答錯或逾時，下回合再試。';
 else if(s.phase==='complete'){message.textContent='遊戲完成 · 心情 +'+s.reward.mood+' · EXP +'+s.reward.exp+(s.levelsGained?' · 升級！':'');if(!lastLevel){document.dispatchEvent(new Event('pet-game-complete'));lastLevel=true;}}
 else message.textContent='目前狀態已改變，遊戲中止，未發獎勵。';
 next.hidden=s.phase!=='round-result';next.disabled=!s.canContinue;
 if(inputs.children.length!==names.length){inputs.replaceChildren();names.forEach((name,i)=>{const b=document.createElement('button');b.textContent=name;b.onclick=()=>{if(s.kind==='memory'&&petDataManager.gameState(performance.now())?.phase==='answer')tone([587,659,784,523][i],180);petDataManager.answerGame(i,performance.now());render();};inputs.append(b);});}
 for(const b of inputs.children)b.disabled=!['choose','answer'].includes(s.phase);
}
window.PetWebSound={reminder(){if(!sound||!audio||audio.state!=='running'||document.hidden||activeTones.size)return false;for(const [i,freq] of [659,523,392].entries())setTimeout(()=>tone(freq,140),i*180);return true;}};document.addEventListener('pointerdown',initAudio,{once:true});
function start(kind){try{initAudio();toneKey=null;lastLevel=false;petDataManager.startGame(kind,performance.now());document.querySelector('#game-picker').hidden=true;inputs.replaceChildren();render();clearInterval(timer);timer=setInterval(render,50);}catch(error){message.textContent=error.message;}}
document.querySelector('#start-rps').onclick=()=>start('rps');document.querySelector('#start-memory').onclick=()=>start('memory');next.onclick=()=>{petDataManager.nextRound(performance.now());render();};
dialog.addEventListener('close',()=>{clearInterval(timer);petDataManager.cancelGame();});
window.openPetGames=()=>{document.querySelector('#game-picker').hidden=false;inputs.replaceChildren();message.textContent='兩種寵物共用遊戲；完成整場只結算一次。';next.hidden=true;dialog.showModal();};
})();
