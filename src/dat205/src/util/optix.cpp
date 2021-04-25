#include "util/optix.hpp"

#include <cstring>
#include <iostream>
#include <string>

using namespace optix;

std::string ptxPath(std::string const& cuda_file) {
  // This is how compiled CUDA shaders will be stored.
  return std::string(sutil::samplesPTXDir()) + std::string("/") + "dat205" + std::string("_generated_") + cuda_file + std::string(".ptx");
}

void run_unsafe_optix_code(std::function<void()> f) {
  #if USE_DEBUG_EXCEPTIONS
  try {
    f();
  } catch (Exception& e) {
    std::cerr << e.getErrorString() << std::endl;
  }
  #else
  f();
  #endif
}

// Configures vertex and index buffer data for the given accelartion structure.
void set_acceleration_properties(Acceleration acceleration) {

  // vertex_buffer_name specifies the name of the vertex buffer variable for underlying geometry, containing float3 vertices.
  // vertex_buffer_stride is used to define the offset between two vertices in the buffer, given in bytes.
  // The default stride is zero, which assumes that the vertices are tightly packed.
  acceleration->setProperty("vertex_buffer_name", "vertex_buffer");
  acceleration->setProperty("vertex_buffer_stride", std::to_string(sizeof(VertexData)));

  // index_buffer_name specifies the name of the index buffer variable for underlying geometry, if any
  // index_buffer_stride can be used analog to vertex_buffer_stride to describe interleaved arrays.
  acceleration->setProperty("index_buffer_name", "index_buffer");
  acceleration->setProperty("index_buffer_stride", std::to_string(sizeof(uint3)));
}

void unregister_buffer(Buffer buffer, std::function<void()> f) {
  run_unsafe_optix_code([&]() {
    buffer->unregisterGLBuffer();
    f();
    buffer->registerGLBuffer();
  });
}

OptixScene::OptixScene(Context& ctx) : m_ctx(ctx) {
  run_unsafe_optix_code([&]() {
    // These shaders can be used for any triangle-based geometry.
    m_boundingbox_triangle_indexed  = m_ctx->createProgramFromPTXFile(ptxPath("boundingbox_triangle_indexed.cu"),  "boundingbox_triangle_indexed");
    m_intersection_triangle_indexed = m_ctx->createProgramFromPTXFile(ptxPath("intersection_triangle_indexed.cu"), "intersection_triangle_indexed");
  });
}

optix::Context& OptixScene::context() {
  return m_ctx;
}

Geometry OptixScene::create_cuboid(float width, float height, float depth) {
  assert(0 < width && 0 < height && 0 < depth);

  std::vector<VertexData> vertices;

  // Creates a cuboid face (i.e. quad).
  //
  // The corner is assumed to be the bot-left corner of the face.
  auto add_face = [&vertices](float3 corner, float3 right, float3 up) {
    VertexData v;
    v.normal = cross(right, up);
    v.tangent = right;

    // Bottom-left Corner
    v.position = corner;
    vertices.push_back(v);

    // Bottom-right Corner
    v.position = corner + right;
    vertices.push_back(v);

    // Top-left Corner
    v.position = corner + up;
    vertices.push_back(v);

    // Top-right Corner
    v.position = corner + right + up;
    vertices.push_back(v);
  };

  // Left Face
  float3 corner = make_float3(-width / 2.0f, -height / 2.0f, -depth / 2.0f);
  float3 right  = make_float3(0.0f, 0.0f, depth);
  float3 up     = make_float3(0.0f, height, 0.0f);
  add_face(corner, right, up);

  // Right Face
  corner = make_float3(width / 2.0f, -height / 2.0f, depth / 2.0f);
  right  = make_float3(0.0f, 0.0f, -depth);
  up     = make_float3(0.0f, height, 0.0f);
  add_face(corner, right, up);

  // Bottom Face
  corner = make_float3(-width / 2.0f, -height / 2.0f, -depth / 2.0f);
  right  = make_float3(width, 0.0f, 0.0f);
  up     = make_float3(0.0f, 0.0f, depth);
  add_face(corner, right, up);

  // Top Face
  corner = make_float3(-width / 2.0f, height / 2.0f, depth / 2.0f);
  right  = make_float3(width, 0.0f, 0.0f);
  up     = make_float3(0.0f, 0.0f, -depth);
  add_face(corner, right, up);

  // Back Face
  corner = make_float3(width / 2.0f, -height / 2.0f, -depth / 2.0f);
  right  = make_float3(-width, 0.0f, 0.0f);
  up     = make_float3(0.0f, height, 0.0f);
  add_face(corner, right, up);

  // Front Face
  corner = make_float3(-width / 2.0f, -height / 2.0f, depth / 2.0f);
  right  = make_float3(width, 0.0f, 0.0f);
  up     = make_float3(0.0f, height, 0.0f);
  add_face(corner, right, up);

  // Add indices for all the faces.
  std::vector<unsigned int> indices;
  for (int f = 0; f < 6; f++) {
    int offset = 4*f;

    // Bot-right triangle
    indices.push_back(offset + 0);
    indices.push_back(offset + 1);
    indices.push_back(offset + 3);

    // Top-left triangle
    indices.push_back(offset + 0);
    indices.push_back(offset + 3);
    indices.push_back(offset + 2);
  }

  // Sanity Checks
  assert(vertices.size() == 4*6);
  assert(indices.size() == 6*6);

  return create_geometry(vertices, indices);
}

// Creats a unit plane on the xz-plane.
Geometry OptixScene::create_plane() {
  std::vector<VertexData> vertices;
  
  VertexData v;
  v.tangent = make_float3(1.0f, 0.0f, 0.0f);
  v.normal = make_float3(0.0f, 1.0f, 0.0f);

  float3 corner = make_float3(-1.0f, 0.0f, -1.0f);
  float3 right  = make_float3(2.0f, 0.0f, 0.0f);
  float3 up     = make_float3(0.0f, 0.0f, 2.0f);

  // Bottom-left Corner
  v.position = corner;
  vertices.push_back(v);

  // Bottom-right Corner
  v.position = corner + right;
  vertices.push_back(v);

  // Top-left Corner
  v.position = corner + up;
  vertices.push_back(v);

  // Top-right Corner
  v.position = corner + right + up;
  vertices.push_back(v);

  std::vector<unsigned int> indices;

  // Bot-right triangle
  indices.push_back(0);
  indices.push_back(1);
  indices.push_back(3);

  // Top-left triangle
  indices.push_back(0);
  indices.push_back(3);
  indices.push_back(2);

  // Sanity Checks
  assert(vertices.size() == 4);
  assert(indices.size() == 6);

  return create_geometry(vertices, indices);
}

// Creates a tessellated sphere with tessU longitudes and tessV latitudes.
// The resulting geometry contains tessU * (tessV - 1) * 2 triangles.
// The last argument is the maximum theta angle, which allows to generate spheres with a whole at the top.
Geometry OptixScene::create_sphere(const int tessU, const int tessV, const float radius, const float maxTheta) {
  assert(3 <= tessU && 3 <= tessV);

  std::vector<VertexData> vertices;
  vertices.reserve((tessU + 1) * tessV);

  std::vector<unsigned int> indices;
  indices.reserve(6 * tessU * (tessV - 1));

  float phi_step   = 2.0f * M_PIf / (float) tessU;
  float theta_step = maxTheta / (float) (tessV - 1);

  // Latitudinal rings.
  // Starting at the south pole going upwards on the y-axis.
  for (int latitude = 0; latitude < tessV; latitude++) // theta angle
  {
    float theta    = (float) latitude * theta_step;
    float sinTheta = sinf(theta);
    float cosTheta = cosf(theta);
    
    float texv = (float) latitude / (float) (tessV - 1); // Range [0.0f, 1.0f]

    // Generate vertices along the latitudinal rings.
    // On each latitude there are tessU + 1 vertices.
    // The last one and the first one are on identical positions, but have different texture coordinates!
    // DAR FIXME Note that each second triangle connected to the two poles has zero area!
    for (int longitude = 0; longitude <= tessU; longitude++) // phi angle
    {
      float phi    = (float) longitude * phi_step;
      float sinPhi = sinf(phi);
      float cosPhi = cosf(phi);
      
      float texu = (float) longitude / (float) tessU; // Range [0.0f, 1.0f]
        
      // Unit sphere coordinates are the normals.
      float3 normal = make_float3(cosPhi * sinTheta,
                                  -cosTheta, // -y to start at the south pole.
                                  -sinPhi * sinTheta);
      VertexData v;

      v.position = normal * radius;
      v.tangent  = make_float3(-sinPhi, 0.0f, -cosPhi);
      v.normal   = normal;
      v.uv       = make_float3(texu, texv, 0.0f);

      vertices.push_back(v);
    }
  }
    
  // We have generated tessU + 1 vertices per latitude.
  const unsigned int columns = tessU + 1;

  // Calculate indices.
  for (int latitude = 0 ; latitude < tessV - 1 ; latitude++)
  {                                           
    for (int longitude = 0 ; longitude < tessU ; longitude++)
    {
      indices.push_back( latitude      * columns + longitude    );  // lower left
      indices.push_back( latitude      * columns + longitude + 1);  // lower right
      indices.push_back((latitude + 1) * columns + longitude + 1);  // upper right 

      indices.push_back((latitude + 1) * columns + longitude + 1);  // upper right 
      indices.push_back((latitude + 1) * columns + longitude    );  // upper left
      indices.push_back( latitude      * columns + longitude    );  // lower left
    }
  }
  
  // Sanity Checks
  assert(vertices.size() == (tessU + 1) * tessV);
  assert(indices.size() == 6 * tessU * (tessV - 1));

  return create_geometry(vertices, indices);
}


Geometry OptixScene::create_geometry(std::vector<VertexData> const& attributes, std::vector<unsigned int> const& indices) {
  Geometry geometry(nullptr);

  run_unsafe_optix_code([&]() {
    geometry = m_ctx->createGeometry();

    Buffer vertex_buffer = m_ctx->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_USER);
    vertex_buffer->setElementSize(sizeof(VertexData));
    vertex_buffer->setSize(attributes.size());

    void *dst = vertex_buffer->map(0, RT_BUFFER_MAP_WRITE_DISCARD);
    memcpy(dst, attributes.data(), sizeof(VertexData) * attributes.size());
    vertex_buffer->unmap();

    Buffer index_buffer = m_ctx->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_UNSIGNED_INT3, indices.size() / 3);
    dst = index_buffer->map(0, RT_BUFFER_MAP_WRITE_DISCARD);
    memcpy(dst, indices.data(), sizeof(uint3) * indices.size() / 3);
    index_buffer->unmap();

    geometry->setBoundingBoxProgram(m_boundingbox_triangle_indexed);
    geometry->setIntersectionProgram(m_intersection_triangle_indexed);

    geometry["vertex_buffer"]->setBuffer(vertex_buffer);
    geometry["index_buffer"]->setBuffer(index_buffer);
    geometry->setPrimitiveCount((unsigned int)(indices.size()) / 3);
  });

  return geometry;
}