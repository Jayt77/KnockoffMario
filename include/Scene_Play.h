#pragma once

#include "Components.h"
#include "Physics.h"
#include "Scene.h"
#include <memory>

class Scene_Play : public Scene
{
    struct PlayerConfig
    {
        float X, Y, CX, CY, SPEED, MAXSPEED, JUMP, GRAVITY;
        std::string WEAPON; 
    };

    protected:

    std::shared_ptr<Entity> m_player;
    std::string m_levelPath;
    PlayerConfig m_playerConfig;
    bool m_drawTextures = true;
    bool m_drawCollision = false;
    bool m_drawDrawGrid = false;
    const Vec2 m_gridSize = { 64, 64 };
    sf::Text m_gridText, m_scoreText;
    Physics m_worldPhysics;
    int m_score = 0;

    void init(const std::string&);
    Vec2 gridToMidPixel(float, float, std::shared_ptr<Entity>);
    void loadLevel(const std::string&);
    void spawnPlayer();
    void spawnBullet(std::shared_ptr<Entity>);
    void sMovement();
    void sLifespan();
    void sCollision();
    void sAnimation();
    void sRender();
    void sDoAction(const Action&);
    void onEnd();
    void setPaused(bool);

    void changePlayerStateTo(PlayerState s);
    void spawnCoin(std::shared_ptr<Entity> tile);
    void spawnBrickDebris(std::shared_ptr<Entity> tile);
    void addScore(int x);

    public:
    Scene_Play(GameEngine*, std::string&);
    void update();
};
