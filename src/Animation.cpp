#include "Animation.h"
#include <cmath>
#include <iostream>

Animation::Animation() {};

Animation::Animation(const std::string& name, const sf::Texture& t) 
: Animation(name, t, 1, 0) {}


Animation::Animation(
        const std::string& name,
        const sf::Texture& t,
        size_t frameCount,
        size_t speed
) : m_sprite(t),
    m_frameCount(frameCount),
    m_currentFrame(0),
    m_speed(speed <= 0 ? 1 : speed),
    m_name(name) 
{
    m_size = Vec2((float)t.getSize().x / frameCount, (float)t.getSize().y);
    m_sprite.setOrigin(m_size.x / 2.0f, m_size.y / 2.0f);
    m_sprite.setTextureRect(
        sf::IntRect(
            std::floor(m_currentFrame) * m_size.x,
            0, 
            m_size.x,
            m_size.y
        )
    );
}

// update the animation to show the next frame, depending on its speed
// animation loops when it reaches the end
void Animation::update() {
    m_currentFrame++;

    //calculate the correct frame of animation to play based on 
    //the current frame and speed and set the texture rectangle properly
    size_t animFrame = (m_currentFrame / m_speed) % m_frameCount;
    m_sprite.setTextureRect(
        sf::IntRect(
            animFrame * m_size.x,
            0,
            m_size.x,
            m_size.y
        )
    );
}

const Vec2& Animation::getSize() const {
    return m_size;
}

const std::string& Animation::getName() const {
    return m_name;
}

sf::Sprite& Animation::getSprite() {
    return m_sprite;
}

bool Animation::hasEnded() const {
    //detect when animation has ended
    return ((m_currentFrame / m_speed) % m_frameCount == m_frameCount - 1);
}
