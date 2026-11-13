#pragma once

#include <unordered_map>
#include <cassert>

#include "MathUtil.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {

//-----------------------------------------------------------------------
/**
 * @brief Owns the render window and converts between world and screen space.
 *
 * Points include camera translation and scale; scalar overloads convert only
 * lengths. Returned points/rectangles are independent values. Sprites use
 * these conversions before drawing; camera setters do not change SFML's view.
 */
//-----------------------------------------------------------------------
class WindowManager {
public:
    /// @brief Opens an SFML window of the given pixel size and title.
    WindowManager(unsigned int width, unsigned int height, const std::string& title);
    ~WindowManager() = default;
    /// @brief Returns shared ownership of the window. Closing it still affects all users.
    /// The object survives while any shared_ptr remains; normal gameplay lets
    /// the engine manage its lifetime and does not retain it after shutdown.
    std::shared_ptr<sf::RenderWindow> GetWindow() const;
    /// @brief Visible window rectangle expressed in world coordinates.
    Rect GetWindowBounds() const;

    /// @brief Positions the camera so worldPt appears at screenPt (pixels).
    void SetCamera(const Point2D& worldPt, const Point2D& screenPt);
    /// @brief Copies the world-space center; later changes to the argument have no effect.
    void SetWorldCenter(const Point2D& worldCenter);
    /// @brief Sets center and scale from a rectangle with the window's aspect ratio.
    void SetWorldRect(const Rect& worldCameraRect);
    /// @brief Sets positive world units per screen pixel; 1 means pixel-for-pixel.
    void SetWorldScale(float worldUnitsPerPixel);
    /// @brief Returns an independent copy of the current world center.
    Point2D GetWorldCenter() const;
    /// @brief Returns world units per screen pixel.
    float GetWorldScale() const;

    /// @brief Converts a pixel length to world units without camera translation.
    float WindowToWorld(float x) const;
    /// @brief Converts a world length to pixels without camera translation.
    float WorldToWindow(float x) const;
    /// @brief Converts screen position and size to a world rectangle.
    Rect WindowToWorld(const Rect& rect) const;
    /// @brief Converts world position and size to a screen rectangle.
    Rect WorldToWindow(const Rect& rect) const;
    /// @brief Converts a world point using the current center, scale, and window size.
    Point2D WorldToWindow(const Point2D& pt) const;
    /// @brief Converts a pixel point back into world coordinates.
    Point2D WindowToWorld(const Point2D& pt) const;

    /// @brief Draws a filled rectangle in SFML window coordinates; channels are 0..255.
    void DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha = 255);
    /// @brief Draws a window-coordinate outline; caller presents the frame afterward.
    void DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha = 255);

private:
    std::shared_ptr<sf::RenderWindow> mWindow;
    Point2D mWorldCenter; // Center of the world in pixels
    float mWorldUnitsPerPixel = 1.0f; // How many world units per pixel
};

}  // namespace ECSEngine
