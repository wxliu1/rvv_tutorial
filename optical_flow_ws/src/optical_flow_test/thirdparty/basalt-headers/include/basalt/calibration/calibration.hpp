/**
BSD 3-Clause License

This file is part of the Basalt project.
https://gitlab.com/VladyslavUsenko/basalt-headers.git

Copyright (c) 2019, Vladyslav Usenko and Nikolaus Demmel.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

@file
@brief Calibration datatypes for muticam-IMU and motion capture calibration
*/

#pragma once

#include <memory>

#include <basalt/spline/rd_spline.h>
#include <basalt/calibration/calib_bias.hpp>
#include <basalt/camera/generic_camera.hpp>

namespace basalt {

/// @brief Struct to store camera-IMU calibration
template <class Scalar>
struct Calibration {
  using Ptr = std::shared_ptr<Calibration>;
  using SE3 = Sophus::SE3<Scalar>;
  using Vec3 = Eigen::Matrix<Scalar, 3, 1>;

  /// @brief Default constructor.
  Calibration() {
    cam_time_offset_ns = 0;

    imu_update_rate = 200;

    // reasonable defaults
    gyro_noise_std.setConstant(0.000282);
    accel_noise_std.setConstant(0.016);
    accel_bias_std.setConstant(0.001);
    gyro_bias_std.setConstant(0.0001);
  }

  /// @brief Cast to other scalar type
  template <class Scalar2>
  Calibration<Scalar2> cast() const {
    Calibration<Scalar2> new_cam;

    for (const auto& v : T_i_c)
      new_cam.T_i_c.emplace_back(v.template cast<Scalar2>());
    for (const auto& v : intrinsics)
      new_cam.intrinsics.emplace_back(v.template cast<Scalar2>());
    for (const auto& v : vignette)
      new_cam.vignette.emplace_back(v.template cast<Scalar2>());

    new_cam.resolution = resolution;
    new_cam.cam_time_offset_ns = cam_time_offset_ns;

    new_cam.calib_accel_bias.getParam() =
        calib_accel_bias.getParam().template cast<Scalar2>();
    new_cam.calib_gyro_bias.getParam() =
        calib_gyro_bias.getParam().template cast<Scalar2>();

    new_cam.imu_update_rate = imu_update_rate;

    new_cam.gyro_noise_std = gyro_noise_std.template cast<Scalar2>();
    new_cam.accel_noise_std = accel_noise_std.template cast<Scalar2>();
    new_cam.gyro_bias_std = gyro_bias_std.template cast<Scalar2>();
    new_cam.accel_bias_std = accel_bias_std.template cast<Scalar2>();

    return new_cam;
  }

  /// @brief Vector of transformations from camera to IMU
  ///
  /// Point in camera coordinate frame \f$ p_c \f$ can be transformed to the
  /// point in IMU coordinate frame as \f$ p_i = T_{ic} p_c, T_{ic} \in
  /// SE(3)\f$
  // 对于camera坐标系下的一个点p_c, 能够被转换到IMU坐标系下的点p_i
  // 即：p_i = T_{ic} p_c, T_{ic} \in SE(3)
  // 这里外参T_i_c的size应该是2，
  // T_i_c[0]表示左目到imu的变换，或者左目到左目的变换
  // T_i_c[1]表示右目到imu的变换，或者右目到左目的变换
  Eigen::aligned_vector<SE3> T_i_c; // 相机到IMU的变换

  /// @brief Vector of camera intrinsics. Can store different camera models. See
  /// \ref GenericCamera.
  Eigen::aligned_vector<GenericCamera<Scalar>> intrinsics;

  /// @brief Camera resolutions.
  Eigen::aligned_vector<Eigen::Vector2i> resolution;

  /// @brief Vector of splines representing radially symmetric vignetting for
  /// each of the camera.
  ///
  /// Splines use time in nanoseconds for evaluation, but in this case we use
  /// distance from the optical center in pixels multiplied by 1e9 as a "time"
  /// parameter.
  std::vector<basalt::RdSpline<1, 4, Scalar>> vignette;

  /// @brief Time offset between cameras and IMU in nanoseconds.
  ///
  /// With raw image timestamp \f$ t_r \f$ and this offset \f$ o \f$ we cam get
  /// a timestamp aligned with IMU clock as \f$ t_c = t_r + o \f$.
  int64_t cam_time_offset_ns;

  /// @brief Static accelerometer bias from calibration.
  CalibAccelBias<Scalar> calib_accel_bias;

  /// @brief Static gyroscope bias from calibration.
  CalibGyroBias<Scalar> calib_gyro_bias;

  /// @brief IMU update rate.
  Scalar imu_update_rate;

  /// @brief Continuous time gyroscope noise standard deviation.
  Vec3 gyro_noise_std;
  /// @brief Continuous time accelerometer noise standard deviation.
  Vec3 accel_noise_std;

  /// @brief Continuous time bias random walk standard deviation for gyroscope.
  Vec3 gyro_bias_std;
  /// @brief Continuous time bias random walk standard deviation for
  /// accelerometer.
  Vec3 accel_bias_std;

  /// @brief Dicrete time gyroscope noise standard deviation.
  ///
  /// \f$ \sigma_d = \sigma_c \sqrt{r} \f$, where \f$ r \f$ is IMU update
  /// rate.
  inline Vec3 dicrete_time_gyro_noise_std() const {
    return gyro_noise_std * std::sqrt(imu_update_rate);
  }

  /// @brief Dicrete time accelerometer noise standard deviation.
  ///
  /// \f$ \sigma_d = \sigma_c \sqrt{r} \f$, where \f$ r \f$ is IMU update
  /// rate.
  inline Vec3 dicrete_time_accel_noise_std() const {
    return accel_noise_std * std::sqrt(imu_update_rate);
  }

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

/// @brief Struct to store motion capture to IMU calibration
template <class Scalar>
struct MocapCalibration {
  using Ptr = std::shared_ptr<MocapCalibration>;
  using SE3 = Sophus::SE3<Scalar>;

  /// @brief Default constructor.
  MocapCalibration() {
    mocap_time_offset_ns = 0;
    mocap_to_imu_offset_ns = 0;
  }

  /// @brief Transformation from motion capture origin to the world (calibration
  /// pattern).
  SE3 T_moc_w;

  /// @brief Transformation from the coordinate frame of the markers attached to
  /// the object to the IMU.
  SE3 T_i_mark;

  /// @brief Initial time alignment between IMU and MoCap clocks based on
  /// message arrival time.
  int64_t mocap_time_offset_ns;

  /// @brief Time offset between IMU and motion capture clock.
  int64_t mocap_to_imu_offset_ns;
};

}  // namespace basalt
