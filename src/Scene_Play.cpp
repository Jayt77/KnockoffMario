#include "Scene_Play.h"
#include "Scene_Menu.h"
#include "Assets.h"
#include "GameEngine.h"
#include "Components.h"
#include "Action.h"

#include "SFML/System/Vector2.hpp"

#include <iostream>
#include <string>
#include <fstream>

Scene_Play::Scene_Play(GameEngine* gameEngine, std::string& levelPath)
    : Scene(gameEngine)
    , m_levelPath(levelPath)
{
    init(m_levelPath);
}

void Scene_Play::init(const std::string& levelPath) {
    registerAction(sf::Keyboard::P, "PAUSE");
    registerAction(sf::Keyboard::Escape, "QUIT");
    registerAction(sf::Keyboard::T, "TOGGLE_TEXTURE"); // toggle drawing Textures
    registerAction(sf::Keyboard::C, "TOGGLE_COLLISION"); // toggle drawing Collision Boxes
    registerAction(sf::Keyboard::G, "TOGGLE_GRID"); // toggle drawing Grid

    // todo: register all other gameplay Actions
    // keymaps for playing
    registerAction(sf::Keyboard::W, "JUMP");
    registerAction(sf::Keyboard::S, "DOWN");
    registerAction(sf::Keyboard::A, "LEFT");
    registerAction(sf::Keyboard::D, "RIGHT");
    registerAction(sf::Keyboard::Space, "SHOOT");

    m_gridText.setCharacterSize(9);
    m_gridText.setFont(m_game->assets().getFont("Mario"));
    m_scoreText.setCharacterSize(20);
    m_scoreText.setFont(m_game->assets().getFont("Mario"));
    m_scoreText.setString("Score: 0");
    loadLevel(levelPath);
}

Vec2 Scene_Play::gridToMidPixel(
    float gridX, 
    float gridY, 
    std::shared_ptr<Entity> entity
) {
    //this function takes in a grid position and an Entity
    //returns a Vec2 indicating where the center position of the Entity should be 
    float offsetX, offsetY;
    auto eSize = entity->getComponent<CAnimation>().animation.getSize();
    float eScale;
    switch ((int)eSize.y) {
        case 16:
            eScale = 4.0;
            break;
        case 24: case 32:
            eScale = 2.0;
            break; 
        default:
            eScale = 1.0;
    }
    offsetX = (m_gridSize.x - eSize.x * eScale) / 2.0;
    offsetY = (m_gridSize.y - eSize.y * eScale) / 2.0;
    return Vec2(
        gridX * m_gridSize.x + m_gridSize.x / 2.0 - offsetX,
        height() - gridY * m_gridSize.y - m_gridSize.y / 2.0 + offsetY
    );
}

void Scene_Play::loadLevel(const std::string& fileName) {
    // reset the EntityManager every time we load a level
    m_entityManager = EntityManager();

    //read in the level file and add the appropriate entities
    std::ifstream file(fileName);
    if (!file) {
        std::cerr << "Could not load config.txt file!\n";
        exit(-1);
    }

    std::string head;
    while (file >> head) {
        if (head == "Tile") {
            std::string name;
            float x, y;
            file >> name >> x >> y;
            auto tile = m_entityManager.addEntity("tile");
            tile->addComponent<CAnimation>(
                m_game->assets().getAnimation(name), true
            );
            tile->addComponent<CTransform>(
                gridToMidPixel(x, y, tile),
                Vec2(0, 0),
                Vec2(4, 4),
                0
            );
            tile->addComponent<CBoundingBox>(m_gridSize);
        }
        else if (head == "Dec") {
            std::string name;
            float x, y;
            file >> name >> x >> y;
            auto dec = m_entityManager.addEntity("dec");
            dec->addComponent<CAnimation>(
                m_game->assets().getAnimation(name), true
            );
            dec->addComponent<CTransform>(
                gridToMidPixel(x, y, dec),
                Vec2(0, 0),
                Vec2(4, 4),
                0
            );
        }
        else if (head == "Player") {
            file >> m_playerConfig.X >> m_playerConfig.Y
                >> m_playerConfig.CX >> m_playerConfig.CY
                >> m_playerConfig.SPEED
                >> m_playerConfig.JUMP
                >> m_playerConfig.MAXSPEED
                >> m_playerConfig.GRAVITY
                >> m_playerConfig.WEAPON;
            spawnPlayer();
        }
        else {
            std::cerr << "head to " << head << "\n";
            std::cerr << "The config file format is incorrect!\n";
            exit(-1);
        }
    }
}

void Scene_Play::spawnPlayer() {
    m_player = m_entityManager.addEntity("player");
    m_player->addComponent<CAnimation>(
        m_game->assets().getAnimation("Stand"),
        true
    );
    m_player->addComponent<CTransform>(
        gridToMidPixel(m_playerConfig.X, m_playerConfig.Y, m_player),
        Vec2(m_playerConfig.SPEED, 0),
        Vec2(-2, 2),
        0
    );
    m_player->addComponent<CBoundingBox>(Vec2(m_playerConfig.CX, m_playerConfig.CY));
    m_player->addComponent<CInput>();
    m_player->addComponent<CState>(PlayerState::STAND);
    m_player->addComponent<CGravity>(m_playerConfig.GRAVITY);
}

void Scene_Play::spawnBullet(std::shared_ptr<Entity> entity) {
    auto bullet = m_entityManager.addEntity("bullet");
    bullet->addComponent<CAnimation>(
        m_game->assets().getAnimation(m_playerConfig.WEAPON),
        true
    );
    bullet->addComponent<CTransform>(
        entity->getComponent<CTransform>().pos,
        Vec2(-5 * entity->getComponent<CTransform>().scale.x, 0),
        entity->getComponent<CTransform>().scale,
        0
    );
    bullet->addComponent<CLifespan>(90, m_currentFrame);
    bullet->addComponent<CBoundingBox>(
        bullet->getComponent<CAnimation>().animation.getSize()
    );
}

void Scene_Play::addScore(int x) {
    m_score += x;
    m_scoreText.setString("Score: " + std::to_string(m_score));
}

void Scene_Play::update() {
    m_entityManager.update();

    if (!m_pause) {
        sMovement();
        sLifespan();
        sCollision();
        m_currentFrame++;
    }
    sAnimation();
    sRender();
     
}

void Scene_Play::sMovement() {
    //player movement / juming based on its CInput component
    
    // reset player speed to zero
    m_player->getComponent<CTransform>().velocity.x = 0;

    if (m_player->getComponent<CInput>().left) {
        m_player->getComponent<CTransform>().velocity.x = -m_playerConfig.SPEED;
        m_player->getComponent<CTransform>().scale.x = 2;
    }
    else if (m_player->getComponent<CInput>().right) {
        m_player->getComponent<CTransform>().velocity.x = m_playerConfig.SPEED;
        m_player->getComponent<CTransform>().scale.x = -2;
    }
    if (m_player->getComponent<CInput>().up) {
        if (m_player->getComponent<CInput>().canJump) {
            m_player->getComponent<CInput>().canJump = false;
            m_player->getComponent<CTransform>().velocity.y = m_playerConfig.JUMP;
        }
    }
    else if (m_player->getComponent<CTransform>().velocity.y <= 0){
        m_player->getComponent<CTransform>().velocity.y = 0;
    }

    if (m_player->getComponent<CInput>().shoot) {
        m_player->getComponent<CInput>().canShoot = false;

        // control fire rate
        if (m_currentFrame % 10 == 0) {
            m_player->getComponent<CInput>().canShoot = true;
            spawnBullet(m_player);
        }
    }

    // update all entities positons
    for (auto e : m_entityManager.getEntities()) {
        if (e->hasComponent<CGravity>()) {
            Vec2& v = e->getComponent<CTransform>().velocity; 
            v.y += e->getComponent<CGravity>().gravity;
            if ( v.y > m_playerConfig.MAXSPEED) {
                v.y = m_playerConfig.MAXSPEED;
            }
        }
        e->getComponent<CTransform>().prevPos = e->getComponent<CTransform>().pos;
        e->getComponent<CTransform>().pos += e->getComponent<CTransform>().velocity;
    }
}

void Scene_Play::sLifespan() {
    //check lifespan of entities with a lifespawn component, destroy if their time is up
    for (auto e : m_entityManager.getEntities()) {
        if (e->hasComponent<CLifespan>()) {
            auto& eLife = e->getComponent<CLifespan>();
            if (m_currentFrame - eLife.frameCreated >= eLife.lifespan) {
                e->destroy();
            }
        }
    }
}

void Scene_Play::sCollision() {
    // Check for bullet and tile collisions
    for (auto b : m_entityManager.getEntities("bullet")) {
        for (auto t : m_entityManager.getEntities("tile")) {
            Vec2 overlap = m_worldPhysics.GetOverlap(b, t);
            Vec2 pOverlap = m_worldPhysics.GetPreviousOverlap(b, t);
            if (0 < overlap.y && -m_gridSize.x < overlap.x) {
                if (0 <= overlap.x && pOverlap.x <= 0) {
                    if (t->getComponent<CAnimation>().animation.getName() == "Brick") {
                        
                        spawnBrickDebris(t);
                    }
                    b->destroy();
                }
            }
            
        }
    }

    //Check for player and coin collisions
    for (auto& c : m_entityManager.getEntities("coin")) {
        Vec2 overlap = m_worldPhysics.GetOverlap(m_player, c);
        if (overlap.x > 0 && overlap.y > 0) {
            c->destroy();
            addScore(100);
        }
    }
    
  
    // reset gravity
    m_player->getComponent<CGravity>().gravity = m_playerConfig.GRAVITY;

    //player / tile collisions and resolutions
    for (auto t : m_entityManager.getEntities("tile")) {
        Vec2 overlap = m_worldPhysics.GetOverlap(m_player, t);
        Vec2 pOverlap = m_worldPhysics.GetPreviousOverlap(m_player, t);
        // check if player is on air
        // check tiles being below player
        float dy = t->getComponent<CTransform>().pos.y -
            m_player->getComponent<CTransform>().pos.y;
        if (0 < overlap.x && -m_gridSize.y < overlap.y && dy > 0) {
            if (0 <= overlap.y && pOverlap.y <= 0) {
                // stand on tile
                m_player->getComponent<CInput>().canJump = true;
                m_player->getComponent<CGravity>().gravity = 0;
                m_player->getComponent<CTransform>().velocity.y = 0;
                // collision resolution
                m_player->getComponent<CTransform>().pos.y -= overlap.y;
            }
        }
        // check if player hits the tile
        if (0 < overlap.x && -m_gridSize.y < overlap.y && dy < 0) {
            if (0 <= overlap.y && pOverlap.y <= 0) {
                m_player->getComponent<CTransform>().pos.y += overlap.y;
                m_player->getComponent<CTransform>().velocity.y = 0;
                if (t->getComponent<CAnimation>().animation.getName() == "Question") {
                    t->getComponent<CAnimation>().animation = 
                        m_game->assets().getAnimation("QuestionHit");
                    spawnCoin(t);
                }
                if (t->getComponent<CAnimation>().animation.getName() == "Brick") {
                    spawnBrickDebris(t);
                }
            }
        }
        // check player and tile side collide
        float dx = t->getComponent<CTransform>().pos.x -
            m_player->getComponent<CTransform>().pos.x;
        if (0 < overlap.y && -m_gridSize.x < overlap.x) {
            if (0 <= overlap.x && pOverlap.x <= 0) {
                if (dx > 0) {
                    // tile is right of player
                    m_player->getComponent<CTransform>().pos.x -= overlap.x;
                }
                else {
                    // tile is left of player
                    m_player->getComponent<CTransform>().pos.x += overlap.x;
                }
            }
        }
    }
    //check to see if the player has fallen down a hole
    if (m_player->getComponent<CTransform>().pos.y > height()) {
        m_player->getComponent<CTransform>().pos =
            gridToMidPixel(m_playerConfig.X, m_playerConfig.Y, m_player);
    }
    //prevent the player walk of the left side of the map
    if (m_player->getComponent<CTransform>().pos.x < 
        m_player->getComponent<CBoundingBox>().size.x / 2.0) {
        m_player->getComponent<CTransform>().pos.x =
            m_player->getComponent<CBoundingBox>().size.x / 2.0;
    }
}

void Scene_Play::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "TOGGLE_TEXTURE") {
            m_drawTextures = !m_drawTextures; 
        }
        else if (action.name() == "TOGGLE_COLLISION") { 
            m_drawCollision = !m_drawCollision; 
        }
        else if (action.name() == "TOGGLE_GRID") { 
            m_drawDrawGrid = !m_drawDrawGrid; 
        }
        else if (action.name() == "PAUSE") { 
            setPaused(!m_pause);
        }
        else if (action.name() == "QUIT") { 
            onEnd();
        }
        else if (action.name() == "JUMP") {
            if (m_player->getComponent<CInput>().canJump) {
                m_player->getComponent<CInput>().up = true;
            }
        }
        else if (action.name() == "DOWN") {
            m_player->getComponent<CInput>().down = true;
        }
        else if (action.name() == "LEFT") {
            m_player->getComponent<CInput>().left = true;
        }
        else if (action.name() == "RIGHT") {
            m_player->getComponent<CInput>().right = true;
        }
        else if (action.name() == "SHOOT") {
            m_player->getComponent<CInput>().shoot = true;
        }
    }
    else if (action.type() == "END") {
        if (action.name() == "JUMP") {
            m_player->getComponent<CInput>().up = false;
        }
        else if (action.name() == "DOWN") {
            m_player->getComponent<CInput>().down = false;
        }
        else if (action.name() == "LEFT") {
            m_player->getComponent<CInput>().left = false;
        }
        else if (action.name() == "RIGHT") {
            m_player->getComponent<CInput>().right = false;
        }
        else if (action.name() == "SHOOT") {
            m_player->getComponent<CInput>().shoot = false;
        }
    }
}

void Scene_Play::sAnimation() {
    if(m_player->getComponent<CTransform>().velocity.y != 0) {
        m_player->getComponent<CInput>().canJump = false;
        if (m_player->getComponent<CInput>().shoot) {
            changePlayerStateTo(PlayerState::AIRSHOOT);
        }
        else {
            changePlayerStateTo(PlayerState::AIR);
        }
    }
    else {
        if (m_player->getComponent<CTransform>().velocity.x != 0) {
            if (m_player->getComponent<CInput>().shoot) {
                changePlayerStateTo(PlayerState::RUNSHOOT);
            }
            else {
                changePlayerStateTo(PlayerState::RUN);
            }
        }
        else {
            if (m_player->getComponent<CInput>().shoot) {
                changePlayerStateTo(PlayerState::STANDSHOOT);
            }
            else {
                changePlayerStateTo(PlayerState::STAND);
            }
        }
    }
    
    // change player animation
    if (m_player->getComponent<CState>().changeAnimate) {
        std::string aniName;
        switch (m_player->getComponent<CState>().state) {
            case PlayerState::STAND:
                aniName = "Stand";
                break;
            case PlayerState::AIR:
                aniName = "Air";
                break;
            case PlayerState::RUN:
                aniName = "Run";
                break;
            case PlayerState::STANDSHOOT:
                aniName = "StandShoot";
                break;
            case PlayerState::AIRSHOOT:
                aniName = "AirShoot";
                break;
            case PlayerState::RUNSHOOT:
                aniName = "RunShoot";
                break;
        }
        m_player->addComponent<CAnimation>(
                m_game->assets().getAnimation(aniName), true
                );
    }

    // if the animation is not repeated, and it has ended, destroy the entity
    for (auto e : m_entityManager.getEntities()) {
        if (e->hasComponent<CAnimation>()) 
        { 
            auto& animation = e->getComponent<CAnimation>().animation;
            if (animation.hasEnded() && !e->getComponent<CAnimation>().repeat) {
               e->destroy();
            }
            animation.update();
        }
    }
}

void Scene_Play::onEnd() {
    m_game->changeScene( "MENU", std::make_shared<Scene_Menu>(m_game));
}

void Scene_Play::sRender() {
    // coloring the background darker so you know that the game is paused
    if (!m_pause) {
        m_game->window().clear(sf::Color(100, 100, 255));
    }
    else {
        m_game->window().clear(sf::Color(50, 50, 150));
    }

    // set the viewport of the window to be centered on the player if it's far enough right
    auto& pPos = m_player->getComponent<CTransform>().pos;
    float windowCenterX = std::max(m_game->window().getSize().x / 2.0f, pPos.x);
    sf::View view = m_game->window().getView();
    view.setCenter(windowCenterX, m_game->window().getSize().y - view.getCenter().y);
    m_game->window().setView(view);

    // draw score text
    m_scoreText.setPosition(windowCenterX - (m_game->window().getSize().x / 2) + 25
       , 25);

    m_game->window().draw(m_scoreText);

    // draw all Entity textures / animations
    if (m_drawTextures) {
        for (auto e : m_entityManager.getEntities()) {
            auto& transform = e->getComponent<CTransform>();
            if (e->hasComponent<CAnimation>()) {
                auto& animation = e->getComponent<CAnimation>().animation;
                animation.getSprite().setRotation(transform.angle);
                animation.getSprite().setPosition(
                    transform.pos.x, transform.pos.y
                );
                animation.getSprite().setScale(
                    transform.scale.x, transform.scale.y
                );
                m_game->window().draw(animation.getSprite());
            }
        }
    }

    // draw all Entity collision bounding boxes with a rectangle shape
    if (m_drawCollision) {
        for (auto e : m_entityManager.getEntities()) {
            if (e->hasComponent<CBoundingBox>()) {
                auto& box = e->getComponent<CBoundingBox>();
                auto& transform = e->getComponent<CTransform>();
                sf::RectangleShape rect;
                rect.setSize(sf::Vector2f(box.size.x-1, box.size.y-1));
                rect.setOrigin(sf::Vector2f(box.halfSize.x, box.halfSize.y));
                rect.setPosition(transform.pos.x, transform.pos.y);
                rect.setFillColor(sf::Color(0, 0, 0, 0));
                rect.setOutlineColor(sf::Color::White);
                rect.setOutlineThickness(1);
                m_game->window().draw(rect);
            }
        }
    }

    // draw the grid so that can easily debug
    if (m_drawDrawGrid) {
        float leftX = m_game->window().getView().getCenter().x - width() / 2.0;
        float rightX = leftX + width() + m_gridSize.x;
        float nextGridX = leftX - ((int)leftX % (int)m_gridSize.x);

        for (float x = nextGridX; x < rightX; x += m_gridSize.x) {
            drawLine(Vec2(x, 0), Vec2(x, height()));
        }

        for (float y=0; y < height(); y += m_gridSize.y) {
            drawLine(Vec2(leftX, height()-y), Vec2(rightX, height()-y));

            for (float x = nextGridX; x < rightX; x += m_gridSize.x) {
                std::string xCell = std::to_string((int)x / (int)m_gridSize.x);
                std::string yCell = std::to_string((int)y / (int)m_gridSize.y);
                m_gridText.setString("(" + xCell + "," + yCell + ")");
                m_gridText.setPosition(x+3, height()-y-m_gridSize.y+2);
                m_game->window().draw(m_gridText);
            }
        }
    }
}

void Scene_Play::setPaused(bool pause) {
    m_pause = pause;
}

void Scene_Play::changePlayerStateTo(PlayerState s) {
    auto& prev = m_player->getComponent<CState>().preState;
    if (prev != s) {
        prev = m_player->getComponent<CState>().state;
        m_player->getComponent<CState>().state = s; 
        m_player->getComponent<CState>().changeAnimate = true;
    }
    else { 
        m_player->getComponent<CState>().changeAnimate = false;
    }
}

void Scene_Play::spawnCoin(std::shared_ptr<Entity> tile) {
    auto coin = m_entityManager.addEntity("coin");
    coin->addComponent<CAnimation>(m_game->assets().getAnimation("CoinSpin"), true);
    coin->addComponent<CTransform>(
        Vec2(
            tile->getComponent<CTransform>().pos.x,
            tile->getComponent<CTransform>().pos.y - m_gridSize.y
            ),
        Vec2(0, 0),
        tile->getComponent<CTransform>().scale,
        0
    );
    coin->addComponent<CLifespan>(1000, m_currentFrame);
}

void Scene_Play::spawnBrickDebris(std::shared_ptr<Entity> tile) {
    tile->getComponent<CAnimation>().animation = 
        m_game->assets().getAnimation("BrickDebris");
    tile->addComponent<CLifespan>(10, m_currentFrame);
}
