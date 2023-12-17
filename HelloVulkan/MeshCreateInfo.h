#ifndef MESH_CREATE_INFO
#define MESH_CREATE_INFO

#include <vector>
#include <string>

class MeshCreateInfo
{
public:
	int texStartBindIndex;
	std::string modelFile;
	std::vector<std::string> textureFiles;
};

#endif
