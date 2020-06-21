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
                    
            bufFd.write(faceBuf)
            bufFd.write(idxBuf)
            bufFd.write(vtxBuf)
            
            faceBufSize = len(faceBuf)
            idxBufSize = len(idxBuf)
            vtxBufSize = len(vtxBuf)
            
            buffers = []
            
            buffers.append({
                'offset': offset,
                'size': faceBufSize,
                'type': 'faces'
            })
            offset += faceBufSize
            
            buffers.append({
                'offset': offset,
                'size': idxBufSize,
                'type': 'indices'
            })
            offset += idxBufSize
            
            buffers.append({
                'offset': offset,
                'size': vtxBufSize,
                'type': 'vertices'
            })
            offset += vtxBufSize
            
            geomDict = {
                'buffers': buffers
            }
            mainFileGeoms.append(geomDict)
    
    mainFileDict['geometries'] = mainFileGeoms
    mainFileJson = json.dumps(mainFileDict, indent=2)
    with open(mainFilePath, 'w') as fd: fd.write(mainFileJson)
    print('Done')
