/* Optional visual QA; requires @napi-rs/canvas. Does not alter source sprites. */
const fs=require('fs'),path=require('path'),vm=require('vm'),{createCanvas,loadImage}=require('@napi-rs/canvas');
const root=path.resolve(__dirname,'../web-preview');
async function main(){
const birds=await Promise.all(['baby','adult'].map(stage=>loadImage(path.join(root,'assets/bird-'+stage+'-states-20261019.png'))));
for(const im of birds){Object.defineProperty(im,'src',{set(){}});Object.defineProperty(im,'naturalWidth',{value:im.width});Object.defineProperty(im,'naturalHeight',{value:im.height});Object.defineProperty(im,'complete',{value:true});}
let next=0;class AssetImage{constructor(){return birds[next++];}}
const c={Image:AssetImage};c.window=c;vm.createContext(c);for(const f of ['shared-sprites.js','shared-view.js','bird-motion-study.js'])vm.runInContext(fs.readFileSync(path.join(root,f),'utf8'),c);
const background=await loadImage(path.join(root,'assets/garden-detailed-day.png'));
const result=createCanvas(1536,576),ctx=result.getContext('2d');ctx.fillStyle='#fff6df';ctx.fillRect(0,0,1536,576);
const moments={idle:5000,sad:2500,sick:1700,sleep:2000,happy:2800,startled:450,feed:2400,curious:1250};let i=0;
for(const [state,time] of Object.entries(moments)){const x=i%4*384,y=Math.floor(i/4)*288;ctx.save();ctx.translate(x,y);ctx.drawImage(background,0,0,384,256);c.BirdMotionStudy.render(ctx,'adult',state,time);ctx.fillStyle='#655043';ctx.font='18px sans-serif';ctx.fillText(state+' · '+time+'ms',12,278);ctx.restore();i++;}
fs.writeFileSync(path.join(root,'assets/bird-motion-study-contact-20261021.png'),result.toBuffer('image/png'));
console.log('Rendered eight motion study frames from actual source PNGs.');
}main().catch(error=>{console.error(error);process.exitCode=1;});
