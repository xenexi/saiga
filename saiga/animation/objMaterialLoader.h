#pragma once

#include <saiga/config.h>
#include <saiga/geometry/triangle_mesh.h>
#include <saiga/opengl/texture/texture.h>


struct SAIGA_GLOBAL ObjMaterial{
    std::string name;
    vec3 color = vec3(0,1,0);

    float Ns = 0.2f;   //specular coefficient
    float Ni = 1;
    float d = 1;    //transparency
    float Tr = 1;
    vec3 Tf;
    int illum ;
    vec3 Ka = vec3(0.2f);   //ambient color
    vec3 Kd = vec3(0.8f);   //diffuse color
    vec3 Ks = vec3(1);   //specular color
    vec3 Ke = vec3(1);

    Texture* map_Ka = nullptr;
    Texture* map_Kd = nullptr;
    Texture* map_Ks = nullptr;
    Texture* map_d = nullptr;
    Texture* map_bump = nullptr;


    Texture* diffuseTexture = nullptr;
    ObjMaterial(const std::string& name=""):name(name){}
};


class SAIGA_GLOBAL ObjMaterialLoader{
public:
    std::string file;
    bool verbose = false;

public:
    ObjMaterialLoader(){}
    ObjMaterialLoader(const std::string &file);




    bool loadFile(const std::string &file);

    ObjMaterial getMaterial(const std::string &name);
private:
    std::vector<ObjMaterial> materials;
    ObjMaterial* currentMaterial = nullptr;
    void parseLine(const std::string &line);

};

