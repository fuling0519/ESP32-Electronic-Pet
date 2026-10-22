from PIL import Image
from pathlib import Path
import hashlib, json, shutil

ROOT = Path(__file__).parent
SOURCE = Path('C:/Users/Student/.codex/generated_images/01a1618c-b77d-7a92-afaa-3ca5d392bf0f')
sources = {
 'shower-source-01.png':'exec-3e512438-8366-4615-bae6-73d4949ccc3d.png',
 'shower-source-02.png':'exec-4e7c531a-4f2a-43ba-a8f8-9a9bb40b3908.png',
 'shower-source-03.png':'exec-b05f97cd-ff9c-4a57-af86-21fcba3be2fd.png',
 'water-source-01.png':'exec-f343b486-8245-4919-81ba-96ffbbd7571d.png',
 'water-source-02.png':'exec-ba8a0db3-3587-40a0-b74d-7ee00348294e.png',
}
metadata={'tool':'built-in image_gen','processing':'crop, integer translation, transparent padding only; no resampling, alpha alteration or recolor','measureThreshold':160,'sources':{},'frames':{}}
for dest, src in sources.items():
 shutil.copy2(SOURCE/src,ROOT/dest)
 im=Image.open(ROOT/dest)
 metadata['sources'][dest]={'originalPath':str(SOURCE/src),'sha256':hashlib.sha256((ROOT/dest).read_bytes()).hexdigest(),'size':list(im.size),'mode':im.mode,'alphaExtrema':list(im.getchannel('A').getextrema())}

shower=Image.open(ROOT/'shower-source-03.png').convert('RGBA')
crop=(145,119,1218,1142)
sheet=shower.crop(crop)
sheet.save(ROOT/'shower-sheet-v1.png')
metadata['frames']['shower']=[{'sourceRect':list(crop),'paste':[0,0],'mainBoundsSource':[157,131,1206,1130],'nozzleSource':[495,852]}]

water=Image.open(ROOT/'water-source-02.png').convert('RGBA')
cell=640
atlas=Image.new('RGBA',(cell*3,cell*2),(0,0,0,0))
anchors=[(355,90),(322,90),(291,90),(355,66),(322,66),(291,66)]
target=(400,100)
frames=[]
for i,(ax,ay) in enumerate(anchors):
 sx,sy=(i%3)*512,(i//3)*512
 frame=water.crop((sx,sy,sx+512,sy+512))
 dx,dy=target[0]-ax,target[1]-ay
 atlas.paste(frame,((i%3)*cell+dx,(i//3)*cell+dy))
 frame.save(ROOT/f'water-frame-source-{i+1:02}.png')
 frames.append({'sourceRect':[sx,sy,sx+512,sy+512],'sourceAnchor':[ax,ay],'pasteWithinCell':[dx,dy],'anchor':list(target),'mainBoundsSourceCell':list(frame.getchannel('A').point(lambda a:255 if a>=160 else 0).getbbox())})
atlas.save(ROOT/'water-sheet-v1.png')
metadata['frames']['water']=frames
manifest={'assets':{'shower':{'file':'group-b/shower-sheet-v1.png','cellWidth':1073,'cellHeight':1023,'columns':1,'frameCount':1,'anchor':[536,512],'referenceWidth':1049,'suggestedWidth':58,'nozzle':[350,733]},'water':{'file':'group-b/water-sheet-v1.png','cellWidth':640,'cellHeight':640,'columns':3,'frameCount':6,'anchor':list(target),'referenceWidth':334,'suggestedWidth':80}},'notes':{'shower':'Handle extends upper-right. Face sprays down and slightly left. Nozzle is visual center of spray holes. Position water.anchor at transformed shower.nozzle.','water':'Top anchors aligned by integer translation; all 6 frames retain source scale. Main flow axis approximately [-0.29,0.96], average endpoint relative to anchor approximately [-110,350]. Stable loop frames 2-5, opening 1 and closing 6.','limits':'Pixel density/style needs shared-scene review at target width. No web integration or browser playback performed by this group.'}}
(ROOT/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
for name in ['shower-sheet-v1.png','water-sheet-v1.png']:
 metadata[name]={'sha256':hashlib.sha256((ROOT/name).read_bytes()).hexdigest()}
(ROOT/'metadata.json').write_text(json.dumps(metadata,indent=2),encoding='utf-8')

# Inspection only: true alpha composition then proportional thumbnail resampling.
preview=Image.new('RGBA',(960,500),(250,241,225,255))
for i,bg in enumerate([(250,241,225,255),(30,35,59,255)]):
 panel=Image.new('RGBA',(480,500),bg)
 thumb=sheet.copy(); thumb.thumbnail((200,200),Image.Resampling.NEAREST)
 panel.alpha_composite(thumb,(40,20))
 for j in range(6):
  frame=atlas.crop(((j%3)*640,(j//3)*640,(j%3+1)*640,(j//3+1)*640))
  frame.thumbnail((140,140),Image.Resampling.NEAREST)
  panel.alpha_composite(frame,((j%3)*150+15,(j//3)*140+215))
 preview.alpha_composite(panel,(i*480,0))
preview.convert('RGB').save(ROOT/'alpha-contact-v1.png')
print(json.dumps(manifest,indent=2))
