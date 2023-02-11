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

struct triangle
{
	glm::vec4 p[3];
	float dp;
};

glm::vec4 Multiply_MatrixVector(glm::vec4 &i, glm::mat4 &m)
{
	glm::vec4 o = m * i;

	float w = i.x * m[0][3] + i.y * m[1][3] + i.z * m[2][3] + m[3][3];
	if (w != 0.0f)
		o.x /= w;
		o.y /= w;
		o.z /= w;

	return o;
}

glm::mat4 Create_MatrixProjection(float f_fov_degrees, float f_aspect_ratio, float f_near, float f_far)
{
	glm::mat4 m;

	float f_fov_rad = 1.0f / tanf(f_fov_degrees * 0.5f / 180.f * 3.14159265358979f);
	m[0][0] = f_aspect_ratio * f_fov_rad;
	m[1][1] = f_fov_rad;
	m[2][2] = f_far / (f_far - f_near);
	m[3][2] = (-f_far * f_near) / (f_far - f_near);
	m[2][3] = 1.0f;
	m[3][3] = 0.0f;

	return m;
}

glm::mat4 Create_MatrixRotationX(float f_angle_rad)
{
	glm::mat4 m;
	m[0][0] = 1.0f;
	m[1][1] = cosf(f_angle_rad);
	m[1][2] = sinf(f_angle_rad);
	m[2][1] = -sinf(f_angle_rad);
	m[2][2] = cosf(f_angle_rad);
	m[3][3] = 1.0f;
	return m;
}

glm::mat4 Create_MatrixRotationY(float f_angle_rad)
{
	glm::mat4 m;
	m[0][0] = cosf(f_angle_rad);
	m[0][2] = sinf(f_angle_rad);
	m[2][0] = -sinf(f_angle_rad);
	m[1][1] = 1.0f;
	m[2][2] = cosf(f_angle_rad);
	m[3][3] = 1.0f;
	return m;
}

glm::mat4 Create_MatrixRotationZ(float f_angle_rad)
{
	glm::mat4 m;
	m[0][0] = cosf(f_angle_rad);
	m[0][1] = sinf(f_angle_rad);
	m[1][0] = -sinf(f_angle_rad);
	m[1][1] = cosf(f_angle_rad);
	m[2][2] = 1.0f;
	m[3][3] = 1.0f;
	return m;
}

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

void DrawTri(triangle tri, float r, float g, float b, float a)
{
	float x1 = tri.p[0].x;
	float y1 = tri.p[0].y;
	float x2 = tri.p[1].x;
	float y2 = tri.p[1].y;
	float x3 = tri.p[2].x;
	float y3 = tri.p[2].y;

	const SDL_Vertex verts[3] = {
		{ SDL_FPoint{ x1, y1 }, SDL_Color{ r, b, g, a }, SDL_FPoint{ 0 }, },
        { SDL_FPoint{ x2, y2 }, SDL_Color{ r, b, g, a }, SDL_FPoint{ 0 }, },
        { SDL_FPoint{ x3, y3 }, SDL_Color{ r, b, g, a }, SDL_FPoint{ 0 }, }
	};

	SDL_SetRenderDrawColor(renderer, r, g, b, a);

	SDL_RenderGeometry( renderer, nullptr, verts, 3, nullptr, 0 );
}

struct mesh
{
	std::vector<triangle> tris;

	bool LoadFromObjectFile(std::string sFilename)
	{
		std::ifstream f(std::string("models/" + sFilename));
		if (!f.is_open())
			return false;

		// Local cache of verts
		std::vector<glm::vec4> verts;

		while (!f.eof())
		{
			char line[128];
			f.getline(line, 128);

			std::strstream s;
			s << line;

			char junk;

			if (line[0] == 'v')
			{
				glm::vec4 v;
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

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

const float f_ASPECT_RATIO = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;

const float f_NEAR = 0.1f;
const float f_FAR = 1000.0f;
const float f_FOV = 90.0f;

glm::mat4 mat_proj = Create_MatrixProjection(f_FOV, f_ASPECT_RATIO, f_NEAR, f_FAR);

mesh obj;

glm::vec3 v_cam;

int main(int argc, char *argv[])
{
	SDL_Init(SCREEN_WIDTH, SCREEN_HEIGHT);

	obj.LoadFromObjectFile("video_ship.obj");

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
		for(auto tri : obj.tris)
		{
			// project tri -> tri_projected
			triangle tri_projected, tri_translated;

			// offset into the screen
			tri_translated = tri;
			for(int i=0; i<3; i++)
				tri_translated.p[i].z += 8.0f;

			glm::vec3 normal, line1, line2;
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
				glm::vec3 light_direction = { 0.0f, 0.0f, -1.0f };
				float l = sqrtf(light_direction.x*light_direction.x + light_direction.y*light_direction.y + light_direction.z*light_direction.z);
				light_direction.x /= l; light_direction.y /= l; light_direction.z /= l;

				// how similar is normal to light direction
				tri_projected.dp = normal.x * light_direction.x + normal.y * light_direction.y + normal.z * light_direction.z;

				for(int i=0; i<3; i++)
				{
					tri_projected.p[i] = Multiply_MatrixVector(tri_translated.p[i], mat_proj);
				}

				for (int i=0; i<3; i++)
				{
					tri_projected.p[i].x += 1.0f;
					tri_projected.p[i].y += 1.0f;
					tri_projected.p[i].x *= 0.5f * SCREEN_WIDTH;
					tri_projected.p[i].y *= 0.5f * SCREEN_HEIGHT;
				}

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
			DrawTri(tri_projected, tri_projected.dp*255.0f, tri_projected.dp*255.0f, tri_projected.dp*255.0f, 255.0f);

			std::cout << std::to_string(tri_projected.dp) << std::endl;
		}

		f_elapsed_time = (SDL_GetTicks() - start_time) / 1000.0;
		float f_fps = (f_elapsed_time > 0) ? 1000.0f / f_elapsed_time : 0.0f;

		SDL_RenderPresent(renderer);
	};

	return 0;
}
