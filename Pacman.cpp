#include "Pacman.h"
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <box2d/box2d.h>

#include "lib/box2d/src/body.h"



using namespace std;
#include "bagel.h"
using namespace bagel;

namespace pacman
{
    /**
     * @brief Checks whether the Pac-Man texture was successfully loaded.
     * @return True if the texture is valid (not null), false otherwise.
     */
    bool PacMan::valid()
    {
        return tex != nullptr;
    }

    /**
    * @brief Processes keyboard input for player-controlled entities and sets movement intentions.
    */
    void PacMan::InputSystem() {
        Mask required = MaskBuilder()
                .set<Input>()
                .set<Intent>()
                .set<PlayerControlled>()
                .build();

        SDL_PumpEvents();
        const bool* keys = SDL_GetKeyboardState(nullptr);
        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type e{id};
            if (World::mask(e).test(required)) {
                const auto& k = World::getComponent<Input>(e);
                auto& in = World::getComponent<Intent>(e);
                if (keys[k.up] && !in.blockedUp) {
                    in.up = true;
                    in.down = in.left = in.right = false;
                    in.blockedUp = in.blockedDown = in.blockedLeft = in.blockedRight = false;
                }
                else if (keys[k.down] && !in.blockedDown) {
                    in.down = true;
                    in.up = in.left = in.right = false;
                    in.blockedUp = in.blockedDown = in.blockedLeft = in.blockedRight = false;
                }
                else if (keys[k.left] && !in.blockedLeft) {
                    in.left = true;
                    in.up = in.down = in.right = false;
                    in.blockedUp = in.blockedDown = in.blockedLeft = in.blockedRight = false;
                }
                else if (keys[k.right] && !in.blockedRight) {
                    in.right = true;
                    in.up = in.down = in.left = false;
                    in.blockedUp = in.blockedDown = in.blockedLeft = in.blockedRight = false;
                }
            }
        }
    }

    /**
     * @brief Applies movement to entities based on their current intent and updates Box2D body velocities.
     */
    void PacMan::MovementSystem()
    {
        static const Mask mask = MaskBuilder()
            .set<Intent>()
            .set<Collider>()
            .set<Position>()
            .build();

        for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
            if (World::mask(e).test(mask)) {
                auto& i = World::getComponent<Intent>(e);
                const auto& c = World::getComponent<Collider>(e);
                bool isPlayer = World::mask(e).test(Component<PlayerControlled>::Bit);
                bool isGhost = World::mask(e).test(Component<Ghost>::Bit);

                const float y = i.up ? -20 : i.down ? 20 : 0;
                const float x = i.left ? -20 : i.right ? 20 : 0;

                b2Body_SetLinearVelocity(c.b, {x,y});
                if (isPlayer) {
                    if (i.up) {
                        b2Body_SetTransform(c.b, b2Body_GetPosition(c.b), {0.0f, -1.0f});
                        i.blockedDown = i.blockedLeft = i.blockedRight = false;
                    }else if (i.down) {
                        b2Body_SetTransform(c.b, b2Body_GetPosition(c.b), {0.0f, 1.0f});
                        i.blockedUp = i.blockedLeft = i.blockedRight = false;
                    } else if (i.left) {
                        b2Body_SetTransform(c.b, b2Body_GetPosition(c.b), {-1.0f, 0.0f});
                        i.blockedUp = i.blockedDown = i.blockedRight = false;
                    }else if (i.right) {
                        b2Body_SetTransform(c.b, b2Body_GetPosition(c.b), {1.0f, 0.0f});
                        i.blockedUp = i.blockedDown = i.blockedLeft = false;
                    }
                }
            }
        }
    }

    /**
     * @brief Renders all drawable entities with textures and positions.
     */
    void PacMan::RenderSystem() {
        static const Mask mask = MaskBuilder()
                .set<Position>()
                .set<Drawable>()
                .build();

        SDL_RenderClear(ren);
        for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
            if (World::mask(e).test(mask)) {
                const auto& t = World::getComponent<Position>(e);
                auto& d = World::getComponent<Drawable>(e);
                bool pacman = World::mask(e).test(Component<PlayerControlled>::Bit);
                bool ghost = World::mask(e).test(Component<Ghost>::Bit);

                if (pacman || ghost) {
                    d.frame++;
                    if (d.frame == 100)
                        d.frame = 0;
                    if (pacman) {
                        auto& stat = World::getComponent<PlayerStats>(e);
                        for (int i = 0 ; i < stat.lives; ++i) {
                            float space = 5.f + (float) i*CLOSE_PACMAN.w;
                            SDL_FRect lives = {(space)*CHARACTER_TEX_SCALE, (BOARD.h + 4.f) * CHARACTER_TEX_SCALE, CLOSE_PACMAN.w*CHARACTER_TEX_SCALE, CLOSE_PACMAN.h*CHARACTER_TEX_SCALE};
                            SDL_RenderTextureRotated(
                                ren, tex, &CLOSE_PACMAN, &lives, 0,
                                nullptr, SDL_FLIP_NONE);
                        }
                    }
                }
                const SDL_FRect dst = {
                    t.p.x-d.size.x/2,
                    t.p.y-d.size.y/2,
                    d.size.x, d.size.y};


                SDL_RenderTextureRotated(
                    ren, tex, &d.part[(d.frame / 10) % 2], &dst, t.a,
                    nullptr, SDL_FLIP_NONE);
            }
        }
        SDL_RenderPresent(ren);
    }

    /**
    * @brief Synchronizes Box2D world step and updates entity positions and angles from physics bodies.
    */
    void PacMan::box_system()
    {
        static const Mask mask = MaskBuilder()
            .set<Collider>()
            .set<Position>()
            .build();
        static constexpr float	BOX2D_STEP = 1.f/FPS;
        b2World_Step(boxWorld, BOX2D_STEP, 4);

        for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
            if (World::mask(e).test(mask)) {
                b2Transform t = b2Body_GetTransform(World::getComponent<Collider>(e).b);
                World::getComponent<Position>(e) = {
                    {t.p.x*BOX_SCALE, t.p.y*BOX_SCALE},
                    RAD_TO_DEG * b2Rot_GetAngle(t.q)
                };
            }
        }
    }

    /**
  * @brief Handles collision events between Pac-Man, ghosts, pellets, and walls.
  */
    void PacMan::CollisionSystem()
    {
        const auto se = b2World_GetSensorEvents(boxWorld);
        for (int i = 0; i < se.beginCount; ++i) {
            b2BodyId sensor = b2Shape_GetBody(se.beginEvents[i].sensorShapeId);
            b2BodyId b = b2Shape_GetBody(se.beginEvents[i].visitorShapeId);
            auto *e = static_cast<ent_type*>(b2Body_GetUserData(b));
            auto *e1 = static_cast<ent_type*>(b2Body_GetUserData(sensor));

            bool sensorIsPlayer = World::mask(*e1).test(Component<PlayerControlled>::Bit);
            bool sensorIsWall = World::mask(*e1).test(Component<Wall>::Bit);

            bool isPlayer = World::mask(*e).test(Component<PlayerControlled>::Bit);
            bool isGhost = World::mask(*e).test(Component<Ghost>::Bit);
            bool isPellet = World::mask(*e).test(Component<Pellet>::Bit);
            bool isWall = World::mask(*e).test(Component<Wall>::Bit);

            if (isWall && sensorIsWall) {
                continue;
            }
            if (sensorIsWall || (isWall && sensorIsPlayer)) {
                //pacman or ghost hit wall
                ent_type player = sensorIsPlayer ? *e1 : *e;
                auto& dir = World::getComponent<Intent>(player);
                const auto& col = World::getComponent<Collider>(player);
                auto& dGhost = World::getComponent<Drawable>(player);
                b2Vec2 pos = b2Body_GetPosition(col.b);

                b2Transform t = b2Body_GetTransform(col.b);
                float angleC = t.q.c;
                float angleS = t.q.s;

                if (dir.up) {
                    dir.blockedUp = true;
                    dir.up = false;
                    ///move pacman slightly to prevent next collision
                    pos.y += 5.0f / BOX_SCALE;
                    b2Body_SetTransform(col.b, pos, {angleC, angleS});
                    if (isGhost) {
                        dir.right = dGhost.frame % 2 == 0;
                        dir.left = dGhost.frame % 2 != 0;
                    }
                }
                else if (dir.down) {
                    dir.blockedDown = true;
                    dir.down = false;
                    ///move pacman slightly to prevent next collision
                    pos.y -= 5.0f / BOX_SCALE;
                    b2Body_SetTransform(col.b, pos, {angleC, angleS});
                    if (isGhost){
                        dir.right = dGhost.frame % 2 == 0;
                        dir.left = dGhost.frame % 2 != 0;
                    }
                }
                else if (dir.left) {
                    dir.blockedLeft = true;
                    dir.left = false;
                    ///move pacman slightly to prevent next collision
                    pos.x += 5.0f / BOX_SCALE;
                    b2Body_SetTransform(col.b, pos, {angleC, angleS});
                    if (isGhost){
                        dir.up = dGhost.frame % 2 == 0;
                        dir.down = dGhost.frame % 2 != 0;
                    }
                }
                else if (dir.right) {
                    dir.blockedRight = true;
                    dir.right = false;
                    ///move pacman slightly to prevent next collision
                    pos.x -= 5.0f / BOX_SCALE;
                    b2Body_SetTransform(col.b, pos, {angleC, angleS});
                    if (isGhost) {
                        dir.up = dGhost.frame % 2 == 0;
                        dir.down = dGhost.frame % 2 != 0;
                    }
                }
            }

            if (sensorIsPlayer && isGhost) {
                //pacman hit ghost
                auto& stats = World::getComponent<PlayerStats>(*e1);
                int lives = stats.lives -1 ;
                if (lives == 0) {
                    //GAME-OVER
                    EndGameSystem();
                    return;

                }
                auto& dGhost = World::getComponent<Drawable>(*e);

                createGhost(dGhost.part[0], dGhost.part[1], {100.f*CHARACTER_TEX_SCALE, 120.f*CHARACTER_TEX_SCALE});
                World::destroyEntity(*e1);
                World::destroyEntity(*e);
                b2DestroyBody(sensor);
                b2DestroyBody(b);
                createPacMan(lives);
                std::cout << "Player hit by ghost! Lives left: " << lives << "\n";

            }

            if (sensorIsPlayer && isPellet) {
                //pacman ate pellet
                auto& stats = World::getComponent<PlayerStats>(*e1);
                const auto& pelletData = World::getComponent<Pellet>(*e);

                if (pelletData.type == ePelletState::Normal) {
                    stats.score += 10;
                } else if (pelletData.type == ePelletState::Power) {
                    stats.score += 50;
                    // TODO: Set ghosts to vulnerable state (if implemented)
                }
                World::destroyEntity(*e);
                b2DestroyBody(b);

            }
        }
    }

    /**
   * @brief Handles ghost AI behavior such as random movement decisions.
   */
    void PacMan::AISystem() {
        Mask required = MaskBuilder()
        .set<Ghost>()
        .set<Intent>()
        .set<Drawable>()
        .build();

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type e{id};
            if (World::mask(e).test(required)) {
                auto& in = World::getComponent<Intent>(e);
                auto& dr = World::getComponent<Drawable>(e);
                if (dr.frame % 240 == 0) {
                    in.up = in.down = in.left = in.right = false;
                    int dir = rand() % 4;
                    switch (dir) {
                        case 0: in.up = true; break;
                        case 1: in.down = true; break;
                        case 2: in.left = true; break;
                        case 3: in.right = true; break;
                    }
                }
            }
        }
    }

    /**
    * @brief Removes all game entities except the background and destroys their physics bodies.
    */
    void PacMan::EndGameSystem() {
        Mask notRequired = MaskBuilder()
            .set<Background>()
            .build();
        Mask required = MaskBuilder()
            .set<Collider>()
            .build();
        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type e{id};
            if (! World::mask(e).test(notRequired) && World::mask(e).test(required)) {
                auto& c = World::getComponent<Collider>(e);
                World::destroyEntity(e);
                b2DestroyBody(c.b);

            }
        }
    }

    /**
     * @brief Creates the main player character (Pac-Man).
     * @param lives Number of lives to initialize Pac-Man with.
     */
    void PacMan::createPacMan(int lives) {
        SDL_FPoint p = {13.f*CHARACTER_TEX_SCALE, 240.f*CHARACTER_TEX_SCALE};

        b2BodyDef pacmanBodyDef = b2DefaultBodyDef();
        pacmanBodyDef.type = b2_kinematicBody;
        pacmanBodyDef.position = {p.x / BOX_SCALE, p.y / BOX_SCALE};
        b2BodyId pacmanBody = b2CreateBody(boxWorld, &pacmanBodyDef);

        //Define shape
        b2ShapeDef pacmanShapeDef = b2DefaultShapeDef();
        pacmanShapeDef.enableSensorEvents = true;
        pacmanShapeDef.isSensor = true;
        pacmanShapeDef.density = 1; // Not needed for static, but harmless

        b2Circle pacmanCircle = {0,0,(OPEN_PACMAN.w*CHARACTER_TEX_SCALE/BOX_SCALE)/2};
        b2CreateCircleShape(pacmanBody, &pacmanShapeDef, &pacmanCircle);

        Entity e = Entity::create();
        e.addAll(
         Position{{},0},
         Drawable{{OPEN_PACMAN,CLOSE_PACMAN}, {OPEN_PACMAN.w*CHARACTER_TEX_SCALE, OPEN_PACMAN.h*CHARACTER_TEX_SCALE},0},
         Collider{pacmanBody},
         Intent{},
         Input{SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_RIGHT, SDL_SCANCODE_LEFT},
         PlayerControlled{},
         PlayerStats{0,lives}
         );
        b2Body_SetUserData(pacmanBody, new ent_type{e.entity()});
    }

    /**
     * @brief Creates a ghost entity in the game.
     * @param r1 First frame of the ghost's sprite.
     * @param r2 Second frame of the ghost's sprite.
     * @param p Starting position of the ghost.
     */

    void PacMan::createGhost(const SDL_FRect& r1,const SDL_FRect& r2, const SDL_FPoint& p) {
        b2BodyDef padBodyDef = b2DefaultBodyDef();
        padBodyDef.type = b2_kinematicBody;
        //padBodyDef.type = b2_staticBody;
        padBodyDef.position = {p.x / BOX_SCALE, p.y / BOX_SCALE};
        b2BodyId padBody = b2CreateBody(boxWorld, &padBodyDef);

        b2ShapeDef padShapeDef = b2DefaultShapeDef();
        padShapeDef.enableSensorEvents = true;
        padShapeDef.density = 1;

        b2Polygon padBox = b2MakeBox((r1.w*CHARACTER_TEX_SCALE/BOX_SCALE)/2, (r1.h*CHARACTER_TEX_SCALE/BOX_SCALE)/2);
        b2CreatePolygonShape(padBody, &padShapeDef, &padBox);

        Entity e = Entity::create();
        e.addAll(
            Position{{},0},
            Drawable{{r1,r2}, {r1.w*CHARACTER_TEX_SCALE, r1.h*CHARACTER_TEX_SCALE},0},
            Collider{padBody},
            Intent{},
            Ghost{}
        );
        b2Body_SetUserData(padBody, new ent_type{e.entity()});
    }

    /**
    * @brief Creates a pellet entity (normal or power) at the specified position.
    * @param p Position of the pellet.
    */
    void PacMan::createPellet(SDL_FPoint p) {
        // 1. Create a static body
        b2BodyDef pelletBodyDef = b2DefaultBodyDef();
        pelletBodyDef.type = b2_staticBody;
        pelletBodyDef.position = {p.x / BOX_SCALE, p.y / BOX_SCALE};
        b2BodyId pelletBody = b2CreateBody(boxWorld, &pelletBodyDef);

        // 2. Define shape
        b2ShapeDef pelletShapeDef = b2DefaultShapeDef();
        pelletShapeDef.enableSensorEvents = true;

        pelletShapeDef.density = 1; // Not needed for static, but harmless

        b2Circle pelletCircle = {0,0,PELLET.w*CHARACTER_TEX_SCALE/BOX_SCALE/2};
        b2CreateCircleShape(pelletBody, &pelletShapeDef, &pelletCircle);

        // 3. Create and assign components
        Entity e = Entity::create();
        e.addAll(
            Position{{}, 0},
            Drawable{{PELLET,{}}, {PELLET.w * CHARACTER_TEX_SCALE, PELLET.h * CHARACTER_TEX_SCALE}, 0},
            Collider{pelletBody},
            Pellet{ePelletState::Normal}
        );
        b2Body_SetUserData(pelletBody, new ent_type{e.entity()});
    }

    /**
    * @brief Creates a wall entity in the game world.
    * @param p Center position of the wall.
    * @param w Width of the wall.
    * @param h Height of the wall.
    */
    void PacMan::createWall(SDL_FPoint p, float w, float h)
    {
        const float width = w;
        const float height = h;


        b2BodyDef wallBodyDef = b2DefaultBodyDef();
        wallBodyDef.type = b2_staticBody;
        wallBodyDef.position = {p.x / BOX_SCALE, p.y / BOX_SCALE};

        b2BodyId wallBody = b2CreateBody(boxWorld, &wallBodyDef);

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.enableSensorEvents = true;
        shapeDef.isSensor = true;
        shapeDef.density = 1; // Not needed for static, but harmless

        b2Polygon box = b2MakeBox(width / 2.0f / BOX_SCALE, height / 2.0f / BOX_SCALE);
        b2ShapeId shape = b2CreatePolygonShape(wallBody, &shapeDef, &box);

        Entity e = Entity::create();
        e.addAll(
                Position{p, 0},
                Collider{wallBody},
                Wall{shape, {width, height}}
        );
        b2Body_SetUserData(wallBody, new ent_type{e.entity()});
    }

    /**
    * @brief Creates a static background entity.
    */
    void PacMan::createBackground()
    {
        Entity::create().addAll(
                Position{{(BOARD.w / 2.f) * CHARACTER_TEX_SCALE, (BOARD.h / 2.f)*CHARACTER_TEX_SCALE}, 0},
                Drawable{{BOARD, {}}, {BOARD.w * CHARACTER_TEX_SCALE, BOARD.h * CHARACTER_TEX_SCALE}, 0},
                Background{}
        );
    }


    /**
    * @brief Initializes the SDL window, renderer, and loads Pac-Man texture.
    * @return True on success, false on failure.
    */
    bool PacMan::prepareWindowAndTexture()
    {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            cout << SDL_GetError() << endl;
            return false;
        }
        if (!SDL_CreateWindowAndRenderer(
            "Pac-Man", BOARD.w*CHARACTER_TEX_SCALE, BOARD.h*CHARACTER_TEX_SCALE + OPEN_PACMAN.h+(15*CHARACTER_TEX_SCALE), 0, &win, &ren)) {
            cout << SDL_GetError() << endl;
            return false;
        }
        SDL_Surface *surf = IMG_Load("res/Pac-Man.png");
        if (surf == nullptr) {
            cout << SDL_GetError() << endl;
            return false;
        }

        tex = SDL_CreateTextureFromSurface(ren, surf);
        if (tex == nullptr) {
            cout << SDL_GetError() << endl;
            return false;
        }

        SDL_DestroySurface(surf);
        return true;
    }

    /**
     * @brief Initializes the Box2D world with zero gravity.
     */
    void PacMan::prepareBoxWorld()
    {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {0,0};
        boxWorld = b2CreateWorld(&worldDef);
    }

    /**
    * @brief Places pellets at predefined positions throughout the board.
    */
    void PacMan::preparePellets()
    {
        float space = 8.f;

        for (int i = 0 ; i < 12; ++i) {
            createPellet({ ((space * i) + 13.f) * CHARACTER_TEX_SCALE, 13.f * CHARACTER_TEX_SCALE});
        }
        for (int i = 0 ; i < 12; ++i) {
            createPellet({ ((space * i) + 125.f) * CHARACTER_TEX_SCALE, 13.f * CHARACTER_TEX_SCALE});
        }


        for (int i = 0 ; i < 26; ++i) {
            if ((i >= 1 && i <= 4) || (i >= 6 && i <= 10) || (i >= 12 && i <= 13) || (i >= 15 && i <= 19) || (i >= 21 && i <= 24)) {
                continue;
            }
            createPellet({ ((space * i) + 13.f) * CHARACTER_TEX_SCALE, 28.f * CHARACTER_TEX_SCALE});
        }


        for (int i = 0 ; i < 26; ++i) {
            createPellet({ ((space * i) + 13.f) * CHARACTER_TEX_SCALE, 45.f * CHARACTER_TEX_SCALE});
        }


        for (int i = 0 ; i < 26; ++i) {
            if ((i >= 1 && i <= 4) ||(i >= 6 && i <= 7) || (i >= 9 && i <= 16) || (i >= 18 && i <= 19) || (i >= 21 && i <= 24)) {
                continue;
            }
            createPellet({ ((space * i) + 13.f) * CHARACTER_TEX_SCALE, 58.f * CHARACTER_TEX_SCALE});
        }

        for (int i = 0 ; i < 26; ++i) {
            if (i == 6 || i == 7 ||  i == 12 || i == 13 || i == 19 || i == 18) {
                continue;
            }
            createPellet({ ((space * i) + 13.f) * CHARACTER_TEX_SCALE, 69.f * CHARACTER_TEX_SCALE});
        }

        for (int i = 0 ; i < 26; ++i) {
            if (i == 12 || i == 13) {
                continue;
            }
            createPellet({ ((space * i) + 13.f) * CHARACTER_TEX_SCALE, 165.f * CHARACTER_TEX_SCALE});
        }
        for (int i = 0 ; i < 26; ++i) {
            if ((i >= 1 && i <= 4) ||(i >= 6 && i <= 10) || (i >= 12 && i <= 13) || (i >= 15 && i <= 19) || (i >= 21 && i <= 24)) {
                continue;
            }
            createPellet({ ((space * i) + 13.f) * CHARACTER_TEX_SCALE, 177.f * CHARACTER_TEX_SCALE});
        }
        for (int i = 0 ; i < 26; ++i) {
            if ((i >= 3 && i <= 4) || (i >= 21 && i <= 22)) {
                continue;
            }
            createPellet({ ((space * i) + 13.f) * CHARACTER_TEX_SCALE, 189.f * CHARACTER_TEX_SCALE});
        }

        for (int i = 0 ; i < 26; ++i) {
            if ((i >= 6 && i <= 7) || (i >= 12 && i <= 13) || (i >= 18 && i <= 19)) {
                continue;
            }
            createPellet({ ((space * i) + 13.f) * CHARACTER_TEX_SCALE, 213.f * CHARACTER_TEX_SCALE});
        }

        for (int i = 0 ; i < 26; ++i) {
            if ((i >= 1 && i <= 10) || (i >= 12 && i <= 13) || (i >= 15 && i <= 24)) {
                continue;
            }
            createPellet({ ((space * i) + 13.f) * CHARACTER_TEX_SCALE, 225.f * CHARACTER_TEX_SCALE});
        }

        for (int i = 2 ; i < 26; ++i) {
            createPellet({ ((space * i) + 13.f) * CHARACTER_TEX_SCALE, 237.f * CHARACTER_TEX_SCALE});
        }

        for (int i = 0 ; i < 11; ++i) {
            createPellet({ 53.f * CHARACTER_TEX_SCALE, (77.f+ (space * i)) * CHARACTER_TEX_SCALE});

        }
        for (int i = 0 ; i < 11; ++i) {
            createPellet({ 173.f * CHARACTER_TEX_SCALE, (77.f+ (space * i)) * CHARACTER_TEX_SCALE});
        }



    }

    /**
    * @brief Creates all wall entities for the maze layout, including borders and inner structures.
    */
    void PacMan::prepareWalls()
    {
        //upper and lower borders
        createWall({WIN_WIDTH  / 2.0f, 11.0f},BOARD.w * CHARACTER_TEX_SCALE,5.f);
        createWall({WIN_WIDTH  / 2.0f, WIN_HEIGHT - 11.f},BOARD.w * CHARACTER_TEX_SCALE,5.f);
        //side borders
        createWall({12.0f, WIN_HEIGHT / 2.0f},5.f, BOARD.h * CHARACTER_TEX_SCALE);
        createWall({WIN_WIDTH - 12.f, WIN_HEIGHT / 2.0f},5.f, BOARD.h * CHARACTER_TEX_SCALE);

        //Top middle
        createWall({WIN_WIDTH / 2.f, 10.0f},29.f, 65 * CHARACTER_TEX_SCALE);

        //Left box 1
        createWall({31 * CHARACTER_TEX_SCALE, 28 * CHARACTER_TEX_SCALE},22 * CHARACTER_TEX_SCALE, 15 * CHARACTER_TEX_SCALE);
        //Top second left
        createWall({75 * CHARACTER_TEX_SCALE, 28 * CHARACTER_TEX_SCALE},32 * CHARACTER_TEX_SCALE, 15 * CHARACTER_TEX_SCALE);
        //Left box 2
        createWall({31 * CHARACTER_TEX_SCALE, 57 * CHARACTER_TEX_SCALE},22 * CHARACTER_TEX_SCALE, 6.5 * CHARACTER_TEX_SCALE);
        //Left box 3
        createWall({14 * CHARACTER_TEX_SCALE, 94 * CHARACTER_TEX_SCALE},60 * CHARACTER_TEX_SCALE, 32 * CHARACTER_TEX_SCALE);
        //Left box 4
        createWall({14 * CHARACTER_TEX_SCALE, 142 * CHARACTER_TEX_SCALE},60 * CHARACTER_TEX_SCALE, 32 * CHARACTER_TEX_SCALE);
        //Left hor 5
        createWall({33 * CHARACTER_TEX_SCALE, 180 * CHARACTER_TEX_SCALE},24 * CHARACTER_TEX_SCALE, 6.5 * CHARACTER_TEX_SCALE);
        //Left hor 6
        createWall({13 * CHARACTER_TEX_SCALE, 205 * CHARACTER_TEX_SCALE},15 * CHARACTER_TEX_SCALE, 6.5 * CHARACTER_TEX_SCALE);
        //Left 7
        createWall({56 * CHARACTER_TEX_SCALE, 230 * CHARACTER_TEX_SCALE},72 * CHARACTER_TEX_SCALE, 6.5 * CHARACTER_TEX_SCALE);
        //Left Vert7
        createWall({63 * CHARACTER_TEX_SCALE, 210 * CHARACTER_TEX_SCALE},7 * CHARACTER_TEX_SCALE, 14 * CHARACTER_TEX_SCALE);
        //Left Vert6
        createWall({40 * CHARACTER_TEX_SCALE, 196 * CHARACTER_TEX_SCALE},6 * CHARACTER_TEX_SCALE, 20 * CHARACTER_TEX_SCALE);
        //Left Vert5
        createWall({65 * CHARACTER_TEX_SCALE, 142 * CHARACTER_TEX_SCALE},7 * CHARACTER_TEX_SCALE, 30 * CHARACTER_TEX_SCALE);
        //Left Vert4
        createWall({65 * CHARACTER_TEX_SCALE, 81 * CHARACTER_TEX_SCALE},7 * CHARACTER_TEX_SCALE, 53 * CHARACTER_TEX_SCALE);
        //Left2 hor 1
        createWall({76 * CHARACTER_TEX_SCALE, 82 * CHARACTER_TEX_SCALE},30 * CHARACTER_TEX_SCALE, 6 * CHARACTER_TEX_SCALE);
        //Left2 hor 2
        createWall({76 * CHARACTER_TEX_SCALE, 179 * CHARACTER_TEX_SCALE},30 * CHARACTER_TEX_SCALE, 6 * CHARACTER_TEX_SCALE);

        //---------------------------------------------------------
        //Right box 1
        createWall({( (BOARD.w - 31) * CHARACTER_TEX_SCALE ), 28 * CHARACTER_TEX_SCALE}, 22 * CHARACTER_TEX_SCALE, 15 * CHARACTER_TEX_SCALE);
        //Top second right
        createWall({( (BOARD.w - 75) * CHARACTER_TEX_SCALE ), 28 * CHARACTER_TEX_SCALE}, 32 * CHARACTER_TEX_SCALE, 15 * CHARACTER_TEX_SCALE);
        //Right box 2
        createWall({( (BOARD.w - 31) * CHARACTER_TEX_SCALE ), 57 * CHARACTER_TEX_SCALE}, 22 * CHARACTER_TEX_SCALE, 6.5 * CHARACTER_TEX_SCALE);
        //Right box 3
        createWall({( (BOARD.w - 14) * CHARACTER_TEX_SCALE ), 94 * CHARACTER_TEX_SCALE}, 60 * CHARACTER_TEX_SCALE, 32 * CHARACTER_TEX_SCALE);
        //Right box 4
        createWall({( (BOARD.w - 14) * CHARACTER_TEX_SCALE ), 142 * CHARACTER_TEX_SCALE}, 60 * CHARACTER_TEX_SCALE, 32 * CHARACTER_TEX_SCALE);
        //Right hor 5
        createWall({( (BOARD.w - 33) * CHARACTER_TEX_SCALE ), 180 * CHARACTER_TEX_SCALE}, 24 * CHARACTER_TEX_SCALE, 6.5 * CHARACTER_TEX_SCALE);
        //Right hor 6
        createWall({( (BOARD.w - 13) * CHARACTER_TEX_SCALE ), 205 * CHARACTER_TEX_SCALE}, 15 * CHARACTER_TEX_SCALE, 6.5 * CHARACTER_TEX_SCALE);
        //Right 7
        createWall({( (BOARD.w - 56) * CHARACTER_TEX_SCALE ), 230 * CHARACTER_TEX_SCALE}, 72 * CHARACTER_TEX_SCALE, 6.5 * CHARACTER_TEX_SCALE);
        //Right Vert7
        createWall({( (BOARD.w - 63) * CHARACTER_TEX_SCALE ), 210 * CHARACTER_TEX_SCALE}, 7 * CHARACTER_TEX_SCALE, 14 * CHARACTER_TEX_SCALE);
        //Right Vert6
        createWall({( (BOARD.w - 40) * CHARACTER_TEX_SCALE ), 196 * CHARACTER_TEX_SCALE}, 6 * CHARACTER_TEX_SCALE, 20 * CHARACTER_TEX_SCALE);
        //Right Vert5
        createWall({( (BOARD.w - 65) * CHARACTER_TEX_SCALE ), 142 * CHARACTER_TEX_SCALE}, 7 * CHARACTER_TEX_SCALE, 30 * CHARACTER_TEX_SCALE);
        //Right Vert4
        createWall({( (BOARD.w - 65) * CHARACTER_TEX_SCALE ), 81 * CHARACTER_TEX_SCALE}, 7 * CHARACTER_TEX_SCALE, 53 * CHARACTER_TEX_SCALE);
        //Right2 hor 1
        createWall({( (BOARD.w - 76) * CHARACTER_TEX_SCALE ), 82 * CHARACTER_TEX_SCALE}, 30 * CHARACTER_TEX_SCALE, 6 * CHARACTER_TEX_SCALE);
        //Right2 hor 2
        createWall({( (BOARD.w - 76) * CHARACTER_TEX_SCALE ), 179 * CHARACTER_TEX_SCALE}, 30 * CHARACTER_TEX_SCALE, 6 * CHARACTER_TEX_SCALE);

        //---------------------------------------------------------
        //top middle vertical
        createWall({WIN_WIDTH / 2.f, 69 * CHARACTER_TEX_SCALE},8 * CHARACTER_TEX_SCALE, 32 * CHARACTER_TEX_SCALE);
        //Top middle Horizontal
        createWall({WIN_WIDTH / 2.f, 57 * CHARACTER_TEX_SCALE},56 * CHARACTER_TEX_SCALE, 8 * CHARACTER_TEX_SCALE);

        //Top middle Horizontal 2
        createWall({WIN_WIDTH / 2.f, 154 * CHARACTER_TEX_SCALE},56 * CHARACTER_TEX_SCALE, 6.5 * CHARACTER_TEX_SCALE);
        //Top middle Vertical 2
        createWall({WIN_WIDTH / 2.f, 166 * CHARACTER_TEX_SCALE},8 * CHARACTER_TEX_SCALE, 32 * CHARACTER_TEX_SCALE);

        //Top middle Horizontal 3
        createWall({WIN_WIDTH / 2.f, 204 * CHARACTER_TEX_SCALE},56 * CHARACTER_TEX_SCALE, 6.5 * CHARACTER_TEX_SCALE);
        //Top middle Vertical 3
        createWall({WIN_WIDTH / 2.f, 216 * CHARACTER_TEX_SCALE},8 * CHARACTER_TEX_SCALE, 32 * CHARACTER_TEX_SCALE);

        //bottom of middle box
        createWall({WIN_WIDTH / 2.f, 130 * CHARACTER_TEX_SCALE},56 * CHARACTER_TEX_SCALE, 6.5 * CHARACTER_TEX_SCALE);
        //left of middle box
        createWall({87 * CHARACTER_TEX_SCALE, 118 * CHARACTER_TEX_SCALE},7 * CHARACTER_TEX_SCALE, 30 * CHARACTER_TEX_SCALE);
        //right of middle box
        createWall({136 * CHARACTER_TEX_SCALE, 118 * CHARACTER_TEX_SCALE},7 * CHARACTER_TEX_SCALE, 30 * CHARACTER_TEX_SCALE);
    }
    /**
    * @brief Constructs the PacMan game instance, initializing systems, walls, pellets, and entities.
    */
    PacMan::PacMan()
    {
        if (!prepareWindowAndTexture())
            return;
        SDL_srand(time(nullptr));

        prepareBoxWorld();
        prepareWalls();

        createBackground();
        preparePellets();

        createPacMan(3);

        createGhost(BLUE_GHOST_DDOWN,BLUE_GHOST_DOWN_1,{100 * CHARACTER_TEX_SCALE, 120.f * CHARACTER_TEX_SCALE});
        createGhost(PINK_GHOST_LEFT,PINK_GHOST_LEFT_1,{(110 + PINK_GHOST_DDOWN.w)*CHARACTER_TEX_SCALE, 120.f * CHARACTER_TEX_SCALE});
        createGhost(RED_GHOST_UP,RED_GHOST_UP_1, {100 * CHARACTER_TEX_SCALE, (120.f - (RED_GHOST_DDOWN.h + 15)) * CHARACTER_TEX_SCALE});
        createGhost(ORANGE_GHOST_RIGHT,ORANGE_GHOST_RIGHT_1,{(110 + PINK_GHOST_DDOWN.w)*CHARACTER_TEX_SCALE, (120.f - (RED_GHOST_DDOWN.h + 15)) * CHARACTER_TEX_SCALE});
    }
    /**
    * @brief Cleans up and destroys SDL and Box2D resources.
    */
    PacMan::~PacMan()
    {
        if (b2World_IsValid(boxWorld))
            b2DestroyWorld(boxWorld);
        if (tex != nullptr)
            SDL_DestroyTexture(tex);
        if (ren != nullptr)
            SDL_DestroyRenderer(ren);
        if (win != nullptr)
            SDL_DestroyWindow(win);

        SDL_Quit();
    }
    /**
    * @brief Main game loop that processes input, updates logic, renders, and handles events.
    */
    void PacMan::run()
    {
        SDL_SetRenderDrawColor(ren, 0,0,0,255);
        auto start = SDL_GetTicks();
        bool quit = false;

        while (!quit) {

            InputSystem();
            AISystem();
            MovementSystem();
            box_system();
            CollisionSystem();
            RenderSystem();

            auto end = SDL_GetTicks();
            if (end-start < GAME_FRAME) {
                SDL_Delay(GAME_FRAME - (end-start));
            }
            start += GAME_FRAME;

            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_EVENT_QUIT)
                    quit = true;
                else if ((e.type == SDL_EVENT_KEY_DOWN) && (e.key.scancode == SDL_SCANCODE_ESCAPE))
                    quit = true;
            }
        }
    }
}// namespace PacMan
