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
    // class MovementSystem
    // {
    // public:
    //     void update() {
    //         for (int i = 0; i < _entities.size(); ++i) {
    //             ent_type e = _entities[i];
    //             if (!World::mask(e).test(mask)) {
    //                 _entities[i] = _entities[_entities.size()-1];
    //                 _entities.pop();
    //                 --i;
    //                 continue;
    //             }else {
    //                 auto& c = World::getComponent<Collider>(e);
    //                 auto& i = World::getComponent<Intent>(e);
    //
    //                 const float y = i.up ? -30 : i.down ? 30 : 0;
    //                 const float x = i.left ? -30 : i.right ? 30 : 0;
    //                 b2Body_SetLinearVelocity(c.b, {x,y});
    //             }
    //         }
    //     }
    //     void updateEntities() {
    //         for (int i = 0; i < World::sizeAdded(); ++i) {
    //             const AddedMask& am = World::getAdded(i);
    //
    //             if ((!am.prev.test(mask)) && (am.next.test(mask))) {
    //                 _entities.push(am.e);
    //             }
    //         }
    //     }
    //
    //     MovementSystem()
    //     {
    //         for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
    //             if (World::mask(e).test(mask)) {
    //                 _entities.push(e);
    //             }
    //         }
    //     }
    // private:
    //     Bag<ent_type,100> _entities;
    //
    //     static const inline Mask mask = MaskBuilder()
    //         .set<Intent>()
    //         .set<Collider>()
    //         .build();
    // };

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

            for (int i = 0; i < _entities.size(); ++i) {
                ent_type e = _entities[i];
                if (!World::mask(e).test(mask)) {
                    _entities[i] = _entities[_entities.size()-1];
                    _entities.pop();
                    --i;
                    continue;
                }else {
                    const auto& k = World::getComponent<Input>(e);
                    auto& i = World::getComponent<Intent>(e);


                    i.up = keys[k.up];
                    i.down = keys[k.down];
                    i.left = keys[k.left];
                    i.right = keys[k.right];
                }
            }
        }
        void updateEntities() {
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


    // void PacMan::InputSystem(){
    //     Mask required = MaskBuilder()
    //         .set<Input>()
    //         .set<PlayerControlled>()
    //         .build();
    //     for (id_type id = 0; id <= World::maxId().id; ++id) {
    //         ent_type e{id};
    //         if (World::mask(e).test(required)) {
    //             bool hasIntent = World::mask(e).test(Component<Intent>::Bit);
    //         }
    //     }
    // }

    /**
     * @brief Detects and handles collisions between entities.
     */
    class CollisionSystem
    {
        public:
            void update() {
                for (int i = 0; i < _entities.size(); ++i) {
                    ent_type e = _entities[i];

                    if (!World::mask(e).test(mask)) {
                        _entities[i] = _entities[_entities.size() - 1];
                        _entities.pop();
                        --i;
                        continue;
                    }
                    auto &p = World::getComponent<Position>(e);
                    auto &c = World::getComponent<Collider>(e);
                    bool isGhost = World::mask(e).test(Component<Ghost>::Bit);
                    bool isPlayer = World::mask(e).test(Component<PlayerControlled>::Bit);



                    // Loop over walls to check for contacts
                    for (int j = 0; j < _walls.size(); ++j) {
                        ent_type wall = _walls[j];
                        auto& wallBody = World::getComponent<Collider>(wall);

                        if (checkBox2DCollision(c.b, wallBody.b)) {
                            if (isGhost) {
                                redirectGhostRandomly(e);
                            } else if (isPlayer) {
                                // Stop Pac-Man
                                b2Body_SetLinearVelocity(c.b, {0,0});
                            }

                            break;
                        }
                    }
                }
            }

            void updateEntities() {
                for (int i = 0; i < World::sizeAdded(); ++i) {
                    const AddedMask& am = World::getAdded(i);

                    if ((!am.prev.test(mask)) && (am.next.test(mask))) {
                        _entities.push(am.e);
                    }
                }
            }

            CollisionSystem() {
                for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
                    if (World::mask(e).test(mask)) {
                        _entities.push(e);
                    }
                    if (World::mask(e).test(wallMask)) {
                        _walls.push(e);
                    }
                }
            }

        private:
            Bag<ent_type, 100> _entities;
            Bag<ent_type, 100> _walls;

            static const inline Mask mask = MaskBuilder()
                .set<Collider>()
                .set<Position>()
                .build();

            static const inline Mask wallMask = MaskBuilder()
                .set<Position>()
                .set<Drawable>()
                .set<Collider>()
                .set<Wall>()
                .build();
            bool checkBox2DCollision(b2BodyId e, b2BodyId wall) {

            }
            void redirectGhostRandomly(ent_type) {

            }
    };


    void PacMan::CollisionSystem() {
        Mask required = MaskBuilder()
            .set<Position>()
            .set<Collider>()
            .build();
        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type e{id};
            if (!World::mask(e).test(required)) {
                continue;
            }
            bool isGhost = World::mask(e).test(Component<Ghost>::Bit);
            bool isWall = World::mask(e).test(Component<Wall>::Bit);
            bool isPlayer = World::mask(e).test(Component<PlayerControlled>::Bit);
        }
    }

    /**
     * @brief Updates pellet-related logic such as state changes or consumption.
     */
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
     * @brief Prepares rendering data for entities with sprites and positions.
     */
    void PacMan::RenderSystem() {
        Mask required = MaskBuilder()
            .set<Position>()
            .set<Drawable>()
            .build();
        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type e{id};
            if (World::mask(e).test(required)) {
                bool hasPowerUP = World::mask(e).test(Component<Pellet>::Bit);
            }
        }
    }

    /**
     * @brief Handles AI and player decision-making logic.
     */
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
    Entity PacMan::createPacMan() {
        Entity e = Entity::create();
        e.addAll(
            Position{{},0},
            Direction{},
            Speed{},
            Drawable{},
            Collider{},
            Input{SDL_SCANCODE_UP,SDL_SCANCODE_DOWN,SDL_SCANCODE_RIGHT,SDL_SCANCODE_LEFT},
            Intent{},
            PlayerStats{},
            PlayerControlled{}
        );
        return e;
    }

    /**
     * @brief Creates a ghost entity.
     * @param pos The starting position of the ghost.
     * @return The created ghost entity.
     */
    Entity PacMan::createGhost() {
        Entity e = Entity::create();
        e.addAll(
            Position{},
            Direction{},
            Speed{},
            Drawable{},
            Collider{},
            Intent{},
            Ghost{},
            AI{},
            Eatable{}
        );
        return e;
    }

    /**
     * @brief Creates a pellet entity (normal or power-up).
     * @param pos The position of the pellet.
     * @return The created pellet entity.
     */
    Entity PacMan::createPellet(SDL_FPoint p) {
        Entity e = Entity::create();
        e.addAll(
            Position{},
            Drawable{},
            Collider{},
            Eatable{},
            Pellet{ePelletState::Normal}
        );
        return e;
    }

    /**
     * @brief Creates a wall entity.
     * @param pos The position of the wall.
     * @return The created wall entity.
     */
    Entity PacMan::createWall(SDL_FPoint p) {
        Entity e = Entity::create();
        e.addAll(
            Position{},
            Drawable{},
            Collider{},
            Wall{}
        );
        return e;
    }

    /**
     * @brief Creates a background tile entity.
     * @param pos The position of the background tile.
     * @return The created background entity.
     */
    Entity PacMan::createBackground(Position pos) {
        Entity e = Entity::create();
        e.addAll(
            pos,
            Drawable{}
        );
        return e;
    }

    /**
     * @brief Creates an entity for tracking player score.
     * @param pos The position of the score display.
     * @return The created score entity.
     */
    Entity PacMan::createScore(Position pos) {
        Entity e = Entity::create();
        e.addAll(
            pos,
            Drawable{},
            PlayerStats{}
        );
        return e;
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
        SDL_Surface *surf = IMG_Load("res/pong.png"); //TODO change to pacman
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
    void PacMan::prepareWalls() const {
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
        wallBodyDef.position = {-5,WIN_HEIGHT/2/BOX_SCALE};
        wall = b2CreateBody(boxWorld, &wallBodyDef);
        b2ShapeId wallShape = b2CreatePolygonShape(wall, &wallShapeDef, &wallBox);
        Entity::create().addAll(
            Scorer{wallShape}
        );

        wallBodyDef.position = {WIN_WIDTH/BOX_SCALE +5, WIN_HEIGHT/2/BOX_SCALE};
        wall = b2CreateBody(boxWorld, &wallBodyDef);
        wallShape = b2CreatePolygonShape(wall, &wallShapeDef, &wallBox);
        Entity::create().addAll(
            Scorer{wallShape}
        );
    }
    PacMan::PacMan()
    {
        if (!prepareWindowAndTexture())
            return;
        SDL_srand(time(nullptr));

        prepareBoxWorld();
        prepareWalls();
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
