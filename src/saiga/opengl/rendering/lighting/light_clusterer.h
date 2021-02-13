/**
 * Copyright (c) 2020 Paul Himmler
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once
#include "saiga/core/camera/camera.h"
#include "saiga/core/geometry/aabb.h"
#include "saiga/core/geometry/intersection.h"
#include "saiga/core/time/timer.h"
#include "saiga/core/window/Interfaces.h"
#include "saiga/opengl/framebuffer.h"
#include "saiga/opengl/query/gpuTimer.h"
#include "saiga/opengl/rendering/lighting/renderer_lighting.h"
#include "saiga/opengl/shader/basic_shaders.h"
#include "saiga/opengl/shaderStorageBuffer.h"
#include "saiga/opengl/world/LineSoup.h"
#include "saiga/opengl/world/pointCloud.h"

namespace Saiga
{
struct clusterItem
{
    int plIdx = -1;
    int slIdx = -1;
    int blIdx = -1;
};
struct cluster
{
    int offset  = 0;
    int plCount = 0;
    int slCount = 0;
    int blCount = 0;
};

struct PointLightClusterData
{
    vec3 world_center;
    float radius;
};

struct SpotLightClusterData
{
    vec3 world_center;  // should be sufficient -> center position of the spot light cone
    float radius;       // should be sufficient -> bounding sphere instead of transformed cone
};

struct BoxLightClusterData
{
    vec3 world_center;
    float radius;  // should be sufficient -> bounding sphere instead of transformed box
};

#ifdef GPU_LIGHT_ASSIGNMENT
class SAIGA_OPENGL_API LightAssignmentComputeShader : public Shader
{
   public:
    GLint location_clusterDataBlockPointLights;  // pointLightClusterData array
    GLint location_clusterDataBlockSpotLights;   // spotLightClusterData array
    GLint location_clusterDataBlockBoxLights;    // boxLightClusterData array
    GLint location_clusterInfoBlock;             // number of lights in cluster arrays

    // Gets accessed based on pixel world space position (or screen space on 2D clustering)
    GLint location_clusterList;  // clusters
    /*
     * Looks like this: [offset, plCount, slCount, blCount], [offset, plCount, slCount, blCount] ...
     * So for each cluster we store an offset in the itemList and the number of specific lights that were assigned.
     */
    GLint location_itemList;  // itemList
    /*
     * Looks like this: [plIdx, slIdx, blIdx], [plIdx, slIdx, blIdx], ...
     * So each item consists of indices for all light types (can be -1, when not set).
     */

    virtual void checkUniforms() override{};
};

class SAIGA_OPENGL_API BuildClusterComputeShader : public Shader
{
   public:
    GLint location_viewFrustumData;  // width, height, depth, splitX, splitY, splitZ

    GLint location_clusterList;  // clusters

    virtual void checkUniforms() override{};
};
#endif

struct SAIGA_OPENGL_API ClustererParameters
{
    bool clusterThreeDimensional = false;
    bool useTimers               = true;

    void fromConfigFile(const std::string& file) {}
};

class SAIGA_OPENGL_API Clusterer
{
   public:
    Clusterer(ClustererParameters _params = ClustererParameters());
    Clusterer& operator=(Clusterer& c) = delete;
    ~Clusterer();

    void init(int width, int height, bool _useTimers);
    void resize(int width, int height);

    inline void enable3DClusters(bool enabled) { clusterThreeDimensional = enabled; }

    inline bool clusters3D() { return clusterThreeDimensional; }

    void loadComputeShaders();

    inline void clearLightData()
    {
        pointLightsClusterData.clear();
        spotLightsClusterData.clear();
        boxLightsClusterData.clear();
    }

    inline void addPointLight(PointLightClusterData& plc) { pointLightsClusterData.push_back(plc); }

    inline void addSpotLight(SpotLightClusterData& slc) { spotLightsClusterData.push_back(slc); }

    inline void addBoxLight(BoxLightClusterData& blc) { boxLightsClusterData.push_back(blc); }

    // Binds Cluster and Item ShaderStoragBuffers at the end.
    void clusterLights(Camera* cam, const ViewPort& viewPort);

    void setDebugShader(std::shared_ptr<Shader> shader){};

    void printTimings()
    {
        if (!useTimers) return;
        // For now:
        for (int i = 0; i < 2; ++i)
        {
            std::cout << "\t " << getTime(i) << "ms " << timerStrings[i] << std::endl;
        }
        std::cout << "\t " << lightAssignmentTimer.getTimeMS() << "ms "
                  << "CPU Light Assignment" << std::endl;
    };

    void renderImGui(bool* p_open = NULL);

    void renderDebug(Camera* cam)
    {
        if (!renderDebugEnabled) return;
        debugCluster.render(cam);
        debugPoints.render(cam);
    };

   public:
    std::vector<PointLightClusterData> pointLightsClusterData;

    std::vector<SpotLightClusterData> spotLightsClusterData;

    std::vector<BoxLightClusterData> boxLightsClusterData;

    std::vector<FilteredMultiFrameOpenGLTimer> gpuTimers;
    Timer lightAssignmentTimer;
    std::vector<std::string> timerStrings;
    void startTimer(int timer)
    {
        if (useTimers) gpuTimers[timer].startTimer();
    }
    void stopTimer(int timer)
    {
        if (useTimers) gpuTimers[timer].stopTimer();
    }
    float getTime(int timer)
    {
        if (!useTimers) return 0;
        return gpuTimers[timer].getTimeMS();
    }

   private:
    int width, height;

    int screenSpaceTileSize = 128;
    int depthSplits = 1;

    bool clusterThreeDimensional = false;
    bool useTimers;
    bool renderDebugEnabled = false;
    bool debugFrustumToView = false;
    LineSoup debugCluster;
    GLPointCloud debugPoints;

    bool clustersDirty = true;

    void build_clusters(Camera* cam);

    vec4 viewPosFromScreenPos(vec4 screen, const mat4& inverseProjection)
    {
        // to ndc
        vec2 ndc(clamp(screen.x() / width, 0.0, 1.0), clamp(screen.y() / height, 0.0, 1.0));

        // to clip
        vec4 clip(ndc.x() * 2.0f - 1.0f, ndc.y() * 2.0f - 1.0f, screen.z(), screen.w());

        // to view
        vec4 view(inverseProjection * clip);
        view /= view.w();

        return view;
    }

    // for eye = vec3(0) this is unoptimized.
    vec3 eyeZIntersection(vec3 eye, vec3 through, float z)
    {
        vec3 viewSpacePlaneOrientation(0.0f, 0.0f, 1.0f);

        vec3 line(through - eye);

        float t = (z - viewSpacePlaneOrientation.dot(eye)) / viewSpacePlaneOrientation.dot(line);

        return eye + t * line;
    }

    vec3 zeroZIntersection(vec3 through, float z) { return through * z / through.z(); }

    struct infoBuf_t
    {
        int clusterX;
        int clusterY;
        int screenSpaceTileSize;
        int screenWidth;
        int screenHeight;
        float zNear;
        float zFar;
        float bias;
        float scale;

        int clusterListCount;
        int itemListCount;
    } clusterInfoBuffer;

    struct clusterBuffer_t
    {
        std::vector<cluster> clusterList;
        /*
         * ClusterList
         * Gets accessed based on pixel world space position (or screen space on 2D clustering)
         * Looks like this: [offset, plCount, slCount, blCount, dlCount], [offset, plCount, slCount, blCount, dlCount]
         * ... So for each cluster we store an offset in the itemList and the number of specific lights that were
         * assigned.
         */
    } clusterBuffer;

    struct cluster_bounds
    {
        std::array<Plane, 6> planes;
    };

    std::vector<cluster_bounds> culling_cluster;

    struct itemBuffer_t
    {
        std::vector<clusterItem> itemList;
        /*
         * ItemList
         * Looks like this: [plIdx, slIdx, blIdx, dlIdx], [plIdx, slIdx, blIdx, dlIdx], ...
         * So each item consists of indices for all light types (can be -1, when not set).
         */
    } itemBuffer;

    ShaderStorageBuffer infoBuffer;
    ShaderStorageBuffer clusterListBuffer;
    ShaderStorageBuffer itemListBuffer;

#ifdef GPU_LIGHT_ASSIGNMENT
    std::shared_ptr<BuildClusterComputeShader> buildClusterShader2D;
    std::shared_ptr<BuildClusterComputeShader> buildClusterShader3D;
    std::shared_ptr<LightAssignmentComputeShader> buildLightToClusterMapShader;
#endif
};
}  // namespace Saiga