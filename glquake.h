/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <GL/gl.h>
//#include <GL/glu.h>

#ifndef _WIN32
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#endif

extern unsigned int d_8to24table[256]; // for GL renderer
extern unsigned int d_8to24table_fbright[256];
extern unsigned int d_8to24table_nobright[256];
extern unsigned int d_8to24table_conchars[256];
extern unsigned int d_8to24TranslucentTable[256];

// wgl uses APIENTRY
#ifndef APIENTRY
#define APIENTRY
#endif

// for platforms (wgl) that do not use GLAPIENTRY
#ifndef GLAPIENTRY
#define GLAPIENTRY APIENTRY
#endif 

#ifdef _WIN32
#define qglGetProcAddress wglGetProcAddress
#else
#define glXGetProcAddress glXGetProcAddressARB
#define qglGetProcAddress(x) glXGetProcAddress((const GLubyte *)(x))
#endif

// GL_ARB_multitexture
void (GLAPIENTRY *qglMultiTexCoord1f) (GLenum, GLfloat);
void (GLAPIENTRY *qglMultiTexCoord2f) (GLenum, GLfloat, GLfloat);
void (GLAPIENTRY *qglMultiTexCoord3f) (GLenum, GLfloat, GLfloat, GLfloat);
void (GLAPIENTRY *qglMultiTexCoord4f) (GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
void (GLAPIENTRY *qglActiveTexture) (GLenum);
void (GLAPIENTRY *qglClientActiveTexture) (GLenum);
#ifndef GL_ACTIVE_TEXTURE_ARB
#define GL_ACTIVE_TEXTURE_ARB			0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB	0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB		0x84E2
#define GL_TEXTURE0_ARB					0x84C0
#define GL_TEXTURE1_ARB					0x84C1
#define GL_TEXTURE2_ARB					0x84C2
#define GL_TEXTURE3_ARB					0x84C3
#define GL_TEXTURE4_ARB					0x84C4
#define GL_TEXTURE5_ARB					0x84C5
#define GL_TEXTURE6_ARB					0x84C6
#define GL_TEXTURE7_ARB					0x84C7
#define GL_TEXTURE8_ARB					0x84C8
#define GL_TEXTURE9_ARB					0x84C9
#define GL_TEXTURE10_ARB				0x84CA
#define GL_TEXTURE11_ARB				0x84CB
#define GL_TEXTURE12_ARB				0x84CC
#define GL_TEXTURE13_ARB				0x84CD
#define GL_TEXTURE14_ARB				0x84CE
#define GL_TEXTURE15_ARB				0x84CF
#define GL_TEXTURE16_ARB				0x84D0
#define GL_TEXTURE17_ARB				0x84D1
#define GL_TEXTURE18_ARB				0x84D2
#define GL_TEXTURE19_ARB				0x84D3
#define GL_TEXTURE20_ARB				0x84D4
#define GL_TEXTURE21_ARB				0x84D5
#define GL_TEXTURE22_ARB				0x84D6
#define GL_TEXTURE23_ARB				0x84D7
#define GL_TEXTURE24_ARB				0x84D8
#define GL_TEXTURE25_ARB				0x84D9
#define GL_TEXTURE26_ARB				0x84DA
#define GL_TEXTURE27_ARB				0x84DB
#define GL_TEXTURE28_ARB				0x84DC
#define GL_TEXTURE29_ARB				0x84DD
#define GL_TEXTURE30_ARB				0x84DE
#define GL_TEXTURE31_ARB				0x84DF
#endif 

// GL_EXT_texture_env_combine, the values for GL_ARB_ are identical
#define GL_COMBINE_EXT					0x8570
#define GL_COMBINE_RGB_EXT				0x8571
#define GL_COMBINE_ALPHA_EXT			0x8572
#define GL_RGB_SCALE_EXT				0x8573
#define GL_CONSTANT_EXT					0x8576
#define GL_PRIMARY_COLOR_EXT			0x8577
#define GL_PREVIOUS_EXT					0x8578
#define GL_SOURCE0_RGB_EXT				0x8580
#define GL_SOURCE1_RGB_EXT				0x8581
#define GL_SOURCE0_ALPHA_EXT			0x8588
#define GL_SOURCE1_ALPHA_EXT			0x8589

// Multitexture
extern GLenum TEXTURE0, TEXTURE1;
extern qboolean mtexenabled;

extern const char *gl_vendor;
extern const char *gl_renderer;
extern const char *gl_version;
extern const char *gl_extensions;
#ifndef _WIN32
extern const char *glx_extensions;
#endif

extern qboolean fullsbardraw;
extern qboolean isIntel; // intel video workaround
extern qboolean gl_mtexable;
extern qboolean gl_texture_env_combine;
extern qboolean gl_texture_env_add;

extern int gl_hardware_max_size;
extern int gl_texture_max_size;

extern int gl_warpimage_size;

// Swap control
GLint (GLAPIENTRY *qglSwapInterval)(GLint interval);

#ifdef _WIN32
#define SWAPCONTROLSTRING "WGL_EXT_swap_control"
#define SWAPINTERVALFUNC "wglSwapIntervalEXT"
#else
#define SWAPCONTROLSTRING "GLX_SGI_swap_control"
#define SWAPINTERVALFUNC "glXSwapIntervalSGI"
#endif

// Anisotropic filtering
#define GL_TEXTURE_MAX_ANISOTROPY_EXT		0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT	0x84FF

extern float gl_hardware_max_anisotropy;
extern float gl_texture_anisotropy;

//====================================================
// mh - new defines for lightmapping
#define GL_BGRA 0x80E1
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367 

//====================================================

#define TEXPREF_NONE			0x0000
#define TEXPREF_MIPMAP			0x0001	// generate mipmaps
#define TEXPREF_LINEAR			0x0002	// force linear
#define TEXPREF_NEAREST			0x0004	// force nearest
#define TEXPREF_ALPHA			0x0008	// allow alpha
#define TEXPREF_PAD				0x0010	// allow padding (UNUSED)
#define TEXPREF_PERSIST			0x0020	// never free (UNUSED)
#define TEXPREF_OVERWRITE		0x0040	// overwrite existing same-name texture
#define TEXPREF_NOPICMIP		0x0080	// always load full-sized
#define TEXPREF_FULLBRIGHT		0x0100	// use fullbright mask palette
#define TEXPREF_NOBRIGHT		0x0200	// use nobright mask palette
#define TEXPREF_CONCHARS		0x0400	// use conchars palette
#define TEXPREF_WARPIMAGE		0x0800	// resize this texture when gl_warpimage_size changes (UNUSED)
#define TEXPREF_BLOOM			0x1000	// bloom texture (UNUSED)
/*
* mode:
* 0 - standard
* 1 - color 0 transparent, odd - translucent, even - full value
* 2 - color 0 transparent
* 3 - special (particle translucency table)
*/
#define TEXPREF_TRANSPARENT		0x2000	// EF_TRANSPARENT	(mode 1)
#define TEXPREF_HOLEY			0x4000	// EF_HOLEY			(mode 2)
#define TEXPREF_SPECIAL_TRANS	0x8000	// EF_SPECIAL_TRANS	(mode 3)

enum srcformat {SRC_INDEXED, SRC_LIGHTMAP, SRC_RGBA, SRC_BLOOM};

typedef struct gltexture_s {
//managed by texture manager
	GLuint				texnum;
	struct gltexture_s	*next;
	model_t				*owner;
//managed by image loading
	char				name[64];
	unsigned int		width;						// size of image as it exists in opengl
	unsigned int		height;						// size of image as it exists in opengl
	unsigned int		flags;						// texture preference flags
	char				source_file[MAX_QPATH];		// relative filepath to data source, or "" if source is in memory
	unsigned int		source_offset;				// byte offset into file, or memory address
	enum srcformat		source_format;				// format of pixel data (indexed, lightmap, or rgba)
	unsigned int		source_width;				// size of image in source data
	unsigned int		source_height;				// size of image in source data
	unsigned short		source_crc;					// generated by source data before modifications
} gltexture_t;

extern	model_t	*loadmodel;

typedef struct
{
//	int		gltexture;
	gltexture_t	*gltexture;
	float	sl, tl, sh, th;
} glpic_t;


// vid_*gl*.c
void GL_BeginRendering (int *x, int *y, int *width, int *height);
void GL_EndRendering (void);

// gl_main.c
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
qboolean R_CullModelForEntity (entity_t *e);

// gl_draw.c
void GL_Upload8 (gltexture_t *glt, byte *data);
void GL_Upload32 (gltexture_t *glt, unsigned *data);
void GL_UploadBloom (gltexture_t *glt, unsigned *data);
void GL_UploadLightmap (gltexture_t *glt, byte *data);
void GL_FreeTexture (gltexture_t *free);
void GL_FreeTextures (model_t *owner);
void GL_ReloadTexture (gltexture_t *glt);
void GL_ReloadTextures_f (void);
gltexture_t *GL_LoadTexture (model_t *owner, char *name, int width, int height, enum srcformat format, byte *data, char *source_file, unsigned source_offset, unsigned flags);
gltexture_t *GL_FindTexture (model_t *owner, char *name);
void GL_Set2D (void);
//void Draw_Pic (int x, int y, qpic_t *pic);
//void Draw_SubPic (int x, int y, qpic_t *pic, int srcx, int srcy, int width, int height);
void GL_SelectTexture (GLenum target);
void GL_Bind (gltexture_t *texture);
void GL_DisableMultitexture (void);
void GL_EnableMultitexture (void);
void GL_Init (void);
void GL_SetupState (void);
void GL_SwapInterval (void);
void GL_UploadWarpImage (void);

// gl_mesh.c
void R_MakeAliasModelDisplayLists (model_t *m, aliashdr_t *hdr);

// gl_misc.c
void R_InitTranslatePlayerTextures (void);
void R_TranslatePlayerSkin (int playernum);

// gl_part.c
void R_InitParticles (void);
void R_DrawParticles (void);
void R_ClearParticles (void);
void R_DarkFieldParticles (entity_t *ent);

// gl_efrag.c
void R_StoreEfrags (efrag_t **ppefrag);
 
// gl_light.c
void R_AnimateLight (void);



extern	int texture_extension_number;
extern	int		texture_mode;


#define MAX_EXTRA_TEXTURES 156   // 255-100+1
extern gltexture_t			*gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models


void GL_SubdivideSurface (msurface_t *fa);
void R_MakeAliasModelDisplayLists (model_t *m, aliashdr_t *hdr);
int R_LightPoint (vec3_t p);
void R_DrawBrushModel (entity_t *e, qboolean Translucent);
void R_AnimateLight (void);
void V_CalcBlend (void);
void R_DrawWorld (void);
void R_RenderDlights (void);
void R_DrawParticles (void);
void R_DrawWaterSurfaces (void);
void R_RenderBrushPoly (msurface_t *fa, qboolean override);
void R_InitParticles (void);
void R_BuildLightmaps (void);
void EmitWaterPolys (msurface_t *fa);
void EmitSkyPolys (msurface_t *fa, qboolean save);
void EmitBothSkyLayers (msurface_t *fa);
void R_DrawSkyChain (msurface_t *s);

void R_MarkLights (dlight_t *light, int bit, mnode_t *node);
void R_RotateForEntity (entity_t *e);
void R_StoreEfrags (efrag_t **ppefrag);
void GL_Set2D (void);
int M_DrawBigCharacter (int x, int y, int num, int numnext); // EER1

void D_ShowLoadingSize (void);

// gl_surf.c
void R_CullSurfaces (void);
void R_DrawBrushModel (entity_t *e, qboolean water);
void R_DrawWorld (void);
void R_DrawTextureChainsWater (void);
void R_DrawSequentialPoly (msurface_t *s);
void R_DrawSequentialWaterPoly (msurface_t *s);
void R_DrawGLPoly34 (glpoly_t *p);
void R_DrawGLPoly56 (glpoly_t *p);
void R_BuildLightmaps (void);

// gl_screen.c
void SCR_TileClear (void);

// gl_anim.c
void R_UpdateWarpTextures (void);
byte *GL_LoadImage (char *name, int *width, int *height);

void R_InitMapGlobals (void);
void R_ParseWorldspawnNewMap (void);
void R_DrawSky (void);
void R_LoadSkyBox (char *skybox);

void R_FogParseServerMessage (void);
void R_FogParseServerMessage2 (void);
float *R_FogGetColor (void);
float R_FogGetDensity (void);
void R_FogEnableGFog (void);
void R_FogDisableGFog (void);
void R_FogStartAdditive (void);
void R_FogStopAdditive (void);
void R_FogSetupFrame (void);

void R_InitBloomTextures (void);
void R_BloomBlend (void);

extern float turbsin[];
#define TURBSCALE (256.0 / (2 * M_PI))
#define WARPCALC(s,t) ((s + turbsin[(int)((t*2)+(cl.time*(128.0/M_PI))) & 255]) * (1.0/64)) // correct warp


extern	int glx, gly, glwidth, glheight;


// private refresh defs

#define ALIAS_BASE_SIZE_RATIO		(1.0 / 11.0)
					// normalizing factor so player model works out to about
					//  1 pixel per triangle
#define MAX_SKIN_HEIGHT 480

#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

#define BACKFACE_EPSILON	0.01
#define COLINEAR_EPSILON	0.001

#define FARCLIP		16384 // 4096
#define NEARCLIP		1 // 4

void R_TimeRefresh_f (void);
void R_ReadPointFile_f (void);
void R_Sky_f (void);
void R_Fog_f (void);

texture_t *R_TextureAnimation (texture_t *base, int frame);


//====================================================


extern	entity_t	r_worldentity;
extern	vec3_t		modelorg, r_entorigin;
extern	entity_t	*currententity;
extern	int			r_visframecount;	// ??? what difs?
extern	int			r_framecount;
extern	mplane_t	frustum[4];
extern	int			rs_c_brush_polys, rs_c_brush_passes, rs_c_alias_polys, rs_c_alias_passes, rs_c_sky_polys, rs_c_sky_passes;
extern	int			rs_c_dynamic_lightmaps, rs_c_particles;

//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_refdef;
extern	mleaf_t		*r_viewleaf, *r_oldviewleaf;
extern	texture_t	*r_notexture_mip;
extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value

extern int	lightmap_bytes;		// 1, 2, or 4
extern int	indexed_bytes;
extern int	rgba_bytes;
extern int	bgra_bytes;

extern	int		gl_lightmap_format;
extern	int		gl_solid_format;
extern	int		gl_alpha_format;

extern	gltexture_t *notexture;
extern	gltexture_t *nulltexture;

extern	gltexture_t *particletexture;
extern	gltexture_t *particletexture1;
extern	gltexture_t *particletexture2;

extern	gltexture_t	*playertextures[MAX_SCOREBOARD];
extern	gltexture_t	*skyboxtextures[6];

extern float globalwateralpha;

//
//
//
extern	cvar_t	scr_weaponfov;

extern	cvar_t	r_norefresh;
extern	cvar_t	r_drawentities;
extern	cvar_t	r_drawworld;
extern	cvar_t	r_drawviewmodel;
extern	cvar_t	r_speeds;
extern	cvar_t	r_waterwarp;
extern	cvar_t	r_fullbright;
extern	cvar_t	r_waterquality;
extern	cvar_t	r_wateralpha;
extern	cvar_t	r_lockalpha;
extern	cvar_t	r_lavafog;
extern	cvar_t	r_slimefog;
extern	cvar_t	r_lavaalpha;
extern	cvar_t	r_slimealpha;
extern	cvar_t	r_telealpha;
extern	cvar_t	r_dynamic;
extern	cvar_t	r_novis;
extern	cvar_t	r_lockfrustum;
extern	cvar_t	r_lockpvs;
extern	cvar_t	r_clearcolor;
extern	cvar_t	r_fastsky;
extern	cvar_t	r_skyquality;
extern	cvar_t	r_skyalpha;
extern	cvar_t	r_skyfog;
extern	cvar_t	r_oldsky;

extern	cvar_t	gl_finish;
extern	cvar_t	gl_clear;
extern	cvar_t	gl_cull;
extern	cvar_t	gl_poly;

extern	cvar_t	gl_smoothmodels;
extern	cvar_t	gl_affinemodels;
extern	cvar_t	gl_polyblend;
extern	cvar_t	gl_keeptjunctions;
extern	cvar_t	gl_reporttjunctions;
extern	cvar_t	gl_flashblend;
extern	cvar_t	gl_nocolors;

extern	cvar_t	gl_zfix; // z-fighting fix

extern	cvar_t	gl_max_size;
extern	cvar_t	gl_playermip;

// Nehahra
extern	cvar_t  gl_fogenable;
extern	cvar_t  gl_fogdensity;
extern	cvar_t  gl_fogred;
extern	cvar_t  gl_foggreen;
extern	cvar_t  gl_fogblue;

extern	cvar_t	r_bloom;
extern	cvar_t	r_bloom_darken;
extern	cvar_t	r_bloom_alpha;
extern	cvar_t	r_bloom_intensity;
extern	cvar_t	r_bloom_diamond_size;
extern	cvar_t	r_bloom_sample_size;
extern	cvar_t	r_bloom_fast_sample;


extern	mplane_t	*mirror_plane;

extern	float	r_world_matrix[16];

extern	const char *gl_vendor;
extern	const char *gl_renderer;
extern	const char *gl_version;
extern	const char *gl_extensions;

void R_InitTranslatePlayerTextures (void);
void R_TranslatePlayerSkin (int playernum);

extern byte *playerTranslation;

//tmp here
extern float RTint[256],GTint[256],BTint[256];



