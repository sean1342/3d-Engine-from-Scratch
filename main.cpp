#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <strstream>
#include <algorithm>

SDL_Window *win;
SDL_Renderer *renderer;
SDL_Surface *surface;
SDL_Rect *screen_rect;

void SDL_Init(float s_width, float s_height)
{
	// returns zero on success else non-zero
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("error initializing SDL: %s\n", SDL_GetError());
    }
	// window and renderer setup
	win = SDL_CreateWindow("3d Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, s_width, s_height, 0);
	renderer = SDL_CreateRenderer(win, -1, 0);
	surface = SDL_GetWindowSurface(win);
	SDL_GetClipRect(surface, screen_rect);
}

void DrawTri(float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b, float a)
{
	const SDL_Vertex verts[3] = {
		{ SDL_FPoint{ x1, y1 }, SDL_Color{ r, b, g, a }, SDL_FPoint{ 0 }, },
        { SDL_FPoint{ x2, y2 }, SDL_Color{ r, b, g, a }, SDL_FPoint{ 0 }, },
        { SDL_FPoint{ x3, y3 }, SDL_Color{ r, b, g, a }, SDL_FPoint{ 0 }, }
	};

	SDL_SetRenderDrawColor(renderer, r, g, b, a);
	// SDL_RenderDrawLine(renderer, tri_1.x, tri_1.y, tri_2.x, tri_2.y);
	// SDL_RenderDrawLine(renderer, tri_1.x, tri_1.y, tri_3.x, tri_3.y);
	// SDL_RenderDrawLine(renderer, tri_2.x, tri_2.y, tri_3.x, tri_3.y);
	SDL_RenderGeometry( renderer, nullptr, verts, 3, nullptr, 0 );
}

struct vec3d
{
	float x, y, z;
};

struct triangle
{
	vec3d p[3];
	float dp;
};

struct mesh
{
	std::vector<triangle> tris;

	bool LoadFromObjectFile(std::string sFilename)
	{
		std::ifstream f(std::string("models/" + sFilename));
		if (!f.is_open())
			return false;

		// Local cache of verts
		std::vector<vec3d> verts;

		while (!f.eof())
		{
			char line[128];
			f.getline(line, 128);

			std::strstream s;
			s << line;

			char junk;

			if (line[0] == 'v')
			{
				vec3d v;
				s >> junk >> v.x >> v.y >> v.z;
				verts.push_back(v);
			}

			if (line[0] == 'f')
			{
				int f[3];
				s >> junk >> f[0] >> f[1] >> f[2];
				tris.push_back({ verts[f[0] - 1], verts[f[1] - 1], verts[f[2] - 1] });
			}
		}

		return true;
	}
};

struct mat4x4
{
	float m[4][4] = { 0 };
};

vec3d MultiplyMatrixVector(vec3d &v, mat4x4 &m)
{
	vec3d o;
	o.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
	o.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
	o.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
	float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];

	if (w != 0.0f)
	{
		o.x /= w; o.y /= w; o.z /= w;
	}

	return o;
}

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

const float f_ASPECT_RATIO = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;

const float f_NEAR = 0.1f;
const float f_FAR = 1000.0f;
const float f_FOV = 90.0f;
const float f_FOV_RAD = 1.0f / tanf(f_FOV * 0.5f / 180.f * 3.14159265358979f);

mat4x4 mat_proj;
mesh mesh_cube;

vec3d v_cam;

int main(int argc, char *argv[])
{
	SDL_Init(SCREEN_WIDTH, SCREEN_HEIGHT);

	mesh_cube.LoadFromObjectFile("teapot.obj");

	
	// mesh_cube.tris = {

	// // SOUTH
	// { 0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 0.0f },
	// { 0.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f },

	// // EAST                                                      
	// { 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f },
	// { 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f },

	// // NORTH                                                     
	// { 1.0f, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f },
	// { 1.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 0.0f, 1.0f },

	// // WEST                                                      
	// { 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 0.0f },
	// { 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f },

	// // TOP                                                       
	// { 0.0f, 1.0f, 0.0f,    0.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f },
	// { 0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 0.0f },

	// // BOTTOM                                                    
	// { 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f },
	// { 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f,    1.0f, 0.0f, 0.0f },

	// };

	mat_proj.m[0][0] = f_ASPECT_RATIO * f_FOV_RAD;
	mat_proj.m[1][1] = f_FOV_RAD;
	mat_proj.m[2][2] = f_FAR / (f_FAR - f_NEAR);
	mat_proj.m[3][2] = (-f_FAR * f_NEAR) / (f_FAR - f_NEAR);
	mat_proj.m[2][3] = 1.0f;
	mat_proj.m[3][3] = 0.0f;

	float f_elapsed_time = 0.0f;

	bool running = true;
	while(running)
	{
		int start_time = SDL_GetTicks();

		// clear win and fill with black
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		std::vector<triangle> vec_tris_to_raster;

		// draw tris
		for(auto tri : mesh_cube.tris)
		{
			// project tri -> tri_projected
			triangle tri_projected, tri_translated;

			tri_translated = tri;
			tri_translated.p[0].z = tri.p[0].z + 10.0f;
			tri_translated.p[1].z = tri.p[1].z + 10.0f;
			tri_translated.p[2].z = tri.p[2].z + 10.0f;

			vec3d normal, line1, line2;
			line1.x = tri_translated.p[1].x - tri_translated.p[0].x;
			line1.y = tri_translated.p[1].y - tri_translated.p[0].y;
			line1.z = tri_translated.p[1].z - tri_translated.p[0].z;

			line2.x = tri_translated.p[2].x - tri_translated.p[0].x;
			line2.y = tri_translated.p[2].y - tri_translated.p[0].y;
			line2.z = tri_translated.p[2].z - tri_translated.p[0].z;

			normal.x = line1.y * line2.z - line1.z * line2.y;
			normal.y = line1.z * line2.x - line1.x * line2.z;
			normal.z = line1.x * line2.y - line1.y * line2.x;
			
			float l = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
			normal.x /= l; normal.y /= l; normal.z /= l;

			// if(normal.z < 0)
			if(normal.x * (tri_translated.p[0].x - v_cam.x) +
			   normal.y * (tri_translated.p[0].y - v_cam.y) +
			   normal.z * (tri_translated.p[0].z - v_cam.z) < 0.0f)
			{
				vec3d light_direction = { 0.0f, 0.0f, -1.0f };
				float l = sqrtf(light_direction.x*light_direction.x + light_direction.y*light_direction.y + light_direction.z*light_direction.z);
				light_direction.x /= l; light_direction.y /= l; light_direction.z /= l;

				// how similar is normal to light direction
				tri_projected.dp = normal.x * light_direction.x + normal.y * light_direction.y + normal.z * light_direction.z;

				tri_projected.p[0] = MultiplyMatrixVector(tri_translated.p[0], mat_proj);
				tri_projected.p[1] = MultiplyMatrixVector(tri_translated.p[1], mat_proj);
				tri_projected.p[2] = MultiplyMatrixVector(tri_translated.p[2], mat_proj);

				tri_projected.p[0].x += 1.0f;
				tri_projected.p[0].y += 1.0f;
				tri_projected.p[1].x += 1.0f;
				tri_projected.p[1].y += 1.0f;
				tri_projected.p[2].x += 1.0f;
				tri_projected.p[2].y += 1.0f;
				tri_projected.p[0].x *= 0.5f * SCREEN_WIDTH;
				tri_projected.p[0].y *= 0.5f * SCREEN_HEIGHT;
				tri_projected.p[1].x *= 0.5f * SCREEN_WIDTH;
				tri_projected.p[1].y *= 0.5f * SCREEN_HEIGHT;
				tri_projected.p[2].x *= 0.5f * SCREEN_WIDTH;
				tri_projected.p[2].y *= 0.5f * SCREEN_HEIGHT;

				vec_tris_to_raster.push_back(tri_projected);
			}
		};

		std::sort(vec_tris_to_raster.begin(), vec_tris_to_raster.end(), [](triangle &t1, triangle &t2)
		{
			float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
			float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
			return z1 > z2;
		});

		for(auto &tri_projected : vec_tris_to_raster)
		{
			DrawTri(tri_projected.p[0].x, tri_projected.p[0].y,
					tri_projected.p[1].x, tri_projected.p[1].y,
					tri_projected.p[2].x, tri_projected.p[2].y,
					tri_projected.dp*255.0f, tri_projected.dp*255.0f, tri_projected.dp*255.0f, 255.0f);
		}

		f_elapsed_time = (SDL_GetTicks() - start_time) / 1000.0;
		float f_fps = (f_elapsed_time > 0) ? 1000.0f / f_elapsed_time : 0.0f;

		std::cout << std::to_string(f_fps) << std::endl;

		SDL_RenderPresent(renderer);
	};

	return 0;
}
