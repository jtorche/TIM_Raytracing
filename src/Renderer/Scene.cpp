#include "Scene.h"
#include "timCore/Common.h"
#include "BVHBuilder.h"
#include "BVHGeometry.h"
#include "TextureManager.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "timCore/flat_hash_map.h"

namespace tim
{
    struct ObjVertexKey 
    { 
        u32 posIndex, normalIndex, texcoordIndex; 
        bool operator==(const ObjVertexKey&) const = default;
    };
}
namespace std
{
    template<> struct hash<tim::ObjVertexKey>
    {
        size_t operator()(const tim::ObjVertexKey& key) const
        {
            size_t h = 0;
            tim::hash_combine(h, key.posIndex, key.normalIndex, key.texcoordIndex);
            return h;
        }
    };
}

namespace tim
{
    Scene::Scene(BVHGeometry& _geometryBuffer, TextureManager& _texManager) : m_geometryBuffer{ _geometryBuffer }, m_texManager{ _texManager }
    {

    }

    namespace
    {
        void transformVertex(vec3& _v, const vec3& _pos, vec3& _scale, bool _swapYZ)
        {
            if (_swapYZ)
                std::swap(_v.y, _v.z);
            _v = _v * _scale + _pos;
        }

        void transformNormal(vec3& _v, const vec3& _pos, vec3& _scale, bool _swapYZ)
        {
            if (_swapYZ)
                std::swap(_v.y, _v.z);
        }
    }

    void Scene::addOBJWithMtl(const fs::path& _path, vec3 _pos, vec3 _scale, BVHBuilder* _builder, bool _swapYZ)
    {
        addOBJInner(_path, _pos, _scale, _builder, nullptr, _swapYZ);
    }

    void Scene::addOBJ(const fs::path& _path, vec3 _pos, vec3 _scale, BVHBuilder* _builder, const Material& _mat, bool _swapYZ)
    {
        addOBJInner(_path, _pos, _scale, _builder, &_mat, _swapYZ);
    }


    static std::vector<Material> loadMtl(TextureManager& _texManager, const fs::path& _path, 
                                         const std::vector<tinyobj::material_t>& mtlMaterials)
    {
        std::vector<Material> materials(mtlMaterials.size());
        ska::flat_hash_map<std::string, u32> texToId;
        for (u32 i = 0; i < mtlMaterials.size(); ++i)
        {
            Material mat = BVHBuilder::createLambertianMaterial({ mtlMaterials[i].diffuse[0], mtlMaterials[i].diffuse[1], mtlMaterials[i].diffuse[2] });
            if (!mtlMaterials[i].diffuse_texname.empty())
            {
                auto it = texToId.find(mtlMaterials[i].diffuse_texname);
                if (it != texToId.end())
                    BVHBuilder::setTextureMaterial(mat, it->second, 0);
                else
                {
                    std::filesystem::path texPath = _path;
                    texPath.replace_filename(mtlMaterials[i].diffuse_texname);
                    u32 texId = _texManager.loadTexture((const char*)texPath.u8string().c_str());
                    TIM_ASSERT(texId != u32(-1));
                    BVHBuilder::setTextureMaterial(mat, texId, 0);
                    texToId[mtlMaterials[i].diffuse_texname] = texId;
                }

            }

            materials[size_t(i)] = mat;
        }

        return materials;
    }

    struct ObjData
    {
        std::vector<vec3> vertexData;
        std::vector<vec3> normalData;
        std::vector<vec2> texcoordData;
        std::vector<u32> indexData;
        int materialId = -1;
    };

    static ObjData loadObj(const tinyobj::attrib_t& attrib, const tinyobj::shape_t& curMesh, 
                           vec3 _pos, vec3 _scale, bool _swapYZ)
    {
        ObjData data;

        for (ubyte numV : curMesh.mesh.num_face_vertices)
            TIM_ASSERT(numV == 3);

        ska::flat_hash_map<ObjVertexKey, u32> vertexHasher;
        for (u32 i = 0; i < curMesh.mesh.indices.size(); ++i)
        {
            TIM_ASSERT(data.materialId == curMesh.mesh.material_ids[i / 3] || data.materialId == -1);
            data.materialId = curMesh.mesh.material_ids[i / 3];

            ObjVertexKey key{ (u32)curMesh.mesh.indices[i].vertex_index, (u32)curMesh.mesh.indices[i].normal_index, (u32)curMesh.mesh.indices[i].texcoord_index };
            auto it = vertexHasher.find(key);
            if (it == vertexHasher.end())
            {
                u32 index = (u32)data.vertexData.size();
                data.vertexData.push_back({ attrib.vertices[key.posIndex * 3], attrib.vertices[key.posIndex * 3 + 1], attrib.vertices[key.posIndex * 3 + 2] });
                transformVertex(data.vertexData.back(), _pos, _scale, _swapYZ);

                data.normalData.push_back({ attrib.normals[key.normalIndex * 3], attrib.normals[key.normalIndex * 3 + 1], attrib.normals[key.normalIndex * 3 + 2] });
                transformNormal(data.normalData.back(), _pos, _scale, _swapYZ);

                if (key.texcoordIndex < attrib.texcoords.size())
                    data.texcoordData.push_back({ attrib.texcoords[key.texcoordIndex * 2], attrib.texcoords[key.texcoordIndex * 2 + 1] });
                else
                    data.texcoordData.push_back({ 0,0 });

                vertexHasher[key] = index;
                data.indexData.push_back(index);
            }
            else
            {
                data.indexData.push_back(it->second);
            }
        }

        return data;
    }

    void Scene::addOBJInner(const fs::path& _path, vec3 _pos, vec3 _scale, BVHBuilder* _builder, const Material* _mat, bool _swapYZ)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> mtlMaterials;
        std::string warn;
        std::string err;

        std::filesystem::path mtlFolder = _path;
        mtlFolder.remove_filename();
        std::u8string mtlFolderStr{ mtlFolder.u8string() };
        mtlFolderStr.erase(mtlFolderStr.end() - 1);

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &mtlMaterials, &warn, &err, (const  char*)_path.u8string().c_str(), _mat ? nullptr : (const  char*)mtlFolderStr.c_str());
        if (ret)
        {
            std::cout << "Warning loading  " << _path << ": " << warn << std::endl;

            std::vector<Material> materials;
            if (!_mat)
                materials = loadMtl(m_texManager, _path, mtlMaterials);

            for (u32 objId = 0; objId < shapes.size(); ++objId)
            {
                ObjData data = loadObj(attrib, shapes[objId], _pos, _scale, _swapYZ);

                if (data.vertexData.size() < (1u << 16))
                {
                    u32 vertexOffset = m_geometryBuffer.addTriangleList((u32)data.vertexData.size(), &data.vertexData[0], &data.normalData[0], &data.texcoordData[0]);
                    TIM_ASSERT(data.indexData.size() % 3 == 0);
                    _builder->addTriangleList(vertexOffset, (u32)data.indexData.size() / 3, &data.indexData[0], _mat ? *_mat : materials[data.materialId]);
                }
                else
                {
                    std::cout << "Mesh " << shapes[objId].name << " has to many vertices (" << data.vertexData.size() << ")\n";
                }
            }
        }
        else
        {
            std::cout << "Error loading  " << _path << ": " << err << std::endl;
        }
    }

    void Scene::loadBlas(const fs::path& _path, vec3 _pos, vec3 _scale, std::vector<std::unique_ptr<BVHBuilder>>& _blas, const Material* _mat, bool _swapYZ)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> mtlMaterials;
        std::string warn;
        std::string err;

        std::filesystem::path mtlFolder = _path;
        mtlFolder.remove_filename();
        std::u8string mtlFolderStr{ mtlFolder.u8string() };
        mtlFolderStr.erase(mtlFolderStr.end() - 1);

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &mtlMaterials, &warn, &err, (const  char*)_path.u8string().c_str(), _mat ? nullptr : (const  char*)mtlFolderStr.c_str());
        if (ret)
        {
            std::cout << "Warning loading  " << _path << ": " << warn << std::endl;

            std::vector<Material> materials;
            if (!_mat)
                materials = loadMtl(m_texManager, _path, mtlMaterials);

            for (u32 objId = 0; objId < shapes.size(); ++objId)
            {
                ObjData data = loadObj(attrib, shapes[objId], _pos, _scale, _swapYZ);

                if (data.vertexData.size() < (1u << 16))
                {
                    Material blasMaterial = _mat ? *_mat : (data.materialId >= 0 ? materials[data.materialId] : BVHBuilder::createLambertianMaterial({ 0.9f, 0.9f, 0.9f }));
                    _blas.emplace_back() = std::make_unique<BVHBuilder>(shapes[objId].name, m_geometryBuffer, false);
                   
                    u32 vertexOffset = m_geometryBuffer.addTriangleList((u32)data.vertexData.size(), &data.vertexData[0], &data.normalData[0], &data.texcoordData[0]);
                    TIM_ASSERT(data.indexData.size() % 3 == 0);
                    _blas.back()->addTriangleList(vertexOffset, (u32)data.indexData.size() / 3, &data.indexData[0], blasMaterial);
                }
                else
                {
                    std::cout << "Mesh " << shapes[objId].name << " has to many vertices (" << data.vertexData.size() << ")\n";
                }
            }
        }
        else
        {
            std::cout << "Error loading  " << _path << ": " << err << std::endl;
        }
    }

    void Scene::build(BVHBuilder* _bvh, bool _useTlasBlas)
    {
#if 1
        const float DIMXY = 3.1f;
        const float DIMZ = 2;

        auto glassMat = BVHBuilder::createTransparentMaterial({ 0.8f,0.8f,0.8f }, 1.1f, 0);
        auto redGlassMat = BVHBuilder::createTransparentMaterial({ 1,0.5,0.5 }, 1.05f, 0.1f);

        Material suzanneMat = BVHBuilder::createPbrMaterial({ 1, 0.2f, 0.2f }, 1);
        Material roomMat = BVHBuilder::createLambertianMaterial({ 0.9f, 0.9f, 0.9f });
        Material redMat = BVHBuilder::createLambertianMaterial({ 0.9f, 0.2f, 0.2f });
        Material blueMat = BVHBuilder::createLambertianMaterial({ 0.2f, 0.2f, 0.9f });
        Material pbrMat = BVHBuilder::createPbrMaterial({ 0.9f, 0.9f, 0.9f }, 0);
        Material pbrMatMetal = BVHBuilder::createPbrMaterial({ 0.9f, 0.9f, 0.9f }, 1);

    #ifndef _DEBUG
        _bvh->addSphere({ { 0, 0, 4.1f }, 0.08f }, BVHBuilder::createEmissiveMaterial({ 1, 1, 1 }));
        _bvh->addSphereLight({ { 0, 0, 4.1f }, 30, { 2, 2, 2 }, 0.1f });

        _bvh->addSphere({ { -9.18164f , 3.32356f , 6.98306f }, 0.05f }, BVHBuilder::createEmissiveMaterial({ 1,0.2f,0.2f }));
        _bvh->addSphereLight({ { -9.18164f , 3.32356f , 6.98306f }, 16, { 3,0.5,0.5 }, 0.1f });

        _bvh->addSphere({ {  2.0f, 0, 1.3f }, 0.2f }, pbrMatMetal);
        _bvh->addSphere({ {  0.5f, 0, 1.3f }, 0.2f }, BVHBuilder::createTransparentMaterial({ 1,0.6f,0.6f }, 1.05f, 0.05f));
        _bvh->addSphere({ { -1.5f, 0, 1.3f }, 0.2f }, BVHBuilder::createPbrMaterial({ 1,0.6f,0.6f }, 0));
    #else
        _bvh->addSphereLight({ { 0, 0, 4.1f }, 30, { 2, 2, 2 }, 0.1f });
    #endif

        u32 texFlame = m_texManager.loadTexture("./data/image/flame.png");
        u32 texDot = m_texManager.loadTexture("./data/image/tex.png");

        if (!_useTlasBlas)
        {
        #ifndef _DEBUG
            BVHBuilder::setTextureMaterial(suzanneMat, texFlame, 0);
            addOBJ("./data/suzanne.obj", { -2.5f, 0, 1.3f }, vec3(1), _bvh, suzanneMat);

            BVHBuilder::setTextureMaterial(suzanneMat, texDot, 0);
            addOBJ("./data/suzanne.obj", { 3.f, 0, 1.3f }, vec3(1), _bvh, suzanneMat);

            addOBJWithMtl("./data/sponza.obj", {}, vec3(0.01f), _bvh, true);
        #else
            BVHBuilder::setTextureMaterial(suzanneMat, texFlame, 0);
            addOBJ("./data/suzanne.obj", { 0, 0, 0 }, vec3(1), _bvh, suzanneMat);
        #endif  
        }
        else
        {
            std::vector<std::unique_ptr<BVHBuilder>> blas;
            
    #ifndef _DEBUG
            loadBlas("./data/suzanne.obj", { -2.5f, 0, 1.3f }, vec3(1), blas, &suzanneMat);
            loadBlas("./data/suzanne.obj", { 3.f, 0, 1.3f }, vec3(1), blas, &suzanneMat);

            std::vector<std::unique_ptr<BVHBuilder>> sponzaBlas;
            loadBlas("./data/sponza.obj", {}, vec3(0.01f), sponzaBlas, nullptr, true);

            if (!sponzaBlas.empty())
            {
                for (auto it = sponzaBlas.begin() + 1; it != sponzaBlas.end(); ++it)
                    sponzaBlas[0]->mergeBlas(std::move(*it));
                
                blas.push_back(std::move(sponzaBlas[0]));
            }
    #else
            loadBlas("./data/suzanne.obj", { 0, 0, 0 }, vec3(1), blas, &suzanneMat);
    #endif
            for (auto& b : blas)
                _bvh->addBlas(std::move(b));
        } 
#endif
    }
}