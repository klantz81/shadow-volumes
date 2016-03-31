#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL.h>
//#include <SDL/SDL_ttf.h>
//#include <SDL/SDL_image.h>

#include <iostream>
#include <vector>

#include "assimp-3.1.1/include/assimp/Importer.hpp"
#include "assimp-3.1.1/include/assimp/scene.h"
#include "assimp-3.1.1/include/assimp/postprocess.h"

#include "glm-0.9.2.6/glm/glm.hpp"
#include "glm-0.9.2.6/glm/gtc/matrix_transform.hpp"
#include "glm-0.9.2.6/glm/gtc/type_ptr.hpp"

#include "src/vector.h"
#include "src/timer.h"
#include "src/glhelper.h"
#include "src/keyboard.h"

struct __dimensions {
	int width, height, bpp;
};

struct __vertex {
	float x,  y,   z;	// vertex
	float nx, ny, nz;	// normal
	float r,  g,   b;	// color
	float tx, ty, tz;	// texture vertex
	int v;			// similar vertex
	int fv0, fv1, fv2;	// filler
};

struct __edge {
	int v0, v1;	// vertex indices
	int f0, f1;	// face indices
};


void aiSceneStats(const aiScene* scene) {
	std::cout << "aiSceneStats()" << std::endl;
	std::cout << scene->mNumAnimations << " animations" << std::endl;
	std::cout << scene->mNumCameras << " cameras" << std::endl;
	std::cout << scene->mNumLights << " lights" << std::endl;
	std::cout << scene->mNumMaterials << " materials" << std::endl;
	std::cout << scene->mNumMeshes << " meshes" << std::endl;
	std::cout << scene->mNumTextures << " textures" << std::endl;
	std::cout << std::endl;
}

void aiNodeStats(const aiNode* node) {
	std::cout << "aiNodeStats()" << std::endl;
	std::cout << node->mNumChildren << " children" << std::endl;
	std::cout << node->mNumMeshes << " meshes" << std::endl;
	std::cout << std::endl;
}

float distance2(float x0, float y0, float z0, float x1, float y1, float z1) {
	return (x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1) + (z0 - z1) * (z0 - z1);
}

unsigned int findEdge(std::vector<__edge>& edges, int v0, int v1) {
	for (unsigned int i = 0; i < edges.size(); i++)
		if (v0 == edges[i].v0 && v1 == edges[i].v1)
			return i;
	return -1;
}

void aiMeshStats(const aiMesh* mesh, std::vector<__vertex>& vertices, std::vector<GLuint>& indices, std::vector<__edge>& edges) {
	std::cout << "aiMeshStats()" << std::endl;
	std::cout << mesh->mNumBones << " bones" << std::endl;
	std::cout << mesh->mNumFaces << " faces" << std::endl;
	std::cout << mesh->mNumVertices << " vertices" << std::endl;
	std::cout << std::endl;

	__vertex v;
	for (int i = 0; i < mesh->mNumVertices; i++) {
		v.x  = mesh->mVertices[i].x;
		v.y  = mesh->mVertices[i].y;
		v.z  = mesh->mVertices[i].z;
		v.nx = mesh->mNormals[i].x;
		v.ny = mesh->mNormals[i].y;
		v.nz = mesh->mNormals[i].z;
		v.r  = v.g  = v.b  = 1.0;
		v.tx = v.ty = v.tz = 0.0;
		v.v  = 0;
		vertices.push_back(v);
	}
	
	for (int i = 0; i < mesh->mNumFaces; i++)
		for (int j = 0; j < mesh->mFaces[i].mNumIndices; j++)		// mNumIndices should be three for triangles
			indices.push_back(mesh->mFaces[i].mIndices[j]);

// map similar vertices
	for (int i = 0; i < vertices.size(); i++) {
		for (int j = 0; j <= i; j++) {
			if (distance2(vertices[j].x, vertices[j].y, vertices[j].z,
				      vertices[i].x, vertices[i].y, vertices[i].z) < 0.000001) {
				vertices[i].v = j;
				break;
			}
		}
	}
// build edge list
	__edge edge;
	for (int i = 0, f = 0; i < indices.size(); i+=3, f++) {
		int v0 = vertices[indices[i+0]].v;
		int v1 = vertices[indices[i+1]].v;
		int v2 = vertices[indices[i+2]].v;

		int e0 = findEdge(edges, v0, v1);
		if (e0 > -1) {
			edges[e0].f1 = f;
		} else {
			edge.v0 = v1;
			edge.v1 = v0;
			edge.f0 = f;
			edge.f1 = -1;
			edges.push_back(edge);
		}
		
		int e1 = findEdge(edges, v1, v2);
		if (e1 > -1) {
			edges[e1].f1 = f;
		} else {
			edge.v0 = v2;
			edge.v1 = v1;
			edge.f0 = f;
			edge.f1 = -1;
			edges.push_back(edge);
		}
		
		int e2 = findEdge(edges, v2, v0);
		if (e2 > -1) {
			edges[e2].f1 = f;
		} else {
			edge.v0 = v0;
			edge.v1 = v2;
			edge.f0 = f;
			edge.f1 = -1;
			edges.push_back(edge);
		}
	}		
}


void buildShadowVolume(vector3& light_pos,
		      std::vector<__vertex>& vertices,          std::vector<GLuint>& indices,
		      std::vector<__edge>& edges,
		      std::vector<__vertex>& edge_vertices,     std::vector<GLuint>& edge_indices, std::vector<GLuint>& profile_indices,
		      std::vector<__vertex>& far_cap_vertices,  std::vector<GLuint>& far_cap_indices,
		      std::vector<__vertex>& near_cap_vertices, std::vector<GLuint>& near_cap_indices) {

	edge_vertices.clear();
	edge_indices.clear();
	profile_indices.clear();
	far_cap_vertices.clear();
	far_cap_indices.clear();
	near_cap_vertices.clear();
	near_cap_indices.clear();
	
// detect the object profile
	for (int i = 0; i < edges.size(); i++) {
		if (edges[i].f0 > -1 && edges[i].f1 > -1) {
		
			int i0_0 = edges[i].f0 * 3 + 0;
			int i1_0 = edges[i].f0 * 3 + 1;
			int i2_0 = edges[i].f0 * 3 + 2;

			int i0_1 = edges[i].f1 * 3 + 0;
			int i1_1 = edges[i].f1 * 3 + 1;
			int i2_1 = edges[i].f1 * 3 + 2;

			int v0_0 = indices[i0_0];
			int v1_0 = indices[i1_0];
			int v2_0 = indices[i2_0];
		  
			int v0_1 = indices[i0_1];
			int v1_1 = indices[i1_1];
			int v2_1 = indices[i2_1];
			
			vector3 p0_0(vertices[v0_0].x, vertices[v0_0].y, vertices[v0_0].z);
			vector3 p1_0(vertices[v1_0].x, vertices[v1_0].y, vertices[v1_0].z);
			vector3 p2_0(vertices[v2_0].x, vertices[v2_0].y, vertices[v2_0].z);
			
			vector3 p0_1(vertices[v0_1].x, vertices[v0_1].y, vertices[v0_1].z);
			vector3 p1_1(vertices[v1_1].x, vertices[v1_1].y, vertices[v1_1].z);
			vector3 p2_1(vertices[v2_1].x, vertices[v2_1].y, vertices[v2_1].z);

			vector3 v_0 = ((p0_0 + p1_0 + p2_0) * (1.0/3.0) - light_pos).unit();
			vector3 v_1 = ((p0_1 + p1_1 + p2_1) * (1.0/3.0) - light_pos).unit();

			vector3 n_0 = (p1_0 - p0_0).cross(p2_0 - p0_0).unit();
			vector3 n_1 = (p1_1 - p0_1).cross(p2_1 - p0_1).unit();

			if ((v_0*n_0) * (v_1*n_1) < 0) {
				if (v_0*n_0 < 0) {
					profile_indices.push_back(edges[i].v1);
					profile_indices.push_back(edges[i].v0);
				} else {
					profile_indices.push_back(edges[i].v0);
					profile_indices.push_back(edges[i].v1);
				}
			}
		}
	}
	
// extrude the profile edges
	for (unsigned int i = 0; i < profile_indices.size(); i+=2) {
		__vertex v;
		v.nx = v.ny = v.nz = 1.0;
		v.r  = v.g  = v.b  = 0.0;
		v.tx = v.ty = v.tz = 0.0;

		int i0 = profile_indices[i+0];
		int i1 = profile_indices[i+1];
		
		vector3 p0(vertices[i0].x, vertices[i0].y, vertices[i0].z);
		vector3 p1(vertices[i1].x, vertices[i1].y, vertices[i1].z);	
		
		vector3 l0 = (p0 - light_pos).unit();
		vector3 l1 = (p1 - light_pos).unit();
		
		v.x = p0.x;
		v.y = p0.y;
		v.z = p0.z;
		edge_vertices.push_back(v);
		edge_indices.push_back(edge_indices.size());
		
		vector3 ep0 = p0 + l0 * 100.0;
		v.x = ep0.x;
		v.y = ep0.y;
		v.z = ep0.z;
		edge_vertices.push_back(v);
		edge_indices.push_back(edge_indices.size());
		
		vector3 ep1 = p1 + l1 * 100.0;
		v.x = ep1.x;
		v.y = ep1.y;
		v.z = ep1.z;
		edge_vertices.push_back(v);
		edge_indices.push_back(edge_indices.size());
		
		v.x = p1.x;
		v.y = p1.y;
		v.z = p1.z;
		edge_vertices.push_back(v);
		edge_indices.push_back(edge_indices.size());
	}
	
// build the near and far caps
	for (int i = 0; i < indices.size(); i+=3) {
		vector3 p0(vertices[indices[i+0]].x, vertices[indices[i+0]].y, vertices[indices[i+0]].z);
		vector3 p1(vertices[indices[i+1]].x, vertices[indices[i+1]].y, vertices[indices[i+1]].z);
		vector3 p2(vertices[indices[i+2]].x, vertices[indices[i+2]].y, vertices[indices[i+2]].z);
		vector3 l = ((p0 + p1 + p2) * (1.0/3.0) - light_pos).unit();
		vector3 n = (p1 - p0).cross(p2 - p0).unit();
		__vertex v;
		v.nx = v.ny = v.nz = 1.0;
		v.r  = v.g  = v.b  = 0.0;
		v.tx = v.ty = v.tz = 0.0;
		if (l*n < 0) {
			v.x = p0.x;
			v.y = p0.y;
			v.z = p0.z;
			near_cap_vertices.push_back(v); near_cap_indices.push_back(near_cap_indices.size());
			
			v.x = p1.x;
			v.y = p1.y;
			v.z = p1.z;
			near_cap_vertices.push_back(v); near_cap_indices.push_back(near_cap_indices.size());
			
			v.x = p2.x;
			v.y = p2.y;
			v.z = p2.z;
			near_cap_vertices.push_back(v); near_cap_indices.push_back(near_cap_indices.size());
		} else {
			p0 = p0 + (p0 - light_pos).unit() * 100.f;
			p1 = p1 + (p1 - light_pos).unit() * 100.f;
			p2 = p2 + (p2 - light_pos).unit() * 100.f;

			v.x = p0.x;
			v.y = p0.y;
			v.z = p0.z;
			far_cap_vertices.push_back(v); far_cap_indices.push_back(far_cap_indices.size());
			
			v.x = p1.x;
			v.y = p1.y;
			v.z = p1.z;
			far_cap_vertices.push_back(v); far_cap_indices.push_back(far_cap_indices.size());
			
			v.x = p2.x;
			v.y = p2.y;
			v.z = p2.z;
			far_cap_vertices.push_back(v); far_cap_indices.push_back(far_cap_indices.size());
		}
	}
}

int main(int argc, char* argv[]) {
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	vector3 light_pos(40000.0f, 20000.0f, 40000.0f);

	float camera_alpha = 30.f;
	float camera_beta = 25.f;
	vector3 camera_pos(-10.0, 15.0, 15.0);

	std::vector<__vertex> vertices;
	std::vector<GLuint> indices;
	std::vector<__edge> edges;

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile("object.obj", aiProcess_Triangulate);
//	const aiScene* scene = importer.ReadFile("monkey.obj", aiProcess_Triangulate);
	aiSceneStats(scene);
	aiNodeStats(scene->mRootNode);
	aiMeshStats(scene->mMeshes[0], vertices, indices, edges);

	std::vector<__vertex> edge_vertices;
	std::vector<GLuint> edge_indices;
	std::vector<GLuint> profile_indices;
	std::vector<__vertex> far_cap_vertices;
	std::vector<GLuint> far_cap_indices;
	std::vector<__vertex> near_cap_vertices;
	std::vector<GLuint> near_cap_indices;
	
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	cTimer timer;

	__dimensions dim = {1280, 720, 32};
	bool active = true;

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Surface *screen = mySDLInit(dim.width, dim.height, dim.bpp, false);
	SDL_Event event;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// load shaders

	GLuint glProgram, glShaderV, glShaderF;
	GLint vertex, normal, color, color_factor, light_position, Projection, View, Model;
	createProgram(glProgram, glShaderV, glShaderF, "src/vertex.sh", "src/fragment.sh");
	vertex          = glGetAttribLocation(glProgram, "vertex");
	normal          = glGetAttribLocation(glProgram, "normal");
	color           = glGetAttribLocation(glProgram, "color");
	color_factor    = glGetUniformLocation(glProgram, "color_factor");
	light_position  = glGetUniformLocation(glProgram, "light_position");
	Projection      = glGetUniformLocation(glProgram, "Projection");
	View            = glGetUniformLocation(glProgram, "View");
	Model           = glGetUniformLocation(glProgram, "Model");

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// generate buffer objects
	GLuint vbo_vertices;
	glGenBuffers(1, &vbo_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(__vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	GLuint ibo_indices;
	glGenBuffers(1, &ibo_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), &indices[0], GL_STATIC_DRAW);

	GLuint vbo_edge_vertices;
	glGenBuffers(1, &vbo_edge_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_edge_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(__vertex) * edge_vertices.size(), &edge_vertices[0], GL_DYNAMIC_DRAW);

	GLuint ibo_edge_indices;
	glGenBuffers(1, &ibo_edge_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_edge_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * edge_indices.size(), &edge_indices[0], GL_DYNAMIC_DRAW);
	
	GLuint ibo_profile_indices;
	glGenBuffers(1, &ibo_profile_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_profile_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * profile_indices.size(), &profile_indices[0], GL_DYNAMIC_DRAW);

	GLuint vbo_near_cap_vertices;
	glGenBuffers(1, &vbo_near_cap_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_near_cap_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(__vertex) * near_cap_vertices.size(), &near_cap_vertices[0], GL_DYNAMIC_DRAW);

	GLuint ibo_near_cap_indices;
	glGenBuffers(1, &ibo_near_cap_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_near_cap_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * near_cap_indices.size(), &near_cap_indices[0], GL_DYNAMIC_DRAW);

	GLuint vbo_far_cap_vertices;
	glGenBuffers(1, &vbo_far_cap_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_far_cap_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(__vertex) * far_cap_vertices.size(), &far_cap_vertices[0], GL_DYNAMIC_DRAW);

	GLuint ibo_far_cap_indices;
	glGenBuffers(1, &ibo_far_cap_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_far_cap_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * far_cap_indices.size(), &far_cap_indices[0], GL_DYNAMIC_DRAW);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	glm::mat4 projection = glm::perspective(45.0f, (float)dim.width / (float)dim.height, 0.1f, 10000.0f);//glm::ortho(0.0f, (float)dim.width, (float)dim.height, 0.0f, -5.0f, 5.0f); 
	glm::mat4 view       = glm::mat4(1.0f);
	glm::mat4 model      = glm::mat4(1.0f);
	float alpha          = 0.f;
	unsigned char *buffer = new unsigned char[dim.width*dim.height*dim.bpp/8];
	
	glClearColor(0.3f, 0.4f, 0.5f, 1.0f);
	
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool a_key = false,
	     s_key = false,
	     d_key = false,
	     w_key = false,
	     animate = true;

	while (active) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			    case SDL_QUIT:
			      active = false;
			      break;
			    case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				  case SDLK_a: a_key = true; break;
				  case SDLK_s: s_key = true; break;
				  case SDLK_d: d_key = true; break;
				  case SDLK_w: w_key = true; break;
				  case SDLK_g:
					saveTGA(buffer, dim.width, dim.height, false);
					break;
				}
				break;
			    case SDL_KEYUP:
				switch (event.key.keysym.sym) {
				  case SDLK_a: a_key = false; break;
				  case SDLK_s: s_key = false; break;
				  case SDLK_d: d_key = false; break;
				  case SDLK_w: w_key = false; break;
				}
				break;
			}
		}

	  	glUseProgram(glProgram);

		glStencilMask(0xFF);
		glDepthMask(GL_TRUE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		if (w_key)
			animate = true;
		if (a_key) {
			alpha += 0.2*timer.elapsed(true);
			animate = false;
		}
		if (d_key) {
			alpha -= 0.2*timer.elapsed(true);
			animate = false;
		}
		if (animate)
			alpha += 0.8*timer.elapsed(true);

		view = glm::mat4(1.0f);
		view = glm::rotate(view, camera_alpha, glm::vec3(1,0,0));
		view = glm::rotate(view, camera_beta,  glm::vec3(0,1,0));
		view = glm::translate(view, glm::vec3(-camera_pos.x, -camera_pos.y, -camera_pos.z));
		
		light_pos = vector3(cos(alpha) * 100.0f, 100.0f, sin(alpha) * 100.0f);
		
		glUniform3f(light_position, light_pos.x, light_pos.y, light_pos.z);
		glUniformMatrix4fv(Projection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(View, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(Model, 1, GL_FALSE, glm::value_ptr(model));
		
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// render the scene in shadow
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);

		glStencilMask(0xFF);
		glDepthMask(GL_TRUE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_indices);
		glEnableVertexAttribArray(vertex);
		glVertexAttribPointer(vertex, 3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL +  0);
		glEnableVertexAttribArray(normal);
		glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL + 12);
		glEnableVertexAttribArray(color);
		glVertexAttribPointer(color,  3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL + 24);
		glUniform1f(color_factor, 0.6f);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// update the stencil buffer
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_STENCIL_TEST);

		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
		glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
		glStencilMask(0xFF);
		glDepthMask(GL_FALSE);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		buildShadowVolume(light_pos, vertices, indices, edges, edge_vertices, edge_indices, profile_indices, far_cap_vertices, far_cap_indices, near_cap_vertices, near_cap_indices);
		
		
// render the sides of the shadow volume
		glBindBuffer(GL_ARRAY_BUFFER, vbo_edge_vertices);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_edge_indices);
		glBufferData(GL_ARRAY_BUFFER, sizeof(__vertex) * edge_vertices.size(), &edge_vertices[0], GL_DYNAMIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * edge_indices.size(), &edge_indices[0], GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(vertex);
		glVertexAttribPointer(vertex, 3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL +  0);
		glEnableVertexAttribArray(normal);
		glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL + 12);
		glEnableVertexAttribArray(color);
		glVertexAttribPointer(color,  3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL + 24);
		glUniform1f(color_factor, 1.f);
		glDrawElements(GL_QUADS, edge_indices.size(), GL_UNSIGNED_INT, 0);

// render the near cap of the shadow volume
		glBindBuffer(GL_ARRAY_BUFFER, vbo_near_cap_vertices);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_near_cap_indices);
		glBufferData(GL_ARRAY_BUFFER, sizeof(__vertex) * near_cap_vertices.size(), &near_cap_vertices[0], GL_DYNAMIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * near_cap_indices.size(), &near_cap_indices[0], GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(vertex);
		glVertexAttribPointer(vertex, 3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL +  0);
		glEnableVertexAttribArray(normal);
		glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL + 12);
		glEnableVertexAttribArray(color);
		glVertexAttribPointer(color,  3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL + 24);
		glUniform1f(color_factor, 1.f);
		glDrawElements(GL_TRIANGLES, near_cap_indices.size(), GL_UNSIGNED_INT, 0);
		
// render the far cap of the shadow volume
		glBindBuffer(GL_ARRAY_BUFFER, vbo_far_cap_vertices);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_far_cap_indices);
		glBufferData(GL_ARRAY_BUFFER, sizeof(__vertex) * far_cap_vertices.size(), &far_cap_vertices[0], GL_DYNAMIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * far_cap_indices.size(), &far_cap_indices[0], GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(vertex);
		glVertexAttribPointer(vertex, 3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL +  0);
		glEnableVertexAttribArray(normal);
		glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL + 12);
		glEnableVertexAttribArray(color);
		glVertexAttribPointer(color,  3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL + 24);
		glUniform1f(color_factor, 1.f);
		glDrawElements(GL_TRIANGLES, far_cap_indices.size(), GL_UNSIGNED_INT, 0);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// render the scene fully lit
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_STENCIL_TEST);

		glStencilFunc(GL_EQUAL, 0, 0xFF);
		glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
		glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
		glStencilMask(0xFF);
		glDepthMask(GL_TRUE);
		glClear(GL_DEPTH_BUFFER_BIT);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		
		glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_indices);
		glEnableVertexAttribArray(vertex);
		glVertexAttribPointer(vertex, 3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL +  0);
		glEnableVertexAttribArray(normal);
		glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL + 12);
		glEnableVertexAttribArray(color);
		glVertexAttribPointer(color,  3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL + 24);
		glUniform1f(color_factor, 1.f);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// render the profile
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		
		glStencilMask(0xFF);
		glDepthMask(GL_TRUE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		
		glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_profile_indices);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * profile_indices.size(), &profile_indices[0], GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(vertex);
		glVertexAttribPointer(vertex, 3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL +  0);
		glEnableVertexAttribArray(normal);
		glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL + 12);
		glEnableVertexAttribArray(color);
		glVertexAttribPointer(color,  3, GL_FLOAT, GL_FALSE, sizeof(__vertex), (char *)NULL + 24);
		glUniform1f(color_factor, 10.f);
		
		if (s_key)
			glDrawElements(GL_LINES, profile_indices.size(), GL_UNSIGNED_INT, 0);
		
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		SDL_GL_SwapBuffers();
	}

	delete [] buffer;
	glDeleteBuffers(1, &vbo_vertices);
	glDeleteBuffers(1, &ibo_indices);
	glDeleteBuffers(1, &vbo_edge_vertices);
	glDeleteBuffers(1, &ibo_edge_indices);
	glDeleteBuffers(1, &ibo_profile_indices);
	glDeleteBuffers(1, &vbo_far_cap_vertices);
	glDeleteBuffers(1, &ibo_far_cap_indices);
	glDeleteBuffers(1, &vbo_near_cap_vertices);
	glDeleteBuffers(1, &ibo_near_cap_indices);
	releaseProgram(glProgram, glShaderV, glShaderF);
	
	SDL_Quit();
	
	return 0;
}
