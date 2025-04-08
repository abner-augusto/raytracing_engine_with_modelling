#ifndef MATRIX4X4_H
#define MATRIX4X4_H

#include <cmath>
#include <iostream>
#include <iomanip> 
#include <cstring>

#include "vec4.h"
#include "raytracer.h"

class Matrix4x4 {
public:
    double m[4][4];

    // Constructor: Identity matrix by default
    Matrix4x4() { set_identity(); }

    // Parameterized Constructor: Initialize with specific values
    Matrix4x4(
        double m00, double m01, double m02, double m03,
        double m10, double m11, double m12, double m13,
        double m20, double m21, double m22, double m23,
        double m30, double m31, double m32, double m33
    ) {
        m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
        m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
        m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
        m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
    }

    // Set to identity matrix
    void set_identity() {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                m[i][j] = (i == j) ? 1.0 : 0.0;
    }

    //Transpose Lines into Columns
    Matrix4x4 transpose(const Matrix4x4& mat) {
        Matrix4x4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i][j] = mat.m[j][i];
            }
        }
        return result;
    }

    // Matrix-Vector3 multiplication
    vec3 transform_vector(const vec3& v) const {
        vec4 v4(v, 0.0);
        vec4 result = (*this) * v4;
        return vec3(result.x, result.y, result.z);
    }

    // Matrix-Point multiplication
    point3 transform_point(const point3& p) const {
        vec4 p4(p, 1.0);
        vec4 result = (*this) * p4;
        return point3(result.x, result.y, result.z);
    }

    // Print the matrix for debugging
    void print() const {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                std::cout << std::setw(10) << m[i][j] << " ";
            }
            std::cout << "\n";
        }
    }
    // Matrix-matrix multiplication
    Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result;

        // Initialize result matrix more effiently
        std::memset(result.m, 0, sizeof(result.m));

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i][j] =
                    m[i][0] * other.m[0][j] +
                    m[i][1] * other.m[1][j] +
                    m[i][2] * other.m[2][j] +
                    m[i][3] * other.m[3][j];
            }
        }

        return result;
    }

    // Perspective projection matrix
    static Matrix4x4 perspective(double fov, double aspect, double znear, double zfar) {
        Matrix4x4 mat;

        double tan_half_fov = std::tan(fov / 2.0);

        mat.m[0][0] = 1.0 / (aspect * tan_half_fov);
        mat.m[1][1] = 1.0 / tan_half_fov;
        mat.m[2][2] = zfar / (zfar - znear);
        mat.m[2][3] = (-zfar * znear) / (zfar - znear);
        mat.m[3][2] = 1.0;
        mat.m[3][3] = 0.0;

        return mat;
    }

    // Matrix-vector4 multiplication
    vec4 operator*(const vec4& v) const {
        return vec4(
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
            m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w
        );
    }

    //Multiplies a 4D vector by the matrix represented by the current object,
    //followed by a perspective division to normalize the resulting vector.
    vec4 mul_vec4_project(const vec4& v) const {
        // Use the matrix-vector multiplication operator
        vec4 result = (*this) * v;

        // Perform perspective divide
        if (result.w != 0.0) {
            result.x /= result.w;
            result.y /= result.w;
            result.z /= result.w;
        }

        return result;
    }

    // Translation matrix
    static Matrix4x4 translation(const vec3& translationVector) {
        Matrix4x4 mat;
        mat.m[0][3] = translationVector.x();
        mat.m[1][3] = translationVector.y();
        mat.m[2][3] = translationVector.z();
        return mat;
    }

    // Scaling matrix
    static Matrix4x4 scaling(double sx, double sy, double sz) {
        Matrix4x4 mat;
        mat.m[0][0] = sx;
        mat.m[1][1] = sy;
        mat.m[2][2] = sz;
        return mat;
    }

    // Rotation matrix (around a specified axis)
    static Matrix4x4 rotation(double angle_deg, char axis) {
        Matrix4x4 mat;
        double angle_rad = angle_deg * M_PI / 180.0; // Convert degrees to radians
        double c = std::cos(angle_rad);
        double s = std::sin(angle_rad);

        switch (axis) {
        case 'x':
        case 'X':
            mat.m[1][1] = c;
            mat.m[1][2] = -s;
            mat.m[2][1] = s;
            mat.m[2][2] = c;
            break;
        case 'y':
        case 'Y':
            mat.m[0][0] = c;
            mat.m[0][2] = s;
            mat.m[2][0] = -s;
            mat.m[2][2] = c;
            break;
        case 'z':
        case 'Z':
            mat.m[0][0] = c;
            mat.m[0][1] = -s;
            mat.m[1][0] = s;
            mat.m[1][1] = c;
            break;
        default:
            throw std::invalid_argument("Invalid axis. Use 'x', 'y', or 'z'.");
        }

        return mat;
    }

    // Create a shearing matrix based on single values for X, Y, and Z
    static Matrix4x4 shearing(double shear_x = 0.0, double shear_y = 0.0, double shear_z = 0.0) {
        Matrix4x4 mat;

        // Apply shearing along X, Y, and Z directions
        mat.m[0][1] = shear_x; // Shear X based on Y
        mat.m[0][2] = 0.0;     // Disable Shear X based on Z

        mat.m[1][0] = shear_y; // Shear Y based on X
        mat.m[1][2] = 0.0;     // Disable Shear Y based on Z

        mat.m[2][0] = shear_z; // Shear Z based on X
        mat.m[2][1] = 0.0;     // Disable Shear Z based on Y

        return mat;
    }

    // Create a reflection matrix based on the world axis
    static Matrix4x4 mirror_simple(char plane) {
        Matrix4x4 mat;

        switch (plane) {
        case 'x':
        case 'X':
            mat.m[2][2] = -1; // Mirror across the XY plane
            break;
        case 'y':
        case 'Y':
            mat.m[0][0] = -1; // Mirror across the YZ plane
            break;
        case 'z':
        case 'Z':
            mat.m[1][1] = -1; // Mirror across the XZ plane
            break;
        default:
            throw std::invalid_argument("Invalid plane. Use 'x', 'y', or 'z'.");
        }

        return mat;
    }

    // Reflection matrix for an arbitrary plane defined by: ax + by + cz + d = 0
    // 'planeNormal' is the plane's normal vector (not necessarily normalized)
    // 'd' is the plane constant, defaulting to 0 for planes through the origin.
    static Matrix4x4 reflection(const vec3& planeNormal, double d = 0.0) {
        // Normalize the plane normal
        vec3 n = unit_vector(planeNormal);
        double a = n.x();
        double b = n.y();
        double c = n.z();

        Matrix4x4 mat;
        mat.m[0][0] = 1.0 - 2.0 * a * a;
        mat.m[0][1] = -2.0 * a * b;
        mat.m[0][2] = -2.0 * a * c;
        mat.m[0][3] = -2.0 * a * d;

        mat.m[1][0] = -2.0 * a * b;
        mat.m[1][1] = 1.0 - 2.0 * b * b;
        mat.m[1][2] = -2.0 * b * c;
        mat.m[1][3] = -2.0 * b * d;

        mat.m[2][0] = -2.0 * a * c;
        mat.m[2][1] = -2.0 * b * c;
        mat.m[2][2] = 1.0 - 2.0 * c * c;
        mat.m[2][3] = -2.0 * c * d;

        return mat;
    }

    // Reflection matrix for an arbitrary plane defined by a normal and a point on the plane
    // 'planeNormal' is the plane's normal vector (not necessarily normalized)
    // 'planePoint' is a point on the plane
    static Matrix4x4 mirror(const vec3& planeNormal, const point3& planePoint) {
        // Normalize the plane normal
        vec3 n = unit_vector(planeNormal);

        // Compute 'd' from the plane equation: ax + by + cz + d = 0
        double d = -dot(n, planePoint);

        // Use the existing mirror function with computed d
        return reflection(n, d);
    }


    // Create rotation matrix baed on quaternion
    static Matrix4x4 quaternion(const vec4& quaternion) {
        Matrix4x4 mat;
        double x = quaternion.x, y = quaternion.y, z = quaternion.z, w = quaternion.w;
        double xx = x * x;
        double yy = y * y;
        double zz = z * z;
        double xy = x * y;
        double xz = x * z;
        double yz = y * z;
        double wx = w * x;
        double wy = w * y;
        double wz = w * z;
        double ww = w * w;

        //mat.m[0][0] = 1.0 - 2.0 * (yy + zz);
        mat.m[0][0] = ww + xx - yy - zz;
        mat.m[0][1] = 2.0 * (xy - wz);
        mat.m[0][2] = 2.0 * (xz + wy);

        mat.m[1][0] = 2.0 * (xy + wz);
        //mat.m[1][1] = 1.0 - 2.0 * (xx + zz);
        mat.m[1][1] = ww - xx + yy - zz;
        mat.m[1][2] = 2.0 * (yz - wx);

        mat.m[2][0] = 2.0 * (xz - wy);
        mat.m[2][1] = 2.0 * (yz + wx);
        //mat.m[2][2] = 1.0 - 2.0 * (xx + yy);
        mat.m[2][2] = ww - xx - yy + zz;

        mat.m[3][3] = ww + xx + yy + zz;

        return mat;
    }

    Matrix4x4 rotateAroundPoint(const point3& point, const vec3& direction, double angle_deg) {
        // Step 1: Translate the point to the origin
        Matrix4x4 translateToOrigin = Matrix4x4::translation(vec3(-point.x(), -point.y(), -point.z()));

        // Step 2: Normalize the direction vector (axis of rotation)
        vec3 axis = unit_vector(direction);

        // Step 3: Compute the quaternion components
        vec4 rotationQuat = rotationQuat.createQuaternion(direction, angle_deg);

        // Step 4: Convert the quaternion to a rotation matrix
        Matrix4x4 rotationMatrix = Matrix4x4::quaternion(rotationQuat);

        // Step 5: Translate back to the original position
        Matrix4x4 translateBack = Matrix4x4::translation(vec3(point.x(), point.y(), point.z()));

        // Step 6: Compute the final transformation matrix
        return translateBack * rotationMatrix * translateToOrigin;
    }

    Matrix4x4 rotateAroundVector(const point3& center, const point3& point, double angle_deg) {
        // Compute the direction vector (axis of rotation)
        vec3 axis = unit_vector(point - center);

        // Call the rotation function using the computed axis
        return rotateAroundPoint(center, axis, angle_deg);
    }

    Matrix4x4 scaleAroundPoint(const point3& point, double sx, double sy, double sz) {
        // Step 1: Translate the point to the origin
        Matrix4x4 translateToOrigin = Matrix4x4::translation(vec3(-point.x(), -point.y(), -point.z()));

        // Step 2: Apply the scaling transformation
        Matrix4x4 scaleMatrix = scaling(sx, sy, sz);

        // Step 3: Translate back to the original position
        Matrix4x4 translateBack = Matrix4x4::translation(vec3(point.x(), point.y(), point.z()));

        // Step 4: Compute the final transformation matrix
        return translateBack * scaleMatrix * translateToOrigin;
    }

    // Assuming a uniform scale, extract the scaling factor from the matrix
    // by averaging the scaling components of the x, y, and z axes.
    double get_uniform_scale() const {

        double scale_x = vec3(m[0][0], m[0][1], m[0][2]).length();
        double scale_y = vec3(m[1][0], m[1][1], m[1][2]).length();
        double scale_z = vec3(m[2][0], m[2][1], m[2][2]).length();
        return (scale_x + scale_y + scale_z) / 3.0;
    }


    Matrix4x4 inverse() const {
        constexpr double SINGULARITY_TOLERANCE = 1e-10;
        Matrix4x4 inv;
        double det;

        // Helper lambda to calculate the determinant of a 3x3 matrix
        auto det3x3 = [](double a11, double a12, double a13,
            double a21, double a22, double a23,
            double a31, double a32, double a33) -> double {
                return a11 * (a22 * a33 - a23 * a32)
                    - a12 * (a21 * a33 - a23 * a31)
                    + a13 * (a21 * a32 - a22 * a31);
            };

        // Calculate cofactors and transpose (adjoint) simultaneously
        inv.m[0][0] = det3x3(m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3]);
        inv.m[0][1] = -det3x3(m[0][1], m[0][2], m[0][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3]);
        inv.m[0][2] = det3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[3][1], m[3][2], m[3][3]);
        inv.m[0][3] = -det3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3]);

        inv.m[1][0] = -det3x3(m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3]);
        inv.m[1][1] = det3x3(m[0][0], m[0][2], m[0][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3]);
        inv.m[1][2] = -det3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[3][0], m[3][2], m[3][3]);
        inv.m[1][3] = det3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3]);

        inv.m[2][0] = det3x3(m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3]);
        inv.m[2][1] = -det3x3(m[0][0], m[0][1], m[0][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3]);
        inv.m[2][2] = det3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[3][0], m[3][1], m[3][3]);
        inv.m[2][3] = -det3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3]);

        inv.m[3][0] = -det3x3(m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);
        inv.m[3][1] = det3x3(m[0][0], m[0][1], m[0][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);
        inv.m[3][2] = -det3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[3][0], m[3][1], m[3][2]);
        inv.m[3][3] = det3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2]);

        // Compute the determinant
        det = m[0][0] * inv.m[0][0] + m[0][1] * inv.m[0][1] + m[0][2] * inv.m[0][2] + m[0][3] * inv.m[0][3];

        if (std::fabs(det) < SINGULARITY_TOLERANCE) {
            throw std::runtime_error("Matrix is singular and cannot be inverted.");
        }

        // Normalize the inverse by the determinant
        det = 1.0 / det;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                inv.m[i][j] *= det;
            }
        }

        return inv;
    }

    double determinant() const {
        // Helper lambda to calculate the determinant of a 3x3 matrix
        auto det3x3 = [](double a11, double a12, double a13,
            double a21, double a22, double a23,
            double a31, double a32, double a33) -> double {
                return a11 * (a22 * a33 - a23 * a32)
                    - a12 * (a21 * a33 - a23 * a31)
                    + a13 * (a21 * a32 - a22 * a31);
            };

        // Expand along the first row
        return m[0][0] * det3x3(m[1][1], m[1][2], m[1][3],
            m[2][1], m[2][2], m[2][3],
            m[3][1], m[3][2], m[3][3])
            - m[0][1] * det3x3(m[1][0], m[1][2], m[1][3],
                m[2][0], m[2][2], m[2][3],
                m[3][0], m[3][2], m[3][3])
            + m[0][2] * det3x3(m[1][0], m[1][1], m[1][3],
                m[2][0], m[2][1], m[2][3],
                m[3][0], m[3][1], m[3][3])
            - m[0][3] * det3x3(m[1][0], m[1][1], m[1][2],
                m[2][0], m[2][1], m[2][2],
                m[3][0], m[3][1], m[3][2]);
    }


};

#endif
