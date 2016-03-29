#pragma once

#include "saiga/opengl/opengl.h"
#include "saiga/util/glm.h"
#include "saiga/opengl/shader/shaderpart.h"
#include "saiga/util/fileChecker.h"

#include <vector>
#include <memory> //for shared pointer


class Shader;

class SAIGA_GLOBAL ShaderPartLoader{
public:


    typedef std::vector<ShaderCodeInjection> ShaderCodeInjections;

    std::string file;
    ShaderCodeInjections injections;

    std::vector<std::shared_ptr<ShaderPart>> shaders;


    ShaderPartLoader();
    ShaderPartLoader(const std::string &file, const ShaderCodeInjections &injections);
    ~ShaderPartLoader();

    bool load();
    bool loadAndPreproccess(const std::string &file, std::vector<std::string> &ret);

    void addShader(std::vector<std::string> &content, GLenum type);

    //combine all loaded shader parts to a shader. the returned shader is linked and ready to use
    template<typename shader_t> shader_t* createShader();

    //like create shader, but the passed shader is updated instead of creating a new one
    void reloadShader(Shader* shader);
};


template<typename shader_t>
shader_t* ShaderPartLoader::createShader()
{
    if(shaders.size()==0){
        std::cerr<<file<<" does not contain any shaders."<<endl;
        std::cerr<<"Use for example '##GL_FRAGMENT_SHADER' to mark the beginning of a fragment shader."<<endl;
        std::cerr<<"Also make sure this makro is at a beginning of a new line."<<endl;
        return nullptr;
    }

    shader_t* shader = new shader_t();
    shader->shaders = shaders;
    shader->createProgram();

    std::cout<<"Loaded: "<<file<<" ( ";
    for(auto& sp : shaders){
        std::cout<<sp->type<<" ";
    }
    std::cout<<")"<<std::endl;


    return shader;
}

extern SAIGA_GLOBAL FileChecker shaderPathes;


