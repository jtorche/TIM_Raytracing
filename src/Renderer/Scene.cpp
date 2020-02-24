#include "Scene.h"
#include "timCore/Common.h"
#include "OBJLoader.h"
#include "BVHBuilder.h"
#include "BVHGeometry.h"

namespace tim
{
    Scene::Scene(BVHGeometry& _geometryBuffer) : m_geometryBuffer{ _geometryBuffer }
    {

    }

    void Scene::addOBJ(const char* _path, vec3 _pos, BVHBuilder* _bvh)
    {
        objl::Loader loader;
        if (loader.LoadFile(_path))
        {
            for (u32 objId = 0; objId < loader.LoadedMeshes.size(); ++objId)
            {
                if (objId > 300)
                    break;

                const objl::Mesh& curMesh = loader.LoadedMeshes[objId];
                if (curMesh.Vertices.size() < (1u << 16))
                {
                    std::vector<vec3> posData(curMesh.Vertices.size());
                    std::vector<vec3> normalData(curMesh.Vertices.size());
                    for (u32 i = 0; i < curMesh.Vertices.size(); ++i)
                    {
                        posData[i] = { curMesh.Vertices[i].Position.X, curMesh.Vertices[i].Position.Y, curMesh.Vertices[i].Position.Z };
                        posData[i] += _pos;
                        normalData[i] = { curMesh.Vertices[i].Normal.X, curMesh.Vertices[i].Normal.Y, curMesh.Vertices[i].Normal.Z };
                    }

                    u32 vertexOffset = m_geometryBuffer.addTriangleList((u32)curMesh.Vertices.size(), &posData[0], &normalData[0]);
                    _bvh->addTriangleList(vertexOffset, (u32)curMesh.Indices.size() / 3, &curMesh.Indices[0]);
                }
                else
                {
                    std::cout << "Mesh " << curMesh.MeshName << " has to many vertices (" << curMesh.Vertices.size() <<")\n";
                }
            }
        }
    }

    void Scene::build(BVHBuilder* _bvh)
    {
#if 0
        const float DIMXY = 5.1f;
        const float DIMZ = 5;

        auto groundMirror = BVHBuilder::createMirrorMaterial({ 0,1,1 }, 0.3f);
        auto ballMirror = BVHBuilder::createMirrorMaterial({ 0,1,1 }, 1);
        auto glassMat = BVHBuilder::createTransparentMaterial({ 0.8f,0.8f,0.8f }, 1.1f, 0);
        auto redGlassMat = BVHBuilder::createTransparentMaterial({ 1,0,0 }, 1.1f, 0);

        m_bvh->addBox(Box{ { -DIMXY, -DIMXY, DIMZ }, {  DIMXY,          DIMXY,           DIMZ + 0.1f } });
        m_bvh->addBox(Box{ { -DIMXY, -DIMXY, 0    }, {  DIMXY,          DIMXY,           0.1f } });

        m_bvh->addBox(Box{ { -DIMXY, -DIMXY, 0    }, { -DIMXY + 0.1f,   DIMXY,           DIMZ + 0.1f } });
        m_bvh->addBox(Box{ {  DIMXY, -DIMXY, 0    }, {  DIMXY + 0.1f,   DIMXY,           DIMZ + 0.1f } });

        m_bvh->addBox(Box{ { -DIMXY, -DIMXY, 0    }, {  DIMXY,         -DIMXY + 0.1f,    DIMZ + 0.1f } });
        // m_bvh->addBox(Box{ { -DIMXY,  DIMXY - DIMXY * 0.5f, 0    }, {  DIMXY,          DIMXY + 0.1f - DIMXY * 0.5f,    DIMZ + 0.1f } }, redGlassMat);

        const float pillarSize = 0.03f;
        const float sphereRad = 0.07f;
        for (u32 i = 0; i < 10; ++i)
        {
            for (u32 j = 0; j < 10; ++j)
            {
                vec3 p = { -DIMXY + i * (DIMXY / 5), -DIMXY + j * (DIMXY / 5), 0 };
                m_bvh->addSphere({ p + vec3(0,0,1), sphereRad }, ballMirror);
                //m_bvh->addSphere({ p + vec3(0,0,1), sphereRad });
                m_bvh->addBox(Box{ p - vec3(pillarSize, pillarSize, 0), p + vec3(pillarSize, pillarSize, 1) }, (i + j) % 2 == 0 ? glassMat : redGlassMat);
            }
        }

        m_bvh->addSphere({ { -2, -2, 4 }, 0.19f }, BVHBuilder::createEmissiveMaterial({ 1, 0.5, 1 }));
        m_bvh->addSphereLight({ { -2, -2, 4 }, 25, { 2, 1, 2 }, 0.2f });

        m_bvh->addSphere({ { 2, 2, 4 }, 0.19f }, BVHBuilder::createEmissiveMaterial({ 1, 1, 0.5 }));
        m_bvh->addSphereLight({ { 2, 2, 4 }, 15, { 2, 2, 1 }, 0.2f });

        m_bvh->addSphere({ { 0, 0, 2 }, 0.5 }, BVHBuilder::createTransparentMaterial({ 1,0.6f,0.6f }, 1.05f, 0.05f));
#endif

        _bvh->addSphereLight({ { 2, 2, 3 }, 20, { 2, 1, 2 }, 0.2f });
        _bvh->addSphereLight({ { -2, -2, 3 }, 20, { 2, 2, 2 }, 0.2f });
        _bvh->addBox(Box{ { -10, -10, -1    }, {  10, 10, -0.5 } });
        addOBJ("../data/sponza.obj", {}, _bvh);

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