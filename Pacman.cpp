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

    void PacMan::prepareWalls()
    {
        b2BodyDef wallBodyDef = b2DefaultBodyDef();
        wallBodyDef.type = b2_staticBody;

        b2ShapeDef wallShapeDef = b2DefaultShapeDef();
        wallShapeDef.density = 1;
        b2Polygon wallBox;

        wallBox = b2MakeBox(WIN_WIDTH/2/BOX_SCALE, 1);
        wallBodyDef.position = {WIN_WIDTH/2/BOX_SCALE,-1};
        b2BodyId wall = b2CreateBody(boxWorld, &wallBodyDef);
        b2CreatePolygonShape(wall, &wallShapeDef, &wallBox);

        wallBodyDef.position = {WIN_WIDTH/2/BOX_SCALE, WIN_HEIGHT/BOX_SCALE +1};
        wall = b2CreateBody(boxWorld, &wallBodyDef);
        b2CreatePolygonShape(wall, &wallShapeDef, &wallBox);

        wallShapeDef.isSensor = true;
        wallShapeDef.enableSensorEvents = true;

        wallBox = b2MakeBox(5, WIN_HEIGHT/2/BOX_SCALE);
        wallBodyDef.position = {0,WIN_HEIGHT/2/BOX_SCALE};
        wall = b2CreateBody(boxWorld, &wallBodyDef);
        b2ShapeId wallShape = b2CreatePolygonShape(wall, &wallShapeDef, &wallBox);

        Entity e = Entity::create();
        e.addAll(
            Wall{wallShape}
        );
        wallBodyDef.position = {WIN_WIDTH/BOX_SCALE, WIN_HEIGHT/2/BOX_SCALE};
        wall = b2CreateBody(boxWorld, &wallBodyDef);
        wallShape = b2CreatePolygonShape(wall, &wallShapeDef, &wallBox);

        Entity e1 = Entity::create();
        e1.addAll(
            Wall{wallShape}
        );

    }

    /**
     * @brief Handles movement logic for entities with Position, Direction, and Speed.
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
                const auto& i = World::getComponent<Intent>(e);
                const auto& c = World::getComponent<Collider>(e);
                bool isPlayer = World::mask(e).test(Component<PlayerControlled>::Bit);

                const float y = i.up ? -30 : i.down ? 30 : 0;
                const float x = i.left ? -30 : i.right ? 30 : 0;

                b2Body_SetLinearVelocity(c.b, {x,y});
                if (isPlayer) {
                    if (i.up) {
                        b2Body_SetTransform(c.b, b2Body_GetPosition(c.b), {0.0f, -1.0f});
                    }else if (i.down) {
                        b2Body_SetTransform(c.b, b2Body_GetPosition(c.b), {0.0f, 1.0f});
                    } else if (i.left) {
                        b2Body_SetTransform(c.b, b2Body_GetPosition(c.b), {-1.0f, 0.0f});
                    }else if (i.right) {
                        b2Body_SetTransform(c.b, b2Body_GetPosition(c.b), {1.0f, 0.0f});
                    }
                }
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
                    auto& in = World::getComponent<Intent>(e);
                    if (keys[k.up] || keys[k.down] || keys[k.left] || keys[k.right]) {
                        in.up = keys[k.up];
                        in.down = keys[k.down];
                        in.left = keys[k.left];
                        in.right = keys[k.right];
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
        //SDL_RenderTexture(ren, tex, &BOARD, nullptr);
        for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
            if (World::mask(e).test(mask)) {
                const auto& t = World::getComponent<Position>(e);
                auto& d = World::getComponent<Drawable>(e);
                bool ball = World::mask(e).test(Component<PlayerControlled>::Bit);
                bool ghost = World::mask(e).test(Component<Ghost>::Bit);
                if (ball || ghost) {
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
     * @brief Detects and handles collisions between entities.
     */
    // class CollisionSystem
    // {
    // public:
    //     void update() {
    //
    //         for (int i = 0; i < _entities.size(); ++i)
    //         {
    //             ent_type e = _entities[i];
    //             if (!World::mask(e).test(required_mask))
    //             {
    //                 _entities[i] = _entities[_entities.size()-1];
    //                 _entities.pop();
    //                 --i;
    //                 continue;
    //             }
    //             else
    //             {
    //                 //Logic
    //
    //                 const auto& col1 = World::getComponent<Collider>(e);
    //                 b2Transform t1 = b2Body_GetTransform(col1.b);
    //                 for (int i = 0 ; i < _walls_entities.size(); ++i) {
    //                     ent_type wall = _walls_entities[i];
    //
    //                 }
    //             }
    //         }
    //     }
    //
    //     void updateEntities(){
    //         for (int i = 0; i < World::sizeAdded(); ++i) {
    //             const AddedMask& am = World::getAdded(i);
    //             if ((!am.prev.test(required_mask)) && (am.next.test(required_mask))) {
    //                 _entities.push(am.e);
    //             }
    //             if ((!am.prev.test(walls_mask)) && (am.next.test(walls_mask))) {
    //                 _walls_entities.push(am.e);
    //             }
    //             if ((!am.prev.test(pellet_mask)) && (am.next.test(pellet_mask))) {
    //                 _pellet_entities.push(am.e);
    //             }
    //         }
    //     }
    //
    //     CollisionSystem()
    //     {
    //         required_mask = MaskBuilder()
    //         .set<Collider>()
    //         .build();
    //         walls_mask = MaskBuilder()
    //         .set<Wall>()
    //         .build();
    //         pellet_mask = MaskBuilder()
    //         .set<Pellet>()
    //         .build();
    //
    //         for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
    //             if (World::mask(e).test(walls_mask)) {
    //                 _walls_entities.push(e);
    //             }
    //             if (World::mask(e).test(pellet_mask)) {
    //                 _pellet_entities.push(e);
    //             }
    //             if (World::mask(e).test(required_mask)) {
    //                 _entities.push(e);
    //             }
    //         }
    //     }
    // private:
    //     Bag<ent_type,100> _entities;
    //     Bag<ent_type,100> _walls_entities;
    //     Bag<ent_type,100> _pellet_entities;
    //     Mask required_mask;
    //     Mask walls_mask;
    //     Mask pellet_mask;
    // };
    //TODO
     void PacMan::CollisionSystem()
     {
         Mask required = MaskBuilder()
             .set<Collider>()
             .build();

         for (id_type id1 = 0; id1 <= World::maxId().id; ++id1)
         {
             ent_type e1{id1};
             if (!World::mask(e1).test(required)) {
                 continue;
             }

             b2ContactEvents contactEvents = b2World_GetContactEvents(boxWorld);
             for (int i = 0; i < contactEvents.hitCount; ++i) {
                 b2ContactHitEvent event = contactEvents.hitEvents[i];
                 b2ShapeId shapeA = event.shapeIdA;
                 b2ShapeId shapeB = event.shapeIdB;
                 // Handle significant collision between shapeA and shapeB
                 std::cout << "Hit " << "\n";
             }

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

                     if (World::mask(entity).test(Component<Ghost>::Bit)) {
                         auto& dir = World::getComponent<Intent>(entity);
                         dir.up = !dir.up;
                         dir.down = !dir.down;
                         dir.left = !dir.left;
                         dir.right = !dir.right;
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

    class AI{
    public:
        void update() {
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
                    auto& in = World::getComponent<Intent>(e);
                    auto& dr = World::getComponent<Drawable>(e);
                    if (dr.frame % 30 == 0) {
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

        void updateEntities(){
            for (int i = 0; i < World::sizeAdded(); ++i) {
                const AddedMask& am = World::getAdded(i);

                if ((!am.prev.test(mask)) && (am.next.test(mask))) {
                    _entities.push(am.e);
                }
            }
        }

        AI()
        {
            mask = MaskBuilder()
            .set<Ghost>()
            .set<Intent>()
            .set<Drawable>()
            .build();

            for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
                if (World::mask(e).test(mask)) {
                    _entities.push(e);
                }
            }
        }
    private:
        Bag<ent_type,100> _entities;
        Mask mask;
    };


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
        SDL_FPoint p = {WIN_WIDTH/2/BOX_SCALE, WIN_HEIGHT/2/BOX_SCALE}; // TODO change to starting position

        // 1. Create a static body
        b2BodyDef pacmanBodyDef = b2DefaultBodyDef();
        pacmanBodyDef.type = b2_kinematicBody;
        pacmanBodyDef.position = {p.x / BOX_SCALE, p.y / BOX_SCALE};
        b2BodyId pacmanBody = b2CreateBody(boxWorld, &pacmanBodyDef);

        // 2. Define shape
        b2ShapeDef pacmanShapeDef = b2DefaultShapeDef();
        pacmanShapeDef.density = 1; // Not needed for static, but harmless

        b2Circle pacmanCircle = {0,0,OPEN_PACMAN.w*CHARACTER_TEX_SCALE/BOX_SCALE/2};
        b2CreateCircleShape(pacmanBody, &pacmanShapeDef, &pacmanCircle);

        Entity e = Entity::create();
        e.addAll(
         Position{{},0},
         Drawable{{OPEN_PACMAN,CLOSE_PACMAN}, {OPEN_PACMAN.w*CHARACTER_TEX_SCALE, OPEN_PACMAN.h*CHARACTER_TEX_SCALE},0},
         Collider{pacmanBody},
         Intent{},
         Input{SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_RIGHT, SDL_SCANCODE_LEFT},
         PlayerControlled{},
         PlayerStats{0,3}
         );
    }

    /**
     * @brief Creates a ghost entity.
     * @param r1
     * @param p The starting position of the ghost.
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

        Entity::create().addAll(
            Position{{},0},
            Drawable{{r1,r2}, {r1.w*CHARACTER_TEX_SCALE, r1.h*CHARACTER_TEX_SCALE},0},
            Collider{padBody},
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
        Entity::create().addAll(
            Position{{}, 0},
            Drawable{{PELLET,{}}, {PELLET.w * CHARACTER_TEX_SCALE, PELLET.h * CHARACTER_TEX_SCALE}, 0},
            Collider{pelletBody},
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
            "Pac-Man", BOARD.w*CHARACTER_TEX_SCALE, BOARD.h*CHARACTER_TEX_SCALE, 0, &win, &ren)) {
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
        prepareWalls();

        createPacMan();
        createPellet({WIN_HEIGHT/4, WIN_WIDTH/4});
        createGhost(BLUE_GHOST_DDOWN,BLUE_GHOST_DOWN_1,{WIN_WIDTH/2 + 14*CHARACTER_TEX_SCALE, WIN_HEIGHT/2});
        createGhost(PINK_GHOST_LEFT,PINK_GHOST_LEFT_1,{WIN_WIDTH/2 - 14*CHARACTER_TEX_SCALE, WIN_HEIGHT/2});
        createGhost(RED_GHOST_UP,RED_GHOST_UP_1,{WIN_WIDTH/2 + 30*CHARACTER_TEX_SCALE, WIN_HEIGHT/2});
        createGhost(ORANGE_GHOST_RIGHT,ORANGE_GHOST_RIGHT_1,{WIN_WIDTH/2 - 30*CHARACTER_TEX_SCALE, WIN_HEIGHT/2});
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
        AI ai;

        while (!quit) {
            //first updateEntities() for all systems
            is.updateEntities();
            ai.updateEntities();
            //then update() for all systems
            ai.update();
            is.update();






            //finally World::step() to clear added() array
            World::step();

            //input_system();
            MovementSystem();
            box_system();
            CollisionSystem();
            // score_system();
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
