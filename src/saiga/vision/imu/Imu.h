/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/vision/VisionTypes.h"

#include <vector>


namespace Saiga::Imu
{
using OmegaType        = Vec3;
using AccelerationType = Vec3;

template <typename T>
using VelocityBase = Vector<T, 3>;
using Velocity     = VelocityBase<double>;

template <typename T>
using GyroBiasBase = Vector<T, 3>;
using GyroBias     = GyroBiasBase<double>;

template <typename T>
using AccBiasBase = Vector<T, 3>;
using AccBias     = AccBiasBase<double>;

// (Linear) velocity and ImuBias stacked as
// [Velocity | GyroBias | AccBias]
template <typename T>
struct VelocityAndBiasBase
{
    VelocityBase<T> velocity  = VelocityBase<T>::Zero();
    GyroBiasBase<T> gyro_bias = GyroBiasBase<T>::Zero();
    AccBiasBase<T> acc_bias   = AccBiasBase<T>::Zero();

    template <typename G>
    VelocityAndBiasBase<G> cast() const
    {
        VelocityAndBiasBase<G> result;
        result.velocity  = velocity.template cast<G>();
        result.gyro_bias = gyro_bias.template cast<G>();
        result.acc_bias  = acc_bias.template cast<G>();
        return result;
    }
};
using VelocityAndBias = VelocityAndBiasBase<double>;


// The angular velocity (omega) is a combined angle-axis representation.
// The length of omega is the angle and the direction is the axis.
template <typename T>
Eigen::Quaternion<T> OmegaToQuaternion(Vector<T, 3> omega, T dt)
{
    T angle = omega.norm();
    if (angle < std::numeric_limits<T>::epsilon())
    {
        return Eigen::Quaternion<T>::Identity();
    }

    auto axis = omega / angle;
    Eigen::AngleAxis<T> aa(angle * dt, axis);
    Eigen::Quaternion<T> q(aa);
    return q;

#if 0
    T theta_half      = omega.norm() * 0.5 * dt;
    T sinc_theta_half = sinc(theta_half);
    T cos_theta_half  = cos(theta_half);
    Eigen::Quaternion<T> dq;
    dq.vec() = sinc_theta_half * omega * 0.5 * dt;
    dq.w()   = cos_theta_half;
    std::cout << dq << std::endl;
    return dq;
#endif
}

template <typename T>
Vector<T, 3> QuaternionToOmega(Eigen::Quaternion<T> q)
{
    Eigen::AngleAxis<T> aa(q);
    return aa.axis() * aa.angle();
}


struct SAIGA_VISION_API Data
{
    // Angular velocity in [rad/s] given by the gyroscope.
    OmegaType omega;

    // Linear acceleration in [m/s^2] given my the accelerometer.
    AccelerationType acceleration;

    // Timestamp of the sensor reading in [s]
    double timestamp = std::numeric_limits<double>::infinity();

    Data() = default;
    Data(const Vec3& omega, const Vec3& acceleration, double timestamp)
        : omega(omega), acceleration(acceleration), timestamp(timestamp)
    {
    }

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    static Data InterpolateAlpha(const Data& before, const Data& after, double alpha)
    {
        Data result;
        result.omega        = (1.0 - alpha) * before.omega + alpha * after.omega;
        result.acceleration = (1.0 - alpha) * before.acceleration + alpha * after.acceleration;
        result.timestamp    = (1.0 - alpha) * before.timestamp + alpha * after.timestamp;
        return result;
    }

    static Data Interpolate(const Data& before, const Data& after, double new_timestamp)
    {
        // don't extrapolate
        SAIGA_ASSERT(before.timestamp <= new_timestamp);
        SAIGA_ASSERT(after.timestamp >= new_timestamp);
        double alpha = (new_timestamp - before.timestamp) / (after.timestamp - before.timestamp);

        Data result;
        result.omega        = (1.0 - alpha) * before.omega + alpha * after.omega;
        result.acceleration = (1.0 - alpha) * before.acceleration + alpha * after.acceleration;
        result.timestamp    = new_timestamp;
        return result;
    }

    void Transform(const Mat3& q)
    {
        omega = q * omega;
        //        acceleration = q * acceleration;
    }
};

SAIGA_VISION_API std::ostream& operator<<(std::ostream& strm, const Imu::Data& data);


struct Sensor
{
    // Number of measurement per second
    // Hz
    double frequency = -1;
    // sqrt(Hz)
    double frequency_sqrt = -1;

    // Standard deviation of the sensor noise per timestep
    // rad / s / sqrt(Hz)
    double omega_sigma;

    // rad / s^2 / sqrt(Hz)
    double omega_random_walk;

    // m / s^2 / sqrt(Hz)
    double acceleration_sigma;

    // m / s^3 / sqrt(Hz)
    double acceleration_random_walk;

    // Transformation from the sensor coordinate system to the devices' coordinate system.
    SE3 sensor_to_body;

};

SAIGA_VISION_API std::ostream& operator<<(std::ostream& strm, const Imu::Sensor& sensor);


struct SAIGA_VISION_API ImuSequence
{
    // Timestamp of the image.
    double time_begin = std::numeric_limits<double>::infinity();
    double time_end   = std::numeric_limits<double>::infinity();

    // All meassurements since the last frame. The frame 0 contains all meassurements since the beginning. All elements
    // in this array should have a smaller timestamp than 'timestamp' above.
    std::vector<Imu::Data> data;


    bool complete() const
    {
        if (data.empty()) return false;

        if (time_begin == data.front().timestamp && time_end == data.back().timestamp)
        {
            return true;
        }
        return false;
    }

    bool Valid() const
    {
        if (!std::isfinite(time_begin) || !std::isfinite(time_begin))
        {
            return false;
        }
        return true;
    }

    double DeltaTime() { return time_end - time_begin; }
    // Adds another Sequence to the end.
    // A double value at the border is removed.
    void Add(const ImuSequence& other);

    // ====== Methods for testing robustness ======
    void AddNoise(double sigma_omega, double sigma_acc);
    void AddBias(const Vec3& bias_gyro, const Vec3& bias_acc);

    // Integrates the orientation and adds the gravity in local space.
    void AddGravity(const Vec3& bias_gyro, const SO3& initial_orientation, const Vec3& global_gravity);
};


// Inserts missing values into the sequences at the 'border'.
SAIGA_VISION_API void InterpolateMissingValues(ArrayView<ImuSequence> sequences);

SAIGA_VISION_API std::ostream& operator<<(std::ostream& strm, const Imu::ImuSequence& sequence);

struct SAIGA_VISION_API Preintegration
{
    Preintegration(const Vec3& bias_gyro = Vec3::Zero(), const Vec3& bias_accel = Vec3::Zero())
        : bias_gyro_lin(bias_gyro), bias_accel_lin(bias_accel)
    {
    }

    void Add(const Data& data, double dt) { Add(data.omega, data.acceleration, dt); }

    // The main integration function.
    // This assumes a constant w and a for dt time.
    void Add(const Vec3& omega_with_bias, const Vec3& acc_with_bias, double dt);

    // Adds the complete sequence using forward integration (explicit Euler).
    void IntegrateForward(const ImuSequence& sequence);

    void IntegrateMidPoint(const ImuSequence& sequence);



    // Predicts the state after this sequence.
    //
    std::pair<SE3, Vec3> Predict(const SE3& initial_pose, const Vec3& initial_velocity, const Vec3& g) const;



    // Integrated values (Initialized to identity/0);
    double delta_t = 0;
    SO3 delta_R;
    Vec3 delta_x = Vec3::Zero();
    Vec3 delta_v = Vec3::Zero();

    // Derivative w.r.t. the gyro bias
    Mat3 J_R_Biasg = Mat3::Zero();

    // Derivative w.r.t. the acc bias
    Mat3 J_P_Biasa = Mat3::Zero();
    Mat3 J_V_Biasa = Mat3::Zero();

   private:
    // Linear bias, which is subtracted from the meassurements.
    // Private because changing the bias invalidates the preintegration.
    Vec3 bias_gyro_lin, bias_accel_lin;
};


}  // namespace Saiga::Imu
