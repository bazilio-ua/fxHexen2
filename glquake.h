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
// glquake.h

#if defined __APPLE__ && defined __MACH__
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <dlfcn.h>
#else

#include <GL/gl.h>

#ifndef _WIN32
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#endif

#endif

extern unsigned int d_8to24table_original[256];
extern unsigned int d_8to24table_opaque[256];
extern unsigned int d_8to24table[256];
extern unsigned int d_8to24table_fullbright[256];
extern unsigned int d_8to24table_fullbright_holey[256];
extern unsigned int d_8to24table_nobright[256];
extern unsigned int d_8to24table_nobright_holey[256];
extern unsigned int d_8to24table_conchars[256];
extern unsigned int d_8to24TranslucentTable[256];

extern unsigned int is_fullbright[256/32];

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
#elif defined __APPLE__ && defined __MACH__
#define qglGetProcAddress(x) dlsym(RTLD_DEFAULT, (x))
#elif defined GLX_GLXEXT_PROTOTYPES
#define glXGetProcAddress glXGetProcAddressARB
#define qglGetProcAddress(x) glXGetProcAddress((const GLubyte *)(x))
#endif

// Multitexture
void (GLAPIENTRY *qglMultiTexCoord1f) (GLenum, GLfloat);
void (GLAPIENTRY *qglMultiTexCoord2f) (GLenum, GLfloat, GLfloat);
void (GLAPIENTRY *qglMultiTexCoord3f) (GLenum, GLfloat, GLfloat, GLfloat);
void (GLAPIENTRY *qglMultiTexCoord4f) (GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
void (GLAPIENTRY *qglActiveTexture) (GLenum);
void (GLAPIENTRY *qglClientActiveTexture) (GLenum);

// GL_ARB_multitexture
#ifndef GL_ARB_multitexture
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
#define GL_ACTIVE_TEXTURE_ARB                                0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB                         0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB                             0x84E2
#endif

// GL_ARB_texture_env_combine, the values for GL_EXT_ are identical
#ifndef GL_ARB_texture_env_combine
#define GL_COMBINE_ARB                                       0x8570
#define GL_COMBINE_RGB_ARB                                   0x8571
#define GL_COMBINE_ALPHA_ARB                                 0x8572
#define GL_RGB_SCALE_ARB                                     0x8573
#define GL_ADD_SIGNED_ARB                                    0x8574
#define GL_INTERPOLATE_ARB                                   0x8575
#define GL_CONSTANT_ARB                                      0x8576
#define GL_PRIMARY_COLOR_ARB                                 0x8577
#define GL_PREVIOUS_ARB                                      0x8578
#define GL_SUBTRACT_ARB                                      0x84E7
#define GL_SOURCE0_RGB_ARB                                   0x8580
#define GL_SOURCE1_RGB_ARB                                   0x8581
#define GL_SOURCE2_RGB_ARB                                   0x8582
#define GL_SOURCE0_ALPHA_ARB                                 0x8588
#define GL_SOURCE1_ALPHA_ARB                                 0x8589
#define GL_SOURCE2_ALPHA_ARB                                 0x858A
#define GL_OPERAND0_RGB_ARB                                  0x8590
#define GL_OPERAND1_RGB_ARB                                  0x8591
#define GL_OPERAND2_RGB_ARB                                  0x8592
#define GL_OPERAND0_ALPHA_ARB                                0x8598
#define GL_OPERAND1_ALPHA_ARB                                0x8599
#define GL_OPERAND2_ALPHA_ARB                                0x859A
#endif


extern char *gl_vendor;
extern char *gl_renderer;
extern char *gl_version;
extern char *gl_extensions;
#ifdef GLX_GLXEXT_PROTOTYPES
extern char *glx_extensions;
#endif

extern qboolean fullsbardraw;
extern qboolean isIntel; // intel video workaround

extern qboolean gl_texture_NPOT;
extern qboolean gl_texture_compression;
extern qboolean gl_swap_control;
extern int		gl_stencilbits;

extern GLint gl_hardware_max_size;

extern int warpimage_size;

// Swap control
GLint (GLAPIENTRY *qglSwapInterval)(GLint interval);

#ifdef _WIN32
#define SWAPCONTROLSTRING "WGL_EXT_swap_control"
#define SWAPINTERVALFUNC "wglSwapIntervalEXT"
#elif defined GLX_GLXEXT_PROTOTYPES
#define SWAPCONTROLSTRING "GLX_SGI_swap_control"
#define SWAPINTERVALFUNC "glXSwapIntervalSGI"
#endif

// Anisotropic filtering
#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT		0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT	0x84FF
#endif

extern float gl_hardware_max_anisotropy;
extern float gl_texture_anisotropy;

// Texture compression
qboolean gl_texture_compression;
void (GLAPIENTRY *qglCompressedTexImage2D) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);

// GL_EXT_texture_compression_s3tc
#ifndef GL_EXT_texture_compression_s3tc
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                      0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                     0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                     0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                     0x83F3
#endif

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
#define TEXPREF_WARP			0x0800	// warp texture
#define TEXPREF_WARPIMAGE		0x1000	// resize this texture when warpimage_size changes
#define TEXPREF_SKY				0x2000	// sky texture
#define TEXPREF_BLOOM			0x4000 	// bloom texture (UNUSED)

// mode: (H2)
// 0 - standard
// 1 - color 0 transparent, odd - translucent, even - full value
// 2 - color 0 transparent
// 3 - special (particle translucency table)
#define TEXPREF_TRANSPARENT		0x8000	// EF_TRANSPARENT	(mode 1)
#define TEXPREF_HOLEY			0x10000	// EF_HOLEY			(mode 2)
#define TEXPREF_SPECIAL_TRANS	0x20000	// EF_SPECIAL_TRANS	(mode 3)

enum srcformat {SRC_INDEXED, SRC_LIGHTMAP, SRC_RGBA, SRC_BLOOM};

typedef struct
{
	vec3_t		basecolor;
	vec3_t		glowcolor;
	vec3_t		flatcolor;
} flatcolors_t;

typedef struct gltexture_s {
//managed by texture manager
	GLuint				texnum;
	struct gltexture_s	*next;
	model_t				*owner;
//managed by image loading
	char				name[64];
	unsigned int		width;						// size of image as it exists in opengl
	unsigned int		height;						// size of image as it exists in opengl
	unsigned int		max_miplevel;
	unsigned int		flags;						// texture preference flags
	char				source_file[MAX_QPATH];		// relative filepath to data source, or "" if source is in memory
	uintptr_t			source_offset;				// byte offset into file, or memory address
	enum srcformat		source_format;				// format of pixel data (indexed, lightmap, or rgba)
	unsigned int		source_width;				// size of image in source data
	unsigned int		source_height;				// size of image in source data
	unsigned short		source_crc;					// generated by source data before modifications
	
	signed char			top_color;					// 0-13 top color, or -1 if never colormapped
	signed char			bottom_color;				// 0-13 bottom color, or -1 if never colormapped
	
	flatcolors_t		colors;
} gltexture_t;

typedef struct
{
	gltexture_t	*gltexture;
	float	sl, tl, sh, th;
} glpic_t;


//============================================================================

// vid_*gl*.c
void GL_BeginRendering (int *x, int *y, int *width, int *height);
void GL_EndRendering (void);

// image.c
byte *Image_LoadImage (char *name, int *width, int *height);
qboolean Image_WriteTGA (char *name, byte *data, int width, int height, int bpp, qboolean upsidedown);

// gl_bloom.c
void R_InitBloomTextures (void);
void R_BloomBlend (void);

// gl_efrag.c
void R_StoreEfrags (efrag_t **efrags);

// gl_fog.c
void R_FogUpdate (float density, float red, float green, float blue, float time);
void R_FogParseServerMessage (void);
void R_FogParseServerMessage2 (void);
float *R_FogGetColor (void);
float R_FogGetDensity (void);
void R_FogEnableGFog (void);
void R_FogDisableGFog (void);
void R_FogSetupFrame (void);
void R_Fog_f (void);

// gl_light.c
void R_AnimateLight (void);
void R_LightPoint (vec3_t p, vec3_t color);
void R_MarkLights (dlight_t *light, int num, mnode_t *node);
void R_SetupDlights (void);
void R_RenderDlight (dlight_t *light);

// gl_main.c
int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, mplane_t *p);
qboolean R_CullBox (vec3_t emins, vec3_t emaxs);
qboolean R_CullSphere (vec3_t origin, float radius);
qboolean R_CullModelForEntity (entity_t *e);
void R_DrawAliasModel (entity_t *e);
void R_DrawSpriteModel (entity_t *e);

// gl_mesh.c
void R_MakeAliasModelDisplayLists (model_t *m, aliashdr_t *hdr);

// gl_misc.c
void R_InitPlayerTextures (void);
void R_TranslatePlayerSkin (int playernum);
void R_TranslateNewPlayerSkin (int playernum); //johnfitz -- this handles cases when the actual texture changes
void R_InitSkyBoxTextures (void);
void R_ParseWorldspawn (void);
void R_TimeRefresh_f (void);

// gl_part.c
void R_InitParticles (void);
void R_SetupParticles (void);
void R_DrawParticle (particle_t *p);
void R_ClearParticles (void);
void R_ReadPointFile_f (void);

// gl_screen.c
void SCR_TileClear (void);

// gl_sky.c
void R_DrawSky (void);
void R_LoadSkyBox (char *skybox);
void R_FastSkyColor (void);
void R_Sky_f (void);
void Sky_ClearAll (void);

// gl_surf.c
void R_MarkLeaves (void);
void R_SetupSurfaces (void);
void R_ClearTextureChains (model_t *model, texchain_t chain);
void R_ChainSurface (msurface_t *surf, texchain_t chain);
void R_DrawTextureChains (model_t *model, entity_t *ent, texchain_t chain);
void R_DrawBrushModel (entity_t *e);
void R_DrawWorld (void);
void R_DrawGLPoly34 (glpoly_t *p);
void R_DrawGLPoly56 (glpoly_t *p);
void R_DrawSequentialPoly (msurface_t *s, float alpha, model_t *model, entity_t *ent);
void R_BuildLightmaps (void);
void R_UploadLightmaps (void);
void R_RebuildAllLightmaps (void);
texture_t *R_TextureAnimation (texture_t *base, int frame);

// gl_texmgr.c
void TexMgr_NewGame (void);
void TexMgr_UploadWarpImage (void);
void TexMgr_LoadPalette (void);
void TexMgr_Init (void);
int TexMgr_PadConditional (int s);
int TexMgr_SafeTextureSize (int s);
void TexMgr_Upload8 (gltexture_t *glt, byte *data);
void TexMgr_Upload32 (gltexture_t *glt, unsigned *data);
void TexMgr_UploadBloom (gltexture_t *glt, unsigned *data);
void TexMgr_UploadLightmap (gltexture_t *glt, byte *data);
void TexMgr_FreeTexture (gltexture_t *texture);
void TexMgr_FreeTextures (unsigned int flags, unsigned int mask);
void TexMgr_FreeTexturesForOwner (model_t *owner);
void TexMgr_ReloadTexture (gltexture_t *glt);
void TexMgr_ReloadTextureTranslation (gltexture_t *glt, int top, int bottom);
void TexMgr_ReloadTextures (void);
void TexMgr_DeleteTextures (void);
void TexMgr_GenerateTextures (void);
gltexture_t *TexMgr_LoadTexture (model_t *owner, char *name, int width, int height, enum srcformat format, byte *data, char *source_file, uintptr_t source_offset, unsigned flags);
gltexture_t *TexMgr_FindTexture (model_t *owner, char *name);
gltexture_t *TexMgr_NewTexture (void);
void GL_SetFilterModes (gltexture_t *glt);
void GL_Set2D (void);
void GL_SelectTexture (GLenum target);
void GL_BindTexture (gltexture_t *texture);
void GL_DeleteTexture (gltexture_t *texture);
void GL_SelectTMU0 (void);
void GL_SelectTMU1 (void);
void GL_SelectTMU2 (void);
void GL_SelectTMU3 (void);
void GL_Init (void);
void GL_GetGLInfo (void);
void GL_SetupState (void);
void GL_SwapInterval (void);
void GL_CheckSwapInterval (void);
void GL_GetPixelFormatInfo (void);
void GL_CheckMultithreadedGL (void);

// gl_warp.c
void R_UpdateWarpTextures (void);


extern float turbsin[];
#define TURBSCALE (256.0 / (2 * M_PI))
#define WARPCALC(s,t) ((s + turbsin[(int)((t*2)+(cl.time*(128.0/M_PI))) & 255]) * (1.0/64)) // correct warp


extern	int glx, gly, glwidth, glheight;

#define	GL_UNUSED_TEXTURE	(~(GLuint)0)

// private refresh defs

#define ALIAS_BASE_SIZE_RATIO		(1.0 / 11.0)
					// normalizing factor so player model works out to about
					//  1 pixel per triangle
#define MAX_SKIN_HEIGHT 480

#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

#define BACKFACE_EPSILON	0.01
#define COLINEAR_EPSILON	0.001

#define FARCLIP			16384 // orig. 4096
#define NEARCLIP		4


//====================================================
// GL Alpha Sorting

#define ALPHA_SURFACE 				2
#define ALPHA_ALIAS 				6
#define ALPHA_SPRITE 				7
#define ALPHA_PARTICLE 				8
#define ALPHA_DLIGHTS 				9

#define MAX_ALPHA_ITEMS			65536
typedef struct gl_alphalist_s 
{
	int			type;
	vec_t		dist;
	void 		*data;
	model_t 	*model;
	
	// for alpha surface
	entity_t	*entity;
	float		alpha;
} gl_alphalist_t;

extern gl_alphalist_t	gl_alphalist[MAX_ALPHA_ITEMS];
extern int				gl_alphalist_num;

qboolean R_SetAlphaSurface(msurface_t *s, float alpha);
float R_GetTurbAlpha (msurface_t *s);
vec_t R_GetAlphaDist (vec3_t origin);
void R_AddToAlpha (int type, vec_t dist, void *data, model_t *model, entity_t *entity, float alpha);
void R_DrawAlpha (void);

//====================================================

extern	entity_t	r_worldentity;
extern	vec3_t		modelorg, r_entorigin;
extern	int			r_visframecount;	// ??? what difs?
extern	int			r_framecount;
extern	mplane_t	frustum[4];
extern	int			rs_c_brush_polys, rs_c_brush_passes, rs_c_alias_polys, rs_c_alias_passes, rs_c_sky_polys, rs_c_sky_passes;
extern	int			rs_c_dynamic_lightmaps, rs_c_particles;
extern	qboolean	r_cache_thrash;		// compatability

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

//
// light style
//
extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value

//
// texture stuff
//
extern int	indexed_bytes;
extern int	rgba_bytes;
extern int	lightmap_bytes;

#define MAX_EXTRA_TEXTURES 156   // 255-100+1
extern gltexture_t			*gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

extern	gltexture_t *notexture;
extern	gltexture_t *nulltexture;

extern	gltexture_t *particletexture;
extern	gltexture_t *particletexture1;
extern	gltexture_t *particletexture2;

extern	gltexture_t	*playertextures[MAX_SCOREBOARD];
extern	gltexture_t	*skyboxtextures[6];

extern	qboolean	oldsky;
extern	char	skybox_name[MAX_OSPATH];
extern float globalwateralpha;

#define	OVERBRIGHT_SCALE	2.0
extern	int		d_overbright;
extern	float	d_overbrightscale;


//
// cvars
//
extern	cvar_t	r_norefresh;
extern	cvar_t	r_drawentities;
extern	cvar_t	r_drawworld;
extern	cvar_t	r_drawviewmodel;
extern	cvar_t	r_speeds;
extern	cvar_t	r_waterwarp;
extern	cvar_t	r_fullbright;
extern	cvar_t	r_ambient;
extern	cvar_t	r_flatturb;
extern	cvar_t	r_waterquality;
extern	cvar_t	r_wateralpha;
extern	cvar_t	r_lockalpha;
extern	cvar_t	r_lavaalpha;
extern	cvar_t	r_slimealpha;
extern	cvar_t	r_telealpha;
extern	cvar_t	r_litwater;
extern	cvar_t	r_noalphasort;
extern	cvar_t	r_dynamic;
extern	cvar_t	r_dynamicscale;
extern	cvar_t	r_novis;
extern	cvar_t	r_lockfrustum;
extern	cvar_t	r_lockpvs;
extern	cvar_t	r_clearcolor;
extern	cvar_t	r_fastsky;
extern	cvar_t	r_fastskycolor;
extern	cvar_t	r_skyquality;
extern	cvar_t	r_skyalpha;
extern	cvar_t	r_skyfog;
extern	cvar_t	r_oldsky;
extern	cvar_t	r_flatworld;
extern	cvar_t	r_flatmodels;

extern	cvar_t	gl_swapinterval;
extern	cvar_t	gl_finish;
extern	cvar_t	gl_clear;
extern	cvar_t	gl_cull;
extern	cvar_t	gl_farclip;
extern	cvar_t	gl_smoothmodels;
extern	cvar_t	gl_affinemodels;
extern	cvar_t	gl_gammablend;
extern	cvar_t	gl_polyblend;
extern	cvar_t	gl_flashblend;
extern	cvar_t	gl_flashblendview;
extern	cvar_t	gl_flashblendscale;
extern	cvar_t	gl_overbright;
extern	cvar_t	gl_oldspr;
extern	cvar_t	gl_nocolors;

// fog cvars (Q1 Nehahra)
extern	cvar_t  gl_fogenable;
extern	cvar_t  gl_fogdensity;
extern	cvar_t  gl_fogred;
extern	cvar_t  gl_foggreen;
extern	cvar_t  gl_fogblue;

extern	cvar_t	gl_bloom;
extern	cvar_t	gl_bloomdarken;
extern	cvar_t	gl_bloomalpha;
extern	cvar_t	gl_bloomintensity;
extern	cvar_t	gl_bloomdiamondsize;
extern	cvar_t	gl_bloomsamplesize;
extern	cvar_t	gl_bloomfastsample;


//tmp here
extern float RTint[256],GTint[256],BTint[256];
extern byte *playerTranslation;

