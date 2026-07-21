#pragma once

#include "ninho/simulation/content.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace ninho::simulation::detail {
namespace shape_volume_detail {

using Point = std::array<double, 3>;

inline Point add(Point a, Point b) noexcept
{
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

inline Point subtract(Point a, Point b) noexcept
{
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

inline Point multiply(Point value, double scale) noexcept
{
    return {value[0] * scale, value[1] * scale, value[2] * scale};
}

inline double dot(Point a, Point b) noexcept
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline Point cross(Point a, Point b) noexcept
{
    return {a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]};
}

inline double length(Point value) noexcept
{
    return std::sqrt(dot(value, value));
}

inline Point average(const std::vector<Point>& points) noexcept
{
    Point result{};
    for (const Point point : points) result = add(result, point);
    return multiply(result, 1.0 / static_cast<double>(points.size()));
}

struct Plane {
    Point normal{};
    double offset{};
};

inline double convex_hull_volume(const std::vector<Point>& vertices)
{
    if (vertices.size() < 4U) return 0.0;
    const Point interior = average(vertices);
    double coordinate_scale = 1.0;
    for (const Point vertex : vertices) {
        for (const double component : vertex) {
            coordinate_scale = std::max(coordinate_scale, std::abs(component));
        }
    }
    const double plane_epsilon = 1.0e-9 * coordinate_scale;
    std::vector<Plane> faces;
    for (std::size_t i = 0; i + 2U < vertices.size(); ++i) {
        for (std::size_t j = i + 1U; j + 1U < vertices.size(); ++j) {
            for (std::size_t k = j + 1U; k < vertices.size(); ++k) {
                Point normal = cross(
                    subtract(vertices[j], vertices[i]),
                    subtract(vertices[k], vertices[i]));
                const double normal_length = length(normal);
                if (normal_length <= plane_epsilon) continue;
                normal = multiply(normal, 1.0 / normal_length);
                double offset = dot(normal, vertices[i]);
                bool has_positive = false;
                bool has_negative = false;
                for (const Point vertex : vertices) {
                    const double distance = dot(normal, vertex) - offset;
                    has_positive = has_positive || distance > plane_epsilon;
                    has_negative = has_negative || distance < -plane_epsilon;
                }
                if (has_positive && has_negative) continue;
                if (dot(normal, interior) - offset > 0.0) {
                    normal = multiply(normal, -1.0);
                    offset = -offset;
                }
                const bool duplicate = std::ranges::any_of(faces,
                    [&](const Plane& face) {
                        return dot(face.normal, normal) >= 1.0 - 1.0e-9
                            && std::abs(face.offset - offset) <= plane_epsilon;
                    });
                if (!duplicate) faces.push_back({normal, offset});
            }
        }
    }

    double volume = 0.0;
    for (const Plane& face : faces) {
        std::vector<Point> polygon;
        for (const Point vertex : vertices) {
            if (std::abs(dot(face.normal, vertex) - face.offset)
                <= 4.0 * plane_epsilon) {
                polygon.push_back(vertex);
            }
        }
        if (polygon.size() < 3U) continue;
        const Point center = average(polygon);
        Point axis_u{};
        for (const Point vertex : polygon) {
            axis_u = subtract(vertex, center);
            const double axis_length = length(axis_u);
            if (axis_length > plane_epsilon) {
                axis_u = multiply(axis_u, 1.0 / axis_length);
                break;
            }
        }
        const Point axis_v = cross(face.normal, axis_u);
        std::ranges::sort(polygon, [&](const Point& lhs, const Point& rhs) {
            const Point left = subtract(lhs, center);
            const Point right = subtract(rhs, center);
            return std::atan2(dot(left, axis_v), dot(left, axis_u))
                < std::atan2(dot(right, axis_v), dot(right, axis_u));
        });
        for (std::size_t index = 0; index < polygon.size(); ++index) {
            const Point a = subtract(center, interior);
            const Point b = subtract(polygon[index], interior);
            const Point c = subtract(polygon[(index + 1U) % polygon.size()], interior);
            volume += std::abs(dot(a, cross(b, c))) / 6.0;
        }
    }
    return volume;
}

} // namespace shape_volume_detail

inline double shape_volume_m3(const ShapeDefinition& shape)
{
    switch (shape.type) {
    case ShapeType::Box:
        return 8.0 * shape.half_extents_m[0] * shape.half_extents_m[1]
            * shape.half_extents_m[2];
    case ShapeType::Sphere:
        return 4.0 / 3.0 * std::numbers::pi * std::pow(shape.radius_m, 3.0);
    case ShapeType::Capsule:
        return std::numbers::pi * shape.radius_m * shape.radius_m
            * (2.0 * shape.half_height_m + 4.0 / 3.0 * shape.radius_m);
    case ShapeType::Compound: {
        double total = 0.0;
        for (const auto& child : shape.children) total += shape_volume_m3(child);
        return total;
    }
    case ShapeType::ConvexHull:
        return shape_volume_detail::convex_hull_volume(shape.vertices_m);
    }
    return 0.0;
}

} // namespace ninho::simulation::detail
