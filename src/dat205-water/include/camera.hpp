#pragma once

#ifndef PINHOLE_CAMERA_H
#define PINHOLE_CAMERA_H

#include <optix.h>
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_namespace.h>


class PinholeCamera {
public:
  PinholeCamera();
  ~PinholeCamera();

  void setViewport(int w, int h);
  void setBaseCoordinates(int x, int y);
  void setSpeedRatio(float f);
  void setFocusDistance(float f);

  void orbit(int x, int y);
  void pan(int x, int y);
  void dolly(int x, int y);
  void focus(int x, int y);
  void zoom(float x);

  bool  getFrustum(optix::float3& pos, optix::float3& u, optix::float3& v, optix::float3& w);
  float getAspectRatio() const;
  
public: // Just to be able to load and save them easily.
  optix::float3 m_center;   // Center of interest point, around which is orbited (and the sharp plane of a depth of field camera).
  float         m_distance; // Distance of the camera from the center of intest.
  float         m_phi;      // Range [0.0f, 1.0f] from positive x-axis 360 degrees around the latitudes.
  float         m_theta;    // Range [0.0f, 1.0f] from negative to positive y-axis.
  float         m_fov;      // In degrees. Default is 60.0f

private:
  bool setDelta(int x, int y);
  
private:
  int   m_width;    // Viewport width.
  int   m_height;   // Viewport height.
  float m_aspect;   // m_width / m_height
  int   m_baseX;
  int   m_baseY;
  float m_speedRatio;
  
  // Derived values:
  int           m_dx;
  int           m_dy;
  bool          m_changed;
  optix::float3 m_cameraPosition;
  optix::float3 m_cameraU;
  optix::float3 m_cameraV;
  optix::float3 m_cameraW;
};

#endif // PINHOLE_CAMERA_H
