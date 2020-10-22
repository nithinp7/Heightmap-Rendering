#ifndef RENDER_H
#define RENDER_H

#define EPSILON 1e-4f
#define PI 3.14159265f

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#include <shader.hpp>

// Reference: https://github.com/nothings/stb/blob/master/stb_image.h#L4
// To use stb_image, add this in *one* C++ source file.
#include <stb_image.h>
#include <chrono>

struct Vertex {
	// position
	glm::vec3 Position;
	// normal
	glm::vec3 Normal;
	// texCoords
	glm::vec4 Color;
};

class Render
{
public:
	//Heightmap attributes
	unsigned int width = 512;
	unsigned int height = 512;

	float m_per_pix = 30.0f;
	float m_per_height = 11.0f;

	unsigned char* pre_data;
	unsigned char* post_data;

	unsigned int VAO0;
	unsigned int VAO1;
	unsigned int VAO2;
	unsigned int VAO3;

	// Heightmap data
	Vertex* vertices_pre;
	Vertex* vertices_post;

	std::vector<unsigned int> indices;

	std::vector<Vertex> line_pre;
	std::vector<Vertex> line_post;

	// constructor
	Render(Shader render, Shader line)
	{
		pre_data = (unsigned char*)malloc(width * height);
		post_data = (unsigned char*)malloc(width * height);
		vertices_pre = (Vertex*)malloc(sizeof(Vertex) * width * height);
		vertices_post = (Vertex*)malloc(sizeof(Vertex) * width * height);

		load_data("../Heightmap/Media/heightmaps/pre.data", pre_data);
		load_data("../Heightmap/Media/heightmaps/post.data", post_data);

		// start constructing the vertex sets
		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
			{
				int indx = width * j + i;

				vertices_pre[indx].Position.x = m_per_pix * (float)i;
				vertices_pre[indx].Position.y = m_per_height * (float)pre_data[indx];
				vertices_pre[indx].Position.z = m_per_pix * (float)j;

				float y = 0.004f * (float)pre_data[indx];

				//TODO: change placeholder
				vertices_pre[indx].Color = glm::vec4(y, y, y, 1.0f);

				vertices_post[indx].Position.x = m_per_pix * (float)i;
				vertices_post[indx].Position.y = m_per_height * (float)post_data[indx];
				vertices_post[indx].Position.z = m_per_pix * (float)j;

				y = 0.004f * (float)post_data[indx];

				//TODO: change placeholder
				vertices_post[indx].Color = glm::vec4(y, y, y, 1.0f);
			}

		// need another pass for setting normals
		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
			{
				int indx = width * j + i;

				vertices_pre[indx].Normal = calc_norm(i, j, &vertices_pre[0]);
				vertices_post[indx].Normal = calc_norm(i, j, &vertices_post[0]);
			}
				
		// pack EBO indices
		for (int i = 0; i < width - 1; i++)
			for (int j = 0; j < height - 1; j++)
			{
				// top triangle 
				indices.push_back(width * j + i);
				indices.push_back(width * j + i + 1);
				indices.push_back(width * (j + 1) + i);

				// bottom triangle
				indices.push_back(width * (j + 1) + i);
				indices.push_back(width * j + i + 1);
				indices.push_back(width * (j + 1) + i + 1);
			}

		unsigned int ax = 100;
		unsigned int ay = 90;
		unsigned int bx = 400;
		unsigned int by = 500;

		float pre_sd = surface_dist(ax, ay, bx, by, true);
		float post_sd = surface_dist(ax, ay, bx, by, false);

		printf("Traversing from (%d, %d) to (%d, %d)...\nSurface Distance Pre-Eruption is: %f\nSurface Distance Post-Eruption is: %f\n",
				ax, ay, bx, by, pre_sd, post_sd);

		printf("\n%d, %d\n", line_pre.size(), line_post.size());

		setup();

		render.use();

		float sc = 0.005f;

		glm::mat4 model;
		model = glm::translate(model, glm::vec3(0.0f, -10.0f, 0.0f));
		model = glm::rotate(model, PI, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(sc, sc, sc));
		render.setMat4("model", model);

		// Set material properties
		render.setVec3("material.specular", 0.3f, 0.3f, 0.3f);
		render.setFloat("material.shininess", 64.0f);

		line.use();
		line.setMat4("model", model);
	}
	
	// render the mesh
	void Draw(Shader render, Shader line, unsigned int pre_texture, unsigned int post_texture, bool renderPre)
	{
		render.use();

		glBindVertexArray(renderPre ? VAO0 : VAO1);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0);
		glBindVertexArray(0);

		line.use();

		glBindVertexArray(renderPre ? VAO2 : VAO3);
		glDrawArrays(GL_LINES, 0, renderPre ? line_pre.size() : line_post.size());
		glBindVertexArray(0);
	}

	void delete_buffers()
	{
		glDeleteVertexArrays(1, &VAO0);
		glDeleteVertexArrays(1, &VAO1);
		glDeleteVertexArrays(1, &VAO2);
		glDeleteVertexArrays(1, &VAO3);
		glDeleteBuffers(1, &VBO0);
		glDeleteBuffers(1, &VBO1);
		glDeleteBuffers(1, &VBO2);
		glDeleteBuffers(1, &VBO3);
		glDeleteBuffers(1, &EBO);

		free(pre_data);
		free(post_data);
		free(vertices_pre);
		free(vertices_post);
	}

private:

	/*  Render data  */
	unsigned int VBO0, VBO1, VBO2, VBO3, EBO;

	void setup()
	{
		// Prepare EBO
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);


		// PRE ERUPTION VERTEX ARRAY
		glGenVertexArrays(1, &VAO0);
		glGenBuffers(1, &VBO0);
		
		glBindVertexArray(VAO0);
		glBindBuffer(GL_ARRAY_BUFFER, VBO0);
		glBufferData(GL_ARRAY_BUFFER, width * height * sizeof(Vertex), &vertices_pre[0], GL_STATIC_DRAW);

		// Position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// Normal
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
		// Color
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 6));


		// POST ERUPTION VERTEX ARRAY
		glGenVertexArrays(1, &VAO1);
		glGenBuffers(1, &VBO1);

		glBindVertexArray(VAO1);
		glBindBuffer(GL_ARRAY_BUFFER, VBO1);
		glBufferData(GL_ARRAY_BUFFER, width * height * sizeof(Vertex), &vertices_post[0], GL_STATIC_DRAW);

		// Position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// Normal
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
		// Color
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 6));
		

		// SURFACE DISTANCE LINES
		// PRE
		glGenVertexArrays(1, &VAO2);
		glGenBuffers(1, &VBO2);

		glBindVertexArray(VAO2);
		glBindBuffer(GL_ARRAY_BUFFER, VBO2);
		glBufferData(GL_ARRAY_BUFFER, line_pre.size() * sizeof(Vertex), &line_pre[0], GL_STATIC_DRAW);

		// Position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// Normal
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
		// Color
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 6));

		// POST
		glGenVertexArrays(1, &VAO3);
		glGenBuffers(1, &VBO3);

		glBindVertexArray(VAO3);
		glBindBuffer(GL_ARRAY_BUFFER, VBO3);
		glBufferData(GL_ARRAY_BUFFER, line_post.size() * sizeof(Vertex), &line_post[0], GL_STATIC_DRAW);

		// Position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// Normal
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
		// Color
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 6));


		glBindVertexArray(0);
	}	

	void load_data(char* path, unsigned char* target)
	{
		ifstream rawFile(path, ios::in | ios::binary);
		rawFile.read((char*)target, width * height);
		if (!rawFile)
		{
			printf("Error reading file: %s\n", path);
			exit(-1);
		}
	}

	// takes care of boundary case but does not check for outright invalid indices
	glm::vec3 calc_norm(int i, int j, Vertex* arr)
	{
		int i0 = i == 0 ? 0 : (i - 1);
		int i1 = i == (width - 1) ? (width - 1) : (i + 1);
		int j0 = j == 0 ? 0 : (j - 1);
		int j1 = j == (height - 1) ? (height - 1) : (j + 1);

		glm::vec3 dfdx = arr[width * j + i1].Position - arr[width * j + i0].Position;
		glm::vec3 dfdy = arr[width * j1 + i].Position - arr[width * j0 + i].Position;

		// TODO: flip order if needed
		return glm::normalize(-glm::cross(dfdx, dfdy));
	}

	// takes coordinate space floats 
	float bilerp_height(float x, float y, unsigned char* data)
	{
		if (x < 0 || x >= width - 1 || y < 0 || y >= height - 1)
		{
			printf("Invalid coordinates in interp_height(...)\n");
			exit(-1);
			return (float)nan("");
		}

		// top left and bottom right grid coordinates 
		unsigned int ax = (unsigned int)x;
		unsigned int ay = (unsigned int)y;
		unsigned int bx = ax + 1;
		unsigned int by = ay + 1;


		// heights in clockwise order around grid cell 
		float z0 = data[ay * width + ax];
		float z1 = data[ay * width + bx];
		float z2 = data[by * width + bx];
		float z3 = data[by * width + ax];

		float dx = x - ax;
		float dy = y - ay;

		// in the top triangle
		if (dx + dy < 1.0f)
		{
			// interpolate first on x axis
			float xlerp = z0 * (1 - dx) + z1 * dx;
			// now interpolate on the y axis and return 
			return xlerp * (1 - dy) + z3 * dy;
		}
		else {
		// in the bottom triangle
			// interpolate first on x axis
			float xlerp = z3 * (1 - dx) + z2 * dx;
			// now interpolate on the y axis and return 
			return xlerp * dy + z1 * (1 - dy);
		}
	}

	// dh is the integrating precision 
	float surface_dist(unsigned int axu, unsigned int ayu, unsigned int bxu, unsigned int byu,
		bool pre, float dh = 0.1f)
	{
		unsigned char* data = pre ? pre_data : post_data;
		std::vector<Vertex>* target = pre ? &line_pre : &line_post;
		
		Vertex v;
		// Normal not used for lines
		v.Normal = glm::vec3(0.0f, 0.0f, 0.0f);
		v.Color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

		
		if (axu >= width || ayu >= height || bxu >= width || byu >= height)
		{
			printf("Invalid coordinates, exiting program...\n");
			exit(-1);
			return (float)nan("");
		}

		// cast to floating point for calculations
		float ax = axu, ay = ayu,
			bx = bxu, by = byu;

		float difx = bx - ax;
		float dify = by - ay;

		float mag = sqrt(difx * difx + dify * dify);

		// avoid divide by 0 error (will only happen when surface distance is practically 0 anyways)
		if (mag < EPSILON)
			return 0.0f;

		float dx = dh * difx / mag;
		float dy = dh * dify / mag;

		float x = ax;
		float y = ay;
		float z = bilerp_height(x, y, data);

		float sd = 0.0f;

		// walk along the height map and integrate surface distance 
		while (sqrt((bx - x) * (bx - x) + (by - y) * (by - y)) > 2.0f * dh)
		{
			float nx = x + dx;
			float ny = y + dy;
			float nz = bilerp_height(nx, ny, data);

			// accumulate incremental arclength
			sd += sqrt(dx * dx * m_per_pix * m_per_pix +
					   dy * dy * m_per_pix * m_per_pix +
					   (nz - z) * (nz - z) * m_per_height * m_per_height);

			v.Position.x = m_per_pix * x;
			v.Position.y = m_per_height * z + 1.0f;
			v.Position.z = m_per_pix * y;

			target->push_back(v);

			v.Position.x = m_per_pix * nx;
			v.Position.y = m_per_height * nz + 1.0f;
			v.Position.z = m_per_pix * ny;

			target->push_back(v);

			x = nx;
			y = ny;
			z = nz;
		}

		return sd;
	}
};
#endif
