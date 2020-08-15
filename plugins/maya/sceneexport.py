import json
import struct
import pymel.core as pmc
import os.path

def exportVSSD(path, camName, wantTris=False, renderdata=None):   
    
    mainFileDict = {}
    mainFilePath = path
    mainFileStem = os.path.basename(path)[:-5]
    mainFileDir  = os.path.dirname(path)
    
    resolution = pmc.ls('defaultResolution')[0]
    renderWidth = resolution.width.get()
    renderHeight = resolution.height.get()

    if renderdata is not None:
        mainFileDict['render'] = {
            'width' : renderWidth,
            'height': renderHeight,
            'spp'   : renderdata['spp']
        }

    cam = pmc.ls(camName)[0].getShape()
    mainFileDict['camera'] = {
        'focal' : cam.getFocalLength(),
        'gate'  : cam.getVerticalFilmAperture(),
        'aspect': renderWidth / renderHeight,
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
            
            smoothLevel = pmc.displaySmoothness(geom, q=True, po=0)[0]
            isSmooth = smoothLevel > 1
            print('Smooth level {}'.format(smoothLevel))

            faceBuf = ''
            idxBuf = ''
            vtxBuf = ''
            nidxs = 0
            for face in geom.f:
                vtxidxs = face.getVertices()
                nvtxidxs = len(vtxidxs)
                if not isSmooth and wantTris:
                    if nvtxidxs > 3:
                        print('Non-triangulated face. Triangulate before exporting')
                        return
                else:
                    faceBuf += struct.pack('<I', nvtxidxs)
                nidxs += nvtxidxs
                for vtxidx in vtxidxs:
                    idxBuf += struct.pack('<I', vtxidx)
            for vertex in geom.vtx:
                p = vertex.getPosition('world')
                vtxBuf += struct.pack('<fff', p.x, p.y, p.z)
            
            hasCreases = False
            if isSmooth:
                edges = geom.edges
                creaseIdxBuf = ''
                creaseValBuf = ''
                creases = pmc.modeling.polyCrease(edges, q=True, v=0)
                for e in range(0, len(edges)):
                    c = creases[e]
                    if c > 0:
                        hasCreases = True
                        vtxs = edges[e].connectedVertices()
                        creaseIdxBuf += struct.pack('<I', vtxs[0].index())
                        creaseIdxBuf += struct.pack('<I', vtxs[1].index())
                        creaseValBuf += struct.pack('<f', c)
                    
            buffers = [
                (idxBuf,  'indices'),
                (vtxBuf,  'vertices')
            ]
            if not wantTris:
                buffers += [
                    (faceBuf, 'faces')
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
            
            
            sg = geom.connections(t='shadingEngine')[0]
            mat = sg.surfaceShader.connections()[0]
            albedo = mat.color.get()
            emittance = mat.incandescence.get()

            geomDict = {
                'triangles' : wantTris,
                'smooth'    : isSmooth,
                'buffers'   : buffersList,
                'material'  : {
                    'albedo'    : list(albedo),
                    'emittance' : list(emittance)
                }
            }
            mainFileGeoms.append(geomDict)
    
    mainFileDict['geometries'] = mainFileGeoms
    mainFileJson = json.dumps(mainFileDict, indent=2)
    with open(mainFilePath, 'w') as fd: fd.write(mainFileJson)
    print('Done')