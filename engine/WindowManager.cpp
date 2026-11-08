#include "WindowManager.h"

namespace ECSEngine {

WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)) {}

std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

Rect WindowManager::GetWindowBounds() const { return Rect(); }

void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {}

void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {}

Rect WindowManager::WindowToWorld(const Rect& rect) const { return Rect(); }

Point2D WindowManager::WindowToWorld(const Point2D& pt) const { return Point2D(); }

Rect WindowManager::WorldToWindow(const Rect& rect) const { return Rect(); }

Point2D WindowManager::WorldToWindow(const Point2D& pt) const { return Point2D(); }

void WindowManager::SetCamera(const Point2D& worldPt, const Point2D& screenPt) {}

Point2D WindowManager::GetWorldCenter() const { return Point2D(); }

float WindowManager::GetWorldScale() const { return 1; }

void WindowManager::SetWorldCenter(const Point2D& worldCenter) {}

void WindowManager::SetWorldScale(float worldUnitsPerPixel) {}

void WindowManager::SetWorldRect(const Rect& worldCameraRect) {}

float WindowManager::WindowToWorld(float x) const { return 0; }

float WindowManager::WorldToWindow(float x) const { return 0; }

}  // namespace ECSEngine
