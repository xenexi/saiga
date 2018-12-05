/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "MotionModel.h"

#include "saiga/util/ini/ini.h"

namespace Saiga
{
void MotionModel::Parameters::fromConfigFile(const std::string& file)
{
    Saiga::SimpleIni ini;
    ini.LoadFile(file.c_str());

    smoothness = ini.GetAddLong("MotionModel", "smoothness", smoothness);
    alpha      = ini.GetAddDouble("MotionModel", "alpha", alpha);
    damping    = ini.GetAddDouble("MotionModel", "damping", damping);
    fps        = ini.GetAddDouble("MotionModel", "fps", fps);

    if (ini.changed()) ini.SaveFile(file.c_str());
}

MotionModel::MotionModel(const MotionModel::Parameters& params) : params(params)
{
    size_t N = 10000;
    data.reserve(N);
    indices.reserve(N);
}

void MotionModel::addRelativeMotion(const SE3& T, size_t frameId, double weight)
{
    std::unique_lock<std::mutex> lock(mut);
    size_t id;
    id = data.size();
    if (id == 0)
    {
        averageWeight = weight;
    }
    else
    {
        double a      = 0.2;
        averageWeight = (1 - a) * averageWeight + a * weight;
    }
    data.push_back({T, weight});
    if (indices.size() < frameId + 1)
    {
        indices.resize(frameId + 1, std::numeric_limits<size_t>::max());
    }
    indices[frameId] = id;
    validVelocity    = false;
}

void MotionModel::updateRelativeMotion(const SE3& T, size_t frameId)
{
    std::unique_lock<std::mutex> lock(mut);
    auto id = indices[frameId];
    if (id != std::numeric_limits<size_t>::max()) data[id].v = T;
    validVelocity = false;
}

void MotionModel::addInvalidMotion(size_t frameId)
{
    addRelativeMotion(SE3(), frameId, averageWeight * 0.5);
}

SE3 MotionModel::getFrameVelocity()
{
    std::unique_lock<std::mutex> lock(mut);
    if (!validVelocity) recomputeVelocity();
    return currentVelocity;
}

SE3 MotionModel::getRealVelocity()
{
    double factor = params.fps;
    return scale(getFrameVelocity(), factor);
}

void MotionModel::renderVelocityGraph()
{
    SE3 v = getRealVelocity();

    Eigen::AngleAxisd aa(v.unit_quaternion());
    Vec3 t = v.translation();

    double vt = t.norm();
    double va = aa.angle();

    grapht.addValue(vt);
    grapht.renderImGui();

    grapha.addValue(va);
    grapha.renderImGui();
}

void MotionModel::recomputeVelocity()
{
    currentVelocity = computeVelocity();
    validVelocity   = true;
}

SE3 MotionModel::computeVelocity()
{
    if (data.empty() || params.smoothness == 0) return SE3();

    // Number of values to consider
    int s = std::min((int)data.size(), params.smoothness);


    weights.resize(s);
    double weightSum = 0;
    for (auto i = 0; i < s; ++i)
    {
        auto dataId = data.size() - s + i;

        double disToCurrent = s - (i + 1);
        auto w              = data[dataId].weight * std::pow(params.alpha, disToCurrent);
        weights[i]          = w;
        weightSum += w;
    }

    if (weightSum == 0) return SE3();

    // normalize weights
    for (auto i = 0; i < s; ++i)
    {
        weights[i] /= weightSum;
    }

    SE3 result           = data[data.size() - s].v;
    double currentWeight = weights[0];

    for (auto i = 1; i < s; ++i)
    {
        double nextWeight = weights[i];
        auto dataId       = data.size() - s + i;

        double slerpAlpha = nextWeight / (currentWeight + nextWeight);

        result = slerp(result, data[dataId].v, slerpAlpha);
        currentWeight += nextWeight;
    }
    return scale(result, params.damping);
}


}  // namespace Saiga
