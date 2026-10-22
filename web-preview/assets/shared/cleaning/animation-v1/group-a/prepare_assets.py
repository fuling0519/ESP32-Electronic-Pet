"""Lossless crop/translation into equal atlas cells. Does not repaint alpha/colors."""
from PIL import Image
from pathlib import Path
import hashlib, json

ROOT = Path(__file__).resolve().parent
SPECS = {
    'sponge': {
        'cell': [704, 704], 'columns': 3, 'anchor': [352, 352],
        'crops': [[40,40,700,700],[770,40,1400,700],[1490,40,2140,700]],
        'pivots': [[365,366],[1086,366],[1815,366]],
        'suggestedWidth': 60, 'referenceFrame': 0,
        'timesMs': [140,120,140],
    },
    'foam': {
        'cell': [512,512], 'columns': 4, 'anchor': [256,256],
        'crops': [[150,195,370,425],[525,190,810,425],[915,135,1290,470],[1330,100,1760,480],
                  [40,480,490,860],[515,505,875,840],[975,555,1245,800],[1460,570,1680,790]],
        'pivots': [[250,310],[665,310],[1100,305],[1538,305],[256,665],[695,665],[1100,680],[1544,680]],
        'suggestedWidth': 64, 'referenceFrame': 4,
    },
}

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

manifest = {'assets': {}}
metadata = {'tool': 'built-in image_gen', 'operations': ['crop', 'integer translation', 'transparent canvas padding'],
            'pixelPolicy': 'No source resampling, color replacement, alpha thresholding, or repainting. Source alpha retained inside recorded crops.',
            'limitations': ['Generated frame alignment is approximate before translation.',
                'Subtle pores and individual bubbles are redrawn between frames; source drawing scale is retained.',
                'Low-alpha edge artifacts remain within crops. Original sources preserve all generated pixels.',
                'Visual approval and browser animation acceptance remain pending.'], 'assets': {}}

for name, spec in SPECS.items():
    src_path = ROOT / f'{name}-source-v1.png'
    src = Image.open(src_path).convert('RGBA')
    cw,ch = spec['cell']; cols=spec['columns']; n=len(spec['crops'])
    atlas=Image.new('RGBA',(cw*cols,ch*((n+cols-1)//cols)),(0,0,0,0))
    frames=[]
    for i,(box,pivot) in enumerate(zip(spec['crops'],spec['pivots'])):
        tile=src.crop(box)
        at=[spec['anchor'][0]+box[0]-pivot[0],spec['anchor'][1]+box[1]-pivot[1]]
        assert at[0]>=0 and at[1]>=0 and at[0]+tile.width<=cw and at[1]+tile.height<=ch
        cell=Image.new('RGBA',(cw,ch),(0,0,0,0))
        cell.paste(tile,at)  # direct RGBA copy, no alpha multiplication
        atlas.paste(cell,((i%cols)*cw,(i//cols)*ch))
        a=cell.getchannel('A')
        substantive=a.point(lambda x:255 if x>=128 else 0).getbbox()
        frames.append({'index':i,'sourceCrop':box,'sourcePivot':pivot,'cellPaste':at,
                       'alphaRange':list(a.getextrema()),'alphaBBox':a.getbbox(),
                       'opaqueCoreBBoxAt128ForMeasurementOnly':substantive,
                       'visiblePixelCount':sum(v>0 for v in a.getdata())})
    out=ROOT/f'{name}-sheet-v1.png'; atlas.save(out)
    ref=frames[spec['referenceFrame']]['opaqueCoreBBoxAt128ForMeasurementOnly']
    asset={'file':f'group-a/{out.name}','cellWidth':cw,'cellHeight':ch,'columns':cols,'frameCount':n,
           'anchor':spec['anchor'],'referenceWidth':ref[2]-ref[0],'suggestedWidth':spec['suggestedWidth']}
    if 'timesMs' in spec: asset['timesMs']=spec['timesMs']
    manifest['assets'][name]=asset
    metadata['assets'][name]={'sourceFile':src_path.name,'sourceSize':list(src.size),'sourceSha256':sha(src_path),
        'atlasFile':out.name,'atlasSize':list(atlas.size),'atlasSha256':sha(out),
        'sourceAlphaRange':list(src.getchannel('A').getextrema()),'referenceFrame':spec['referenceFrame'],'frames':frames}
(ROOT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
(ROOT/'metadata.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
print(json.dumps(manifest,indent=2))
