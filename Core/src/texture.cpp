#include "pch.h"
#include "texture.h"
#include "GL/glew.h"
#include "lodepng/lodepng.h"

#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII

namespace Core {

	Texture::Texture() { }

	Texture::Texture(std::filesystem::path entry) {

		std::string extension = entry.extension().string();
		std::string path = entry.string();

		if (extension == ".png")
			Texture::loadPNGFile(path.c_str());

		//if (extension == ".DDS")
		//	Texture::loadDDS(path.c_str());
		//else if (extension == ".png")
		//	Texture::loadPNGfile(path.c_str());
	}

	Texture::~Texture() {

		delete[] data;
		//glDeleteTextures(1, &textureId);
	}

	//// ref: http://www.opengl-tutorial.org/beginners-tutorials/tutorial-5-a-textured-cube/
	//void Texture::loadDDS(const char* path) {

	//	unsigned char header[124];

	//	FILE* fp;

	//	/* try to open the file */
	//	fp = fopen(path, "rb");
	//	if (fp == NULL) {
	//		printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", path); getchar();
	//		return;
	//	}

	//	/* verify the type of file */
	//	char filecode[4];
	//	fread(filecode, 1, 4, fp);
	//	if (strncmp(filecode, "DDS ", 4) != 0) {
	//		fclose(fp);
	//		return;
	//	}

	//	/* get the surface desc */
	//	fread(&header, 124, 1, fp);

	//	unsigned int height = *(unsigned int*)&(header[8]);
	//	unsigned int width = *(unsigned int*)&(header[12]);
	//	unsigned int linearSize = *(unsigned int*)&(header[16]);
	//	unsigned int mipMapCount = *(unsigned int*)&(header[24]);
	//	unsigned int fourCC = *(unsigned int*)&(header[80]);


	//	unsigned char* buffer;
	//	unsigned int bufsize;
	//	/* how big is it going to be including all mipmaps? */
	//	bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
	//	buffer = (unsigned char*)malloc(bufsize * sizeof(unsigned char));
	//	fread(buffer, 1, bufsize, fp);
	//	/* close the file pointer */
	//	fclose(fp);

	//	unsigned int components = (fourCC == FOURCC_DXT1) ? 3 : 4;
	//	unsigned int format;
	//	switch (fourCC)
	//	{
	//	case FOURCC_DXT1:
	//		format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	//		break;
	//	case FOURCC_DXT3:
	//		format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	//		break;
	//	case FOURCC_DXT5:
	//		format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	//		break;
	//	default:
	//		free(buffer);
	//		return;
	//	}

	//	// Create one OpenGL texture
	//	glGenTextures(1, &textureId);

	//	// "Bind" the newly created texture : all future texture functions will modify this texture
	//	glBindTexture(GL_TEXTURE_2D, textureId);
	//	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	//	unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
	//	unsigned int offset = 0;

	//	unsigned int size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
	//	glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, size, buffer + offset);

	//	free(buffer);

	//	float maxAniso = 0.0f;
	//	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
	//	//maxAniso = 4;

	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
	//	glGenerateMipmap(GL_TEXTURE_2D);

	//	glBindTexture(GL_TEXTURE_2D, 0);
	//}

	void Texture::loadPNGFile(const char* path) {

		if (path == "") {
			std::cout << "Please provide a filename to preview" << std::endl;
			return;
		}

		std::vector<unsigned char> buffer;
		std::vector<unsigned char> image;
		unsigned w, h;

		lodepng::load_file(buffer, path);

		lodepng::State state;
		unsigned error = lodepng::decode(image, w, h, state, buffer);

		if (error) {
			std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
			return;
		}

		width = w;
		height = h;
		bitDepth = state.info_png.color.bitdepth;
		channels = lodepng_get_channels(&state.info_png.color);

		int size = width * height;
		int step = image.size() / (width * height);
		data = new unsigned char[size * channels];
		for (int i = 0; i < size; i++)
			for (int j = 0; j < channels; j++)
				data[i * channels + j] = image[i * step + j];
	}

	Texture* Texture::mergeTextures(Texture* tex0, Texture* tex1, int ch0, int ch1) {

		int totalTexels = tex0->width * tex0->height;
		int newNumChannels = ch0 + ch1;
		unsigned char* data = new unsigned char[totalTexels * newNumChannels];

		for (int i = 0; i < totalTexels; i++) {

			for (int j = 0; j < ch0; j++)
				data[i * newNumChannels + j] = tex0->data[i * tex0->channels + j];

			for (int j = 0; j < ch1; j++)
				data[i * newNumChannels + ch0 + j] = tex1->data[i * tex1->channels + j];
		}

		Texture* newTexture = new Texture;
		newTexture->data = data;
		newTexture->channels = ch0 + ch1;
		newTexture->bitDepth = tex0->bitDepth;
		newTexture->width = tex0->width;
		newTexture->height = tex0->height;
		return newTexture;
	}

	Texture* Texture::loadTexturePartial(Texture* tex, int ch0) {

		int totalTexels = tex->width * tex->height;
		int newNumChannels = ch0;
		unsigned char* data = new unsigned char[totalTexels * newNumChannels];

		for (int i = 0; i < totalTexels; i++) {
			for (int j = 0; j < ch0; j++)
				data[i * newNumChannels + j] = tex->data[i * tex->channels + j];
		}

		Texture* newTexture = new Texture;
		newTexture->data = data;
		newTexture->channels = ch0;
		newTexture->bitDepth = tex->bitDepth;
		newTexture->width = tex->width;
		newTexture->height = tex->height;
		return newTexture;
	}

	unsigned int Texture::loadToGPU() {

		int channelType;
		switch (channels) {
		case 1: 
			channelType = GL_R; 
			break;
		case 2: 
			channelType = GL_RG; 
			break;
		case 3: 
			channelType = GL_RGB; 
			break;
		case 4: 
			channelType = GL_RGBA; 
			break;
		}

		float maxAniso = 0.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);

		unsigned int textureId;
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
		glTexImage2D(GL_TEXTURE_2D, 0, channelType, width, height, 0, channelType, GL_UNSIGNED_BYTE, &data[0]);
		glGenerateMipmap(GL_TEXTURE_2D);
		return textureId;
	}

	//void Texture::loadPNG(const char* path) {

	//	unsigned width, height;
	//	std::vector<unsigned char> image;
	//	lodepng::decode(image, width, height, path, LodePNGColorType::LCT_RGB, 8);

	//	float maxAniso = 0.0f;
	//	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);

	//	glGenTextures(1, &textureId);
	//	glBindTexture(GL_TEXTURE_2D, textureId);

	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);

	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, &image[0]);
	//	glGenerateMipmap(GL_TEXTURE_2D);
	//}

	unsigned int Texture::loadPNG_RGBA8(const char* path) {

		unsigned width, height;
		std::vector<unsigned char> image;
		lodepng::decode(image, width, height, path, LodePNGColorType::LCT_RGBA, 8);

		unsigned int textureId;
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
		glGenerateMipmap(GL_TEXTURE_2D);

		return textureId;
	}

}
