(()=>{'use strict';
const dialog=document.querySelector('#game-dialog'),message=document.querySelector('#game-message'),inputs=document.querySelector('#game-inputs'),next=document.querySelector('#game-next');
let timer=null,lastLevel=false,toneKey=null,audio=null;const activeTones=new Set();
const resultArt=document.createElement('div');message.after(resultArt);
resultArt.className='game-duel';
const stageBoard=document.createElement('div');stageBoard.className='game-stage';message.before(stageBoard);
const gameTitle=document.querySelector('#game-dialog h2');
function handIcon(index){const art=document.createElement('span');art.className='game-hand';art.style.backgroundPosition=(index*50)+'% center';art.setAttribute('aria-hidden','true');return art;}
let selectedMove=0;
const keyboardHint=document.createElement('p');keyboardHint.textContent='鍵盤：猜拳左右選招、Enter確認；記憶音符用方向鍵作答。Esc退出。';message.before(keyboardHint);
document.addEventListener('keydown',event=>{
 if(!dialog.open||event.isComposing||event.altKey||event.ctrlKey||event.metaKey||!window.PetGameKeys)return;
 if(['INPUT','SELECT','TEXTAREA'].includes(event.target?.tagName))return;
 if(event.key==='Enter'&&event.target?.tagName==='BUTTON'&&!Array.from(inputs.children).includes(event.target))return;
 const s=petDataManager.gameState(performance.now()),decision=PetGameKeys.resolve(s,event.key,selectedMove,event.repeat);
 if(!decision)return;event.preventDefault();initAudio();
 if(decision.select!=null){selectedMove=decision.select;inputs.children[selectedMove]?.focus();render();return;}
 if(s.kind==='memory')tone([587,659,784,523][decision.answer],180);
 petDataManager.answerGame(decision.answer,performance.now());render();
});
let sound=PetLocalStore.read('sound',x=>typeof x==='boolean')??true;
const soundButton=document.querySelector('#web-sound');
// Speaker icon; muted adds a cross instead of the sound waves.
function soundLabel(){const label='網頁聲音：'+(sound?'開':'關');soundButton.innerHTML='<svg viewBox="0 0 16 16" width="18" height="18" aria-hidden="true"><path d="M2 6h3l4-3v10l-4-3H2z" fill="currentColor"/>'+(sound?'<path d="M11 5.5q2 2.5 0 5M12.5 3.5q3.5 4.5 0 9" fill="none" stroke="currentColor" stroke-width="1.5"/>':'<path d="M11 5.5l4 5M15 5.5l-4 5" stroke="currentColor" stroke-width="1.5"/>')+'</svg>';soundButton.setAttribute('aria-pressed',String(sound));soundButton.setAttribute('aria-label',label);soundButton.title=label;}
function initAudio(){try{const Audio=window.AudioContext||window.webkitAudioContext;if(Audio){audio??=new Audio();audio.resume().catch(()=>{});}}catch{audio=null;}}
function tone(freq,ms){if(!sound||!audio||document.hidden)return;try{const oscillator=audio.createOscillator(),gain=audio.createGain();oscillator.frequency.value=freq;gain.gain.value=.07;oscillator.connect(gain);gain.connect(audio.destination);activeTones.add(oscillator);oscillator.onended=()=>{activeTones.delete(oscillator);oscillator.disconnect();gain.disconnect();};oscillator.start();oscillator.stop(audio.currentTime+ms/1000);}catch{}}
soundButton.onclick=()=>{sound=!sound;if(!sound)for(const oscillator of activeTones)try{oscillator.stop();}catch{};PetLocalStore.write('sound',sound,true);initAudio();if(sound)tone(1047,120);soundLabel();};soundLabel();

function render(){
 const s=petDataManager.gameState(performance.now());if(!s)return;
 if(s.kind==='memory'&&s.phase==='show'&&s.lit!=null){const key=s.round+':'+s.demoIndex;if(key!==toneKey){toneKey=key;tone([587,659,784,523][s.lit],250);}}
 const names=s.kind==='rps'?['石頭','布','剪刀']:['↑','→','↓','←'];
 dialog.dataset.kind=s.kind;dialog.dataset.phase=s.phase;
 gameTitle.textContent=s.kind==='rps'?'猜拳小擂台':'雲朵記憶音符';
 stageBoard.textContent=s.phase==='prepare'?'☁ '+s.prepareSeconds+' ♫':s.phase==='countdown'?String(s.countdown):s.phase==='complete'?'✧ 完成挑戰 ✧':s.kind==='rps'?'✊  ×  ✋  ×  ✌':s.phase==='show'&&s.lit!=null?'☁ '+names[s.lit]+' ♫':'☁  ♫  ☀';
 stageBoard.className='game-stage'+(s.phase==='countdown'?' is-countdown':'')+(s.kind==='memory'&&s.phase==='show'&&s.lit!=null?' is-note-lit':'');
 if(s.phase==='prepare')message.textContent='準備一下，5秒後開始！第一回合記住2個音符。';else if(s.phase==='show')message.textContent='第 '+s.round+'／3 回合 · 記住'+s.noteCount+'個音符：'+(s.lit==null?'·':names[s.lit]);
 else if(s.phase==='answer')message.textContent='第 '+s.round+'／3 回合 · 回答第 '+(s.index+1)+' 個方向（每步 5 秒）';
 else if(s.phase==='choose')message.textContent='第 '+s.round+'／3 回合 · 選擇出招';
 else if(s.phase==='countdown')message.textContent='準備揭曉！';
 else if(s.phase==='last-note')message.textContent='最後一個音符……';
 else if(s.phase==='round-result')message.textContent=s.kind==='rps'?names[s.player]+' 對 '+names[s.opponent]+' · '+{win:'贏了',draw:'平手',loss:'輸了'}[s.outcome]:s.correct?'答對！':'答錯或逾時，下回合再試。';
 else if(s.phase==='complete'){message.textContent='遊戲完成 · 心情 +'+s.reward.mood+' · EXP +'+s.reward.exp+(s.levelsGained?' · 升級！':'');if(!lastLevel){document.dispatchEvent(new Event('pet-game-complete'));lastLevel=true;}}
 else message.textContent='目前狀態已改變，遊戲中止，未發獎勵。';
 next.hidden=s.phase!=='round-result';next.disabled=!s.canContinue;
 resultArt.replaceChildren();
 if(s.kind==='rps'&&['round-result','complete'].includes(s.phase)&&s.player!=null){
  for(const [label,move] of [['你',s.player],['對手',s.opponent]]){
   const card=document.createElement('div');card.className='duel-card';const caption=document.createElement('strong');caption.textContent=label+' · '+names[move];card.append(handIcon(move),caption);resultArt.append(card);
  }
 }
 if(inputs.children.length!==names.length){inputs.replaceChildren();names.forEach((name,i)=>{const b=document.createElement('button');b.textContent=name;if(s.kind==='rps')b.prepend(handIcon(i));else{const note=document.createElement('span');note.className='cloud-note';note.textContent=['☁ ♫','☀ ♪','☁ ♩','☾ ♬'][i];b.prepend(note);}if(s.kind==='rps'&&window.SharedPetView){const art=document.createElement('canvas');art.width=72;art.height=54;art.style.imageRendering='pixelated';art.setAttribute('aria-hidden','true');SharedPetView.paint(art.getContext('2d'),['kRock','kPaper','kScissors'][i],0,0,0,3,'#57483a');b.prepend(art);}b.onclick=()=>{if(s.kind==='memory'&&petDataManager.gameState(performance.now())?.phase==='answer')tone([587,659,784,523][i],180);petDataManager.answerGame(i,performance.now());render();};inputs.append(b);});}
 for(const [i,b] of Array.from(inputs.children).entries()){b.className=s.kind==='memory'&&s.phase==='show'&&s.lit===i?'note-active':'';b.disabled=!['choose','answer'].includes(s.phase);if(s.kind==='rps')b.setAttribute('aria-pressed',String(i===selectedMove));}
}
// Firmware death tune (D4 C#4 D4 B3) on a soft triangle with gentle fades instead of hard square steps.
function death(){if(!sound||!audio||audio.state!=='running'||document.hidden)return false;try{let at=audio.currentTime+.05;for(const [freq,ms] of [[294,500],[277,500],[294,500],[247,1000]]){const oscillator=audio.createOscillator(),gain=audio.createGain(),end=at+ms/1000;oscillator.type='triangle';oscillator.frequency.value=freq;gain.gain.setValueAtTime(0,at);gain.gain.linearRampToValueAtTime(.09,at+.04);gain.gain.setValueAtTime(.09,end-.12);gain.gain.linearRampToValueAtTime(0,end);oscillator.connect(gain);gain.connect(audio.destination);activeTones.add(oscillator);oscillator.onended=()=>{activeTones.delete(oscillator);oscillator.disconnect();gain.disconnect();};oscillator.start(at);oscillator.stop(end);at=end+.05;}return true;}catch{return false;}}
// Camera shutter for the farewell photo: a short low click, then a bright tick.
function shutter(){if(!sound||!audio||audio.state!=='running'||document.hidden)return false;tone(392,45);setTimeout(()=>tone(1568,70),70);return true;}
window.PetWebSound={death,shutter,reminder(){if(!sound||!audio||audio.state!=='running'||document.hidden||activeTones.size)return false;for(const [i,freq] of [659,523,392].entries())setTimeout(()=>tone(freq,140),i*180);return true;}};document.addEventListener('pointerdown',initAudio,{once:true});
function start(kind){try{initAudio();selectedMove=0;toneKey=null;lastLevel=false;petDataManager.startGame(kind,performance.now());document.querySelector('#game-picker').hidden=true;inputs.replaceChildren();render();if(kind==='rps')inputs.children[0]?.focus();clearInterval(timer);timer=setInterval(render,50);}catch(error){message.textContent=error.message;}}
document.querySelector('#start-rps').onclick=()=>start('rps');
// Toy play runs in the garden itself, so the picker closes first (closing cancels no toy session yet).
document.querySelector('#start-toy').onclick=()=>{dialog.close();if(window.PetToyPlay?.start())document.querySelector('#bird')?.scrollIntoView({block:'nearest'});};document.querySelector('#start-memory').onclick=()=>start('memory');next.onclick=()=>{petDataManager.nextRound(performance.now());render();};
// The close event arrives after a toy session has already started from the picker; leave that one running.
dialog.addEventListener('close',()=>{clearInterval(timer);if(petDataManager.gameState(performance.now())?.kind!=='toy')petDataManager.cancelGame();});
window.openPetGames=()=>{document.querySelector('#game-picker').hidden=false;inputs.replaceChildren();message.textContent='兩種寵物共用遊戲；完成整場只結算一次。';next.hidden=true;dialog.showModal();};
})();
