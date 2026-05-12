ffi_begin export vfx
ffi vxNew.ptr W.u4 H.u4 D.u4
ffi vxFree.void Slb.ptr
ffi vxClone.ptr Slb.ptr
ffi vxW.u4 Slb.ptr
ffi vxH.u4 Slb.ptr
ffi vxD.u4 Slb.ptr
ffi vxGet.u4 Slb.ptr X.int Y.int Z.int
ffi vxGetTr.u4 Slb.ptr X.int Y.int Z.int
ffi vxSetNilAbyss.void Slb.ptr Nil.u4 Abyss.u4
ffi vxSet.void Slb.ptr X.int Y.int Z.int Color.u4
ffi vxSetTr.void Slb.ptr X.int Y.int Z.int Color.u4
ffi vxGetI.u4 Slb.ptr Index.u4
ffi vxSetI.void Slb.ptr Index.u4 Color.u4
ffi vxSetRaw.void Slb.ptr X.int Y.int Z.int Color.u4
ffi vxClear.void Slb.ptr Color.u4
ffi vxDbg.int Slb.ptr NewDbgState.int
ffi vxCamera.void OX.float OY.float OZ.float
                  XX.float XY.float XZ.float
                  YX.float YY.float YZ.float
                  ZX.float ZY.float ZZ.float
                  Scale.float FocalLength.float
                  W.u4 H.u4 Flags.u4
ffi vxOrient.void Slb.ptr Flags.u4
                    SX.float SY.float SZ.float
                    CX.float CY.float CZ.float
                    AX.float AY.float AZ.float
                    X.float Y.float Z.float
                    R.float G.float B.float

ffi vxBegin.void
ffi vxEnd.void
ffi vxSlab.void Slb.ptr
ffi vxShader.void Flags.u4
ffi vxAmbient.void SkyColor.u4 R.float G.float B.float
ffi vxRender.void CBuf.ptr
ffi vxRenderSample.u4 X.u4 Y.u4

ffi vxSampleRay.text Slb.ptr ScreenX.u4 ScreenY.u4

ffi vxProjectionSample.text Slb.ptr Gfx.ptr X.int Y.int Z.int

ffi vxSampleAABB.text Slb.ptr
ffi vxSampleWorldAABB.text Slb.ptr
ffi vxSamplePastedAABB.text Slb.ptr Brush.ptr
ffi vxMargins.text Slb.ptr
ffi vxCopyBox.ptr Slb.ptr X0.int Y0.int Z0.int X1.int Y1.int Z1.int

ffi vxTexture.void Slb.ptr Gfx.ptr BrushType.int Invert.int
ffi vxPaintMargin.void Margin.float

ffi vxRadialCut.void Slb.ptr Hrz.ptr Vrt.ptr Corona.ptr

ffi vxCut.void Slb.ptr Brush.ptr Flags.u4

ffi vxPaste.void Slb.ptr Brush.ptr Flags.u4

ffi vxSave.void Filename.text Slb.ptr

ffi vxLoad.ptr Filename.text

ffi vxInfo.text Slb.ptr
ffi vxSetInfo.void Slb.ptr Filename.text

ffi vxCompact.void Slb.ptr

ffi vxFillInterior.void Slb.ptr

ffi vxGetClayColor.u4 Slb.ptr
ffi vxSetClayColor.void Slb.ptr Color.u4

ffi vxRecolor.void Slb.ptr Src.u4 Dst.u4

ffi vxMeshLock.void Slb.ptr State.int

ffi vxSmoothNormals.void Slb.ptr Amount.u4

ffi vxDbgGfx.void Gfx.ptr

ffi vxSetSoftness.void Gfx.ptr Value.int

ffi vxEnvTexture.void Slb.ptr Gfx.ptr Type.int

ffi vxCarveHeightmap.void Slb.ptr Gfx.ptr

ffi vxScaled.void Slb.ptr Src.ptr

ffi vxSetImportTexture.void Type.int Gfx.ptr

ffi vxImportPly.ptr Slb.ptr Filename.text
ffi vxImportObj.ptr Slb.ptr Filename.text

ffi vxImport.ptr Filename.text


ffi vxMeshFree.void Mesh.ptr
ffi vxPolygonize.ptr Slb.ptr

ffi vxExportObj.int Filename.text Slb.ptr Algorithm.int

ffi vxExportCSV.int Filename.text Slb.ptr
