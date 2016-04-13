#include "saiga/rendering/overlay/Layout.h"


Layout::Layout(int width, int height, float targetWidth, float targetHeight):width(width),height(height),targetWidth(targetWidth),targetHeight(targetHeight) {
    aspect = (float)width/(float)height;
    this->targetWidth = aspect;
    this->targetHeight = 1.0f;
    proj = glm::ortho(0.0f,this->targetWidth,0.0f,this->targetHeight,-1.0f,1.0f);
    scale = vec3(aspect,1.0f,1.0f);
}

aabb Layout::transform(Object3D *obj, const aabb &box, vec2 relPos, float relSize, Alignment alignmentX, Alignment alignmentY, bool scaleX)
{
    //scale to correct size
    if(scaleX){
        vec3 s = box.max-box.min;
        float ds = relSize/s.y;
        obj->scale = vec3(ds);
        obj->scale.x *= 1.0f/aspect;
    }else{
        vec3 s = box.max-box.min;
        float ds = relSize/s.x;
        obj->scale = vec3(ds);
        obj->scale.y *= 1.0f/aspect;
    }

    aabb resultBB = aabb(box.min*obj->scale,box.max*obj->scale);


    //alignment
    vec3 center = box.getPosition()*obj->scale;
    vec3 bbmin = box.min*obj->scale;
    vec3 bbmax = box.max*obj->scale;


    vec3 alignmentOffset(0);

    switch(alignmentX){
    case LEFT:
        alignmentOffset.x += bbmin.x;
        break;
    case RIGHT:
        alignmentOffset.x += bbmax.x;
        break;
    case CENTER:
        alignmentOffset.x += center.x;
        break;
    }

    switch(alignmentY){
    case LEFT:
        alignmentOffset.y += bbmin.y;
        break;
    case RIGHT:
        alignmentOffset.y += bbmax.y;
        break;
    case CENTER:
        alignmentOffset.y += center.y;
        break;
    }

    obj->position = vec3(relPos,0)-alignmentOffset;
    resultBB.scale(scale);
    resultBB.setPosition(obj->position);

    obj->scale *= scale;
    obj->position  *= scale;

    obj->calculateModel();

    //    cout<<width<<" "<<height<<endl;
    //    cout<<"LAYOUT "<<aspect<<" "<<obj->scale<<" "<<obj->position<<" "<<box<<endl;
//    resultBB.translate(vec3(relPos,0)*scale-alignmentOffset);
    return resultBB;
}

aabb Layout::transformNonUniform(Object3D *obj, const aabb &box, vec2 relPos, vec2 relSize, Layout::Alignment alignmentX, Layout::Alignment alignmentY)
{

    vec3 s = box.max-box.min;
    obj->scale = vec3(relSize.x,relSize.y,1.0f) / vec3(s.x,s.y,1.0f);


    //alignment
    vec3 center = box.getPosition()*obj->scale;
    vec3 bbmin = box.min*obj->scale;
    vec3 bbmax = box.max*obj->scale;

    vec3 alignmentOffset(0);

    switch(alignmentX){
    case LEFT:
        alignmentOffset.x += bbmin.x;
        break;
    case RIGHT:
        alignmentOffset.x += bbmax.x;
        break;
    case CENTER:
        alignmentOffset.x += center.x;
        break;
    }

    switch(alignmentY){
    case LEFT:
        alignmentOffset.y += bbmin.y;
        break;
    case RIGHT:
        alignmentOffset.y += bbmax.y;
        break;
    case CENTER:
        alignmentOffset.y += center.y;
        break;
    }



    //move
    obj->position = vec3(relPos,0)-alignmentOffset;
    obj->scale *= scale;
    obj->position  *= scale;
    obj->calculateModel();



    aabb resultBB = box;
    resultBB.min = vec3(obj->model * vec4(box.min,1));
    resultBB.max = vec3(obj->model * vec4(box.max,1));

    return resultBB;
}

glm::vec2 Layout::transformToLocal(glm::vec2 p)
{
    return vec2(p.x/width*targetWidth,p.y/height*targetHeight);
}



