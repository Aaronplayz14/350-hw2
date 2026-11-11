#ifndef MATHUTIL_H
#define MATHUTIL_H

#include <iostream>
#include <cmath>
#include <vector>

namespace ECSEngine {

#include <cstdio>

struct Point2D {
    float x, y;
    Point2D(float x = 0, float y = 0) : x(x), y(y) {}
    Point2D(int x, int y) : x(x), y(y) {}

    /// @brief Euclidean distance from this point to another.
    /// @param other The point to measure to.
    /// @return The distance in the same units as the coordinates.
    float Distance(const Point2D& other) const {
        // Differences first, then one sqrt of the sum of squares.
        float xDif = other.x - x;
        float yDif = other.y - y;
        float d = sqrtf(xDif * xDif + yDif * yDif);
        return d;
    }
    /// @brief Component-wise vector addition. @return The sum of the two points.
    Point2D operator+(const Point2D& other) const { return Point2D(x + other.x, y + other.y); }

    /// @brief Offsets both components by a scalar. @return The shifted point.
    Point2D operator+(const float& other) const {
        // A scalar shifts the point diagonally rather than along one axis.
        return Point2D(x + other, y + other);
    }

    /// @brief Component-wise vector subtraction. @return The difference.
    Point2D operator-(const Point2D& other) const { return Point2D(x - other.x, y - other.y); }

    /// @brief Subtracts a scalar from both components. @return The shifted point.
    Point2D operator-(const float& other) const {
        // Same diagonal behaviour as scalar +, subtracting from both axes.
        return Point2D(x - other, y - other);
    }

    /// @brief Scales both components by a scalar. @return The scaled point.
    Point2D operator*(const float& scalar) const { return Point2D(x * scalar, y * scalar); }
    Point2D& operator+=(const float& scalar) {
        x += scalar;
        y += scalar;
        return *this;
    }
    Point2D& operator+=(const Point2D& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    Point2D& operator-=(const Point2D& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    bool operator==(const Point2D& other) const {
        // Exact float comparison; geometry here does not need a tolerance.
        return x == other.x && y == other.y;
    }
    Point2D& operator*=(const float& scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    /// @brief Scales both components down by a scalar.
    ///
    /// Dividing by zero is reported on stderr and leaves the point at the
    /// origin rather than producing infinities that would silently corrupt
    /// every later calculation.
    /// @param scalar The divisor; must not be zero.
    /// @return A reference to this point, for chaining.
    Point2D& operator/=(const float& scalar) {
        if (scalar == 0) {
            std::cerr << "WARNING: Cannot divide by 0, setting Point2D to origin\n";
            x = 0;
            y = 0;
        } else {
            x /= scalar;
            y /= scalar;
        }

        return *this;
    }

    /// @brief Dot product of this point with another. @return The scalar projection product.
    float operator*(const Point2D& other) const { return x * other.x + y * other.y; }

    /// @brief Dot product with a point treated as a direction.
    /// @param b The vector to dot against this one.
    /// @return The dot product of the two vectors.
    float Dot(Point2D b) const { return x * b.x + y * b.y; }

    /// @brief 2D cross product (z component of the 3D result).
    ///
    /// The sign of the result indicates which side of this vector b falls on,
    /// which is how Crosses tests whether segments intersect.
    /// @param b The vector to cross with this one.
    /// @return The z component of the cross product.
    float Cross(Point2D b) const { return x * b.y - y * b.x; }

    /// @brief Static dot product of two points. @return The dot product of a and b.
    static float Dot(Point2D a, Point2D b) {
        // Static form lets callers work on two unrelated points without
        // constructing a temporary; result matches the member version.
        return a.x * b.x + a.y * b.y;
    }

    /// @brief Static 2D cross product of two points.
    /// @return The z component of the cross product of a and b.
    static float Cross(Point2D a, Point2D b) { return a.x * b.y - a.y * b.x; }

    /// @brief Rescales this point to unit length, keeping its direction.
    ///
    /// A point already at the origin has no direction to preserve, so the
    /// warning is printed and the point is left at the origin.
    /// @return None. Modifies this point in place.
    void Normalize() {
        // Magnitude against the origin is the length of this vector.
        float mag = static_cast<float>(this->Distance(Point2D()));
        if (mag == 0) {
            std::cerr << "WARNING: Point2D at origin, cannot normalize\n";
            x = 0;
            y = 0;
        } else {
            x /= mag;
            y /= mag;
        }
    }

    Point2D Rotate(float radians) const {
        float cosTheta = std::cos(radians);
        float sinTheta = std::sin(radians);

        return Point2D(x * cosTheta - y * sinTheta, x * sinTheta + y * cosTheta);
    }

    Point2D RotateReverse(float radians) const {
        float cosTheta = std::cos(radians);
        float sinTheta = std::sin(radians);

        return Point2D(x * cosTheta + y * sinTheta, -x * sinTheta + y * cosTheta);
    }

    static Point2D Rotate(const Point2D& p, float radians) {
        float cosTheta = std::cos(radians);
        float sinTheta = std::sin(radians);

        return Point2D(p.x * cosTheta - p.y * sinTheta, p.x * sinTheta + p.y * cosTheta);
    }

    static Point2D RotateReverse(const Point2D& p, float radians) {
        float cosTheta = std::cos(radians);
        float sinTheta = std::sin(radians);

        return Point2D(p.x * cosTheta + p.y * sinTheta, -p.x * sinTheta + p.y * cosTheta);
    }
};

[[maybe_unused]] static std::ostream& operator<<(std::ostream& os, const Point2D& p) {
    os << "(" << p.x << ", " << p.y << ")";
    return os;
}

[[maybe_unused]] static Point2D operator*(float number, const Point2D& rhs) {
    return Point2D(number * rhs.x, number * rhs.y);
}

struct Line {
    Point2D p1, p2;

    Line(Point2D p1 = {0, 0}, Point2D p2 = {0, 0}) : p1(p1), p2(p2) {}
    Line(float x1, float y1, float x2, float y2) : p1(x1, y1), p2(x2, y2) {}

    /// @brief Distance between the two endpoints. @return The segment length.
    float Length() const { return static_cast<float>(p1.Distance(p2)); }

    /// @brief Finds the point on this segment nearest to a given point.
    ///
    /// Two dot products reject the cases where the nearest point falls past
    /// either endpoint; the projection onto the segment handles the rest.
    /// @param p The point to measure from.
    /// @return The closest point lying on the segment, which may be p1 or p2.
    Point2D ClosestPoint(const Point2D& p) const {
        // Classic closest-point-on-segment: reject the endpoint regions with
        // dot products, then project onto the segment for the middle case.
        Point2D ab = p2 - p1;
        Point2D ac = p - p1;
        Point2D bc = p - p2;

        // Projection falls before p1, so p1 is nearest.
        if (Point2D::Dot(ab, ac) <= 0) {
            return p1;
        } else if (Point2D::Dot(ab, bc) >= 0) {
            return p2;
        }

        // Otherwise project p onto ab and walk that fraction along the segment.
        float x = Point2D::Dot(ab, ac) / this->Length();
        return p1 + (ab * (x / this->Length()));
    }

    /// @brief Tests whether this segment intersects another.
    ///
    /// Solves for the two segment parameters using cross products and accepts
    /// the intersection only when both fall within [0, 1], meaning the point
    /// lies on both segments rather than on their infinite extensions.
    /// Parallel segments are rejected outright because they either never meet
    /// or overlap, which this method does not attempt to detect.
    /// @param other The segment to test against this one.
    /// @param crossingPoint Receives the intersection point when they cross.
    /// @return true if the segments cross, false if they do not.
    bool Crosses(const Line& other, Point2D& crossingPoint) const {
        Point2D ab = p2 - p1;
        Point2D xy = other.p2 - other.p1;

        // A zero cross product means the segments are parallel; bail out
        // before dividing so we never produce a NaN parameter.
        if (Point2D::Cross(ab, xy) == 0 || Point2D::Cross(xy, ab) == 0) {
            return false;
        }

        // Solve for the parameter along each segment, then check that both
        // land within the segments rather than on their infinite extensions.
        float t = Point2D::Cross((other.p1 - p1), xy) / Point2D::Cross(ab, xy);
        float u = Point2D::Cross((p1 - other.p1), ab) / Point2D::Cross(xy, ab);

        if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
            crossingPoint = p1 + (ab * t);
            return true;
        } else {
            return false;
        }
    }
};

[[maybe_unused]] static std::ostream& operator<<(std::ostream& os, const Line& l) {
    os << "L{" << l.p1 << ", " << l.p2 << "}";
    return os;
}

struct Circle {
    Point2D center;
    float radius;

    Circle(Point2D c = {0, 0}, float r = 0) : center(c), radius(r) {}
    Circle(float x, float y, float r) : center(x, y), radius(r) {}
};

struct Rect {
    Point2D topLeft;
    float width, height;

    /// @brief Builds a rectangle from its left edge, top edge, width and height.
    Rect(float left, float top, float width, float height)
        : topLeft(Point2D(left, top)), width(width), height(height) {}

    Rect(Point2D tl = {0, 0}, float w = 0.0, float h = 0.0) : topLeft(tl), width(w), height(h) {}

    // Creates bounding box around p1 and p2 with positive width/height
    Rect(Point2D p1, Point2D p2)
        : topLeft(std::min(p1.x, p2.x), std::min(p1.y, p2.y)),
          width(fabs(p1.x - p2.x)),
          height(fabs(p1.y - p2.y)) {}

    Rect(Point2D center, float radius)
        : topLeft(center.x - radius, center.y - radius), width(2 * radius), height(2 * radius) {}

    /// @brief Grows this rectangle so it also contains another rectangle.
    /// @param other The rectangle to enclose.
    /// @return A reference to this rectangle, for chaining.
    Rect& operator|=(const Rect& other) {
        // Union = min of the left/top edges, max of the right/bottom edges.
        float xMin, xMax, yMin, yMax;
        xMin = std::min(topLeft.x, other.topLeft.x);
        yMin = std::min(topLeft.y, other.topLeft.y);
        xMax = std::max(topLeft.x + width, other.topLeft.x + other.width);
        yMax = std::max(topLeft.y + height, other.topLeft.y + other.height);

        topLeft.x = xMin;
        topLeft.y = yMin;
        width = xMax - xMin;
        height = yMax - yMin;

        return *this;
    }
    /// @brief Grows this rectangle so it also contains a single point.
    /// @param other The point to enclose.
    /// @return A reference to this rectangle, for chaining.
    Rect& operator|=(const Point2D& other) {
        float xMin, xMax, yMin, yMax;
        xMin = std::min(topLeft.x, other.x);
        yMin = std::min(topLeft.y, other.y);
        xMax = std::max(topLeft.x + width, other.x);
        yMax = std::max(topLeft.y + height, other.y);

        topLeft.x = xMin;
        topLeft.y = yMin;
        width = xMax - xMin;
        height = yMax - yMin;
        return *this;
    }
    /// @brief Grows this rectangle so it also contains an entire line segment.
    /// @param other The segment to enclose; both endpoints are considered.
    /// @return A reference to this rectangle, for chaining.
    Rect& operator|=(const Line& other) {
        // Both endpoints must be enclosed, so each edge takes three candidates.
        float xMin, xMax, yMin, yMax;
        xMin = std::min(std::min(topLeft.x, other.p1.x), other.p2.x);
        yMin = std::min(std::min(topLeft.y, other.p1.y), other.p2.y);
        xMax = std::max(std::max(topLeft.x + width, other.p1.x), other.p2.x);
        yMax = std::max(std::max(topLeft.y + height, other.p1.y), other.p2.y);

        topLeft.x = xMin;
        topLeft.y = yMin;
        width = xMax - xMin;
        height = yMax - yMin;
        return *this;
    }
    /// @brief Shrinks this rectangle down to its overlap with another.
    ///
    /// When the rectangles miss each other, every field is set to NAN. This is
    /// the only signal the collision loop gets, so callers must test the
    /// result with std::isnan before trusting the dimensions.
    /// @param other The rectangle to intersect with this one.
    /// @return A reference to this rectangle, for chaining.
    Rect& operator&=(const Rect& other) {
        // Intersection = max of the left/top edges, min of the right/bottom.
        float xLB, xUB, yLB, yUB;
        xLB = std::max(topLeft.x, other.topLeft.x);
        yLB = std::max(topLeft.y, other.topLeft.y);
        xUB = std::min(topLeft.x + width, other.topLeft.x + other.width);
        yUB = std::min(topLeft.y + height, other.topLeft.y + other.height);

        // Lower bounds above upper bounds means the boxes do not overlap.
        if (xLB <= xUB && yLB <= yUB) {
            topLeft.x = xLB;
            topLeft.y = yLB;
            width = xUB - xLB;
            height = yUB - yLB;
        } else {
            // NaN marks "no overlap" and propagates through any later
            // arithmetic, so callers detect the miss with std::isnan.
            topLeft.x = NAN;
            topLeft.y = NAN;
            width = NAN;
            height = NAN;
        }

        return *this;
    }

    Rect& operator+=(const Point2D& other) {
        topLeft += other;
        return *this;
    }

    Rect operator+(const Point2D& other) const { return Rect(topLeft + other, width, height); }

    /// @brief Shrinks the rectangle by moving each edge inward.
    ///
    /// The top-left corner is moved by half the total reduction on each axis
    /// so the result stays centred. Width and height are clamped at zero, so
    /// an inset larger than the rectangle collapses it instead of inverting.
    /// @param inset The number of pixels to remove from each edge.
    /// @return None. Modifies this rectangle in place.
    void Inset(float inset) {
        // Clamped so an oversized inset collapses the rect instead of
        // producing a negative width that would flip the edges.
        float newWidth = std::max(width - inset * 2, 0.0f);
        float newHeight = std::max(height - inset * 2, 0.0f);
        // Shift by half the reduction on each axis to keep the rect centred.
        topLeft += Point2D((width - newWidth) / 2, (height - newHeight) / 2);
        width = newWidth;
        height = newHeight;
    }

    /// @brief Tests whether a point falls within the rectangle.
    ///
    /// The boundaries are inclusive, so a point exactly on an edge counts as
    /// inside.
    /// @param p The point to test.
    /// @return true if p lies within this rectangle, false otherwise.
    bool IsInside(const Point2D& p) const {
        // Inclusive on every edge, so points exactly on the border count.
        if (p.x >= topLeft.x && p.x <= topLeft.x + width && p.y >= topLeft.y &&
            p.y <= topLeft.y + height) {
            return true;
        }
        return false;
    }

    std::vector<Point2D> Points() const {
        std::vector<Point2D> points = {topLeft + Point2D(), topLeft + Point2D(width, 0.0),
                                       topLeft + Point2D(0.0, height),
                                       topLeft + Point2D(width, height)};
        return points;
    }
};

[[maybe_unused]] static std::ostream& operator<<(std::ostream& os, const Rect& r) {
    os << "R{" << r.topLeft << ", " << r.width << "x" << r.height << "}";
    return os;
}

}  // namespace ECSEngine

#endif  // MATHUTIL_H
