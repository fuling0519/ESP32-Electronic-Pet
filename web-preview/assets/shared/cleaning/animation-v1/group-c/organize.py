from pathlib import Path
from PIL import Image
import json, hashlib

HERE = Path(__file__).resolve().parent
SOURCE = Path('C:/Users/Student/.codex/generated_images/01a1618c-efaf-76d0-aa68-241caaad8332')
SOURCE_FILES = {
    'drops-source-initial-v1.png': 'exec-9105685b-88b8-4302-89c3-c12974122013.png',
    'drops-source-v1.png': 'exec-cc759cb1-a5cc-4790-9ba0-eca8c78dad28.png',
    'sparkle-source-v1.png': 'exec-b2baf2a6-0ce7-45a6-a62a-e5fae8f9f881.png',
}

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()

for target, source in SOURCE_FILES.items():
    if not (HERE / target).exists():
        (HERE / target).write_bytes((SOURCE / source).read_bytes())

metadata = {'method': 'alpha>=160 measures primary shape; crop with 8 px margin then integer translation. Preserve all RGBA within retained crop. No recolor, alpha replacement, resampling or generated art painted by script.', 'sources': {}, 'assets': {}}
for name in SOURCE_FILES:
    im = Image.open(HERE / name)
    metadata['sources'][name] = {'size': list(im.size), 'mode': im.mode, 'alphaExtrema': im.getchannel('A').getextrema(), 'sha256': sha(HERE / name)}

def build(key, centers):
    source = Image.open(HERE / f'{key}-source-v1.png').convert('RGBA')
    atlas = Image.new('RGBA', (1536, 1024))
    frames = []
    widths = []
    for i in range(5):
        sx, sy = (i % 3) * 512, (i // 3) * 512
        cell = source.crop((sx, sy, sx+512, sy+512))
        primary = cell.getchannel('A').point(lambda a: 255 if a >= 160 else 0).getbbox()
        x0,y0,x1,y1 = primary
        box = (max(0,x0-8),max(0,y0-8),min(512,x1+8),min(512,y1+8))
        crop = cell.crop(box)
        center = centers[i] if centers else ((x0+x1)//2,(y0+y1)//2)
        offset = (256-center[0],256-center[1])
        dest = (sx+box[0]+offset[0],sy+box[1]+offset[1])
        atlas.paste(crop,dest)
        assert atlas.crop((dest[0],dest[1],dest[0]+crop.width,dest[1]+crop.height)).tobytes() == crop.tobytes()
        widths.append(x1-x0)
        frames.append({'frame':i,'sourceCell':[sx,sy,sx+512,sy+512], 'primaryBBoxInCell':list(primary), 'sourceCrop':[sx+box[0],sy+box[1],sx+box[2],sy+box[3]], 'sourceAnchorInCell':list(center), 'translation':list(offset), 'destinationTopLeft':list(dest), 'copiedRgbaExact':True})
    target = HERE / f'{key}-sheet-v1.png'
    atlas.save(target)
    assert atlas.crop((1024,512,1536,1024)).getchannel('A').getbbox() is None
    for i in range(5):
        assert atlas.crop(((i%3)*512,(i//3)*512,(i%3+1)*512,(i//3+1)*512)).getchannel('A').getbbox() is not None
    metadata['assets'][key] = {'frames':frames,'referenceWidth':max(widths),'size':list(atlas.size),'sha256':sha(target),'sixthCellAlphaZero':True,'alphaExtrema':atlas.getchannel('A').getextrema()}
    for color, label in [((242,244,250,255),'light'),((41,36,58,255),'dark')]:
        preview = Image.new('RGBA',atlas.size,color)
        preview.alpha_composite(atlas)
        preview.convert('RGB').save(HERE / f'{key}-alpha-preview-{label}.png')

# First two are falling particle centers; last three retain the impact center.
build('drops', [(273,333),(254,317),(244,500),(270,428),(258,433)])
build('sparkle', None)
manifest = {'assets': {}}
for key, suggested in [('drops',18),('sparkle',22)]:
    manifest['assets'][key] = {'file':f'group-c/{key}-sheet-v1.png','cellWidth':512,'cellHeight':512,'columns':3,'frameCount':5,'anchor':[256,256],'referenceWidth':metadata['assets'][key]['referenceWidth'],'suggestedWidth':suggested}
(HERE / 'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
(HERE / 'metadata.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
print(json.dumps(manifest,indent=2))
