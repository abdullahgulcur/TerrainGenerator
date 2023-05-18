#include "pch.h"
#include "mesh.h"

#include "GL/glew.h"

namespace Core {

	Mesh::Mesh(std::string path) {

        Mesh::loadModel(path);
	}

	Mesh::~Mesh() {

        glDeleteVertexArrays(1, &VAO);
	}

    // ref: https://learnopengl.com/Model-Loading/Model
    void Mesh::loadModel(std::string path)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        // process ASSIMP's root node recursively
        Mesh::processNode(scene->mRootNode, scene);
    }

    // ref: https://learnopengl.com/Model-Loading/Model
    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void Mesh::processNode(aiNode* node, const aiScene* scene)
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* aimesh = scene->mMeshes[node->mMeshes[i]];
            Mesh::processMesh(aimesh, scene);
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    // ref: https://learnopengl.com/Model-Loading/Model
    void Mesh::processMesh(aiMesh* mesh, const aiScene* scene)
    {
        // data to fill
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        /* For AABB Box boundaries */
        int minX = 99999;
        int minY = 99999;
        int minZ = 99999;
        int maxX = -99999;
        int maxY = -99999;
        int maxZ = -99999;

        // walk through each of the mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.position = vector;

            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.normal = vector;
            }
            // texture coordinates
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.texCoord = vec;
            }
            else
                vertex.texCoord = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);

            /* Calculate AABB Box boundaries */
            if (vector.x > maxX)
                maxX = vector.x;

            if (vector.y > maxY)
                maxY = vector.y;

            if (vector.z > maxZ)
                maxZ = vector.z;

            if (vector.x < minX)
                minX = vector.x;

            if (vector.y < minY)
                minY = vector.y;

            if (vector.z < minZ)
                minZ = vector.z;
        }

        /* Set Box boundaries */
        aabbBox.start.x = minX;
        aabbBox.start.y = minY;
        aabbBox.start.z = minZ;
        aabbBox.start.w = 1.f;
        aabbBox.end.x = maxX;
        aabbBox.end.y = maxY;
        aabbBox.end.z = maxZ;
        aabbBox.end.w = 1.f;

        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Create VAO for actual mesh
        Mesh::createVAO(VAO, vertices, indices);

        indiceCount = indices.size();
    }

    void Mesh::createVAO(unsigned int& VAO, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {

        glGenVertexArrays(1, &VAO);

        unsigned int VBO;
        glGenBuffers(1, &VBO);
        unsigned int EBO;
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

        glBindVertexArray(0);
    }

}
