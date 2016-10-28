#include "application_solar.hpp"
#include "launcher.hpp"

#include "utils.hpp"
#include "shader_loader.hpp"
#include "model_loader.hpp"

#include <glbinding/gl/gl.h>
// use gl definitions from glbinding 
using namespace gl;

//dont load gl bindings from glfw
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <random>

//we decided to declare these matrices here, because they are used in different fragments of the code and this solution seemed to be easier one than creating a special method for returning a value of it.
glm::fmat4 model_matrix{};
glm::fmat4 normal_matrix{};

int number_of_stars = 3000;
std::vector<GLfloat> stars{};

std::random_device rd;     // only used once to initialise (seed) engine

model mercury_model{};
model venus_model{};
model earth_model{};
model mars_model{};
model jupiter_model{};
model saturn_model{};
model uranus_model{};
model neptune_model{};
model sun_model{};
model moon_model{};
model star_model{};
//model star_model{stars, model::POSITION|model::NORMAL};

float ApplicationSolar::generate_random_numbers(float a, float b)
{
    std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
    std::uniform_real_distribution<float> uni(a,b); // guaranteed unbiased
    
    auto random_float = uni(rng);
    return random_float;
}


ApplicationSolar::ApplicationSolar(std::string const& resource_path)
 :Application{resource_path}
 ,planet_object{}
{
  int new_stars_size = number_of_stars * 4;
  stars.resize(new_stars_size);
    std::generate(stars.begin(), stars.end(),
    [&]
    {
        return generate_random_numbers(-100.0f, 100.0f);
    });
  star_model.data = stars;
  model::attrib_flag_t contained_attributes = model::POSITION|model::NORMAL;
  // number of components per vertex
  std::size_t component_num = 0;
    
  for (auto const& supported_attribute : model::VERTEX_ATTRIBS)
  {
      // check if buffer contains attribute
      if (supported_attribute.flag & contained_attributes)
      {
          // write offset, explicit cast to prevent narrowing warning
          star_model.offsets.insert(std::pair<model::attrib_flag_t, GLvoid*>{supported_attribute, (GLvoid*)uintptr_t(star_model.vertex_bytes)});
          // move offset pointer forward
          star_model.vertex_bytes += supported_attribute.size * supported_attribute.components;
          // increase number of components
          component_num += supported_attribute.components;
      }
  }
  // set number of vertice sin buffer
  star_model.vertex_num = star_model.data.size() / component_num;
  initializeGeometry();
  initializeShaderPrograms();
}

//cpu representations
model_object planet_o{};
model_object star{};

//please find declaration of struct "planet" in framework/include/structs.hpp
planet mercury_properties{mercury_model, planet_o, "Mercury",  0.3f, 0, 2.0f};
planet venus_properties{venus_model, planet_o, "Venus", 0.4f, 1, 6.0f};
planet earth_properties{earth_model, planet_o, "Earth", 0.5f, 2, 9.0f};
planet mars_properties{mars_model, planet_o, "Mars", 0.3f, 3, 14.0f};
planet jupiter_properties{jupiter_model, planet_o, "Jupiter", 1.6f, 4, 20.0f};
planet saturn_properties{saturn_model, planet_o, "Saturn", 1.2f, 5, 30.0f};
planet uranus_properties{uranus_model, planet_o, "Uranus", 0.8f, 6, 40.0f};
planet neptune_properties{neptune_model, planet_o, "Neptune", 0.6f, 7, 50.0f};
planet sun_properties{sun_model, planet_o, "Sun", 1.5f, 0, 0.0f};
//speed and distance of the Moon is equal to the speed and distance of the Earth
planet moon_properties{moon_model, planet_o, "Moon", 0.3f, 2, 9.0f};
//appropriate container to store the planets with their properties
planet properties[10] = {mercury_properties, venus_properties, earth_properties, mars_properties, jupiter_properties, saturn_properties, uranus_properties, neptune_properties, sun_properties, moon_properties};


void ApplicationSolar::upload_planet_transforms(planet const& model) const
{
    std::string planet_name = model.name;
    std::string moon = "Moon";
    std::string sun = "Sun";
    std::string earth = "Earth";
    glm::fmat4 model_matrix_earth{};
    
    //we need to consider two "special" cases - Sun and Moon, which we identify by their names (3rd value in the struct)
    if (planet_name.compare(sun) == 0)
    {
        //Sun is a "static" object, it doesn't change its position(that's why translation using (0, 0, 0) vector) and doesn't rotate
        model_matrix = glm::translate(model_matrix, glm::fvec3{0.0f, 0.0f, 0.0f});
    }
    else if (planet_name.compare(moon) == 0)
    {
        //Moon is spinning around Earth, that's why we have to rotate its model_matrix around Earth's model_matrix - we defined a variable for that, which is initial empty. But when we look at the container of planets, we see that Earth comes always first before Moon, so when we will be iterating the container as usual (for (int i=...)), model_matrix_earth will be always set before it comes to computing matrice for the Moon.
        model_matrix = glm::rotate(model_matrix_earth, float(glfwGetTime() + model.speed), glm::fvec3{0.0f, 1.0f, 0.0f});
        model_matrix = glm::translate(model_matrix, glm::fvec3{model.distance, 0.0f, 0.0f});
        model_matrix = glm::rotate(model_matrix, float(glfwGetTime()*4.0f), glm::fvec3{0.0f, 1.0f, 1.0f});
        //this translation is optional - it's only to make the Moon visible good enough
        model_matrix = glm::translate(model_matrix, glm::fvec3{3.0f, 0.0f, 0.0f});
    }
    else
    {
        //obviously, the planets are rotating around y-axis, that's why we put glm::fvec3{0.0f, 1.0f, 0.0f} as the last argument of the function. glfwGetTime() returns the value of the GLFW timer. Unless the timer has been set using glfwSetTime, the timer measures time elapsed since GLFW was initialized. And the greater the value of the second argument of the glm::rotate function, the slower the rotation.
        model_matrix = glm::rotate(model_matrix, float(glfwGetTime() + model.speed), glm::fvec3{0.0f, 1.0f, 0.0f});
        //we need to "move away" the planet from the Sun in the x-axis. The value for that is specified in the struct "planet".
        model_matrix = glm::translate(model_matrix, glm::fvec3{model.distance, 0.0f, -1.0f});
        if (planet_name.compare(earth) == 0)
        {
            model_matrix_earth = model_matrix;
        }
    }
    //scaling the matrix comes as the last operation for each object in properties[]
    model_matrix = glm::scale(model_matrix, glm::fvec3{model.size, model.size, model.size});
    //additionally, we also compute the normal_matrix in this function - extra matrix for normal transformation to keep the planets orthogonal to surface
    normal_matrix = glm::inverseTranspose(glm::inverse(m_view_transform) * model_matrix);
}

void ApplicationSolar::render() const
{
    //bind shader to upload uniforms
    glUseProgram(m_shaders.at("planet").handle);
    
    for (int i = 0; i<10; i++)
    {
        upload_planet_transforms(properties[i]);
        glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ModelMatrix"),
                       1, GL_FALSE, glm::value_ptr(model_matrix));
        glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("NormalMatrix"),
                       1, GL_FALSE, glm::value_ptr(normal_matrix));
        // bind the VAO to draw
        glBindVertexArray(properties[i].planet_object.vertex_AO);
    
        // draw bound vertex array using bound shader
        glDrawElements(properties[i].planet_object.draw_mode, properties[i].planet_object.num_elements, model::INDEX.type, NULL);
        //after drawing the element, the matrices must again be empty, otherwise we would generate further planets based on the previous calculations of these matrices.
        model_matrix = {};
        normal_matrix = {};
    }
    
    glUseProgram(m_shaders.at("star").handle);
    glBindVertexArray(star.vertex_AO);
    glDrawArrays(gl::GL_POINTS, 0, star_model.vertex_num);
}

void ApplicationSolar::updateView()
{
    // vertices are transformed in camera space, so camera transform must be inverted
    glm::fmat4 view_matrix = glm::inverse(m_view_transform);
    // upload matrix to gpu
    glUseProgram(m_shaders.at("planet").handle);
    glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ViewMatrix"),
                           1, GL_FALSE, glm::value_ptr(view_matrix));
    glUseProgram(m_shaders.at("star").handle);
    glUniformMatrix4fv(m_shaders.at("star").u_locs.at("ViewMatrix"),
                       1, GL_FALSE, glm::value_ptr(view_matrix));
}

void ApplicationSolar::updateProjection()
{
  // upload matrix to gpu
  glUseProgram(m_shaders.at("planet").handle);
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ProjectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(m_view_projection));
  glUseProgram(m_shaders.at("star").handle);
  glUniformMatrix4fv(m_shaders.at("star").u_locs.at("ProjectionMatrix"),
                       1, GL_FALSE, glm::value_ptr(m_view_projection));
}

// update uniform locations
void ApplicationSolar::uploadUniforms()
{
    uploadUniforms_p();
    uploadUniforms_s();
}

void ApplicationSolar::uploadUniforms_p()
{
    updateUniformLocations();
    
    // bind new shader
    glUseProgram(m_shaders.at("planet").handle);
    
    updateView();
    updateProjection();
}

void ApplicationSolar::uploadUniforms_s()
{
    updateUniformLocations();
    
    // bind new shader
    glUseProgram(m_shaders.at("star").handle);
    
    updateView();
    updateProjection();
}



// handle key input
// W,S - depth
// L,H - horizontal
// K,J - vertical
// UP/DOWN - rotate horizontal
// LEFT/RIGHT - rotate vertical
void ApplicationSolar::keyCallback(int key, int scancode, int action, int mods)
{
    //chosen key w and it is pressed
  if (key == GLFW_KEY_W && action == GLFW_PRESS)
  {
    //m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.0f, 0.0f, -0.1f});
      //changed the 3rd value of the glm::fvec3, so that we can see some reasonable change
    m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.0f, 0.0f, -1.0f});
  }
  else if (key == GLFW_KEY_S && action == GLFW_PRESS)
  {
    //m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.0f, 0.0f, 0.1f});
    m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.0f, 0.0f, 1.0f});
  }
  //arrow up
  else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
  {
      m_view_transform = glm::rotate(m_view_transform, 0.1f, glm::fvec3{1.0f, 0.0f, 0.0f});
  }
  //arrow down
  else if (key == GLFW_KEY_UP && action == GLFW_PRESS)
  {
      m_view_transform = glm::rotate(m_view_transform, -0.1f, glm::fvec3{1.0f, 0.0f, 0.0f});
  }
  else if (key == GLFW_KEY_L && action == GLFW_PRESS)
  {
      m_view_transform = glm::translate(m_view_transform, glm::fvec3{-0.1f, 0.0f, 0.0f});
  }
  else if (key == GLFW_KEY_H && action == GLFW_PRESS)
  {
      m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.1f, 0.0f, 0.0f});
  }
  else if (key == GLFW_KEY_K && action == GLFW_PRESS)
  {
      m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.0f, -0.1f, 0.0f});
  }
  else if (key == GLFW_KEY_J && action == GLFW_PRESS)
  {
      m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.0f, 0.1f, 0.0f});
  }
  else if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
  {
      m_view_transform = glm::rotate(m_view_transform, 0.1f, glm::fvec3{0.0f, 0.1f, 0.0f});
  }
  else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
  {
      m_view_transform = glm::rotate(m_view_transform, -0.1f, glm::fvec3{0.0f, 0.1f, 0.0f});
  }
  else
  {
      //do nothing
      return;
  }
  //update view matrix
  updateView();
}

void ApplicationSolar::mouseScrollCallback(double x, double y)
{
    //scrolling changes the depth
    m_view_transform = glm::translate(m_view_transform, glm::fvec3{0.0f, 0.0f, y});
    updateView();
}

// load shader programs
void ApplicationSolar::initializeShaderPrograms()
{
  // store shader program objects in container
  m_shaders.emplace("planet", shader_program{m_resource_path + "shaders/simple.vert",
                                           m_resource_path + "shaders/simple.frag"});
    m_shaders.emplace("star", shader_program{m_resource_path + "shaders/stars.vert",
        m_resource_path + "shaders/stars.frag"});
  // request uniform locations for shader program
  m_shaders.at("planet").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet").u_locs["ProjectionMatrix"] = -1;
  m_shaders.at("star").u_locs["ViewMatrix"] = -1;
  m_shaders.at("star").u_locs["ProjectionMatrix"] = -1;
}

// load models
void ApplicationSolar::initializeGeometry()
{
    for (int i=0; i<10; i++)
    {
        properties[i].planet_model = model_loader::obj(m_resource_path + "models/sphere.obj", model::NORMAL);
        
        // generate vertex array object
        glGenVertexArrays(1, &properties[i].planet_object.vertex_AO);
        // bind the array for attaching buffers
        glBindVertexArray(properties[i].planet_object.vertex_AO);
        
        // generate generic buffer
        glGenBuffers(1, &properties[i].planet_object.vertex_BO);
        // bind this as an vertex array buffer containing all attributes
        glBindBuffer(GL_ARRAY_BUFFER, properties[i].planet_object.vertex_BO);
        // configure currently bound array buffer
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * properties[i].planet_model.data.size(), properties[i].planet_model.data.data(), GL_STATIC_DRAW);
        
        // activate first attribute on gpu
        glEnableVertexAttribArray(0);
        // first attribute is 3 floats with no offset & stride
        glVertexAttribPointer(0, model::POSITION.components, model::POSITION.type, GL_FALSE, properties[i].planet_model.vertex_bytes, properties[i].planet_model.offsets[model::POSITION]);
        // activate second attribute on gpu
        glEnableVertexAttribArray(1);
        // second attribute is 3 floats with no offset & stride
        glVertexAttribPointer(1, model::NORMAL.components, model::NORMAL.type, GL_FALSE, properties[i].planet_model.vertex_bytes, properties[i].planet_model.offsets[model::NORMAL]);
        
        // generate generic buffer
        glGenBuffers(1, &properties[i].planet_object.element_BO);
        // bind this as an vertex array buffer containing all attributes
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, properties[i].planet_object.element_BO);
        // configure currently bound array buffer
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, model::INDEX.size * properties[i].planet_model.indices.size(), properties[i].planet_model.indices.data(), GL_STATIC_DRAW);
        
        // store type of primitive to draw
        properties[i].planet_object.draw_mode = GL_TRIANGLES;
        // transfer number of indices to model object
        properties[i].planet_object.num_elements = GLsizei(properties[i].planet_model.indices.size());
    }
    
    // generate vertex array object
    glGenVertexArrays(1, &star.vertex_AO);
    // bind the array for attaching buffers
    glBindVertexArray(star.vertex_AO);
    
    // generate generic buffer
    glGenBuffers(1, &star.vertex_BO);
    // bind this as an vertex array buffer containing all attributes
    glBindBuffer(GL_ARRAY_BUFFER, star.vertex_BO);
    // configure currently bound array buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * star_model.data.size(), star_model.data.data(), GL_STATIC_DRAW);
    
    // activate first attribute on gpu
    glEnableVertexAttribArray(0);
    // first attribute is 3 floats with no offset & stride
    glVertexAttribPointer(0, model::POSITION.components, model::POSITION.type, GL_FALSE, star_model.vertex_bytes, star_model.offsets[model::POSITION]);
    // activate second attribute on gpu
    glEnableVertexAttribArray(1);
    // second attribute is 3 floats with no offset & stride
    glVertexAttribPointer(1, model::NORMAL.components, model::NORMAL.type, GL_FALSE, star_model.vertex_bytes, star_model.offsets[model::NORMAL]);
}

ApplicationSolar::~ApplicationSolar()
{
    for (int i=0; i<10; i++)
    {
        glDeleteBuffers(1, &properties[i].planet_object.vertex_BO);
        glDeleteBuffers(1, &properties[i].planet_object.element_BO);
        glDeleteVertexArrays(1, &properties[i].planet_object.vertex_AO);
    }
}

// exe entry point
int main(int argc, char* argv[])
{
  Launcher::run<ApplicationSolar>(argc, argv);
  //Here, the lines which would come after "run" would not be executed because it takes the ESC to finish running the application. We tried to implement it in a suggested way as it was in the lab, but at the end of the day we came up to another solution.
    
  //ApplicationSolar app = ApplicationSolar(argv[1]);
  //for (int i=0; i<10; i++)
  //{
      //app.upload_planet_transforms(properties[i]);
      //app.render();
      //model_matrix = {};
      //normal_matrix = {};
  //}
}