/* The 陪玩 picker (toy play and the garden chorus both run in the garden itself) and the page's sound switch and tones. */
(()=>{'use strict';
const dialog=document.querySelector('#game-dialog'),message=document.querySelector('#game-message');
let audio=null;const activeTones=new Set();
let sound=PetLocalStore.read('sound',x=>typeof x==='boolean')??true;
const soundButton=document.querySelector('#web-sound');
// Speaker icon; muted adds a cross instead of the sound waves.
function soundLabel(){const label='網頁聲音：'+(sound?'開':'關');soundButton.innerHTML='<svg viewBox="0 0 16 16" width="18" height="18" aria-hidden="true"><path d="M2 6h3l4-3v10l-4-3H2z" fill="currentColor"/>'+(sound?'<path d="M11 5.5q2 2.5 0 5M12.5 3.5q3.5 4.5 0 9" fill="none" stroke="currentColor" stroke-width="1.5"/>':'<path d="M11 5.5l4 5M15 5.5l-4 5" stroke="currentColor" stroke-width="1.5"/>')+'</svg>';soundButton.setAttribute('aria-pressed',String(sound));soundButton.setAttribute('aria-label',label);soundButton.title=label;}
function initAudio(){try{const Audio=window.AudioContext||window.webkitAudioContext;if(Audio){audio??=new Audio();audio.resume().catch(()=>{});}}catch{audio=null;}}
function tone(freq,ms){if(!sound||!audio||document.hidden)return;try{const oscillator=audio.createOscillator(),gain=audio.createGain();oscillator.frequency.value=freq;gain.gain.value=.07;oscillator.connect(gain);gain.connect(audio.destination);activeTones.add(oscillator);oscillator.onended=()=>{activeTones.delete(oscillator);oscillator.disconnect();gain.disconnect();};oscillator.start();oscillator.stop(audio.currentTime+ms/1000);}catch{}}
soundButton.onclick=()=>{sound=!sound;if(!sound)for(const oscillator of activeTones)try{oscillator.stop();}catch{};PetLocalStore.write('sound',sound,true);initAudio();if(sound)tone(1047,120);soundLabel();};soundLabel();
// Firmware death tune (D4 C#4 D4 B3) on a soft triangle with gentle fades instead of hard square steps.
function death(){if(!sound||!audio||audio.state!=='running'||document.hidden)return false;try{let at=audio.currentTime+.05;for(const [freq,ms] of [[294,500],[277,500],[294,500],[247,1000]]){const oscillator=audio.createOscillator(),gain=audio.createGain(),end=at+ms/1000;oscillator.type='triangle';oscillator.frequency.value=freq;gain.gain.setValueAtTime(0,at);gain.gain.linearRampToValueAtTime(.09,at+.04);gain.gain.setValueAtTime(.09,end-.12);gain.gain.linearRampToValueAtTime(0,end);oscillator.connect(gain);gain.connect(audio.destination);activeTones.add(oscillator);oscillator.onended=()=>{activeTones.delete(oscillator);oscillator.disconnect();gain.disconnect();};oscillator.start(at);oscillator.stop(end);at=end+.05;}return true;}catch{return false;}}
// Camera shutter for the farewell photo: a short low click, then a bright tick.
function shutter(){if(!sound||!audio||audio.state!=='running'||document.hidden)return false;tone(392,45);setTimeout(()=>tone(1568,70),70);return true;}
// note(): a short game tone that follows the page's sound switch (garden chorus).
window.PetWebSound={death,shutter,note(freq,ms=220){initAudio();tone(freq,ms);},reminder(){if(!sound||!audio||audio.state!=='running'||document.hidden||activeTones.size)return false;for(const [i,freq] of [659,523,392].entries())setTimeout(()=>tone(freq,140),i*180);return true;}};document.addEventListener('pointerdown',initAudio,{once:true});
// Both games run in the garden, so the picker closes before one starts.
function play(game){dialog.close();if(game.start())document.querySelector('#bird')?.scrollIntoView({block:'nearest'});}
document.querySelector('#start-toy').onclick=()=>play(window.PetToyPlay);
document.querySelector('#start-chorus').onclick=()=>play(window.PetChorus);
// Device mode lists only the games the ESP32 can settle.
// Card icons come from the games' own pixel art, painted once.
let iconsPainted=false;
function paintIcons(){if(iconsPainted)return;iconsPainted=true;
 window.PetToyPlay?.icon(document.querySelector('#start-toy .game-card-art'));window.PetChorus?.icon(document.querySelector('#start-chorus .game-card-art'));}
window.openPetGames=()=>{paintIcons();const device=petDataManager.mode==='ble',games=device?window.PetDeviceSession?.games()||[]:['toy','chorus'];
 document.querySelector('#start-toy').hidden=!games.includes('toy');document.querySelector('#start-chorus').hidden=!games.includes('chorus');
 message.textContent=device?'和電子雞上的寵物一起玩，玩完由電子雞結算':'選一個遊戲，陪牠在花園裡玩一下吧';dialog.showModal();};
})();
