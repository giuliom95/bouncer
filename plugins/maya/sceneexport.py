import json
import struct
import pymel.core as pmc
import os.path

def exportVSSD(path, camName):   
    
    mainFileDict = {}
    mainFilePath = path
    mainFileStem = os.path.basename(path)[:-5]
    mainFileDir  = os.path.dirname(path)
    
    cam = pmc.ls(camName)[0].getShape()
    mainFileDict['camera'] = {
        'focal' : cam.getFocalLength(),
        'gate'  : cam.getVerticalFilmAperture(),
        'aspect': cam.getAspectRatio(),
        'eye'   : list(cam.getEyePoint(space='world')),
        'up'    : list(cam.upDirection(space='world')),
        'look'  : list(cam.viewDirection(space='world'))
    }
    
    bufPath = os.path.join(mainFileDir, '{}.bin'.format(mainFileStem))

    geomList = pmc.ls(type='mesh', visible=True)
    mainFileGeoms = []
    offset = 0
    with open(bufPath, 'wb') as bufFd:
        for geom in geomList:
            print('Processing {}...'.format(geom))
            faceBuf = ''
            idxBuf = ''
            vtxBuf = ''
            nidxs = 0
            for face in geom.f:
                vtxidxs = face.getVertices()
                nvtxidxs = len(vtxidxs)
                faceBuf += struct.pack('<I', nvtxidxs)
                nidxs += nvtxidxs
                for vtxidx in vtxidxs:
                    idxBuf += struct.pack('<I', vtxidx)
            for vertex in geom.vtx:
                p = vertex.getPosition('world')
                vtxBuf += struct.pack('<fff', p.x, p.y, p.z)
            
            edges = geom.edges
            creaseIdxBuf = ''
            creaseValBuf = ''
            creases = pmc.modeling.polyCrease(edges, q=True, v=0)
            hasCreases = False
            for e in range(0, len(edges)):
                c = creases[e]
                if c > 0:
                    hasCreases = True
                    vtxs = edges[e].connectedVertices()
                    creaseIdxBuf += struct.pack('<I', vtxs[0].index())
                    creaseIdxBuf += struct.pack('<I', vtxs[1].index())
                    creaseValBuf += struct.pack('<f', c)
                    
            buffers = [
                (faceBuf, 'faces'),
                (idxBuf,  'indices'),
                (vtxBuf,  'vertices')
            ]
            if hasCreases:
                buffers += [
                    (creaseIdxBuf, 'creaseindices'),
                    (creaseValBuf, 'creasevalues')
                ]
            
            buffersList = []

            for b in buffers:
                print('Writing buffer {}'.format(b[1]))
                bufFd.write(b[0])
                s = len(b[0])
                buffersList.append({
                    'offset': offset,
                    'size': s,
                    'type': b[1] 
                })
                offset += s
            
            smoothLevel = pmc.displaySmoothness(geom, q=True, po=0)[0]
            isSmooth = smoothLevel > 1
            print('Smooth level {}'.format(smoothLevel))

            sg = geom.connections(t='shadingEngine')[0]
            mat = sg.surfaceShader.connections()[0]
            albedo = mat.color.get()
            emittance = mat.incandescence.get()

            geomDict = {
                'smooth'    : isSmooth,
                'buffers'   : buffersList,
                'albedo'    : list(albedo),
                'emittance' : list(emittance)
            }
            mainFileGeoms.append(geomDict)
    
    mainFileDict['geometries'] = mainFileGeoms
    mainFileJson = json.dumps(mainFileDict, indent=2)
    with open(mainFilePath, 'w') as fd: fd.write(mainFileJson)
    print('Done')