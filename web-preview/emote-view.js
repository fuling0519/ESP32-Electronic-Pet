/* Small pixel emote bubbles above the pet: what it needs right now, or a short thank-you after care. */
(()=>{'use strict';
// '#' outline, 'w' fill; the tail points down-left towards the pet's head.
const BUBBLE=[
 '..#############..',
 '.#wwwwwwwwwwwww#.',
 '#wwwwwwwwwwwwwww#',
 '#wwwwwwwwwwwwwww#',
 '#wwwwwwwwwwwwwww#',
 '#wwwwwwwwwwwwwww#',
 '#wwwwwwwwwwwwwww#',
 '#wwwwwwwwwwwwwww#',
 '#wwwwwwwwwwwwwww#',
 '#wwwwwwwwwwwwwww#',
 '#wwwwwwwwwwwwwww#',
 '.#wwwwwwwwwwwww#.',
 '..#ww##########..',
 '..#w#............',
 '..##.............'];
// 9×9 icons drawn at column 4, row 2 of the bubble.
const ICONS={
 heart:['.........','.dd...dd.','dppd.dppd','dlppppppd','dpppppppd','.dpppppd.','..dpppd..','...dpd...','....d....'],
 note:['....kk...','....kkk..','....k.kk.','....k..k.','....k....','.kkkk....','knnkk....','kkkk.....','.kk......'],
 food:['.........','.........','..s.s.s..','.sSsSsSs.','BbbbbbbbB','.BbbbbbB.','..BbbbB..','...BBB...','.........'],
 potion:['...kkk...','...kck...','...kwk...','..kwwwk..','.kgggggk.','.kgwgggk.','.kggggGk.','..kGGGk..','...kkk...'],
 sponge:['.........','.....q.q.','..q...q..','.OOOOOOO.','OyyhyyyyO','OyyyyyhyO','OyhyyyyyO','.OOOOOOO.','.........'],
 heartbreak:['.........','.DD...DD.','DPPD.DPPD','DLPPwPPPD','DPPwPPPPD','.DPPwPPD.','..DPwPD..','...DPD...','....D....']};
const COLORS={'#':'#3d2f4a',w:'#fffaf0',d:'#b8486a',p:'#ef7f9c',l:'#ffc4d2',k:'#675279',n:'#b48ad0',y:'#f3d47e',o:'#c9922f',
 s:'#f3d47e',S:'#c9922f',b:'#7fb2e8',B:'#3d6aa8',g:'#60bb91',G:'#2b765f',c:'#a37150',
 O:'#b07a2a',h:'#d9a646',q:'#9fd3ee',D:'#8a5068',P:'#c98a9c',L:'#e7c3cd'};
function canvas(key){
 const icon=ICONS[key];if(!icon||typeof document==='undefined')return null;
 const c=document.createElement('canvas');c.width=17;c.height=15;const g=c.getContext('2d');
 const plot=(rows,ox,oy)=>rows.forEach((row,y)=>[...row].forEach((ch,x)=>{if(COLORS[ch]){g.fillStyle=COLORS[ch];g.fillRect(ox+x,oy+y,1,1);}}));
 plot(BUBBLE,0,0);plot(icon,4,2);c.setAttribute('aria-hidden','true');return c;
}
window.PetEmote={canvas,keys:Object.keys(ICONS)};
})();
