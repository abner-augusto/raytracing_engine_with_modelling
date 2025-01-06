#ifndef MATRIX4X4_H
#define MATRIX4X4_H

#include <cmath>
#include <iostream>
#include <iomanip> 

#include "vec4.h"
#include "raytracer.h"

class Matrix4x4 {
public:
    double m[4][4];

    // Constructor: Identity matrix by default
    Matrix4x4() { set_identity(); }

    // Set to identity matrix
    void set_identity() {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                m[i][j] = (i == j) ? 1.0 : 0.0;
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
    static Matrix4x4 translation(double tx, double ty, double tz) {
        Matrix4x4 mat;
        mat.m[0][3] = tx;
        mat.m[1][3] = ty;
        mat.m[2][3] = tz;
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

    // Rotation matrix (around the Z axis)
    static Matrix4x4 rotation_z(double angle_rad) {
        Matrix4x4 mat;
        double c = std::cos(angle_rad);
        double s = std::sin(angle_rad);
        mat.m[0][0] = c;
        mat.m[0][1] = -s;
        mat.m[1][0] = s;
        mat.m[1][1] = c;
        return mat;
    }

    // Create a shearing matrix based on single values for X, Y, and Z
    static Matrix4x4 shearing(double shear_x = 0.0, double shear_y = 0.0, double shear_z = 0.0) {
        Matrix4x4 mat;

        // Apply shearing along X, Y, and Z directions
        mat.m[0][1] = shear_x; // Shear X based on Y
        mat.m[0][2] = shear_x; // Shear X based on Z

        mat.m[1][0] = shear_y; // Shear Y based on X
        mat.m[1][2] = shear_y; // Shear Y based on Z

        mat.m[2][0] = shear_z; // Shear Z based on X
        mat.m[2][1] = shear_z; // Shear Z based on Y

        return mat;
    }

    static Matrix4x4 mirror(const std::string& plane) {
        Matrix4x4 mat;

        if (plane == "xy") {
            mat.m[2][2] = -1; // Mirror across the XY plane
        }
        else if (plane == "yz") {
            mat.m[0][0] = -1; // Mirror across the YZ plane
        }
        else if (plane == "xz") {
            mat.m[1][1] = -1; // Mirror across the XZ plane
        }
        else {
            throw std::invalid_argument("Invalid plane. Use 'xy', 'yz', or 'xz'.");
        }

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

    // Matrix-matrix multiplication
    Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i][j] = 0.0;
                for (int k = 0; k < 4; ++k) {
                    result.m[i][j] += m[i][k] * other.m[k][j];
                }
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

    // Assuming a uniform scale, extract the scaling factor from the matrix
    // by averaging the scaling components of the x, y, and z axes.
    double get_uniform_scale() const {

        double scale_x = vec3(m[0][0], m[0][1], m[0][2]).length();
        double scale_y = vec3(m[1][0], m[1][1], m[1][2]).length();
        double scale_z = vec3(m[2][0], m[2][1], m[2][2]).length();
        return (scale_x + scale_y + scale_z) / 3.0;
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

    Matrix4x4 inverse() const {
        Matrix4x4 inv;
        double det;

        // Calculate the cofactors for the first row
        inv.m[0][0] =   m[1][1] * m[2][2] * m[3][3] -
                        m[1][1] * m[2][3] * m[3][2] -
                        m[2][1] * m[1][2] * m[3][3] +
                        m[2][1] * m[1][3] * m[3][2] +
                        m[3][1] * m[1][2] * m[2][3] -
                        m[3][1] * m[1][3] * m[2][2];

        inv.m[0][1] =  -m[0][1] * m[2][2] * m[3][3] +
                        m[0][1] * m[2][3] * m[3][2] +
                        m[2][1] * m[0][2] * m[3][3] -
                        m[2][1] * m[0][3] * m[3][2] -
                        m[3][1] * m[0][2] * m[2][3] +
                        m[3][1] * m[0][3] * m[2][2];

        inv.m[0][2] =   m[0][1] * m[1][2] * m[3][3] -
                        m[0][1] * m[1][3] * m[3][2] -
                        m[1][1] * m[0][2] * m[3][3] +
                        m[1][1] * m[0][3] * m[3][2] +
                        m[3][1] * m[0][2] * m[1][3] -
                        m[3][1] * m[0][3] * m[1][2];

        inv.m[0][3] =  -m[0][1] * m[1][2] * m[2][3] +
                        m[0][1] * m[1][3] * m[2][2] +
                        m[1][1] * m[0][2] * m[2][3] -
                        m[1][1] * m[0][3] * m[2][2] -
                        m[2][1] * m[0][2] * m[1][3] +
                        m[2][1] * m[0][3] * m[1][2];

        inv.m[1][0] =  -m[1][0] * m[2][2] * m[3][3] +
                        m[1][0] * m[2][3] * m[3][2] +
                        m[2][0] * m[1][2] * m[3][3] -
                        m[2][0] * m[1][3] * m[3][2] -
                        m[3][0] * m[1][2] * m[2][3] +
                        m[3][0] * m[1][3] * m[2][2];

        inv.m[1][1] =   m[0][0] * m[2][2] * m[3][3] -
                        m[0][0] * m[2][3] * m[3][2] -
                        m[2][0] * m[0][2] * m[3][3] +
                        m[2][0] * m[0][3] * m[3][2] +
                        m[3][0] * m[0][2] * m[2][3] -
                        m[3][0] * m[0][3] * m[2][2];

        inv.m[1][2] =  -m[0][0] * m[1][2] * m[3][3] +
                        m[0][0] * m[1][3] * m[3][2] +
                        m[1][0] * m[0][2] * m[3][3] -
                        m[1][0] * m[0][3] * m[3][2] -
                        m[3][0] * m[0][2] * m[1][3] +
                        m[3][0] * m[0][3] * m[1][2];

        inv.m[1][3] =   m[0][0] * m[1][2] * m[2][3] -
                        m[0][0] * m[1][3] * m[2][2] -
                        m[1][0] * m[0][2] * m[2][3] +
                        m[1][0] * m[0][3] * m[2][2] +
                        m[2][0] * m[0][2] * m[1][3] -
                        m[2][0] * m[0][3] * m[1][2];


        inv.m[2][0] =   m[1][0] * m[2][1] * m[3][3] -
                        m[1][0] * m[2][3] * m[3][1] -
                        m[2][0] * m[1][1] * m[3][3] +
                        m[2][0] * m[1][3] * m[3][1] +
                        m[3][0] * m[1][1] * m[2][3] -
                        m[3][0] * m[1][3] * m[2][1];

        inv.m[2][1] =  -m[0][0] * m[2][1] * m[3][3] +
                        m[0][0] * m[2][3] * m[3][1] +
                        m[2][0] * m[0][1] * m[3][3] -
                        m[2][0] * m[0][3] * m[3][1] -
                        m[3][0] * m[0][1] * m[2][3] +
                        m[3][0] * m[0][3] * m[2][1];

        inv.m[2][2] =   m[0][0] * m[1][1] * m[3][3] -
                        m[0][0] * m[1][3] * m[3][1] -
                        m[1][0] * m[0][1] * m[3][3] +
                        m[1][0] * m[0][3] * m[3][1] +
                        m[3][0] * m[0][1] * m[1][3] -
                        m[3][0] * m[0][3] * m[1][1];

        inv.m[2][3] =  -m[0][0] * m[1][1] * m[2][3] +
                        m[0][0] * m[1][3] * m[2][1] +
                        m[1][0] * m[0][1] * m[2][3] -
                        m[1][0] * m[0][3] * m[2][1] -
                        m[2][0] * m[0][1] * m[1][3] +
                        m[2][0] * m[0][3] * m[1][1];

        inv.m[3][0] =  -m[1][0] * m[2][1] * m[3][2] +
                        m[1][0] * m[2][2] * m[3][1] +
                        m[2][0] * m[1][1] * m[3][2] -
                        m[2][0] * m[1][2] * m[3][1] -
                        m[3][0] * m[1][1] * m[2][2] +
                        m[3][0] * m[1][2] * m[2][1];

        inv.m[3][1] =   m[0][0] * m[2][1] * m[3][2] -
                        m[0][0] * m[2][2] * m[3][1] -
                        m[2][0] * m[0][1] * m[3][2] +
                        m[2][0] * m[0][2] * m[3][1] +
                        m[3][0] * m[0][1] * m[2][2] -
                        m[3][0] * m[0][2] * m[2][1];

        inv.m[3][2] =  -m[0][0] * m[1][1] * m[3][2] +
                        m[0][0] * m[1][2] * m[3][1] +
                        m[1][0] * m[0][1] * m[3][2] -
                        m[1][0] * m[0][2] * m[3][1] -
                        m[3][0] * m[0][1] * m[1][2] +
                        m[3][0] * m[0][2] * m[1][1];

        inv.m[3][3] =   m[0][0] * m[1][1] * m[2][2] -
                        m[0][0] * m[1][2] * m[2][1] -
                        m[1][0] * m[0][1] * m[2][2] +
                        m[1][0] * m[0][2] * m[2][1] +
                        m[2][0] * m[0][1] * m[1][2] -
                        m[2][0] * m[0][2] * m[1][1];

        // Compute the determinant
        det = m[0][0] * inv.m[0][0] + m[0][1] * inv.m[0][1] + m[0][2] * inv.m[0][2] + m[0][3] * inv.m[0][3];

        if (std::fabs(det) < 1e-10) {
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

};

#endif