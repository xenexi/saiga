/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once
#include "saiga/core/image/image.h"
#include "saiga/core/util/table.h"

#include "torch/torch.h"

#include <torch/script.h>

namespace Saiga
{
/**
 * Writes some information of the given tensor to std::cout.
 */
inline void PrintTensorInfo(at::Tensor t)
{
    if(!t.has_storage())
    {
        std::cout << "[undefined tensor]" << std::endl;
        return;
    }
    auto mi = t.min().item().toFloat();
    auto ma = t.max().item().toFloat();

    float mean = 0;
    if (t.dtype() == at::kFloat)
    {
        mean = t.mean().item().toFloat();
    }
    std::cout << "Tensor " << t.sizes() << " " << t.dtype() << " " << t.device() << " Min/Max " << mi << " " << ma
              << " Mean " << mean << std::endl;
}


inline void PrintModelParams(torch::nn::Module module)
{
    Table tab({40,25,10, 15});
    size_t sum= 0;

    tab << "Name" << "Params" << "Params" << "Sum";
    for (auto& t : module.named_parameters())
    {
        size_t local_sum =1;
        for(auto i : t.value().sizes())
        {
            local_sum*=i;
        }
        sum+=local_sum;
        std::stringstream strm;
        strm << t.value().sizes();
        tab << t.key() << strm.str()<< local_sum  << sum;

    }
    std::cout << std::endl;
}

/**
 * Convert an image view to a floating point tensor in the range [0,1].
 * Only uchar images are supported so far.
 * TODO: Add normalizations for other types
 */
template <typename T>
at::Tensor ImageViewToTensor(ImageView<T> img, bool normalize = true)
{
    using ScalarType = typename ImageTypeTemplate<T>::ChannelType;
    constexpr int c  = channels(ImageTypeTemplate<T>::type);


    auto type         = at::typeMetaToScalarType(caffe2::TypeMeta::Make<ScalarType>());
    at::Tensor tensor = torch::from_blob(img.data, {img.h, img.w, c}, type);

    // In pytorch image tensors are usually represented as channel first.
    tensor = tensor.permute({2, 0, 1});

    if (normalize)
    {
        // Convert to float
        if constexpr (!std::is_same<ScalarType, float>::value)
        {
            tensor = tensor.toType(at::kFloat);
        }

        // Normalize to [0,1]
        if constexpr (std::is_same<ScalarType, unsigned char>::value)
        {
            tensor = (1.f / 255.f) * tensor;
        }
    }

    return tensor;
}


/**
 * Convert a tensor to an image view of the given type
 */
template <typename T>
TemplatedImage<T> TensorToImage(at::Tensor tensor)
{
    using ScalarType = typename ImageTypeTemplate<T>::ChannelType;
    constexpr int c  = channels(ImageTypeTemplate<T>::type);
    auto type        = at::typeMetaToScalarType(caffe2::TypeMeta::Make<ScalarType>());

    if (tensor.dim() == 4)
    {
        tensor = tensor.squeeze();
    }

    // In pytorch image tensors are usually represented as channel first.
    tensor = tensor.permute({1, 2, 0});
    tensor = tensor.cpu();

    SAIGA_ASSERT(tensor.dtype() == at::kFloat);



    // Normalize to [0,1]
    if constexpr (std::is_same<ScalarType, unsigned char>::value)
    {
        tensor = 255.f * tensor;
        tensor = tensor.clamp(0, 255);
        tensor = tensor.toType(at::kByte);
    }

    int h = tensor.size(0);
    int w = tensor.size(1);

    ImageView<T> out_view(h, w, tensor.data_ptr<unsigned char>());

    TemplatedImage<T> img(h, w);
    out_view.copyTo(img.getImageView());

    return img;
}


/**
 * Save the tensor so it can be loaded from python and C++.
 * For Python loading just use
 *    x = torch.load(path)
 *
 * Python -> c++
 *   std::vector<char> f = get_the_bytes();
 *   torch::IValue x = torch::pickle_load(f);
 */
inline bool SaveTensor(at::Tensor t, const std::string& file)
{
    auto bytes = torch::jit::pickle_save(t);
    if (bytes.size() == 0)
    {
        return false;
    }
    std::ofstream fout(file, std::ios::out | std::ios::binary);
    if (!fout.is_open())
    {
        return false;
    }
    fout.write(bytes.data(), bytes.size());
    fout.close();
    return true;
}

/**
 * RGB image normalization of a 3 channel float-tensor using the Pytorch standart weights.
 */
inline at::Tensor NormalizeRGB(at::Tensor x)
{
    torch::data::transforms::Normalize<> color_normalize =
        torch::data::transforms::Normalize<>({0.485, 0.456, 0.406}, {0.229, 0.224, 0.225});
    return color_normalize(x);
}

/**
 * Inverse normalization to the function above.
 */
inline at::Tensor UnNormalizeRGB(at::Tensor x)
{
    torch::data::transforms::Normalize<> un_color_normalize1 =
        torch::data::transforms::Normalize<>({-0.485, -0.456, -0.406}, {1, 1, 1});
    torch::data::transforms::Normalize<> un_color_normalize2 =
        torch::data::transforms::Normalize<>({0, 0, 0}, {1.0 / 0.229, 1.0 / 0.224, 1.0 / 0.225});
    x = un_color_normalize2(x);
    x = un_color_normalize1(x);
    return x;
}

}  // namespace Saiga
