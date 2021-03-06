#include "LIBGPU.H"

#include "EMULATOR.H"
#include "EMULATOR_GLOBALS.H"
#include "EMULATOR_PRIVATE.H"

#include <stdint.h>

#include "LIBETC.H"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

DISPENV activeDispEnv;
DRAWENV activeDrawEnv;	// word_33BC
DRAWENV byte_9CCA4;
int dword_3410 = 0;
char byte_3352 = 0;

struct VertexBufferSplit
{
	TextureID      textureId;
	unsigned short vIndex;
	unsigned short vCount;
	BlendMode      blendMode;
	TexFormat      texFormat;
};

//#define DEBUG_POLY_COUNT

#if defined(DEBUG_POLY_COUNT)
static int polygon_count = 0;
#endif

struct Vertex g_vertexBuffer[MAX_NUM_POLY_BUFFER_VERTICES];
struct VertexBufferSplit g_splits[MAX_NUM_INDEX_BUFFERS];
int g_vertexIndex;
int g_splitIndex;

void ClearVBO()
{
	g_vertexIndex = 0;
	g_splitIndex = 0;
	g_splits[g_splitIndex].texFormat = (TexFormat)0xFFFF;
}

u_short s_lastSemiTrans = 0xFFFF;
u_short s_lastPolyType = 0xFFFF;

void ResetPolyState()
{
	s_lastSemiTrans = 0xFFFF;
	s_lastPolyType = 0xFFFF;
}

//#define WIREFRAME_MODE

#if defined(USE_32_BIT_ADDR)
unsigned int terminator[2] = { -1, 0 };
#else
unsigned int terminator = -1;
#endif

static char unk_1B00[1024];//unk_1B00
static char unk_1F00[16384];//unk_1F00
static unsigned int fontStreamCount = 0;//dword_E80
static unsigned int fontCurrentStream = 0;//dword_E84
static unsigned int fontUsedCharacterCount = 0;//dword_1888
static DR_MODE fontMode = { 0, 0, 0, 0 };//unk_D10
static unsigned short fontTpage = getTPage(2, 0, 0, 0);//word_5F00
static unsigned short fontClut = getClut(960, 384);//word_5F02
#define MAX_NUM_FONT_STREAMS (8)
#define MAX_FONT_CHARACTER_COUNT (1024)

#pragma pack(push,1)

struct Font
{
	TILE tile;
	unsigned int unk01;
	unsigned int unk02;
	unsigned int characterCount;
	SPRT_8* pSprites;
	char* unk05;
	unsigned int unk06;
	unsigned int unk07;
};

#pragma pack(pop)

struct Font font[MAX_NUM_FONT_STREAMS];//dword_D00

void(*drawsync_callback)(void) = NULL;

void* off_3348[] =
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

int ClearImage(RECT16* rect, u_char r, u_char g, u_char b)
{
	Emulator_Clear(rect->x, rect->y, rect->w, rect->h, r, g, b);
	return 0;
}

int ClearImage2(RECT16* rect, u_char r, u_char g, u_char b)
{
	Emulator_Clear(rect->x, rect->y, rect->w, rect->h, r, g, b);
	return 0;
}

void DrawAggregatedSplits();

int DrawSync(int mode)
{
	// Update VRAM seems needed to be here
	//Emulator_UpdateVRAM();

	if (drawsync_callback != NULL)
	{
		drawsync_callback();
	}

	if (g_splitIndex > 0) // don't do flips if nothing to draw.
	{
		DrawAggregatedSplits();
		Emulator_EndScene();
	}

	return 0;
}

int LoadImagePSX(RECT16* rect, u_long* p)
{
	Emulator_CopyVRAM((unsigned short*)p, 0, 0, rect->w, rect->h, rect->x, rect->y);
	return 0;
}

int MargePrim(void* p0, void* p1)
{
#if defined(USE_32_BIT_ADDR)
	int v0 = ((int*)p0)[1];
	int v1 = ((int*)p1)[1];
#else
	int v0 = ((unsigned char*)p0)[3];
	int v1 = ((unsigned char*)p1)[3];
#endif

	v0 += v1;
	v1 = v0 + 1;

#if defined(USE_32_BIT_ADDR)
	if (v1 < 0x12)
#else
	if (v1 < 0x11)
#endif
	{
#if defined(USE_32_BIT_ADDR)
		((int*)p0)[1] = v1;
		((int*)p1)[1] = 0;
#else
		((char*)p0)[3] = v1;
		((int*)p1)[0] = 0;
#endif

		return 0;
	}

	return -1;
}

int MoveImage(RECT16* rect, int x, int y)
{
	Emulator_CopyVRAM(NULL, x, y, rect->w, rect->h, rect->x, rect->y);
	return 0;
}

int ResetGraph(int mode)
{
	UNIMPLEMENTED();
	return 0;
}

int SetGraphDebug(int level)
{
	UNIMPLEMENTED();
	return 0;
}

int StoreImage(RECT16* rect, u_long* p)
{
	Emulator_ReadVRAM((unsigned short*)p, rect->x, rect->y, rect->w, rect->h);
	return 0;
}

u_long* ClearOTag(u_long* ot, int n)
{
	//Nothing to do here.
	if (n == 0)
		return NULL;

	//last is special terminator
#if defined(USE_32_BIT_ADDR)
	setaddr(&ot[n - 2], &terminator);
	setlen(&ot[n - 2], 0);
#else
	setaddr(&ot[n - 1], &terminator);
	setlen(&ot[n - 1], 0);
#endif


#if defined(USE_32_BIT_ADDR)
	for (int i = n - 4; i >= 0; i -= 2)
#else
	for (int i = n - 2; i >= 0; i--)
#endif
	{
#if defined(USE_32_BIT_ADDR)
		setaddr(&ot[i], (unsigned long)&ot[i + 2]);
#else
		setaddr(&ot[i], (unsigned long)&ot[i + 1]);Ma
#endif
	}

	return NULL;
}

u_long* ClearOTagR(u_long* ot, int n)
{
	//Nothing to do here.
	if (n == 0)
		return NULL;

	//First is special terminator
	setaddr(ot, &terminator);
	setlen(ot, 0);

#if defined(USE_32_BIT_ADDR)
	for (int i = 2; i < n * 2; i += 2)
#else
	for (int i = 1; i < n; i++)
#endif
	{
#if defined(USE_32_BIT_ADDR)
		setaddr(&ot[i], (unsigned long)&ot[i - 2]);
#else
		setaddr(&ot[i], (unsigned long)&ot[i - 1]);
#endif
		setlen(&ot[i], 0);
	}

	return NULL;
}

void SetDispMask(int mask)
{
	UNIMPLEMENTED();
}

int FntPrint(char* text, ...)
{
	UNIMPLEMENTED();
	return 0;
}

DISPENV* GetDispEnv(DISPENV* env)//(F)
{
	memcpy(env, &activeDispEnv, sizeof(DISPENV));
	return env;
}

DISPENV* PutDispEnv(DISPENV* env)//To Finish
{
	memcpy((char*)&activeDispEnv, env, sizeof(DISPENV));
	return 0;
}

DISPENV* SetDefDispEnv(DISPENV* env, int x, int y, int w, int h)//(F)
{
	env->disp.x = x;
	env->disp.y = y;
	env->disp.w = w;
	env->screen.x = 0;
	env->screen.y = 0;
	env->screen.w = 0;
	env->screen.h = 0;
	env->isrgb24 = 0;
	env->isinter = 0;
	env->pad1 = 0;
	env->pad0 = 0;
	env->disp.h = h;
	return 0;
}

DRAWENV* GetDrawEnv(DRAWENV* env)
{
	UNIMPLEMENTED();
	return NULL;
}

DRAWENV* PutDrawEnv(DRAWENV* env)//Guessed
{
	memcpy((char*)&activeDrawEnv, env, sizeof(DRAWENV));
	return 0;
}

DRAWENV* SetDefDrawEnv(DRAWENV* env, int x, int y, int w, int h)//(F)
{
	env->clip.x = x;
	env->clip.y = y;
	env->clip.w = w;
	env->tw.x = 0;
	env->tw.y = 0;
	env->tw.w = 0;
	env->tw.h = 0;
	env->r0 = 0;
	env->g0 = 0;
	env->b0 = 0;
	env->dtd = 1;
	env->clip.h = h;

	if (GetVideoMode() == 0)
	{
		env->dfe = h < 0x121 ? 1 : 0;
	}
	else
	{
		env->dfe = h < 0x101 ? 1 : 0;
	}

	env->ofs[0] = x;
	env->ofs[1] = y;

	env->tpage = 10;
	env->isbg = 0;

	return env;
}

void SetDrawEnv(DR_ENV* dr_env, DRAWENV* env)
{

}

void SetDrawMode(DR_MODE* p, int dfe, int dtd, int tpage, RECT16* tw)
{
	setDrawMode(p, dfe, dtd, tpage, tw);
}

void SetDrawMove(DR_MOVE* p, RECT16* rect, int x, int y)
{
	char uVar1;
	ulong uVar2;
	
	uVar1 = 5;
	if ((rect->w == 0) || (rect->h == 0)) {
		uVar1 = 0;
	}
	p->code[0] = 0x1000000;
	p->code[1] = 0x80000000;
	*(char *)((int)&p->tag + 3) = uVar1;
	uVar2 = *(ulong *)rect;
	p->code[3] = y << 0x10 | x & 0xffffU;
	p->code[2] = uVar2;
	p->code[4] = *(ulong *)&rect->w;
}

u_long DrawSyncCallback(void(*func)(void))
{
	drawsync_callback = func;
	return 0;
}

u_short GetClut(int x, int y)
{
	return getClut(x, y);
}

void AddSplit(bool semiTrans, int page, TextureID textureId)
{
	VertexBufferSplit& curSplit = g_splits[g_splitIndex];
	BlendMode blendMode = semiTrans ? GET_TPAGE_BLEND(page) : BM_NONE;
	TexFormat texFormat = GET_TPAGE_FORMAT(page);

	if (curSplit.blendMode == blendMode && curSplit.texFormat == texFormat && curSplit.textureId == textureId)
	{
		return;
	}

	curSplit.vCount = g_vertexIndex - curSplit.vIndex;

	VertexBufferSplit& split = g_splits[++g_splitIndex];

	split.textureId = textureId;
	split.vIndex    = g_vertexIndex;
	split.vCount    = 0;
	split.blendMode = blendMode;
	split.texFormat = texFormat;
}

void MakeTriangle()
{
	g_vertexBuffer[g_vertexIndex + 5] = g_vertexBuffer[g_vertexIndex + 3];
	g_vertexBuffer[g_vertexIndex + 3] = g_vertexBuffer[g_vertexIndex];
	g_vertexBuffer[g_vertexIndex + 4] = g_vertexBuffer[g_vertexIndex + 2];
}

void DrawSplit(const VertexBufferSplit& split)
{
	Emulator_SetTexture(split.textureId, split.texFormat);
	Emulator_SetBlendMode(split.blendMode);
	Emulator_DrawTriangles(split.vIndex, split.vCount / 3);
}

void DrawAggregatedSplits()
{
	if (g_emulatorPaused)
	{
		for (int i = 0; i < 3; i++)
		{
			struct Vertex* vert = &g_vertexBuffer[g_polygonSelected + i];
			vert->r = 255;
			vert->g = 0;
			vert->b = 0;

			eprintf("==========================================\n");
			eprintf("POLYGON: %d\n", i);
			eprintf("X: %d Y: %d\n", vert->x, vert->y);
			eprintf("U: %d V: %d\n", vert->u, vert->v);
			eprintf("TP: %d CLT: %d\n", vert->page, vert->clut);
			eprintf("==========================================\n");
		}
		
		Emulator_UpdateInput();
	}

	// next code ideally should be called before EndScene
	Emulator_UpdateVertexBuffer(g_vertexBuffer, g_vertexIndex);

	for (int i = 1; i <= g_splitIndex; i++)
		DrawSplit(g_splits[i]);

	ClearVBO();
}

// forward declarations
int ParsePrimitive(uintptr_t primPtr);
int ParseLinkedPrimitiveList(uintptr_t packetStart, uintptr_t packetEnd);

void AggregatePTAGsToSplits(u_long* p, bool singlePrimitive)
{
	if (!p)
		return;

	if (singlePrimitive)
	{
		// single primitive
		ParsePrimitive((uintptr_t)p);
		g_splits[g_splitIndex].vCount = g_vertexIndex - g_splits[g_splitIndex].vIndex;
	}
	else
	{
		P_TAG* pTag = (P_TAG*)p;

		// P_TAG as primitive list
		//do
		while ((uintptr_t)pTag != (uintptr_t)&terminator)
		{
			if (pTag->len > 0)
			{
				int lastSize = ParseLinkedPrimitiveList((uintptr_t)pTag, (uintptr_t)pTag + (uintptr_t)(pTag->len * 4) + 4 + LEN_OFFSET);
				if (lastSize == -1)
					break; // safe bailout
			}
			pTag = (P_TAG*)pTag->addr;
		}
	}
}

//------------------------------------------------------------------

void DrawOTagEnv(u_long* p, DRAWENV* env)
{
	do
	{
		PutDrawEnv(env);
		DrawOTag(p);
	} while (g_emulatorPaused);

#if defined(PGXP)
	// Reset the ztable.
	memset(&pgxp_vertex_buffer[0], 0, pgxp_vertex_index * sizeof(PGXPVertex));

	// Reset the ztable index of.
	pgxp_vertex_index = 0;

	// Reset last quad match
	last_pgxp_quad_match = 0;

	// Reset last tri match
	last_pgxp_tri_match = 0;
#endif
}

void DrawOTag(u_long* p)
{
	if (Emulator_BeginScene())
	{
		ClearVBO();
		ResetPolyState();
	}

#if defined(DEBUG_POLY_COUNT)
	polygon_count = 0;
#endif

	if (activeDrawEnv.isbg)
	{
		ClearImage(&activeDrawEnv.clip, activeDrawEnv.r0, activeDrawEnv.g0, activeDrawEnv.b0);
	}
	else
	{
		Emulator_BlitVRAM();
	}

	AggregatePTAGsToSplits(p, FALSE);

	DrawAggregatedSplits();
	Emulator_EndScene();

#if defined(PGXP)
	/* Reset the ztable */
	memset(&pgxp_vertex_buffer[0], 0, pgxp_vertex_index * sizeof(PGXPVertex));

	/* Reset the ztable index of */
	pgxp_vertex_index = 0;
#endif
}

void DrawPrim(void* p)
{
	if (Emulator_BeginScene())
	{
		ClearVBO();
		ResetPolyState();

		if (activeDrawEnv.isbg)
		{
			ClearImage(&activeDrawEnv.clip, activeDrawEnv.r0, activeDrawEnv.g0, activeDrawEnv.b0);
		}
		else {
			Emulator_BlitVRAM();
		}
	}

#if defined(DEBUG_POLY_COUNT)
	polygon_count = 0;
#endif

	AggregatePTAGsToSplits((u_long*)p, TRUE);
	DrawAggregatedSplits();

#if defined(PGXP)
	/* Reset the ztable */
	memset(&pgxp_vertex_buffer[0], 0, pgxp_vertex_index * sizeof(PGXPVertex));

	/* Reset the ztable index of */
	pgxp_vertex_index = 0;
#endif
}

// parses primitive and pushes it to VBO
// returns primitive size
// -1 means invalid primitive
int ParsePrimitive(uintptr_t primPtr)
{
	P_TAG* pTag = (P_TAG*)primPtr;

	int textured = (pTag->code & 0x4) != 0;

	int blend_mode = 0;

	if (textured)
	{
		if ((pTag->code & 0x1) != 0)
		{
			blend_mode = 2;
		}
		else
		{
			blend_mode = 1;
		}
	}
	else
	{
		blend_mode = 0;
	}

	bool semi_transparent = (pTag->code & 2) != 0;

	int primitive_size = -1;	// -1

	switch (pTag->code & ~3)
	{
		case 0x0:
		{
			primitive_size = 4;
			break;
		}
		case 0x20:
		{
			POLY_F3* poly = (POLY_F3*)pTag;

			AddSplit(semi_transparent, activeDrawEnv.tpage, whiteTexture);

			Emulator_GenerateVertexArrayTriangle(&g_vertexBuffer[g_vertexIndex], &poly->x0, &poly->x1, &poly->x2);
			Emulator_GenerateTexcoordArrayTriangleZero(&g_vertexBuffer[g_vertexIndex], 0);
			Emulator_GenerateColourArrayTriangle(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r0, &poly->r0);

			g_vertexIndex += 3;

			primitive_size = sizeof(POLY_F3);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x24:
		{
			POLY_FT3* poly = (POLY_FT3*)pTag;
			activeDrawEnv.tpage = poly->tpage;

			AddSplit(semi_transparent, poly->tpage, vramTexture);

			Emulator_GenerateVertexArrayTriangle(&g_vertexBuffer[g_vertexIndex], &poly->x0, &poly->x1, &poly->x2);
			Emulator_GenerateTexcoordArrayTriangle(&g_vertexBuffer[g_vertexIndex], &poly->u0, &poly->u1, &poly->u2, poly->tpage, poly->clut, GET_TPAGE_DITHER(lastTpage));
			Emulator_GenerateColourArrayTriangle(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r0, &poly->r0);

			g_vertexIndex += 3;

			primitive_size = sizeof(POLY_FT3);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x28:
		{
			POLY_F4* poly = (POLY_F4*)pTag;

			AddSplit(semi_transparent, activeDrawEnv.tpage, whiteTexture);

			Emulator_GenerateVertexArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->x0, &poly->x1, &poly->x3, &poly->x2);
			Emulator_GenerateTexcoordArrayQuadZero(&g_vertexBuffer[g_vertexIndex], 0);
			Emulator_GenerateColourArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r0, &poly->r0, &poly->r0);

			MakeTriangle();

			g_vertexIndex += 6;
			primitive_size = sizeof(POLY_F4);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x2C:
		{
			POLY_FT4* poly = (POLY_FT4*)pTag;
			activeDrawEnv.tpage = poly->tpage;

			AddSplit(semi_transparent, poly->tpage, vramTexture);

			Emulator_GenerateVertexArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->x0, &poly->x1, &poly->x3, &poly->x2);
			Emulator_GenerateTexcoordArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->u0, &poly->u1, &poly->u3, &poly->u2, poly->tpage, poly->clut, GET_TPAGE_DITHER(lastTpage));
			Emulator_GenerateColourArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r0, &poly->r0, &poly->r0);

			MakeTriangle();

			g_vertexIndex += 6;

			primitive_size = sizeof(POLY_FT4);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x30:
		{
			POLY_G3* poly = (POLY_G3*)pTag;

			AddSplit(semi_transparent, activeDrawEnv.tpage, whiteTexture);

			Emulator_GenerateVertexArrayTriangle(&g_vertexBuffer[g_vertexIndex], &poly->x0, &poly->x1, &poly->x2);
			Emulator_GenerateTexcoordArrayTriangleZero(&g_vertexBuffer[g_vertexIndex], 1);
			Emulator_GenerateColourArrayTriangle(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r1, &poly->r2);

			g_vertexIndex += 3;

			primitive_size = sizeof(POLY_G3);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x34:
		{
			POLY_GT3* poly = (POLY_GT3*)pTag;
			activeDrawEnv.tpage = poly->tpage;

			AddSplit(semi_transparent, poly->tpage, vramTexture);

			Emulator_GenerateVertexArrayTriangle(&g_vertexBuffer[g_vertexIndex], &poly->x0, &poly->x1, &poly->x2);
			Emulator_GenerateTexcoordArrayTriangle(&g_vertexBuffer[g_vertexIndex], &poly->u0, &poly->u1, &poly->u2, poly->tpage, poly->clut, GET_TPAGE_DITHER(lastTpage));
			Emulator_GenerateColourArrayTriangle(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r1, &poly->r2);

			g_vertexIndex += 3;

			primitive_size = sizeof(POLY_GT3);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x38:
		{
			POLY_G4* poly = (POLY_G4*)pTag;

			AddSplit(semi_transparent, activeDrawEnv.tpage, whiteTexture);

			Emulator_GenerateVertexArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->x0, &poly->x1, &poly->x3, &poly->x2);
			Emulator_GenerateTexcoordArrayQuadZero(&g_vertexBuffer[g_vertexIndex], 1);
			Emulator_GenerateColourArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r1, &poly->r3, &poly->r2);

			MakeTriangle();

			g_vertexIndex += 6;

			primitive_size = sizeof(POLY_G4);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x3C:
		{
			POLY_GT4* poly = (POLY_GT4*)pTag;
			activeDrawEnv.tpage = poly->tpage;

			AddSplit(semi_transparent, poly->tpage, vramTexture);

			Emulator_GenerateVertexArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->x0, &poly->x1, &poly->x3, &poly->x2);
			Emulator_GenerateTexcoordArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->u0, &poly->u1, &poly->u3, &poly->u2, poly->tpage, poly->clut, GET_TPAGE_DITHER(lastTpage));
			Emulator_GenerateColourArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r1, &poly->r3, &poly->r2);

			MakeTriangle();

			g_vertexIndex += 6;

			primitive_size = sizeof(POLY_GT4);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x40:
		{
			LINE_F2* poly = (LINE_F2*)pTag;

			AddSplit(semi_transparent, activeDrawEnv.tpage, whiteTexture);

			Emulator_GenerateLineArray(&g_vertexBuffer[g_vertexIndex], &poly->x0, &poly->x1);
			Emulator_GenerateTexcoordArrayLineZero(&g_vertexBuffer[g_vertexIndex], 0);
			Emulator_GenerateColourArrayLine(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r0);

			MakeTriangle();

			g_vertexIndex += 6;

			primitive_size = sizeof(LINE_F2);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x48: // TODO (unused)
		{
			LINE_F3* poly = (LINE_F3*)pTag;
			/*
						for (int i = 0; i < 2; i++)
						{
							AddSplit(POLY_TYPE_LINES, semi_transparent, activeDrawEnv.tpage, whiteTexture);

							if (i == 0)
							{
								//First line
								Emulator_GenerateLineArray(&g_vertexBuffer[g_vertexIndex], &poly->x0, &poly->x1, NULL, NULL);
								Emulator_GenerateColourArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->r0, NULL, NULL, NULL);
								g_vertexIndex += 2;
							}
							else
							{
								//Second line
								Emulator_GenerateLineArray(&g_vertexBuffer[g_vertexIndex], &poly->x1, &poly->x2, NULL, NULL);
								Emulator_GenerateColourArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->r0, NULL, NULL, NULL);
								g_vertexIndex += 2;
							}
			#if defined(DEBUG_POLY_COUNT)
							polygon_count++;
			#endif
						}
			*/

			primitive_size = sizeof(LINE_F3);
			break;
		}
		case 0x50:
		{
			LINE_G2* poly = (LINE_G2*)pTag;

			AddSplit(semi_transparent, activeDrawEnv.tpage, whiteTexture);

			Emulator_GenerateLineArray(&g_vertexBuffer[g_vertexIndex], &poly->x0, &poly->x1);
			Emulator_GenerateTexcoordArrayLineZero(&g_vertexBuffer[g_vertexIndex], 0);
			Emulator_GenerateColourArrayLine(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r1);

			MakeTriangle();

			g_vertexIndex += 6;

			primitive_size = sizeof(LINE_G2);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x60:
		{
			TILE* poly = (TILE*)pTag;

			AddSplit(semi_transparent, activeDrawEnv.tpage, whiteTexture);

			Emulator_GenerateVertexArrayRect(&g_vertexBuffer[g_vertexIndex], &poly->x0, poly->w, poly->h);
			Emulator_GenerateTexcoordArrayQuadZero(&g_vertexBuffer[g_vertexIndex], 0);
			Emulator_GenerateColourArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r0, &poly->r0, &poly->r0);

			MakeTriangle();

			g_vertexIndex += 6;

			primitive_size = sizeof(TILE);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif

			break;
		}
		case 0x64:
		{
			SPRT* poly = (SPRT*)pTag;

			AddSplit(semi_transparent, activeDrawEnv.tpage, vramTexture);

			Emulator_GenerateVertexArrayRect(&g_vertexBuffer[g_vertexIndex], &poly->x0, poly->w, poly->h);
			Emulator_GenerateTexcoordArrayRect(&g_vertexBuffer[g_vertexIndex], &poly->u0, activeDrawEnv.tpage, poly->clut, poly->w, poly->h);
			Emulator_GenerateColourArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r0, &poly->r0, &poly->r0);

			MakeTriangle();

			g_vertexIndex += 6;

			primitive_size = sizeof(SPRT);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x68:
		{
			TILE_1* poly = (TILE_1*)pTag;

			AddSplit(semi_transparent, activeDrawEnv.tpage, whiteTexture);

			Emulator_GenerateVertexArrayRect(&g_vertexBuffer[g_vertexIndex], &poly->x0, 1, 1);
			Emulator_GenerateTexcoordArrayQuadZero(&g_vertexBuffer[g_vertexIndex], 0);
			Emulator_GenerateColourArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r0, &poly->r0, &poly->r0);

			MakeTriangle();

			g_vertexIndex += 6;

			primitive_size = sizeof(TILE_1);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x70:
		{
			TILE_8* poly = (TILE_8*)pTag;

			AddSplit(semi_transparent, activeDrawEnv.tpage, whiteTexture);

			Emulator_GenerateVertexArrayRect(&g_vertexBuffer[g_vertexIndex], &poly->x0, 8, 8);
			Emulator_GenerateTexcoordArrayQuadZero(&g_vertexBuffer[g_vertexIndex], 0);
			Emulator_GenerateColourArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r0, &poly->r0, &poly->r0);

			MakeTriangle();

			g_vertexIndex += 6;

			primitive_size = sizeof(TILE_8);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x74:
		{
			SPRT_8* poly = (SPRT_8*)pTag;

			AddSplit(semi_transparent, activeDrawEnv.tpage, vramTexture);

			Emulator_GenerateVertexArrayRect(&g_vertexBuffer[g_vertexIndex], &poly->x0, 8, 8);
			Emulator_GenerateTexcoordArrayRect(&g_vertexBuffer[g_vertexIndex], &poly->u0, activeDrawEnv.tpage, poly->clut, 8, 8);
			Emulator_GenerateColourArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r0, &poly->r0, &poly->r0);

			MakeTriangle();

			g_vertexIndex += 6;

			primitive_size = sizeof(SPRT_8);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x78:
		{
			TILE_16* poly = (TILE_16*)pTag;

			AddSplit(semi_transparent, activeDrawEnv.tpage, whiteTexture);

			Emulator_GenerateVertexArrayRect(&g_vertexBuffer[g_vertexIndex], &poly->x0, 16, 16);
			Emulator_GenerateTexcoordArrayQuadZero(&g_vertexBuffer[g_vertexIndex], 0);
			Emulator_GenerateColourArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r0, &poly->r0, &poly->r0);

			MakeTriangle();

			g_vertexIndex += 6;

			primitive_size = sizeof(TILE_16);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0x7C:
		{
			SPRT_16* poly = (SPRT_16*)pTag;

			AddSplit(semi_transparent, activeDrawEnv.tpage, vramTexture);

			Emulator_GenerateVertexArrayRect(&g_vertexBuffer[g_vertexIndex], &poly->x0, 16, 16);
			Emulator_GenerateTexcoordArrayRect(&g_vertexBuffer[g_vertexIndex], &poly->u0, activeDrawEnv.tpage, poly->clut, 16, 16);
			Emulator_GenerateColourArrayQuad(&g_vertexBuffer[g_vertexIndex], &poly->r0, &poly->r0, &poly->r0, &poly->r0);

			MakeTriangle();

			g_vertexIndex += 6;

			primitive_size = sizeof(SPRT_16);
	#if defined(DEBUG_POLY_COUNT)
			polygon_count++;
	#endif
			break;
		}
		case 0xE0:
		{
			switch (pTag->code)
			{
			case 0xE1:
				{
		#if defined(USE_32_BIT_ADDR)
					unsigned short tpage = ((unsigned short*)pTag)[4];
		#else
					unsigned short tpage = ((unsigned short*)pTag)[2];
		#endif
					//if (tpage != 0)
					{
						activeDrawEnv.tpage = tpage;
					}

					primitive_size = sizeof(DR_TPAGE);
		#if defined(DEBUG_POLY_COUNT)
					polygon_count++;
		#endif

					break;
				}
				default:
				{
					eprinterr("Primitive type error");
					assert(FALSE);
					break;
				}
			}
			break;
		}
		case 0x80: {
			eprinterr("DR_MOVE unimplemented\n");
			primitive_size = sizeof(DR_MOVE);
			break;
		}
		default:
		{
			//Unhandled poly type
			eprinterr("Unhandled primitive type: %02X type2:%02X\n", pTag->code, pTag->code & ~3);
			break;
		}
	}

	return primitive_size;
}

int ParseLinkedPrimitiveList(uintptr_t packetStart, uintptr_t packetEnd)
{
	uintptr_t currentAddress = packetStart;

	int lastSize = -1;

	while (currentAddress != packetEnd)
	{
		lastSize = ParsePrimitive(currentAddress);

		if (lastSize == -1)	// not valid packets submitted
			break;

		currentAddress += lastSize;
	}

	g_splits[g_splitIndex].vCount = g_vertexIndex - g_splits[g_splitIndex].vIndex;

	return lastSize;
}

void SetSprt16(SPRT_16* p)
{
	setSprt16(p);
}

void SetSprt8(SPRT_8* p)
{
	setSprt8(p);
}

void SetTile(TILE* p)
{
	setTile(p);
}

void SetPolyGT4(POLY_GT4* p)
{
	setPolyGT4(p);
}

void SetSemiTrans(void* p, int abe)
{
	setSemiTrans(p, abe);
}

void SetShadeTex(void* p, int tge)
{
	setShadeTex(p, tge);
}

void SetSprt(SPRT* p)
{
	setSprt(p);
}

void SetDumpFnt(int id)
{
	if (id >= 0 && fontStreamCount >= id)
	{
		fontCurrentStream = id;
		//sw v0, GPU_printf///@FIXME?
	}
}

void SetLineF3(LINE_F3* p)
{
	setLineF3(p);
}

void FntLoad(int tx, int ty)
{
	unsigned long FontClut[] = { 0xFFFF0000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	unsigned long FontTex[] = {
	  0x00000000,0x00010000,0x00101000,0x00000000,0x00111000,0x01111100,0x00001000,0x00011000,0x01000000,0x00000100,0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00101000,0x00101000,0x01010100,0x01001100,0x00010100,0x00001100,0x00100000,0x00001000,0x01010100,0x00010000,0x00000000,0x00000000,0x00000000,0x01000000,
	  0x00000000,0x00010000,0x00000000,0x01111100,0x00010100,0x00100000,0x00010100,0x00000000,0x00010000,0x00010000,0x00111000,0x00010000,0x00000000,0x00000000,0x00000000,0x00100000,0x00000000,0x00010000,0x00000000,0x00101000,0x00111000,0x00010000,0x00001000,0x00000000,0x00010000,0x00010000,0x00010000,0x01111100,0x00000000,0x01111100,0x00000000,
	  0x00010000,0x00000000,0x00010000,0x00000000,0x00101000,0x01010000,0x00001000,0x01010100,0x00000000,0x00010000,0x00010000,0x00111000,0x00010000,0x00000000,0x00000000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x01111100,0x01010100,0x01100100,0x00100100,0x00000000,0x00100000,0x00001000,0x01010100,0x00010000,0x00011000,0x00000000,
	  0x00011000,0x00000100,0x00000000,0x00010000,0x00000000,0x00101000,0x00111000,0x01100100,0x01011000,0x00000000,0x01000000,0x00000100,0x00010000,0x00000000,0x00011000,0x00000000,0x00011000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00001100,
	  0x00000000,0x00000000,0x00000000,0x00111000,0x00010000,0x00111000,0x00111000,0x00100000,0x01111100,0x00110000,0x01111100,0x00111000,0x00111000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00111000,0x01000100,0x00011000,0x01000100,0x01000100,0x00110000,0x00000100,0x00001000,0x01000000,0x01000100,0x01000100,0x00000000,0x00000000,
	  0x00100000,0x00000000,0x00001000,0x01000100,0x01100100,0x00010000,0x01000000,0x01000000,0x00101000,0x00111100,0x00000100,0x00100000,0x01000100,0x01000100,0x00011000,0x00011000,0x00010000,0x01111100,0x00010000,0x00100000,0x01010100,0x00010000,0x00100000,0x00110000,0x00100100,0x01000000,0x00111100,0x00100000,0x00111000,0x01111000,0x00011000,
	  0x00011000,0x00001000,0x00000000,0x00100000,0x00010000,0x01001100,0x00010000,0x00010000,0x01000000,0x01111100,0x01000000,0x01000100,0x00010000,0x01000100,0x01000000,0x00000000,0x00000000,0x00010000,0x01111100,0x00010000,0x00010000,0x01000100,0x00010000,0x00001000,0x01000100,0x00100000,0x01000100,0x01000100,0x00010000,0x01000100,0x00100000,
	  0x00011000,0x00011000,0x00100000,0x00000000,0x00001000,0x00000000,0x00111000,0x00111000,0x01111100,0x00111000,0x00100000,0x00111000,0x00111000,0x00010000,0x00111000,0x00011000,0x00011000,0x00011000,0x00000000,0x00000000,0x00000000,0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	  0x00000000,0x00000000,0x00001100,0x00000000,0x00000000,0x00000000,0x00000000,0x00111000,0x00111000,0x00111100,0x00111000,0x00111100,0x01111100,0x01111100,0x00111000,0x01000100,0x00111000,0x01000000,0x01000100,0x00000100,0x01000100,0x01000100,0x00111000,0x01000100,0x01000100,0x01000100,0x01000100,0x01001000,0x00000100,0x00000100,0x01000100,
	  0x01000100,0x00010000,0x01000000,0x00100100,0x00000100,0x01101100,0x01000100,0x01000100,0x01110100,0x01000100,0x01000100,0x00000100,0x01001000,0x00000100,0x00000100,0x00000100,0x01000100,0x00010000,0x01000000,0x00010100,0x00000100,0x01010100,0x01001100,0x01000100,0x01010100,0x01111100,0x00111100,0x00000100,0x01001000,0x00111100,0x00111100,
	  0x00000100,0x01111100,0x00010000,0x01000000,0x00001100,0x00000100,0x01000100,0x01010100,0x01000100,0x01110100,0x01000100,0x01000100,0x00000100,0x01001000,0x00000100,0x00000100,0x01100100,0x01000100,0x00010000,0x01000000,0x00010100,0x00000100,0x01000100,0x01100100,0x01000100,0x00000100,0x01000100,0x01000100,0x01000100,0x01001000,0x00000100,
	  0x00000100,0x01000100,0x01000100,0x00010000,0x01000100,0x00100100,0x00000100,0x01000100,0x01000100,0x01000100,0x00111000,0x01000100,0x00111100,0x00111000,0x00111100,0x01111100,0x00000100,0x00111000,0x01000100,0x00111000,0x00111000,0x01000100,0x01111100,0x01000100,0x01000100,0x00111000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00111100,0x00111000,0x00111100,0x00111000,0x01111100,0x01000100,0x01000100,0x01000100,0x01000100,0x01000100,0x01111100,0x00010000,0x00000000,0x00010000,0x00101000,0x00000000,0x01000100,0x01000100,0x01000100,0x01000100,
	  0x00010000,0x01000100,0x01000100,0x01000100,0x01000100,0x01000100,0x01000000,0x00010000,0x00000100,0x00010000,0x01000100,0x00000000,0x01000100,0x01000100,0x01000100,0x00000100,0x00010000,0x01000100,0x01000100,0x01000100,0x00101000,0x00101000,0x00100000,0x00010000,0x00001000,0x00010000,0x00000000,0x00000000,0x00111100,0x01000100,0x00111100,
	  0x00111000,0x00010000,0x01000100,0x01000100,0x01000100,0x00010000,0x00010000,0x01111100,0x00010000,0x00010000,0x00010000,0x00000000,0x00000000,0x00000100,0x01010100,0x00010100,0x01000000,0x00010000,0x01000100,0x00101000,0x01010100,0x00101000,0x00010000,0x00001000,0x00010000,0x00100000,0x00010000,0x00000000,0x00000000,0x00000100,0x00100100,
	  0x00100100,0x01000100,0x00010000,0x01000100,0x00101000,0x01101100,0x01000100,0x00010000,0x00000100,0x00010000,0x01000000,0x00010000,0x00000000,0x00000000,0x00000100,0x01011000,0x01000100,0x00111000,0x00010000,0x00111000,0x00010000,0x01000100,0x01000100,0x00010000,0x01111100,0x00010000,0x00000000,0x00010000,0x00000000,0x00000000,0x00000000,
	  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x01110000,0x00000000,0x00011100,0x00000000,0x00000000 };
	
	LoadClut2(FontClut, tx, ty + 128);
	LoadTPage(FontTex, 0, 0, tx, ty, 128, 32);
}

void AddPrim(void* ot, void* p)
{
	addPrim(ot, p);
}

void AddPrims(void* ot, void* p0, void* p1)
{
	addPrims(ot, p0, p1);
}

void CatPrim(void* p0, void* p1)
{
	catPrim(p0, p1);
}

u_short LoadTPage(u_long* pix, int tp, int abr, int x, int y, int w, int h)
{
	RECT16 imageArea;
	imageArea.x = x;
	imageArea.y = y;
	imageArea.h = h;

	enum
	{
		TP_4BIT,
		TP_8BIT,
		TP_16BIT
	};

	switch (tp)
	{
	case TP_4BIT:
	{
		//loc_278
		if (w >= 0)
		{
			imageArea.w = w >> 2;
		}
		else
		{
			imageArea.w = (w + 3) >> 2;
		}
		break;
	}
	case TP_8BIT:
	{
		//loc_290
		imageArea.w = (w + (w >> 31)) >> 1;
		break;
	}
	case TP_16BIT:
	{
		//loc_2A4
		imageArea.w = w;
		break;
	}
	}

	//loc_2AC
	LoadImagePSX(&imageArea, pix);
	return GetTPage(tp, abr, x, y) & 0xFFFF;
}

u_short GetTPage(int tp, int abr, int x, int y)
{
	return getTPage(tp, abr, x, y);
}

u_short LoadClut(u_long* clut, int x, int y)
{
	RECT16 rect;//&var_18
	setRECT16(&rect, x, y, 256, 1);
	LoadImagePSX(&rect, clut);
	return GetClut(x, y) & 0xFFFF;
}

u_short LoadClut2(u_long* clut, int x, int y)
{
	RECT16 drawArea;
	drawArea.x = x;
	drawArea.y = y;
	drawArea.w = 16;
	drawArea.h = 1;
	LoadImagePSX(&drawArea, clut);
	return getClut(x, y);
}

u_long* KanjiFntFlush(int id)
{
	UNIMPLEMENTED();
	return 0;
}

u_long* FntFlush(int id)
{
	UNIMPLEMENTED();
	return 0;
}

int KanjiFntOpen(int x, int y, int w, int h, int dx, int dy, int cx, int cy, int isbg, int n)
{
	UNIMPLEMENTED();
	return 0;
}

int FntOpen(int x, int y, int w, int h, int isbg, int n)
{
	RECT16 rect;
	int characterCount = n;
	int i;
	SPRT_8* pSprite;

	//Maximum number of font streams is 8.
	if (fontStreamCount >= MAX_NUM_FONT_STREAMS)
	{
		return -1;
	}

	//loc_338
	if (fontStreamCount == 0)
	{
		fontUsedCharacterCount = 0;
	}

	//loc_348
	font[fontStreamCount].unk07 = w < 1;

	if (characterCount + fontUsedCharacterCount >= MAX_FONT_CHARACTER_COUNT)
	{
		characterCount = MAX_FONT_CHARACTER_COUNT - fontUsedCharacterCount;
	}

	//loc_37C
	rect.x = 0;
	rect.x = 0;
	rect.w = 256;
	rect.h = 256;
	SetDrawMode(&fontMode, 0, 0, fontTpage, &rect);

	//Should we clear the draw area?
	if (isbg != FALSE)
	{
		SetTile(&font[fontStreamCount].tile);
		setRGB0(&font[fontStreamCount].tile, 0, 0, 0);
		SetSemiTrans(&font[fontStreamCount].tile, (isbg ^ 2) < 1);
	}

	//loc_460
	setXY0(&font[fontStreamCount].tile, x, y);
	setWH(&font[fontStreamCount].tile, w, h);

	font[fontStreamCount].characterCount = n;
	font[fontStreamCount].unk06 = 0;
	font[fontStreamCount].unk05 = &unk_1B00[fontUsedCharacterCount];
	font[fontStreamCount].pSprites = (SPRT_8*)&unk_1F00[fontUsedCharacterCount << 4];
	font[fontStreamCount].unk05[0] = 0;
	
	if (n > 0)
	{
		pSprite = &font[fontStreamCount].pSprites[0];

		for (int i = 0; i < n; i++, pSprite++)
		{
			SetSprt8(pSprite);
			pSprite->clut = fontClut;
		}
	}

	fontUsedCharacterCount += n;
	fontStreamCount += 1;

	return fontStreamCount - 1;
}

void SetPolyF4(POLY_F4* p)
{
	setPolyF4(p);
}

void SetPolyFT4(POLY_FT4* p)
{
	setPolyFT4(p);
}

void SetPolyG4(POLY_G4* p)
{
	setPolyG4(p);
}

void TermPrim(void* p)
{
	termPrim(p);
}