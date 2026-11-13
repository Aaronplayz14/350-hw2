#include "WindowManager.h"

namespace ECSEngine {

WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)) {}

std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

Rect WindowManager::GetWindowBounds() const { 
    return Rect(WindowToWorld(Point2D(0.f, 0.f)), mWindow->getSize().x * mWorldUnitsPerPixel, mWindow->getSize().y * mWorldUnitsPerPixel); 
}

void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {
    sf::RectangleShape rect({r.width, r.height});
    rect.setFillColor(sf::Color(red, green, blue, alpha));
    rect.setPosition({r.topLeft.x, r.topLeft.y});
    mWindow->draw(rect);
}

void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {
    sf::RectangleShape rect({r.width, r.height});
    rect.setFillColor(sf::Color::Transparent);
    rect.setOutlineThickness(3);
    rect.setOutlineColor(sf::Color(red, green, blue, alpha));
    rect.setPosition({r.topLeft.x, r.topLeft.y});
    mWindow->draw(rect);
}

Rect WindowManager::WindowToWorld(const Rect& rect) const { 
    return Rect(WindowToWorld(rect.topLeft), rect.width * mWorldUnitsPerPixel, rect.height * mWorldUnitsPerPixel); 
}

Point2D WindowManager::WindowToWorld(const Point2D& pt) const { 
    return Point2D(mWorldCenter.x + (pt.x - mWindow->getSize().x / 2.f) * mWorldUnitsPerPixel, 
                   mWorldCenter.y + (pt.y - mWindow->getSize().y / 2.f) * mWorldUnitsPerPixel); 
}

Rect WindowManager::WorldToWindow(const Rect& rect) const { 
    return Rect(WorldToWindow(rect.topLeft), rect.width / mWorldUnitsPerPixel, rect.height / mWorldUnitsPerPixel); 
}

Point2D WindowManager::WorldToWindow(const Point2D& pt) const { 
    return Point2D((pt.x - mWorldCenter.x) / mWorldUnitsPerPixel + mWindow->getSize().x / 2.f, 
                   (pt.y - mWorldCenter.y) / mWorldUnitsPerPixel + mWindow->getSize().y / 2.f); 
}

void WindowManager::SetCamera(const Point2D& worldPt, const Point2D& screenPt) {
    SetWorldCenter(Point2D(worldPt.x - (screenPt.x - mWindow->getSize().x / 2.f) * mWorldUnitsPerPixel, 
                           worldPt.y - (screenPt.y - mWindow->getSize().y / 2.f) * mWorldUnitsPerPixel));
}

Point2D WindowManager::GetWorldCenter() const { 
    return mWorldCenter; 
}

float WindowManager::GetWorldScale() const { 
    return mWorldUnitsPerPixel; 
}

void WindowManager::SetWorldCenter(const Point2D& worldCenter) {
    mWorldCenter = worldCenter;
}

void WindowManager::SetWorldScale(float worldUnitsPerPixel) {
    mWorldUnitsPerPixel = worldUnitsPerPixel;
}

void WindowManager::SetWorldRect(const Rect& worldCameraRect) {
    assert(worldCameraRect.width / worldCameraRect.height == static_cast<float>(mWindow->getSize().x) / static_cast<float>(mWindow->getSize().y));
    mWorldUnitsPerPixel = worldCameraRect.width / mWindow->getSize().x;
    mWorldCenter = Point2D(worldCameraRect.topLeft.x + worldCameraRect.width / 2.f,
                           worldCameraRect.topLeft.y + worldCameraRect.height / 2.f);
}

float WindowManager::WindowToWorld(float x) const { 
    return x * mWorldUnitsPerPixel; // Scalar conversion scales a length only.
}

float WindowManager::WorldToWindow(float x) const { 
    return x / mWorldUnitsPerPixel; // Scalar conversion scales a length only.
}

}  // namespace ECSEngine
