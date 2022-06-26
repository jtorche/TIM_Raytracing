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

    void Scene::addOBJ(const char* _path, vec3 _pos, vec3 _scale, BVHBuilder* _bvh, const Material& _mat)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, _path);
        if (ret)
        {
            std::cout << "Warning loading  " << _path << ": " << warn << std::endl;
            for (u32 objId = 0; objId < shapes.size(); ++objId)
            {
                std::vector<vec3> vertexData;
                std::vector<vec3> normalData;
                std::vector<vec2> texcoordData;
                std::vector<u32> indexData;
                ska::flat_hash_map<ObjVertexKey, u32> vertexHasher;

                const tinyobj::shape_t& curMesh = shapes[objId];
                for (ubyte numV : curMesh.mesh.num_face_vertices)
                    TIM_ASSERT(numV == 3);
                
                for (u32 i = 0; i < curMesh.mesh.indices.size(); ++i)
                {
                    ObjVertexKey key{ (u32)curMesh.mesh.indices[i].vertex_index, (u32)curMesh.mesh.indices[i].normal_index, (u32)curMesh.mesh.indices[i].texcoord_index };
                    auto it = vertexHasher.find(key);
                    if (it == vertexHasher.end())
                    {
                        u32 index = (u32)vertexData.size();
                        vertexData.push_back({ attrib.vertices[key.posIndex * 3], attrib.vertices[key.posIndex * 3 + 1], attrib.vertices[key.posIndex * 3 + 2] });
                        vertexData.back() += _pos;
                        normalData.push_back({ attrib.normals[key.normalIndex * 3], attrib.normals[key.normalIndex * 3 + 1], attrib.normals[key.normalIndex * 3 + 2] });
                        
                        if (key.texcoordIndex < attrib.texcoords.size())
                            texcoordData.push_back({ attrib.texcoords[key.texcoordIndex * 2], attrib.texcoords[key.texcoordIndex * 2 + 1] });
                        else
                            texcoordData.push_back({ 0,0 });

                        vertexHasher[key] = index;
                        indexData.push_back(index);
                    }
                    else
                    {
                        indexData.push_back(it->second);
                    }
                }

                if (vertexData.size() < (1u << 16))
                {
                    u32 vertexOffset = m_geometryBuffer.addTriangleList((u32)vertexData.size(), &vertexData[0], &normalData[0], &texcoordData[0]);
                    TIM_ASSERT(indexData.size() % 3 == 0);
                    _bvh->addTriangleList(vertexOffset, (u32)indexData.size() / 3, &indexData[0], _mat);
                }
                else
                {
                    std::cout << "Mesh " << curMesh.name << " has to many vertices (" << vertexData.size() << ")\n";
                }
            }
        }
        else
        {
            std::cout << "Error loading  " << _path << ": " << err << std::endl;
        }
    }

    void Scene::build(BVHBuilder* _bvh)
    {
#if 1
        const float DIMXY = 3.1f;
        const float DIMZ = 2;

        auto groundMirror = BVHBuilder::createMirrorMaterial({ 0,1,1 }, 0.3f);
        auto ballMirror = BVHBuilder::createMirrorMaterial({ 0,1,1 }, 1);
        auto glassMat = BVHBuilder::createTransparentMaterial({ 0.8f,0.8f,0.8f }, 1.1f, 0);
        auto redGlassMat = BVHBuilder::createTransparentMaterial({ 1,0,0 }, 1.1f, 0);

        _bvh->addBox(Box{ { -DIMXY, -DIMXY, DIMZ }, {  DIMXY,          DIMXY,           DIMZ + 0.1f } });
        _bvh->addBox(Box{ { -DIMXY, -DIMXY, 0    }, {  DIMXY,          DIMXY,           0.1f } });
        
        _bvh->addBox(Box{ { -DIMXY, -DIMXY, 0    }, { -DIMXY + 0.1f,   DIMXY,           DIMZ + 0.1f } });
        _bvh->addBox(Box{ {  DIMXY, -DIMXY, 0    }, {  DIMXY + 0.1f,   DIMXY,           DIMZ + 0.1f } });
        
        _bvh->addBox(Box{ { -DIMXY, -DIMXY, 0    }, {  DIMXY,         -DIMXY + 0.1f,    DIMZ + 0.1f } });
         //m_bvh->addBox(Box{ { -DIMXY,  DIMXY - DIMXY * 0.5f, 0    }, {  DIMXY,          DIMXY + 0.1f - DIMXY * 0.5f,    DIMZ + 0.1f } }, redGlassMat);

        const float pillarSize = 0.03f;
        const float pillarHeight = 0.8f;
        const float sphereRad = 0.1f;
        for (u32 i = 0; i < 10; ++i)
        {
            for (u32 j = 0; j < 8; ++j)
            {
                vec3 p = { -DIMXY + i * (DIMXY / 4), -DIMXY + j * (DIMXY / 4), 0 };
                _bvh->addSphere({ p + vec3(0,0,pillarHeight), sphereRad }, ballMirror);
                //m_bvh->addSphere({ p + vec3(0,0,1), sphereRad });
                _bvh->addBox(Box{ p - vec3(pillarSize, pillarSize, 0), p + vec3(pillarSize, pillarSize, pillarHeight) }, (i + j) % 2 == 0 ? glassMat : redGlassMat);
            }
        }

        _bvh->addSphere({ { -1.5f, -1.5f, 1.5f }, 0.08f }, BVHBuilder::createEmissiveMaterial({ 1, 0.5, 1 }));
        _bvh->addSphereLight({ { -1.5f, -1.5f, 1.5f }, 15, { 2, 1, 2 }, 0.1f });

        _bvh->addSphere({ { 1, 1, 1.5f }, 0.08f }, BVHBuilder::createEmissiveMaterial({ 1, 1, 0.5 }));
        _bvh->addSphereLight({ { 1, 1, 1.5 }, 8, { 2, 2, 1 }, 0.1f });

        _bvh->addSphere({ { 0, 0, 1.3f }, 0.2f }, BVHBuilder::createTransparentMaterial({ 1,0.6f,0.6f }, 1.05f, 0.05f));
        _bvh->addSphere({ { -1.5f, 0, 1.3f }, 0.2f }, BVHBuilder::createPbrMaterial({ 1,0.6f,0.6f }));

        Material suzanneMat = BVHBuilder::createLambertianMaterial({ 0.9f, 0.9f, 0.9f, });
        u32 texId = m_texManager.loadTexture("./data/image/tex.png");
        BVHBuilder::setTextureMaterial(suzanneMat, texId, 0);

        addOBJ("./data/suzanne.obj", { 1,1.7f,1 }, vec3(1), _bvh, suzanneMat);
        addOBJ("./data/longboard/longboard.obj", { 1,1.7f,4 }, vec3(1), _bvh, suzanneMat);
        addOBJ("./data/mesh2.obj", { -1,-1.7f,1.3f }, vec3(0.6), _bvh, BVHBuilder::createPbrMaterial({ 0.9f, 0.9f, 0.9f, }));
#endif

        // _bvh->addSphereLight({ { 2, 2, 1 }, 20, { 2, 1, 2 }, 0.2f });
        // _bvh->addSphereLight({ { -2, -2, 3 }, 20, { 2, 2, 2 }, 0.2f });
        // _bvh->addBox(Box{ { -10, -10, -1 }, {  10, 10, -0.5 } });
        // addOBJ("../data/sponza.obj", {}, _bvh);

        //m_bvh->addSphere({ { 2, 2, 2 }, 2 }, BVHBuilder::createEmissiveMaterial({ 1, 1, 0 }));

        //m_bvh->addPointLight({ { -3, -2, 4 }, 30, { 0, 1, 1 } });

        //AreaLight areaLight;
        //areaLight.pos = { 0, 0, 3 };
        //areaLight.width = { -0.3, 0, 0 };
        //areaLight.height = { 0, 0.3, 0 };
        //areaLight.color = { 1,1,1 };
        //areaLight.attenuationRadius = 40;
        //m_bvh->addAreaLight(areaLight);
    }
}