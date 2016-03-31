#include "glhelper.h"

char* loadFile(const char *filename) {
	char* data;
	int len;
	std::ifstream ifs(filename, std::ifstream::in);

	ifs.seekg(0, std::ios::end);
	len = ifs.tellg();

	ifs.seekg(0, std::ios::beg);
	data = new char[len + 1];

	ifs.read(data, len);
	data[len] = 0;

	ifs.close();

	return data;
}

unsigned char* loadImage(const char* image, int& width, int& height, int& bpp) {
	SDL_Surface *s = IMG_Load(image);
	width = s->w; height = s->h; bpp = s->format->BitsPerPixel;
	unsigned char *buffer = new unsigned char[width * height * bpp / 8];
	memcpy(buffer, s->pixels, width * height * bpp / 8);
	SDL_FreeSurface(s);
	return buffer;
}

void setupOrtho(int width, int height) {
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

SDL_Surface* mySDLInit(const int WIDTH, const int HEIGHT, const int BPP, const bool fullscreen) {
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	SDL_Surface *s = SDL_SetVideoMode(WIDTH, HEIGHT, BPP, (fullscreen ? SDL_FULLSCREEN : 0) | SDL_HWSURFACE | SDL_OPENGL);
//	SDL_Surface *s = SDL_SetVideoMode(WIDTH, HEIGHT, BPP, (fullscreen ? SDL_FULLSCREEN : 0) | SDL_HWSURFACE | SDL_OPENGL | SDL_NOFRAME);
	std::cerr << SDL_GetError() << std::endl;
	
	GLenum err = glewInit();
	std::cerr << glewGetErrorString(err) << std::endl;

	glViewport(0, 0, WIDTH, HEIGHT);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glClearStencil(0);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);//EQUAL);

	glEnable(GL_STENCIL_TEST);

	glEnable(GL_MULTISAMPLE);

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glLineWidth(1.0f);

	std::cerr << glGetString(GL_VENDOR)<< std::endl;
	std::cerr << glGetString(GL_VERSION)<< std::endl;
	std::cerr << glGetString(GL_RENDERER)<< std::endl;
	std::cerr << glGetString(GL_SHADING_LANGUAGE_VERSION)<< std::endl;
	std::cerr << glewGetString(GLEW_VERSION)<< std::endl;
	//std::cerr << glGetString(GL_EXTENSIONS)<< std::endl;

	return s;
}

void setupTexture(GLuint& texture) {
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void setupTextureRGB(GLuint& texture, int w, int h, const unsigned char texture_[]) {
	setupTexture(texture);
	std::cout << "RGB setup texture = " << texture << std::endl;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_);
}

void setupTextureRGBA(GLuint& texture, int w, int h, const unsigned char texture_[]) {
	setupTexture(texture);
	std::cout << "RGBA setup texture = " << texture << std::endl;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_);
}










void setupTexture(GLuint& texture, SDL_Surface *s) {
	setupTexture(texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->w, s->h, 0, s->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, s->pixels);
}

void setupTextureFloat(GLuint& texture, int w, int h, const float texturef[]) {
	setupTexture(texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, texturef);
}

void setupTextureFloat32(GLuint& texture, int w, int h, const float texturef[]) {
	setupTexture(texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_LUMINANCE, GL_FLOAT, texturef);
}


void setupTextureImage(GLuint& texture, int w, int h, int bpp, const unsigned char texture_[]) {
	if      (bpp == 32) setupTextureRGBA(texture, w, h, texture_);
	else if (bpp == 24) setupTextureRGB(texture, w, h, texture_);
}

void setupTextureTGA(GLuint& texture, const char* tga_file, unsigned char*& buffer, int& width, int& height, int& bpp) {
	std::fstream inf(tga_file, std::ios_base::in | std::ios_base::binary);
	unsigned char header[18];
	inf.read((char *)header, 18);
	width  = header[12] + (header[13] << 8);
	height = header[14] + (header[15] << 8);
	bpp    = header[16];
	buffer = new unsigned char[width*height*bpp/8];
	std::cout<<(long long)(buffer)<<std::endl;
	inf.read((char *)buffer, width*height*bpp/8);
	std::cout << width << " " << height << " " << bpp << std::endl;
	setupTexture(texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, bpp == 32 ? GL_BGRA : GL_BGR, GL_UNSIGNED_BYTE, buffer);
}

void deleteTextureTGA(GLuint& texture, unsigned char*& buffer) {
	delete [] buffer;
	glDeleteTextures(1, &texture);
}

void deleteTexture(GLuint& texture) {
	glDeleteTextures(1, &texture);
}

void setupCubeMap(GLuint& texture) {
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void setupCubeMap(GLuint& texture, SDL_Surface *xpos, SDL_Surface *xneg, SDL_Surface *ypos, SDL_Surface *yneg, SDL_Surface *zpos, SDL_Surface *zneg) {
	setupCubeMap(texture);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, xpos->w, xpos->h, 0, xpos->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, xpos->pixels);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, xneg->w, xneg->h, 0, xneg->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, xneg->pixels);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, ypos->w, ypos->h, 0, ypos->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, ypos->pixels);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, yneg->w, yneg->h, 0, yneg->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, yneg->pixels);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, zpos->w, zpos->h, 0, zpos->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, zpos->pixels);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, zneg->w, zneg->h, 0, zneg->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, zneg->pixels);
}

void deleteCubeMap(GLuint& texture) {
	glDeleteTextures(1, &texture);
}

void createProgram(GLuint& glProgram, GLuint& glShaderV, GLuint& glShaderF, const char* vertex_shader, const char* fragment_shader) {
	glShaderV = glCreateShader(GL_VERTEX_SHADER);
	glShaderF = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar* vShaderSource = loadFile(vertex_shader);
	const GLchar* fShaderSource = loadFile(fragment_shader);
	glShaderSource(glShaderV, 1, &vShaderSource, NULL);
	glShaderSource(glShaderF, 1, &fShaderSource, NULL);
	delete [] vShaderSource;
	delete [] fShaderSource;
	glCompileShader(glShaderV);
	glCompileShader(glShaderF);
	glProgram = glCreateProgram();
	glAttachShader(glProgram, glShaderV);
	glAttachShader(glProgram, glShaderF);
	glLinkProgram(glProgram);
	glUseProgram(glProgram);

	int  vlength,    flength,    plength;
	char vlog[2048], flog[2048], plog[2048];
	glGetShaderInfoLog(glShaderV, 2048, &vlength, vlog);
	glGetShaderInfoLog(glShaderF, 2048, &flength, flog);
	glGetProgramInfoLog(glProgram, 2048, &flength, plog);
	std::cout << vlog << std::endl << std::endl << flog << std::endl << std::endl << plog << std::endl << std::endl;
}

void releaseProgram(GLuint& glProgram, GLuint glShaderV, GLuint glShaderF) {
	glDetachShader(glProgram, glShaderF);
	glDetachShader(glProgram, glShaderV);
	glDeleteShader(glShaderF);
	glDeleteShader(glShaderV);
	glDeleteProgram(glProgram);
}

void saveTGA(unsigned char* buffer, int width, int height, bool video) {
	static int i = 0;
	std::stringstream out;
	if (video) {
		if      (i < 10)
			out << "video000" << (i++) << ".tga";
		else if (i < 100)
			out << "video00" << (i++) << ".tga";
		else if (i < 1000)
			out << "video0" << (i++) << ".tga";
		else if (i < 10000)
			out << "video" << (i++) << ".tga";
	} else {
		if      (i < 10)
			out << "screencaptures/capture000" << (i++) << ".tga";
		else if (i < 100)
			out << "screencaptures/capture00" << (i++) << ".tga";
		else if (i < 1000)
			out << "screencaptures/capture0" << (i++) << ".tga";
		else if (i < 10000)
			out << "screencaptures/capture" << (i++) << ".tga";
	}
	std::string s = out.str();
	
	glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
	std::fstream of(s.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	char header[18] = { 0 };
	header[2] = 2;
	header[12] = width & 0xff;
	header[13] = width >> 8;
	header[14] = height & 0xff;
	header[15] = height >> 8;
	header[16] = 32;
	of.write(header, 18);
	of.write((char *)buffer, width * height * 4);
}

void saveTGARGBA(unsigned char* buffer, int width, int height, bool video) {
	static int i = 0;
	std::stringstream out;
	if (video) {
		if      (i < 10)
			out << "video000" << (i++) << ".tga";
		else if (i < 100)
			out << "video00" << (i++) << ".tga";
		else if (i < 1000)
			out << "video0" << (i++) << ".tga";
		else if (i < 10000)
			out << "video" << (i++) << ".tga";
	} else {
		if      (i < 10)
			out << "screencaptures/capture000" << (i++) << ".tga";
		else if (i < 100)
			out << "screencaptures/capture00" << (i++) << ".tga";
		else if (i < 1000)
			out << "screencaptures/capture0" << (i++) << ".tga";
		else if (i < 10000)
			out << "screencaptures/capture" << (i++) << ".tga";
	}
	std::string s = out.str();
	
	std::fstream of(s.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	char header[18] = { 0 };
	header[2] = 2;
	header[12] = width & 0xff;
	header[13] = width >> 8;
	header[14] = height & 0xff;
	header[15] = height >> 8;
	header[16] = 32;
	of.write(header, 18);
	of.write((char *)buffer, width * height * 4);
}

void saveTGADouble(double* buffer, int width, int height) {
	static int i = 0;
	std::stringstream out;
	
		if      (i < 10)
			out << "screencaptures/image000" << (i++) << ".tga";
		else if (i < 100)
			out << "screencaptures/image00" << (i++) << ".tga";
		else if (i < 1000)
			out << "screencaptures/image0" << (i++) << ".tga";
		else if (i < 10000)
			out << "screencaptures/image" << (i++) << ".tga";

	std::string s = out.str();

	std::fstream of(s.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);

	unsigned char *buffer_ = new unsigned char[width * height * 4];
	for (int j = 0; j < width; j++) {
		for (int i = 0; i < width; i++) {
			buffer_[j * (width * 4) + i * 4 + 0] = (unsigned char)(buffer[j * width + i]);
			buffer_[j * (width * 4) + i * 4 + 1] = (unsigned char)(buffer[j * width + i]);
			buffer_[j * (width * 4) + i * 4 + 2] = (unsigned char)(buffer[j * width + i]);
			buffer_[j * (width * 4) + i * 4 + 3] = 255;
		}
	}
	
	char header[18] = { 0 };
	header[2] = 2;
	header[12] = width & 0xff;
	header[13] = width >> 8;
	header[14] = height & 0xff;
	header[15] = height >> 8;
	header[16] = 32;
	of.write(header, 18);
	of.write((char *)buffer_, width * height * 4);

	delete [] buffer_;
}

// saves the texture int the portable pixmap format
void savePPM(unsigned char* buffer, int width, int height, int bpp, const char* image_out) {
	std::fstream of(image_out, std::ios_base::out | std::ios_base::trunc);
	of << "P3" << std::endl;
	of << width << " " << height << std::endl;
	of << "255" << std::endl;
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			of << (int)buffer[j * width * bpp / 8 + i * bpp / 8 + 0] << " "
			   << (int)buffer[j * width * bpp / 8 + i * bpp / 8 + 1] << " "
			   << (int)buffer[j * width * bpp / 8 + i * bpp / 8 + 2] << " ";
		}
		of << std::endl;
	}
}

void glerror(const char* prepend) {
	std::cout << prepend << " -- " << gluErrorString(glGetError()) << std::endl;
}