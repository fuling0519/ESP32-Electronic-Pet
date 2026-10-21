"""Compare real UiController/U8g2 captures to V3/V9 PNGs and export evidence."""
from hashlib import sha256
import json
from pathlib import Path
from PIL import Image, ImageChops

ROOT=Path(__file__).resolve().parents[1]
CAPTURE=ROOT/'.pio/rps-preview'
OUT=ROOT/'docs/previews/touch-implemented'
ROLES=[('bird-baby',14,5),('bird-adult',13,5),('wyvern-baby',8,7),('wyvern-adult',8,7)]


def read(name):
    return ImageChops.invert(Image.open(CAPTURE/f'touch-{name}.pbm').convert('L'))


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    report={'status':'REAL_UI_FRAMEBUFFER_VERIFIED_NOT_HARDWARE_ACCEPTANCE','roles':[]}
    report['implementation_sha256']={str(p.relative_to(ROOT)).replace('\\','/'):
        sha256(p.read_bytes()).hexdigest() for p in [
            ROOT/'include/HardwareConfig.h',ROOT/'src/main.cpp',
            ROOT/'src/ui/UiController.cpp',ROOT/'src/ui/UiController.h',
            ROOT/'src/ui/PetIcons.cpp',ROOT/'src/ui/TouchSprite.h',
            ROOT/'src/hardware/TouchGesture.cpp',ROOT/'src/hardware/TouchGesture.h',
            ROOT/'src/hardware/TouchInput.cpp',ROOT/'src/hardware/TouchInput.h',
            ROOT/'src/hardware/Sound.cpp']}
    for key,count,fps in ROLES:
        species,stage=key.split('-')
        source=ROOT/f'assets/pets/{species}/{stage}/happy.png'
        art=Image.open(source).getchannel('A')
        tiles=[art.crop((x,y,x+64,y+44)) for y in range(0,art.height,44) for x in range(0,art.width,64)][:count]
        base=read(f'{key}-base'); frames=[]
        for index,tile in enumerate(tiles):
            actual=read(f'{key}-happy-{index}')
            expected=base.copy(); expected.paste(tile,(32,6))
            assert actual.tobytes()==expected.tobytes(),(key,index,'happy mismatch')
            # Match the previously approved V9 OLED preview at native size.
            preview=Image.open(ROOT/f'docs/previews/touch-proposal/user-art-v9-authored-timing/{key}/frame-{index+1:02d}-128x64.png').convert('L')
            assert actual.tobytes()==preview.tobytes(),(key,index,'V9 preview mismatch')
            actual.save(OUT/f'{key}-happy-{index:02d}-128x64.png')
            frames.append(actual)
        shock=Image.open(ROOT/f'assets/pets/{species}/{stage}/shock.png').getchannel('A')
        for phase,dx in [(0,1),(1,0)]:
            expected=base.copy(); expected.paste(0,(32,6,96,50))
            mask=Image.new('L',(128,64)); mask.paste(shock.crop((0,0,64,44)),(32+dx,6))
            expected=ImageChops.lighter(expected,mask)
            actual=read(f'{key}-shock-{phase}')
            assert actual.tobytes()==expected.tobytes(),(key,phase,'shock mismatch')
            actual.save(OUT/f'{key}-shock-{phase}-128x64.png')
        if stage=='baby':
            expected=base.copy();expected.paste(shock.crop((0,44,64,88)),(32,6))
            actual=read(f'{key}-recovered')
            assert actual.tobytes()==expected.tobytes(),(key,'recovery mismatch')
            actual.save(OUT/f'{key}-recovered-128x64.png')
        repetitions=1 if fps==5 else 7
        frames*=repetitions
        boundaries=[(i*100+fps//2)//fps*10 for i in range(len(frames)+1)]
        durations=[b-a for a,b in zip(boundaries,boundaries[1:])]
        output=OUT/f'{key}-happy-512x256.gif'
        large=[im.resize((512,256),Image.Resampling.NEAREST) for im in frames]
        large[0].save(output,save_all=True,append_images=large[1:],duration=durations,loop=0,disposal=2)
        gif=Image.open(output); assert gif.n_frames==len(large)
        for i,(im,d) in enumerate(zip(large,durations)):
            gif.seek(i); assert gif.info['duration']==d and gif.convert('L').tobytes()==im.tobytes()
        report['roles'].append({'role':key,'fps':fps,'authored_frames':count,
                                'happy_matches_v9_and_source':True,'shock_matches_v3':True,
                                'happy_source_sha256':sha256(source.read_bytes()).hexdigest(),
                                'gif':output.name,'gif_sha256':sha256(output.read_bytes()).hexdigest(),
                                'duration_ms':sum(durations)})
    (OUT/'validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('PASS: 43 happy frames exactly match approved V9 framebuffer pixels, eight V3 poke poses and two baby recovery poses match source; four GIFs verified.')


if __name__=='__main__': main()
