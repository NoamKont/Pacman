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

    bool PacMan::valid()
    {
        return tex != nullptr;
    }

    /**
     * @brief Handles movement logic for entities with Position, Direction, and Speed.
     */
    void PacMan::MovementSystem()
    {
        static const Mask mask = MaskBuilder()
            .set<Intent>()
            .set<Collider>()
            .build();

        for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
            if (World::mask(e).test(mask)) {
                const auto& i = World::getComponent<Intent>(e);
                const auto& c = World::getComponent<Collider>(e);

                const float y = i.up ? -30 : i.down ? 30 : 0;
                const float x = i.left ? -30 : i.right ? 30 : 0;

                b2Body_SetLinearVelocity(c.b, {x,y});
            }
        }
    }

    /**
     * @brief Processes user input for entities that are player-controlled.
     */

    class InputSystem
    {
    public:
        void update() {
            SDL_PumpEvents();
            const bool* keys = SDL_GetKeyboardState(nullptr);

            for (int i = 0; i < _entities.size(); ++i)
            {
                ent_type e = _entities[i];
                if (!World::mask(e).test(mask))
                {
                    _entities[i] = _entities[_entities.size()-1];
                    _entities.pop();
                    --i;
                    continue;
                }
                else
                {
                    const auto& k = World::getComponent<Input>(e);
                    auto& i = World::getComponent<Intent>(e);
                    if (keys[k.up] || keys[k.down] || keys[k.left] || keys[k.right]) {
                        i.up = keys[k.up];
                        i.down = keys[k.down];
                        i.left = keys[k.left];
                        i.right = keys[k.right];
                    }
                }
            }
        }

        void updateEntities(){
            for (int i = 0; i < World::sizeAdded(); ++i) {
                const AddedMask& am = World::getAdded(i);

                if ((!am.prev.test(mask)) && (am.next.test(mask))) {
                    _entities.push(am.e);
                }
            }
        }

        InputSystem()
        {
            for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
                if (World::mask(e).test(mask)) {
                    _entities.push(e);
                }
            }
        }
    private:
        Bag<ent_type,100> _entities;
        static const inline Mask mask = MaskBuilder()
            .set<Input>()
            .set<Intent>()
            .set<PlayerControlled>()
            .build();
    };

    /**
     * @brief Prepares rendering data for entities with sprites and positions.
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
                bool ball = World::mask(e).test(Component<PlayerControlled>::Bit);
                if (ball) {
                    d.frame++;
                    if (d.frame == 100)
                        d.frame = 0;
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
     * @brief Detects and handles collisions between entities.
     */
    //TODO
    void PacMan::CollisionSystem()
    {
        Mask required = MaskBuilder()
            .set<Position>()
            .set<Collider>()
            .build();

        for (id_type id1 = 0; id1 <= World::maxId().id; ++id1)
        {
            ent_type e1{id1};
            if (!World::mask(e1).test(required)) {
                continue;
            }

            const auto& pos1 = World::getComponent<Position>(e1);
            const auto& col1 = World::getComponent<Collider>(e1);

            b2Transform t1 = b2Body_GetTransform(col1.b);
            SDL_FRect rect1 = {
                    t1.p.x * BOX_SCALE, t1.p.y * BOX_SCALE,
                    16, 16  // Assuming tile size. Adjust if needed per entity.
            };

            for (id_type id2 = id1 + 1; id2 <= World::maxId().id; ++id2) {
                ent_type e2{id2};
                if (!World::mask(e2).test(required))
                    continue;

                const auto &pos2 = World::getComponent<Position>(e2);
                const auto &col2 = World::getComponent<Collider>(e2);

                b2Transform t2 = b2Body_GetTransform(col2.b);
                SDL_FRect rect2 = {
                        t2.p.x * BOX_SCALE, t2.p.y * BOX_SCALE,
                        16, 16
                };

                if (!SDL_HasRectIntersectionFloat(&rect1, &rect2))
                    continue;

                ///Check who intersect with who
                bool isPlayer1 = World::mask(e1).test(Component<PlayerControlled>::Bit);
                bool isPlayer2 = World::mask(e2).test(Component<PlayerControlled>::Bit);
                bool isGhost1 = World::mask(e1).test(Component<Ghost>::Bit);
                bool isGhost2 = World::mask(e2).test(Component<Ghost>::Bit);

                /// Player hit ghost - reduce life
                if ((isGhost1 && isPlayer2) || (isGhost2 && isPlayer1)) {
                    ent_type player = isPlayer1 ? e1 : e2;

                    if (World::mask(player).test(Component<PlayerStats>::Bit)) {
                        auto& stats = World::getComponent<PlayerStats>(player);
                        stats.lives -= 1;
                        std::cout << "Player hit by ghost! Lives left: " << stats.lives << "\n";
                    }
                }

                bool isWall1 = World::mask(e1).test(Component<Wall>::Bit);
                bool isWall2 = World::mask(e2).test(Component<Wall>::Bit);

                /// Handle wall collisions – stop movement or bounce back
                if ((isWall1 && (isPlayer2 || isGhost2)) || (isWall2 && (isPlayer1 || isGhost1))) {
                    ent_type entity = (isWall1 ? e2 : e1);

                    if (World::mask(entity).test(Component<Direction>::Bit)) {
                        auto& dir = World::getComponent<Direction>(entity);
                        ///Add a bounce back here
                    }

                    if (World::mask(entity).test(Component<Speed>::Bit)) {
                        auto& speed = World::getComponent<Speed>(entity);
                        speed.value = 0;
                    }
                }

                /// Handle pellet collisions - increase score/power up
                bool isPellet1 = World::mask(e1).test(Component<Pellet>::Bit);
                bool isPellet2 = World::mask(e2).test(Component<Pellet>::Bit);

                if ((isPlayer1 && isPellet2) || (isPlayer2 && isPellet1)) {
                    ent_type pellet = isPellet1 ? e1 : e2;
                    ent_type player = isPlayer1 ? e1 : e2;

                    ///Score handling
                    if (World::mask(player).test(Component<PlayerStats>::Bit)) {
                        auto& stats = World::getComponent<PlayerStats>(player);
                        const auto& pelletData = World::getComponent<Pellet>(pellet);

                        if (pelletData.type == ePelletState::Normal) {
                            stats.score += 10;
                        } else if (pelletData.type == ePelletState::Power) {
                            stats.score += 50;
                            // TODO: Set ghosts to vulnerable state (if implemented)
                        }
                    }

                    World::destroyEntity(pellet);
                }


            }











//            SDL_FRect r1 = World::getComponent<Collider>(e1).rect;
//
//
//            bool isGhost = World::mask(e).test(Component<Ghost>::Bit);
//            bool isWall = World::mask(e).test(Component<Wall>::Bit);
//            bool isPlayer = World::mask(e).test(Component<PlayerControlled>::Bit);
        }
    }

    /**
     * @brief Updates pellet-related logic such as state changes or consumption.
     */
    //TODO - Check if needed and how to use
    void PacMan::PelletSystem() {
        Mask required = MaskBuilder()
            .set<Pellet>()
            .set<Eatable>()
            .build();
        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type e{id};
            if (World::mask(e).test(required)) {
                bool isGhost = World::mask(e).test(Component<Ghost>::Bit);
            }
        }
    }

    /**
     * @brief Handles AI and player decision-making logic.
     */
    //TODO - Check if needed for ghosts
    void PacMan::DecisionSystem() {
        Mask required = MaskBuilder()
            .set<Position>()
            .set<Direction>()
            .build();
        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type e{id};
            if (World::mask(e).test(required)) {
                bool isAI = World::mask(e).test(Component<AI>::Bit);
                bool isRealPlayer = World::mask(e).test(Component<PlayerControlled>::Bit);
            }
        }
    }

    /**
     * @brief Updates and tracks player scores.
     */
    //TODO
    void PacMan::ScoreSystem() {
        Mask required = MaskBuilder()
            .set<PlayerStats>()
            .build();
        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type e{id};
            if (World::mask(e).test(required)) {
                bool isGhost = World::mask(e).test(Component<Ghost>::Bit);
                bool hasPowerUP = World::mask(e).test(Component<Pellet>::Bit);
            }
        }
    }

    /**
     * @brief Creates the player character (Pac-Man) entity.
     * @param pos The starting position of Pac-Man.
     * @return The created Pac-Man entity.
     */
    void PacMan::createPacMan() {
        b2BodyDef ballBodyDef = b2DefaultBodyDef();
        ballBodyDef.type = b2_kinematicBody;
        ballBodyDef.fixedRotation = false;
        ballBodyDef.position = {WIN_WIDTH/2/BOX_SCALE, WIN_HEIGHT/2/BOX_SCALE};//TODO change to start position

        b2ShapeDef ballShapeDef = b2DefaultShapeDef();
        ballShapeDef.enableSensorEvents = true;
        ballShapeDef.density = 1;
        ballShapeDef.material.friction = 0;
        ballShapeDef.material.restitution = 1.0f;
        b2Circle ballCircle = {0,0,CLOSE_PACMAN.w*CHARACTER_TEX_SCALE/BOX_SCALE/2};

        b2BodyId ballBody = b2CreateBody(boxWorld, &ballBodyDef);
        b2CreateCircleShape(ballBody, &ballShapeDef, &ballCircle);

        Entity e = Entity::create();
        e.addAll(
            Position{{},0},
            Drawable{},
            Collider{},
            Input{SDL_SCANCODE_UP,SDL_SCANCODE_DOWN,SDL_SCANCODE_RIGHT,SDL_SCANCODE_LEFT},
            Intent{},
            PlayerStats{},
            PlayerControlled{}
        );
    }

    /**
     * @brief Creates a ghost entity.
     * @param pos The starting position of the ghost.
     * @return The created ghost entity.
     */
    void PacMan::createGhost(const SDL_FRect& r1,const SDL_FRect& r2, const SDL_FPoint& p) {
        b2BodyDef padBodyDef = b2DefaultBodyDef();
        padBodyDef.type = b2_kinematicBody;
        padBodyDef.position = {p.x / BOX_SCALE, p.y / BOX_SCALE};
        b2BodyId padBody = b2CreateBody(boxWorld, &padBodyDef);

        b2ShapeDef padShapeDef = b2DefaultShapeDef();
        padShapeDef.density = 1;

        b2Polygon padBox = b2MakeBox(r1.w*PAD_TEX_SCALE/BOX_SCALE/2, r1.h*PAD_TEX_SCALE/BOX_SCALE/2);
        b2CreatePolygonShape(padBody, &padShapeDef, &padBox);

        Entity e = Entity::create();
        e.addAll(
            Position{{},0},
            Drawable{{r1,r2}, {r1.w*CHARACTER_TEX_SCALE, r1.h*CHARACTER_TEX_SCALE},0},
            Collider{},
            Intent{},
            Ghost{}
        );
    }

    /**
     * @brief Creates a pellet entity (normal or power-up).
     * @param pos The position of the pellet.
     * @return The created pellet entity.
     */
    //TODO - Check if works and how to add all pellets in the board automatically
    void PacMan::createPellet(SDL_FPoint p) {
        // 1. Create a static body
        b2BodyDef pelletBodyDef = b2DefaultBodyDef();
        pelletBodyDef.type = b2_staticBody;
        pelletBodyDef.position = {p.x / BOX_SCALE, p.y / BOX_SCALE};
        b2BodyId pelletBody = b2CreateBody(boxWorld, &pelletBodyDef);

        // 2. Define shape
        b2ShapeDef pelletShapeDef = b2DefaultShapeDef();
        pelletShapeDef.density = 1; // Not needed for static, but harmless

        b2Circle pelletCircle = {0,0,PELLET.w*CHARACTER_TEX_SCALE/BOX_SCALE/2};
        b2CreateCircleShape(pelletBody, &pelletShapeDef, &pelletCircle);

        // 3. Create and assign components
        Entity e = Entity::create();
        e.addAll(
            Position{{}, 0},
            Drawable{{PELLET,{}}, {PELLET.w * CHARACTER_TEX_SCALE, PELLET.h * CHARACTER_TEX_SCALE}, 0},
            Collider{pelletBody},
            Eatable{},
            Pellet{ePelletState::Normal}
        );
    }

    /**
     * @brief Creates a wall entity.
     * @param pos The position of the wall.
     * @return The created wall entity.
     */
    //TODO over the Pac-Man.png
    void PacMan::createWall(SDL_FPoint p) {
        Entity e = Entity::create();
        e.addAll(
            Position{},
            Drawable{},
            Collider{},
            Wall{}
        );
    }

    /**
     * @brief Creates a background tile entity.
     * @param pos The position of the background tile.
     * @return The created background entity.
     */
    //TODO - maybe do it in prepareWindowAnd.... function
    void PacMan::createBackground(Position pos) {
        Entity e = Entity::create();
        e.addAll(
            pos,
            Drawable{}
        );
    }

    /**
     * @brief Creates an entity for tracking player score.
     * @param pos The position of the score display.
     * @return The created score entity.
     */
    //TODO
    void PacMan::createScore(Position pos) {
        Entity e = Entity::create();
        e.addAll(
            pos,
            Drawable{},
            PlayerStats{}
        );
    }


    bool PacMan::prepareWindowAndTexture()
    {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            cout << SDL_GetError() << endl;
            return false;
        }

        if (!SDL_CreateWindowAndRenderer(
            "Pac-Man", WIN_WIDTH, WIN_HEIGHT, 0, &win, &ren)) {
            cout << SDL_GetError() << endl;
            return false;
            }
        SDL_Surface *surf = IMG_Load("res/pacman.jpg");
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
    void PacMan::prepareBoxWorld()
    {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {0,0};
        boxWorld = b2CreateWorld(&worldDef);
    }

    PacMan::PacMan()
    {
        if (!prepareWindowAndTexture())
            return;
        SDL_srand(time(nullptr));

        prepareBoxWorld();
        createPacMan();
    }

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

    void PacMan::run()
    {
        SDL_SetRenderDrawColor(ren, 0,0,0,255);
        auto start = SDL_GetTicks();
        bool quit = false;

        InputSystem is;

        while (!quit) {
            //first updateEntities() for all systems
            is.updateEntities();

            //then update() for all systems
            is.update();






            //finally World::step() to clear added() array
            World::step();

            //input_system();
            // move_system();
            // box_system();
            // score_system();
            // draw_system();

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
