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
// gl_part.c

#include "quakedef.h"

#define MAX_PARTICLES			16384 // was 7000	// default max # of particles at one time

#define	SFL_FLUFFY			1	// All largish flakes
#define	SFL_MIXED			2	// Mixed flakes
#define	SFL_HALF_BRIGHT		4	// All flakes start darker
#define	SFL_NO_MELT			8	// Flakes don't melt when his surface, just go away
#define	SFL_IN_BOUNDS		16	// Flakes cannot leave the bounds of their box
#define	SFL_NO_TRANS		32	// All flakes start non-translucent
#define SFL_64				64
#define SFL_128				128

int		ramp1[8] = { 416,416+2,416+4,416+6,416+8,416+10,416+12,416+14};
int		ramp2[8] = { 384+4,384+6,384+8,384+10,384+12,384+13,384+14,384+15};
int		ramp3[8] = {0x6d, 0x6b, 6, 5, 4, 3};
int		ramp4[16] = { 416,416+1,416+2,416+3,416+4,416+5,416+6,416+7,416+8,416+9,416+10,416+11,416+12,416+13,416+14,416+15};
int		ramp5[16] = { 400,400+1,400+2,400+3,400+4,400+5,400+6,400+7,400+8,400+9,400+10,400+11,400+12,400+13,400+14,400+15};
int		ramp6[16] = { 256,256+1,256+2,256+3,256+4,256+5,256+6,256+7,256+8,256+9,256+10,256+11,256+12,256+13,256+14,256+15};
int		ramp7[16] = { 384,384+1,384+2,384+3,384+4,384+5,384+6,384+7,384+8,384+9,384+10,384+11,384+12,384+13,384+14,384+15};
int		ramp8[16] = {175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 13, 14, 15, 16, 17, 18};
//int		ramp9[16] = { 272,272+1,272+2,272+3,272+4,272+5,272+6,272+7,272+8,272+9,272+10,272+11,272+12,272+13,272+14,272+15};
int		ramp9[16] = { 416,416+1,416+2,416+3,416+4,416+5,416+6,416+7,416+8,416+9,416+10,416+11,416+12,416+13,416+14,416+15};
int		ramp10[16] = { 432,432+1,432+2,432+3,432+4,432+5,432+6,432+7,432+8,432+9,432+10,432+11,432+12,432+13,432+14,432+15};
int		ramp11[8] = { 424,424+1,424+2,424+3,424+4,424+5,424+6,424+7};
int		ramp12[8] = { 136,137,138,139,140,141,142,143};

particle_t	*active_particles, *free_particles, *particles;

int				r_numparticles;

float			texturescalefactor; // compensate for apparent size of different particle textures

static vec3_t		rider_origin;

cvar_t		leak_color = {"leak_color","251", true};

cvar_t		snow_flurry= {"snow_flurry","1", true};
cvar_t		snow_active= {"snow_active","1", true};

cvar_t	r_particles = {"r_particles","1",true};

/*
===============
R_ParticleTextureLookup

generate nice antialiased 32x32 circle for particles
===============
*/
int R_ParticleTextureLookup (int x, int y, int sharpness)
{
	int r; // distance from point x,y to circle origin, squared
	int a; // alpha value to return

	x -= 16;
	y -= 16;
	r = x * x + y * y;
	r = r > 255 ? 255 : r;
	a = sharpness * (255 - r);
	a = min(a,255);
	return a;
}

/*
===============
R_InitParticleTextures
===============
*/
void R_InitParticleTextures (void)
{
	int		x,y;
	static byte	particle1_data[64*64*4];
	static byte	particle2_data[2*2*4];
	byte			*dst;

	//
	// particle texture 1 - circle
	//
	dst = particle1_data;
	for (x=0 ; x<64 ; x++)
	{
		for (y=0 ; y<64 ; y++)
		{
			*dst++ = 255;
			*dst++ = 255;
			*dst++ = 255;
			*dst++ = R_ParticleTextureLookup(x, y, 8);
		}
	}
	particletexture1 = GL_LoadTexture (NULL, "particle1", 64, 64, SRC_RGBA, particle1_data, "", (unsigned)particle1_data, TEXPREF_PERSIST | TEXPREF_ALPHA | TEXPREF_LINEAR);

	//
	// particle texture 2 - square
	//
	dst = particle2_data;
	for (x=0 ; x<2 ; x++)
	{
		for (y=0 ; y<2 ; y++)
		{
			*dst++ = 255;
			*dst++ = 255;
			*dst++ = 255;
			*dst++ = x || y ? 0 : 255;
		}
	}
	particletexture2 = GL_LoadTexture (NULL, "particle2", 2, 2, SRC_RGBA, particle2_data, "", (unsigned)particle2_data, TEXPREF_PERSIST | TEXPREF_ALPHA | TEXPREF_NEAREST);

	// set default
	particletexture = particletexture1;
	texturescalefactor = 1.25;
}

/*
===============
R_Particles -- johnfitz
===============
*/
void R_Particles (void)
{
	switch ((int)(r_particles.value))
	{
	default:
	case 1:
		particletexture = particletexture1;
		texturescalefactor = 1.25;
		break;
	case 2:
		particletexture = particletexture2;
		texturescalefactor = 1.0;
		break;
	}
}

/*
===============
R_InitParticles
===============
*/
void R_InitParticles (void)
{
	r_numparticles = MAX_PARTICLES;

	particles = (particle_t *)
			Hunk_AllocName (r_numparticles * sizeof(particle_t), "particles");

	Cvar_RegisterVariable (&leak_color);

	//JFM: snow test
	Cvar_RegisterVariable (&snow_flurry);
	Cvar_RegisterVariable (&snow_active);
	
	Cvar_RegisterVariableCallback (&r_particles, R_Particles);

	R_InitParticleTextures ();
}

/*
===============
R_ClearParticles
===============
*/
void R_ClearParticles (void)
{
	int		i;
	
	free_particles = &particles[0];
	active_particles = NULL;

	for (i=0 ;i<r_numparticles ; i++)
		particles[i].next = &particles[i+1];
	particles[r_numparticles-1].next = NULL;
}

/*
===============
R_AllocParticle
===============
*/
static inline particle_t *R_AllocParticle (void)
{
	particle_t *p;

	if (!free_particles)
		return NULL;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	return p;
}

/*
===============
R_DarkFieldParticles
===============
*/
void R_DarkFieldParticles (entity_t *ent)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;
	vec3_t		org;

	org[0] = ent->origin[0];
	org[1] = ent->origin[1];
	org[2] = ent->origin[2];
	for (i=-16 ; i<16 ; i+=8)
		for (j=-16 ; j<16 ; j+=8)
			for (k=0 ; k<32 ; k+=8)
			{
				p = R_AllocParticle ();
				if (!p)
					return;
		
				p->die = cl.time + 0.2 + (rand()&7) * 0.02;
				p->color = 150 + rand()%6;
				p->type = pt_slowgrav;
				
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;
	
				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);
	
				VectorNormalize (dir);
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}


/*
===============
R_ParseParticleEffect

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect (void)
{
	vec3_t		org, dir;
	int			i, count, msgcount, color;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord (net_message);
	for (i=0 ; i<3 ; i++)
		dir[i] = MSG_ReadChar (net_message) * (1.0/16);
	msgcount = MSG_ReadByte (net_message);
	color = MSG_ReadByte (net_message);

	if (msgcount == 255)
		count = 1024;
	else
		count = msgcount;

	R_RunParticleEffect (org, dir, color, count);
}

/*
===============
R_ParseParticleEffect2

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect2 (void)
{
	vec3_t		org, dmin, dmax;
	int			i, msgcount, color, effect;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord (net_message);
	for (i=0 ; i<3 ; i++)
		dmin[i] = MSG_ReadFloat (net_message);
	for (i=0 ; i<3 ; i++)
		dmax[i] = MSG_ReadFloat (net_message);
	color = MSG_ReadShort (net_message);
	msgcount = MSG_ReadByte (net_message);
	effect = MSG_ReadByte (net_message);

	R_RunParticleEffect2 (org, dmin, dmax, color, effect, msgcount);
}

/*
===============
R_ParseParticleEffect3

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect3 (void)
{
	vec3_t		org, box;
	int			i, msgcount, color, effect;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord (net_message);
	for (i=0 ; i<3 ; i++)
		box[i] = MSG_ReadByte (net_message);
	color = MSG_ReadShort (net_message);
	msgcount = MSG_ReadByte (net_message);
	effect = MSG_ReadByte (net_message);

	R_RunParticleEffect3 (org, box, color, effect, msgcount);
}

/*
===============
R_ParseParticleEffect4

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect4 (void)
{
	vec3_t		org;
	int			i, msgcount, color, effect;
	float		radius;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord (net_message);
	radius = MSG_ReadByte (net_message);
	color = MSG_ReadShort (net_message);
	msgcount = MSG_ReadByte (net_message);
	effect = MSG_ReadByte (net_message);

	R_RunParticleEffect4 (org, radius, color, effect, msgcount);
}

/*
===============
R_ParticleExplosion
===============
*/
void R_ParticleExplosion (vec3_t org)
{
	int			i, j;
	particle_t	*p;
	
	for (i=0 ; i<1024 ; i++)
	{
		p = R_AllocParticle ();
		if (!p)
			return;

		p->die = cl.time + 5;
		p->color = ramp1[0];
		p->ramp = rand()&3;
		if (i & 1)
		{
			p->type = pt_explode;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()&31)-16);
				p->vel[j] = (rand()&511)-256;
			}
		}
		else
		{
			p->type = pt_explode2;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()&31)-16);
				p->vel[j] = (rand()&511)-256;
			}
		}
	}
}

/*
===============
R_RunParticleEffect
===============
*/
void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	particle_t	*p;
	
	for (i=0 ; i<count ; i++)
	{
		p = R_AllocParticle ();
		if (!p)
			return;

		if (count == 1024)
		{	// rocket explosion
			p->die = cl.time + 5;
			p->color = ramp1[0];
			p->ramp = rand()&3;
			if (i & 1)
			{
				p->type = pt_explode;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = org[j] + ((rand()&31)-16);
					p->vel[j] = (rand()&511)-256;
				}
			}
			else
			{
				p->type = pt_explode2;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = org[j] + ((rand()&31)-16);
					p->vel[j] = (rand()&511)-256;
				}
			}
		}
		else
		{
			p->die = cl.time + 0.1*(rand()%5);
//			p->color = (color&~7) + (rand()&7);
//			p->color = 265 + (rand() % 9);
			p->color = 256 + 16 + 12 + (rand() & 3);
			p->type = pt_slowgrav;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()&15)-8);
				p->vel[j] = dir[j]*15;// + (rand()%300)-150;
			}
		}
	}
}

/*
===============
R_RunParticleEffect2
===============
*/
void R_RunParticleEffect2 (vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count)
{
	int			i, j;
	particle_t	*p;
	float		num;

	for (i=0 ; i<count ; i++)
	{
		p = R_AllocParticle ();
		if (!p)
			return;

//		p->die = cl.time + 0.1*(rand()%5);
		p->die = cl.time + 2;//0.1*(rand()%5);
		p->color = color;
		p->type = effect;
		p->ramp = 0;
		for (j=0 ; j<3 ; j++)
		{
			num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
			p->org[j] = org[j] + ((rand()&8)-4); //added randomness to org
			p->vel[j] = dmin[j] + ((dmax[j] - dmin[j]) * num);
		}

	}
}

/*
===============
R_RunParticleEffect3
===============
*/
void R_RunParticleEffect3 (vec3_t org, vec3_t box, int color, int effect, int count)
{
	int			i, j;
	particle_t	*p;
	float		num;

	for (i=0 ; i<count ; i++)
	{
		p = R_AllocParticle ();
		if (!p)
			return;

//		p->die = cl.time + 0.1*(rand()%5);
		p->die = cl.time + 2;//0.1*(rand()%5);
		p->color = color;
		p->type = effect;
		p->ramp = 0;
		for (j=0 ; j<3 ; j++)
		{
			num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
			p->org[j] = org[j] + ((rand()&15)-8);
			p->vel[j] = (box[j] * num * 2) - box[j];
		}
	}
}

/*
===============
R_RunParticleEffect4
===============
*/
void R_RunParticleEffect4 (vec3_t org, float radius, int color, int effect, int count)
{
	int			i, j;
	particle_t	*p;
	float		num;

	for (i=0 ; i<count ; i++)
	{
		p = R_AllocParticle ();
		if (!p)
			return;

//		p->die = cl.time + 0.1*(rand()%5);
		p->die = cl.time + 2;//0.1*(rand()%5);
		p->color = color;
		p->type = effect;
		p->ramp = 0;
		for (j=0 ; j<3 ; j++)
		{
			num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
			p->org[j] = org[j] + ((rand()&15)-8);
			p->vel[j] = (radius * num * 2) - radius;
		}
	}
}


/*
===============
R_LavaSplash
===============
*/
void R_LavaSplash (vec3_t org)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i++)
		for (j=-16 ; j<16 ; j++)
			for (k=0 ; k<1 ; k++)
			{
				p = R_AllocParticle ();
				if (!p)
					return;
		
				p->die = cl.time + 2 + (rand()&31) * 0.02;
				p->color = 224 + (rand()&7);
				p->type = pt_slowgrav;
				
				dir[0] = j*8 + (rand()&7);
				dir[1] = i*8 + (rand()&7);
				dir[2] = 256;
	
				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (rand()&63);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}

/*
===============
R_TeleportSplash
===============
*/
void R_TeleportSplash (vec3_t org)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i+=4)
		for (j=-16 ; j<16 ; j+=4)
			for (k=-24 ; k<32 ; k+=4)
			{
				p = R_AllocParticle ();
				if (!p)
					return;
		
				p->die = cl.time + 0.2 + (rand()&7) * 0.02;
				p->color = 7 + (rand()&7);
				p->type = pt_slowgrav;
				
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;
	
				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}

/*
===============
R_RunQuakeEffect
===============
*/
void R_RunQuakeEffect (vec3_t org, float distance)
{
	int			i;
	particle_t	*p;
	float		num,num2;

	for (i=0 ; i<100 ; i++)
	{
		p = R_AllocParticle ();
		if (!p)
			return;

		p->die = cl.time + 0.3*(rand()%5);
		p->color = (rand() &3) + ((rand() % 3)*16) + (13 * 16) + 256 + 11;
		p->type = pt_quake;
		p->ramp = 0;

		num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
		num2 = distance * num;
		num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
		p->org[0] = org[0] + cos(num * 2 * M_PI)*num2;
		p->org[1] = org[1] + sin(num * 2 * M_PI)*num2;
		num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
		p->org[2] = org[2] + 15*num;
		p->org[2] = org[2];

		num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
		p->vel[0] = (num * 40) - 20;
		num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
		p->vel[1] = (num * 40) - 20;
		num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
		p->vel[2] = 65*num + 80;
	}
}

/*
===============
R_SunStaffTrail
===============
*/
void R_SunStaffTrail(vec3_t source, vec3_t dest)
{
	int i;
	vec3_t vec, dist;
	float length, size;
	particle_t *p;

	VectorSubtract(dest, source, vec);
	length = VectorNormalize(vec);
	dist[0] = vec[0];
	dist[1] = vec[1];
	dist[2] = vec[2];

	size = 10;

	while(length > 0)
	{
		p = R_AllocParticle ();
		if (!p)
			return;

		length -= size;

		p->die = cl.time+2;

		p->ramp = rand()&3;
		p->color = ramp6[(int)(p->ramp)];

		p->type = pt_spit;

		for(i = 0; i < 3; i++)
		{
			p->org[i] = source[i] + ((rand()&3)-2);
		}

		p->vel[0] = (rand()%10)-5;
		p->vel[1] = (rand()%10)-5;
		p->vel[2] = (rand()%10);

		VectorAdd(source, dist, source);
	}
}

/*
===============
RiderParticle
===============
*/
void RiderParticle(int count, vec3_t origin)
{
	int			i;
	particle_t	*p;
	float radius,angle;

	VectorCopy(origin, rider_origin);

	for (i=0 ; i<count ; i++)
	{
		p = R_AllocParticle ();
		if (!p)
			return;

		p->die = cl.time + 4;
		p->color = 256+16+15;
		p->type = pt_rd;
		p->ramp = 0;

		VectorCopy(origin,p->org);

		//num = (rand ()&0x7fff) / ((float)0x7fff);
		angle = (rand() % 360) / (2 * M_PI);
		radius = 300 + rand() & 255;
		p->org[0] += sin(angle) * radius;
		p->org[1] += cos(angle) * radius;
		p->org[2] += (rand() & 255) - 30; 

		p->vel[0] = (rand() & 255) - 127;
		p->vel[1] = (rand() & 255) - 127;
		p->vel[2] = (rand() & 255) - 127;

	}
}

/*
===============
GravityWellParticle
===============
*/
void GravityWellParticle(int count, vec3_t origin, int color)
{
	int			i;
	particle_t	*p;
	float radius,angle;

	VectorCopy(origin, rider_origin);

	for (i=0 ; i<count ; i++)
	{
		p = R_AllocParticle ();
		if (!p)
			return;

		p->die = cl.time + 4;
		p->color = color + (rand() & 15);
		p->type = pt_gravwell;
		p->ramp = 0;

		VectorCopy(origin,p->org);

		angle = (rand() % 360) / (2 * M_PI);
		radius = 300 + rand() & 255;
		p->org[0] += sin(angle) * radius;
		p->org[1] += cos(angle) * radius;
		p->org[2] += (rand() & 255) - 30; 

		p->vel[0] = (rand() & 255) - 127;
		p->vel[1] = (rand() & 255) - 127;
		p->vel[2] = (rand() & 255) - 127;
	}
}

/*
===============
R_RocketTrail

===============
*/
void R_RocketTrail (vec3_t start, vec3_t end, int type)
{
	vec3_t	vec, dist;
	float	len,size,lifetime;
	int			j;
	particle_t	*p;
	static int tracercount;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	dist[0] = vec[0];
	dist[1] = vec[1];
	dist[2] = vec[2];
	size = 1;  
	lifetime = 2;
	switch(type)
	{
		case 9: // Spit
			break;

		case 8: // Ice
			size *= 5*3;
			dist[0] *= 5*3;
			dist[1] *= 5*3;
			dist[2] *= 5*3;
			break;

		case rt_acidball: // Ice
			size=5;
			lifetime = .8;
			break;

		default:
			size = 3;
			dist[0] *= 3;
			dist[1] *= 3;
			dist[2] *= 3;
			break;

	}

	while (len > 0)
	{
		len -= size;

		p = R_AllocParticle ();
		if (!p)
			return;
		
		VectorCopy (vec3_origin, p->vel);
		p->die = cl.time + lifetime;

		switch(type)
		{
			case rt_rocket_trail: // rocket trail
				p->ramp = (rand()&3);
				p->color = ramp3[(int)p->ramp];
				p->type = pt_fire;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				break;

			case rt_smoke: // smoke smoke
				p->ramp = (rand()&3) + 2;
				p->color = ramp3[(int)p->ramp];
				p->type = pt_fire;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				break;

			case rt_blood: // blood
				p->type = pt_slowgrav;
				p->color = 134 + (rand()&7);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				break;


			case rt_tracer:;
			case rt_tracer2:;// tracer
				p->die = cl.time + 0.5;
				p->type = pt_static;
				if (type == 3)
					p->color = 130 + (rand() & 6);
//			p->color = 243 + (rand() & 3);
				else
					p->color = 230 + ((tracercount&4)<<1);
			
				tracercount++;

				VectorCopy (start, p->org);
				if (tracercount & 1)
				{
					p->vel[0] = 30*vec[1];
					p->vel[1] = 30*-vec[0];
				}
				else
				{
					p->vel[0] = 30*-vec[1];
					p->vel[1] = 30*vec[0];
				}
				break;
			
			case rt_slight_blood:// slight blood
				p->type = pt_slowgrav;
				p->color = 134 + (rand()&7);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				len -= size;
				break;

			case rt_bloodshot:// bloodshot trail
				p->type = pt_darken;
				p->color = 136 + (rand()&5);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				len -= size;
				break;

			case rt_voor_trail:// voor trail
				p->color = 9*16 + 8 + (rand()&3);
				p->type = pt_static;
				p->die = cl.time + 0.3;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&15)-8);
				break;

			case rt_fireball: // Fireball
				p->ramp = rand()&3;
				p->color = ramp4[(int)(p->ramp)];
				p->type = pt_fireball;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);		
				p->org[2] += 2; // compensate for model
				p->vel[0] = (rand() % 200) - 100;
				p->vel[1] = (rand() % 200) - 100;
				p->vel[2] = (rand() % 200) - 100;
				break;

			case rt_acidball: // Acid ball
				p->ramp = rand()&3;
				p->color = ramp10[(int)(p->ramp)];
				p->type = pt_acidball;
				p->die = cl.time + 0.5;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				p->org[2] += 2; // compensate for model
				p->vel[0] = (rand() % 40) - 20;
				p->vel[1] = (rand() % 40) - 20;
				p->vel[2] = (rand() % 40) - 20;
				break;

			case rt_ice: // Ice
				p->ramp = rand()&3;
				p->color = ramp5[(int)(p->ramp)];
				p->type = pt_ice;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				p->org[2] += 2; // compensate for model
				p->vel[0] = (rand() % 16) - 8;
				p->vel[1] = (rand() % 16) - 8;
				p->vel[2] = (rand() % 20) - 40;
				break;

			case rt_spit: // Spit
				p->ramp = rand()&3;
				p->color = ramp6[(int)(p->ramp)];
				p->type = pt_spit;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				p->org[2] += 2; // compensate for model
				p->vel[0] = (rand() % 10) - 5;
				p->vel[1] = (rand() % 10) - 5;
				p->vel[2] = (rand() % 10);
				break;

			case rt_spell: // Spell
				p->ramp = rand()&3;
				p->color = ramp6[(int)(p->ramp)];
				p->type = pt_spell;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				p->vel[0] = (rand() % 10) - 5;
				p->vel[1] = (rand() % 10) - 5;
				p->vel[2] = (rand() % 10);
				p->vel[0] = vec[0]*-10;
				p->vel[1] = vec[1]*-10;
				p->vel[2] = vec[2]*-10;
				break;

			case rt_vorpal: // vorpal missile
				p->type = pt_vorpal;
				p->color = 44 + (rand()&3) + 256;
				for (j=0 ; j<2 ; j++)
					p->org[j] = start[j] + ((rand()%48)-24);

				p->org[2] = start[2] + ((rand()&15)-8);

				break;

			case rt_setstaff: // set staff
				p->type = pt_setstaff;
				p->color = ramp9[0];
				p->ramp = rand()&3;

				for (j=0 ; j<2 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);

				p->org[2] = start[2] + ((rand()%10)-5);

				p->vel[0] = (rand() &7) - 4;
				p->vel[1] = (rand() &7) - 4;
				break;

			case rt_magicmissile: // magic missile
				p->type = pt_magicmissile;
				p->color = 148 + (rand()&11);
				p->ramp = rand()&3;
				for (j=0 ; j<2 ; j++)
					p->org[j] = start[j] + ((rand()%48)-24);

				p->org[2] = start[2] + ((rand()%48)-24);

				p->vel[2] = -((rand()&15)+8);
				break;

			case rt_boneshard: // bone shard
				p->type = pt_boneshard;
				p->color = 368 + (rand()&16);
				for (j=0 ; j<2 ; j++)
					p->org[j] = start[j] + ((rand()%48)-24);

				p->org[2] = start[2] + ((rand()%48)-24);

				p->vel[2] = -((rand()&15)+8);
				break;

			case rt_scarab: // scarab staff
				p->type = pt_scarab;
				p->color = 250 + (rand()&3);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + (rand()&7);

				p->vel[2] = -(rand()&7);
				break;

		}		
		VectorAdd (start, dist, start);
	}
}

/*
===============
R_RainEffect

===============
*/
void R_RainEffect (vec3_t org, vec3_t e_size, int x_dir, int y_dir, int color, int count)
{
	int i,holdint;
	particle_t	*p;
	float z_time;
	
	for (i=0 ; i<count ; i++)
	{
		p = R_AllocParticle ();
		if (!p)
			return;
		
		p->vel[0] = x_dir;  //X and Y motion
		p->vel[1] = y_dir;
		p->vel[2] = -((rand()% 956)) ;
		if (p->vel[2] > -256)
		{
			p->vel[2] += -256;
		}
		
		z_time = -(e_size[2]/p->vel[2]);
		p->color = color;
		p->die = cl.time + z_time;
		p->ramp = (rand()&3);
		//p->veer = veer;
		
		p->type = pt_rain;
		
		holdint=e_size[0];
		p->org[0] = org[0] + (rand() % holdint);
		holdint=e_size[1];
		p->org[1] = org[1] + (rand() % holdint);
		p->org[2] = org[2];
		
	}
}

/*
===============
R_SnowEffect

MG
===============
*/
void R_SnowEffect (vec3_t org1,vec3_t org2,int flags,vec3_t alldir,int count)
{
	int i,j,holdint;
	particle_t	*p;
	mleaf_t		*l;
	
	count *= Cvar_VariableValue("snow_active");
	for (i=0 ; i<count ; i++)
	{
		p = R_AllocParticle ();
		if (!p)
			return;
		
		p->vel[0] = alldir[0];  //X and Y motion
		p->vel[1] = alldir[1];
		p->vel[2] = alldir[2] * ((rand() & 15) + 7)/10;
		
		p->flags = flags;

		if(flags & SFL_FLUFFY || (flags&SFL_MIXED && (rand()&3)))
			p->count = (rand()&31)+10;//From 10 to 41 scale, will be divided
		else
			p->count = 10;


		if(flags&SFL_HALF_BRIGHT)//Start darker
			p->color = 26 + (rand()%5);
		else
			p->color = 18 + (rand()%12);

		if(!(flags&SFL_NO_TRANS))//Start translucent
			p->color += 256;

		p->die = cl.time + 7;
		p->ramp = (rand()&3);
		//p->veer = veer;
		p->type = pt_snow;
		
		holdint=org2[0] - org1[0];
		p->org[0] = org1[0] + (rand() % holdint);
		holdint=org2[1] - org1[1];
		p->org[1] = org1[1] + (rand() % holdint);
		p->org[2] = org2[2];
		

		j=50;
		l = Mod_PointInLeaf (p->org, cl.worldmodel);
//		while(SV_PointContents(p->org)!=CONTENTS_EMPTY && j<50)
		while(l->contents!=CONTENTS_EMPTY && j)
		{//Make sure it doesn't start in a solid
			holdint=org2[0] - org1[0];
			p->org[0] = org1[0] + (rand() % holdint);
			holdint=org2[1] - org1[1];
			p->org[1] = org1[1] + (rand() % holdint);
			j--;//No infinite loops
			l = Mod_PointInLeaf (p->org, cl.worldmodel);
		}
		if(l->contents!=CONTENTS_EMPTY)
			Sys_Error ("Snow entity top plane is not in an empty area (sorry!)");

		VectorCopy(org1,p->min_org);
		VectorCopy(org2,p->max_org);
	}
}

/*
===============
R_ColoredParticleExplosion
===============
*/
void R_ColoredParticleExplosion (vec3_t org,int color,int radius,int counter)
{
	int			i, j;
	particle_t	*p;

	for (i=0 ; i<counter ; i++)
	{
		p = R_AllocParticle ();
		if (!p)
			return;

		p->die = cl.time + 3;
		p->color = color;
		p->ramp = (rand()&3);

		if (i & 1)
		{
			p->type = pt_c_explode;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%(radius*2))-radius);
				p->vel[j] = (rand()&511)-256;
			}
		}
		else
		{
			p->type = pt_c_explode2;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%(radius*2))-radius);
				p->vel[j] = (rand()&511)-256;
			}
		}
	}
}


/*
===============
R_UpdateParticles

all the particle behavior, separated from R_DrawParticles
===============
*/
void R_UpdateParticles (void)
{
	particle_t		*p, *kill;
	float			grav,grav2,percent;
	int				i;
	float			time2, time3, time4;
	float			time1;
	float			dvel;
	float			frametime;
	float			vel0, vel1, vel2;
	float			colindex;
	vec3_t			diff;

	if (cls.state == ca_disconnected)
		return;

	frametime = cl.time - cl.oldtime;
//	Con_Printf("%10.5f\n",frametime);
	time4 = frametime * 20;
	time3 = frametime * 15;
	time2 = frametime * 10;
	time1 = frametime * 5;
	grav = frametime * sv_gravity.value * 0.05;
	grav2 = frametime * sv_gravity.value * 0.025;
	dvel = 4*frametime;
	percent = (frametime / HX_FRAME_TIME);
	
	for ( ;; ) 
	{
		kill = active_particles;
		if (kill && kill->die < cl.time)
		{
			active_particles = kill->next;
			kill->next = free_particles;
			free_particles = kill;
			continue;
		}
		break;
	}

	for (p=active_particles ; p ; p=p->next)
	{
		for ( ;; )
		{
			kill = p->next;
			if (kill && kill->die < cl.time)
			{
				p->next = kill->next;
				kill->next = free_particles;
				free_particles = kill;
				continue;
			}
			break;
		}

		if (p->type==pt_rain)
		{
			vel0 = p->vel[0]*.001;
			vel1 = p->vel[1]*.001;
			vel2 = p->vel[2]*.001;
			for(i=0;i<4;i++)
			{
				p->org[0] += vel0;
				p->org[1] += vel1;
				p->org[2] += vel2;
			}
			p->org[0] += p->vel[0]*(frametime-.004);
			p->org[1] += p->vel[1]*(frametime-.004);
			p->org[2] += p->vel[2]*(frametime-.004);
		}
		else if (p->type==pt_snow)
		{
			if(p->vel[0]==0&&p->vel[1]==0&&p->vel[2]==0)
			{//Stopped moving
				if(p->color==256+31)//Most translucent white
				{//Go away
					p->die=-1;
				}
				else
				{//Count fifty and fade in translucency once each time
					p->ramp+=1;
					if(p->ramp>=7)
					{
						p->color+=1;//Get more translucent
						p->ramp=0;
					}
				}
			}
			else
			{//FIXME: If flake going fast enough, can go through, do a check in increments ot 10, max?
				//if not in_bounds Get length of diff, add in increments of 4 & check solid
				mleaf_t		*l;

				if (Cvar_VariableValue("snow_flurry")==1)
				if(rand()&31)
				{//Add flurry movement
					float			snow_speed;
					vec3_t			save_vel;
					snow_speed = p->vel[0] * p->vel[0] + p->vel[1] * p->vel[1] + p->vel[2]*p->vel[2];
					snow_speed = sqrt(snow_speed);
					
					VectorCopy(p->vel,save_vel);
					
					save_vel[0] += ( (rand()*(2.0/RAND_MAX)) - 1)*30;
					save_vel[1] += ( (rand()*(2.0/RAND_MAX)) - 1)*30;
					if((rand()&7)||p->vel[2]>10)
						save_vel[2] += ( (rand()*(2.0/RAND_MAX)) - 1)*30;
					
					VectorNormalize(save_vel);
					VectorScale(save_vel,snow_speed,p->vel);//retain speed but use new dir
				}

/*				VectorScale(p->vel,frametime,diff);
				speed = VectorNormalize(diff);
				in_solid=false;
				if(!(p->flags&SFL_IN_BOUNDS))
				{//Not cut off by bounds
					if(speed>=8)
					{//Moving more than 8 pixels this turn
						for(i=4;i<speed;i+=4)
						{//Check for solid in increments of 4
							VectorScale(diff,i,save_org);
							VectorAdd(p->org,save_org,save_org);
//							if(SV_PointContents(save_org)!=CONTENTS_EMPTY)
							l = Mod_PointInLeaf (save_org, cl.worldmodel);
							if (l->contents!=CONTENTS_EMPTY)
							{
								in_solid=true;
								VectorCopy(save_org,p->org);
								break;
							}
						}
					}
				}
*/
//				if(!in_solid)
				{
					VectorScale(p->vel,frametime,diff);
					VectorAdd(p->org,diff,p->org);
				}
				
				if(p->flags&SFL_IN_BOUNDS)
				{//Always stay inside the boundry!
					if(p->org[0]<p->min_org[0]||p->org[0]>p->max_org[0]||
						p->org[1]<p->min_org[1]||p->org[1]>p->max_org[1]||
						p->org[2]<p->min_org[2]||p->org[2]>p->max_org[2])
					{
						p->die=-1;
					}
				}
				else
				{
					//IF hit solid, go to last position, no velocity, fade out.
					l = Mod_PointInLeaf (p->org, cl.worldmodel);
					if(l->contents!=CONTENTS_EMPTY) //||in_solid==true
					{
						if(p->flags&SFL_NO_MELT)
						{//Don't melt, just die
							p->die=-1;
						}
						else
						{//still have small prob of snow melting on emitter
							VectorScale(diff,0.2,p->vel);
							i=6;
							while(l->contents!=CONTENTS_EMPTY )
							{
								p->org[0] -= p->vel[0];
								p->org[1] -= p->vel[1];
								p->org[2] -= p->vel[2];
								i--;//no infinite loops
								if (!i)
								{
									p->die=-1;	//should never happen now!
									break;
								}
								l = Mod_PointInLeaf (p->org, cl.worldmodel);
							}
							p->vel[0]=p->vel[1]=p->vel[2]=0;
							p->ramp=0;
						}
					}
				}
			}
		}
		else
		{
			p->org[0] += p->vel[0]*frametime;
			p->org[1] += p->vel[1]*frametime;
			p->org[2] += p->vel[2]*frametime;
		}

		switch (p->type)
		{
		case pt_static:
			break;

		case pt_fire:
			p->ramp += time1;
			if ((int)p->ramp >= 6)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp3[(int)p->ramp];
			}
			p->vel[2] += grav;
			break;

		case pt_explode:
			p->ramp += time2;
			if ((int)p->ramp >=8)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp1[(int)p->ramp];
			}
			for (i=0 ; i<3 ; i++)
			{
				p->vel[i] += p->vel[i]*dvel;
			}
			p->vel[2] -= grav;
			break;

		case pt_explode2:
			p->ramp += time3;
			if ((int)p->ramp >=8)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp2[(int)p->ramp];
			}
			for (i=0 ; i<3 ; i++)
			{
				p->vel[i] -= p->vel[i]*frametime;
			}
			p->vel[2] -= grav;
			break;

      case pt_c_explode:
			p->ramp += time2;
			if ((int)p->ramp >=8)
			{
				p->die = -1;
			}
			else if (time2)
			{
				p->color--;
			}
			for (i=0 ; i<3 ; i++)
			{
				p->vel[i] += p->vel[i]*dvel;
			}
			p->vel[2] -= grav;
			break;

      case pt_c_explode2:
			p->ramp += time3;
			if ((int)p->ramp >=8)
			{
				p->die = -1;
			}
			else if (time3)
			{
				p->color -= 2;
			}
			for (i=0 ; i<3 ; i++)
			{
				p->vel[i] -= p->vel[i]*frametime;
			}
			p->vel[2] -= grav;
			break;

/*	//jfm:not used
		case pt_blob:
			for (i=0 ; i<3 ; i++)
			{
				p->vel[i] += p->vel[i]*dvel;
			}
			p->vel[2] -= grav;
			break;

		case pt_blob2:
			for (i=0 ; i<2 ; i++)
			{
				p->vel[i] -= p->vel[i]*dvel;
			}
			p->vel[2] -= grav;
			break;
*/
		case pt_grav:
		case pt_slowgrav:
			p->vel[2] -= grav;
			break;

		case pt_fastgrav:
			p->vel[2] -= grav*4;
			break;

      case pt_rain:
			break;

      case pt_snow:
			break;

	  case pt_fireball:
			p->ramp += time3;
			if ((int)p->ramp >= 16)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp4[(int)p->ramp];
			}
			break;

		case pt_acidball:
			p->ramp += time4*1.4;
			if ((int)p->ramp >= 23)
			{
				p->die = -1;
			}
			else if ((int)p->ramp >= 15)
			{
				p->color = ramp11[(int)p->ramp - 15];
			}
			else
			{
				p->color = ramp10[(int)p->ramp];
			}
			p->vel[2] -= grav;
			break;

		case pt_spit:
			p->ramp += time3;
			if ((int)p->ramp >= 16)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp6[(int)p->ramp];
			}
//			p->vel[2] += grav*2;
			break;

		case pt_ice:
			p->ramp += time4;
			if ((int)p->ramp >= 16)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp5[(int)p->ramp];
			}
			p->vel[2] -= grav;
			break;

		case pt_spell:
			p->ramp += time2;
			if ((int)p->ramp >= 16)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp7[(int)p->ramp];
			}
//			p->vel[2] += grav*2;
			break;

		case pt_test:
			p->vel[2] += 1.3;
			p->ramp += time3;
			if ((int)p->ramp >= 13 || ((int)p->ramp > 10 && (int)p->vel[2] < 20) )
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp8[(int)p->ramp];
			}
			break;

		case pt_quake:
			p->vel[0] *= 1.05;
			p->vel[1] *= 1.05;
			p->vel[2] -= grav*4;
			break;

		case pt_rd:
			if (!frametime)
			{
				break;
			}

			p->ramp += percent;
			if ((int)p->ramp > 50) 
			{
				p->ramp = 50;
				p->die = -1;
			}
			p->color = 256+16+16 - (p->ramp/(50/16));

			VectorSubtract(rider_origin, p->org, diff);

/*			p->org[0] += diff[0] * p->ramp / 80;
			p->org[1] += diff[1] * p->ramp / 80;
			p->org[2] += diff[2] * p->ramp / 80;
*/

			vel0 = 1 / (51 - p->ramp);
			p->org[0] += diff[0] * vel0;
			p->org[1] += diff[1] * vel0;
			p->org[2] += diff[2] * vel0;
			
			break;

		case pt_gravwell:
			if (!frametime)
			{
				break;
			}

			p->ramp += percent;
			if ((int)p->ramp > 35) 
			{
				p->ramp = 35;
				p->die = -1;
			}

			VectorSubtract(rider_origin, p->org, diff);

/*			p->org[0] += diff[0] * p->ramp / 80;
			p->org[1] += diff[1] * p->ramp / 80;
			p->org[2] += diff[2] * p->ramp / 80;
*/

			vel0 = 1 / (36 - p->ramp);
			p->org[0] += diff[0] * vel0;
			p->org[1] += diff[1] * vel0;
			p->org[2] += diff[2] * vel0;
			
			break;

		case pt_vorpal:
			--p->color; 
			if ((int)p->color <= 37 + 256)
			{
				p->die = -1;
			}
			break;

		case pt_setstaff:
			p->ramp += time1;
			if ((int)p->ramp >= 16)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp9[(int)p->ramp];
			}

			p->vel[0] *= 1.08 * percent;
			p->vel[1] *= 1.08 * percent;
			p->vel[2] -= grav2;
			break;

		case pt_redfire:
			p->ramp += frametime*3;
			if ((int)p->ramp >= 8)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp12[(int)p->ramp]+256;
			}

			p->vel[0] *= .9;
			p->vel[1] *= .9;
			p->vel[2] += grav/2;
			break;

		case pt_magicmissile:
			--p->color; 
			if ((int)p->color < 149)
			{
				p->color = 149;
			}
			p->ramp += time1;
			if ((int)p->ramp > 16)
			{
				p->die = -1;
			}
			break;

		case pt_boneshard:
			--p->color; 
			if ((int)p->color < 368)
			{
				p->die = -1;
			}
			break;

		case pt_scarab:
			--p->color; 
			if ((int)p->color < 250)
			{
				p->die = -1;
			}
			break;

		case pt_darken:
			p->vel[2] -= grav;	//Also gravity
			--p->color; 
			colindex=0;
			while(colindex<224)
			{
				if(colindex==192 || colindex == 200)
				{
					colindex+=8;
				}
				else
				{
					colindex+=16;
				}
				if (p->color==colindex)
				{
					p->die = -1;
				}
			}
			break;
		}
	}
}


/*
===============
R_DrawParticles

moved all non-drawing code to R_UpdateParticles
===============
*/
void R_DrawParticles (void)
{
	particle_t		*p;
	vec3_t			up, right, p_up, p_right, p_upright;
	float			scale;
	byte			*color, alpha;

	if (!r_particles.value)
		return;

	VectorScale (vup, 1.5, up);
	VectorScale (vright, 1.5, right);

	GL_Bind(particletexture);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDepthMask (GL_FALSE); // don't bother writing Z (fix for particle z-buffer bug)
	glBegin (GL_QUADS); // quads save fillrate

	for (p=active_particles ; p ; p=p->next)
	{
		// hack a scale up to keep particles from disapearing
		scale = (p->org[0] - r_origin[0])*vpn[0]
			+ (p->org[1] - r_origin[1])*vpn[1]
			+ (p->org[2] - r_origin[2])*vpn[2];

		if (p->type==pt_rain)
		{
			if (scale < 20)
				scale = 1 + 0.08; // added .08 to be consistent
			else
				scale = 1 + scale * 0.004;

			scale /= 2.0; // quad is half the size of triangle 
			//scale *= texturescalefactor; // compensate for apparent size of different particle textures
			scale *= 1.0; // compensate for apparent size of different particle textures
		}
		else if (p->type==pt_snow)
		{
			if (scale < 20)
				scale = p->count/10 + 0.08; // added .08 to be consistent	
			else
				scale = p->count/10 + scale * 0.004;

			scale /= 2.0; // quad is half the size of triangle 
			//scale *= texturescalefactor; // compensate for apparent size of different particle textures
			scale *= 1.0; // compensate for apparent size of different particle textures

			if(p->count>=40)	
				scale *= 4.0;
			else if(p->count>=30)
				scale *= 2.0;
			else
				scale *= 1.0; // default scale
		}
		else
		{
			if (scale < 20)
				scale = 1 + 0.08; // added .08 to be consistent
			else
				scale = 1 + scale * 0.004;

			scale /= 2.0; // quad is half the size of triangle
			if (p->type==pt_grav)
				scale *= 1.0; // compensate for apparent size of different particle textures
			else
				scale *= texturescalefactor; // compensate for apparent size of different particle textures
		}

		// particle fade out
		if (p->color <= 255)
		{
			color = (byte *)&d_8to24table[(int)p->color]; // fix gcc warnings
			alpha = 255; // reserved for alpha
			glColor4ub (color[0], color[1], color[2], alpha);
		}
		else
		{
			color = (byte *)&d_8to24TranslucentTable[(int)p->color-256]; // fix gcc warnings
			glColor4ubv (color);
		}

		glTexCoord2f (0,0);
		glVertex3fv (p->org);

		glTexCoord2f (0.5,0);
		VectorMA (p->org, scale, up, p_up);
		glVertex3fv (p_up);

		glTexCoord2f (0.5,0.5);
		VectorMA (p_up, scale, right, p_upright);
		glVertex3fv (p_upright);

		glTexCoord2f (0,0.5);
		VectorMA (p->org, scale, right, p_right);
		glVertex3fv (p_right);
	}

	glEnd ();
	glDepthMask (GL_TRUE); // back to normal Z buffering
	glDisable (GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glColor3f(1,1,1); //EER1 tst
}


void R_ReadPointFile_f (void)
{
	FILE	*f;
	vec3_t	org;
	int		r;
	int		c;
	particle_t	*p;
	char	name[MAX_OSPATH];
	byte	color;
	
	color = (byte)Cvar_VariableValue("leak_color");
	sprintf (name,"maps/%s.pts", sv.name);

	COM_FOpenFile (name, &f, NULL);
	if (!f)
	{
		Con_Printf ("couldn't open %s\n", name);
		return;
	}
	
	Con_Printf ("Reading %s...\n", name);
	c = 0;
	for ( ;; )
	{
		r = fscanf (f,"%f %f %f\n", &org[0], &org[1], &org[2]);
		if (r != 3)
			break;
		c++;
		
		p = R_AllocParticle ();
		if (!p)
		{
			Con_Printf ("Not enough free particles\n");
			break;
		}
		
		p->die = 99999;
		p->color = color; // (-c)&15;
		p->type = pt_static;
		VectorCopy (vec3_origin, p->vel);
		VectorCopy (org, p->org);
	}

	fclose (f);
	Con_Printf ("%i points read\n", c);
}

