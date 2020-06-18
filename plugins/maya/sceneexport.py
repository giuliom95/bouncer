import json
import struct
import pymel.core as pmc
import os.path

def exportVSSD(path, camName):   
    
    mainFileDict = {}
    mainFilePath = path
    mainFileName = os.path.basename(path)
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
    
    geomIdx = 0
    geomList = pmc.ls(geometry=True)
    mainFileGeoms = []
    for geom in geomList:
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
                
        meshBaseFileName = '{0}_{1:04}'.format(mainFileName, geomIdx)        
        faceBufFileName = meshBaseFileName + '_faces.bin'
        idxBufFileName  = meshBaseFileName + '_indexes.bin'
        vtxBufFileName  = meshBaseFileName + '_vertices.bin'
        faceBufAbsPath = os.path.join(mainFileDir, faceBufFileName)
        idxBufAbsPath  = os.path.join(mainFileDir, idxBufFileName)
        vtxBufAbsPath  = os.path.join(mainFileDir, vtxBufFileName)
        with open(faceBufAbsPath, 'wb') as fd: fd.write(faceBuf)
        with open(idxBufAbsPath,  'wb') as fd: fd.write(idxBuf)
        with open(vtxBufAbsPath,  'wb') as fd: fd.write(vtxBuf)
        
        geomDict = {
            'faces': {
                'amount': len(geom.f),
                'path':   os.path.join('./', faceBufFileName)
            },
            'indices': {
                'amount': nidxs,
                'path':   os.path.join('./', idxBufFileName)
            },
            'vertices': {
                'amount': len(geom.vtx),
                'path':   os.path.join('./', vtxBufFileName)
            }
        }
        mainFileGeoms.append(geomDict)
        geomIdx += 1
    
    mainFileDict['geometries'] = mainFileGeoms
    mainFileJson = json.dumps(mainFileDict, indent=2)
    with open(mainFilePath, 'w') as fd: fd.write(mainFileJson)
