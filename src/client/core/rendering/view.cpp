#include "view.hpp"
#include "system/logger.hpp"
#include <algorithm>

View::View() = default;

View::View(int id)
    : id_(id) {}

void View::Initialize(LPDIRECT3DDEVICE9 device)
{
    device_ = device;
}

void View::InternalCreateTexture(int width, int height)
{
    // Remember the size even when the device is not there yet: a browser can be created before the
    // game device is captured, and OnDeviceReset() then builds the texture from these values.
    width_ = width;
    height_ = height;

    if (!device_) 
    {
        LOG_DEBUG("[View] No D3D device yet - the texture is created once one is captured");
        return;
    }

    wrapper_ = std::make_unique<ViewTexture>(device_, width, height);
}

void View::Create(int width, int height)
{
    InternalCreateTexture(std::max(1, width), std::max(1, height));
    active_ = true;
}

void View::Resize(int width, int height)
{
    if (!active_) 
        return;

    if (width == width_ && height == height_ && wrapper_) 
        return;

    InternalCreateTexture(std::max(1, width), std::max(1, height));
}

void View::UpdateTexture(const uint8_t* data, const cef_rect_t* dirtyRects, size_t count)
{
    if (!wrapper_ || !data) 
        return;

    if (count == 0 || !dirtyRects) 
    {
        LOG_DEBUG("[View] No dirty rects provided, performing full update");
        wrapper_->Update(data, width_, height_, false);
        return;
    }

    for (size_t i = 0; i < count; ++i)
    {
        const auto& r = dirtyRects[i];
        if (r.width <= 0 || r.height <= 0) 
            continue;

        wrapper_->UpdatePartial(data, width_, height_, r.x, r.y, r.width, r.height, false);
    }
}

void View::OnPaint(const void* buffer, int width, int height)
{
    if (!wrapper_ || !buffer) 
        return;

    wrapper_->Update(buffer, width, height, false);
}

void View::Draw()
{
    if (!wrapper_ || !active_) 
        return;

    wrapper_->Draw(posX_, posY_);
}

void View::Clear()
{
    if (wrapper_)
        wrapper_->Clear();
}

cef_rect_t View::rect() const
{
    cef_rect_t rc{0,0, std::max(1, width_), std::max(1, height_)};
    return rc;
}

void View::OnDeviceLost()
{
    LOG_DEBUG("[View] OnDeviceLost for view {}", id_);

    if (wrapper_) 
        wrapper_->OnDeviceLost();
}

void View::OnDeviceReset(LPDIRECT3DDEVICE9 device)
{
    LOG_DEBUG("[View] OnDeviceReset for view {}", id_);

    device_ = device;

    // A view created before the device was captured never got its texture (InternalCreateTexture
    // needs the device), and nothing else would ever build it: do it now.
    if (!wrapper_ && device_ && width_ > 0 && height_ > 0)
        wrapper_ = std::make_unique<ViewTexture>(device_, width_, height_);

    if (wrapper_)
        wrapper_->OnDeviceReset(device);
}
