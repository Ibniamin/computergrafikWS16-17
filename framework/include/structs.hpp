#ifndef STRUCTS_HPP
#define STRUCTS_HPP

#include <map>
#include <glbinding/gl/gl.h>

#include "model_loader.hpp"

// use gl definitions from glbinding 
using namespace gl;

// gpu representation of model
struct model_object {
  // vertex array object
  GLuint vertex_AO = 0;
  // vertex buffer object
  GLuint vertex_BO = 0;
  // index buffer object
  GLuint element_BO = 0;
  // primitive type to draw
  GLenum draw_mode = GL_NONE;
  // indices number, if EBO exists
  GLsizei num_elements = 0;
};

//DODANE
struct star_object
{
    // vertex array object
    GLuint vertex_AO = 0;
    // vertex buffer object
    GLuint vertex_BO = 0;
};
//DODANE

// gpu representation of texture
struct texture_object {
  // handle of texture object
  GLuint handle = 0;
  // binding point
  GLenum target = GL_NONE;
};

struct planet
{
    //std::string name;
    model planet_model;
    model_object planet_object;
    std::string name;               //name just needed for recignition in upload_planet_transforms method
    float size;                     //scale factor for glm::scale function
    int speed;                      //value needed for rotation speed: the greater the value the slower the rotation around the Sun
    float distance;          //distance from origin
    
};

// shader handle and uniform storage
struct shader_program {
  shader_program(std::string const& vertex, std::string const& fragment)
   :vertex_path{vertex}
   ,fragment_path{fragment}
   ,handle{0}
   {}

  // path to shader source
  std::string vertex_path; 
  std::string fragment_path; 
  // object handle
  GLuint handle;
  // uniform locations mapped to name
  std::map<std::string, GLint> u_locs{};
};
#endif