#define DeviceRAM 0x0020


#define ConfigReboot 0x1000
#define ConfigModeset 0x1008
#define ConfigInterrupt 0x100C
#define ConfigAcceleration 0x1010

#define STREAMBUFFERAADDRESS 0x2000
#define STREAMBUFFERACONFIG 0x2008

#define RasterPrimitive 0x3000
#define RasterEmit 0x3004
#define RasterClear 0x3008
#define RasterFlush 0x3FFC
#define InfoFifoDepth 0x4004
#define INFOSTATUS	0x4008

#define DrawVertexCoordX 0x5000
#define DrawVertexCoordY 0x5004
#define DrawVertexCoordZ 0x5008
#define DrawVertexCoordW 0x500C

#define DrawVertexColorR 0x5010
#define DrawVertexColorG 0x5014
#define DrawVertexColorB 0x5018
#define DrawVertexColorAlpha 0x501C

#define DrawClearColorR 0x5100
#define DrawClearColorG 0x5104
#define DrawClearColorB 0x5108
#define DrawClearColorAlpha 0x510C

#define FrameColumns 0x8000
#define FrameRows 0x8004
#define FrameRowPitch 0x8008
#define FramePixelFormat 0x800C
#define FrameStartAddress 0x8010

#define EncoderWidth 0x9000
#define EncoderHeight 0x9004
#define EncoderOffsetX 0x9008
#define EncoderOffsetY 0x900C
#define EncoderFrame 0x9010
