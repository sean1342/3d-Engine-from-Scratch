#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
// #include <glm/gtx/transform.hpp>
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

glm::mat4 Matrix_PointAt(glm::vec3 pos, glm::vec3 target, glm::vec3 up)
{
	glm::vec3 new_forward = glm::normalize(target - pos);
	glm::vec3 a = new_forward * glm::dot(up, new_forward);
	glm::vec3 new_up = glm::normalize(up - a);
	glm::vec3 new_right = glm::cross(new_up, new_forward);

	glm::mat4 m;
	m[0][0] = new_right.x;		m[0][1] = new_right.y;		m[0][2] = new_right.z;		m[0][3] = 0.0f;
	m[1][0] = new_up.x;			m[1][1] = new_up.y;			m[1][2] = new_up.z;			m[1][3] = 0.0f;
	m[2][0] = new_forward.x;	m[2][1] = new_forward.y;	m[2][2] = new_forward.z;	m[2][3] = 0.0f;
	m[3][0] = pos.x;			m[3][1] = pos.y;			m[3][2] = pos.z;			m[3][3] = 1.0f;
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

mesh obj;

glm::vec4 v_cam;
glm::vec4 v_look_dir;

int main(int argc, char *argv[])
{
	SDL_Init(SCREEN_WIDTH, SCREEN_HEIGHT);

	obj.LoadFromObjectFile("axis.obj");

	glm::mat4 mat_proj = glm::perspective(glm::radians(f_FOV), 1/f_ASPECT_RATIO, f_NEAR, f_FAR);

	v_look_dir = glm::vec4(0, 0, 1, 1);
	glm::vec3 v_up = glm::vec3(0, 1, 0);
	glm::vec3 v_target = v_cam + v_look_dir;

	glm::mat4 mat_cam = Matrix_PointAt(v_cam, v_target, v_up);

	glm::mat4 mat_view = glm::inverse(mat_cam);

	float f_elapsed_time = 0.0f;

	bool running = true;
	while(running)
	{
		int start_time = SDL_GetTicks();

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		std::vector<triangle> vec_tris_to_raster;

		// draw tris
		for(auto tri : obj.tris)
		{
			triangle tri_projected, tri_translated;

			glm::mat4 mat_trans = glm::mat4(1.0f);
			mat_trans = glm::translate(mat_trans, glm::vec3(0.0f, 0.0f, 8.0f));

			// tri_translated.p[0] = tri.p[0] * mat_trans;
			// tri_translated.p[1] = tri.p[1] * mat_trans;
			// tri_translated.p[2] = tri.p[2] * mat_trans;

			tri_translated = tri;
			for(int i=0; i<3; i++)
				tri_translated.p[i].z += 16.0f;

			glm::vec3 normal, line1, line2;
			line1 = tri_translated.p[1] - tri_translated.p[0];
			line2 = tri_translated.p[2] - tri_translated.p[0];

			normal = glm::normalize(glm::cross(line1, line2));

			glm::vec3 v_cam_ray = tri_translated.p[0] - v_cam;

			if(glm::dot(normal, v_cam_ray) < 0.0f)
			{
				glm::vec3 light_direction = glm::normalize(glm::vec3({ 0.0f, 0.0f, -1.0f }));

				// how similar is normal to light direction
				tri_projected.dp = std::max(0.1f, glm::dot(normal, light_direction));

				for(int i=0; i<3; i++)
					tri_projected.p[i] = glm::normalize(mat_proj * tri_translated.p[i]);

				// offset verts to scale into view space on screen
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

		// sort triangles to render from back to front (painters algorithm)
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
