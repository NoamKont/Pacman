#pragma once
#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include "bagel.h"
/**
 * @file PacMan.h
 * @brief Declarations for the core components, systems, and entity factories of a Pac-Man game.
 *
 * This module defines Data components used by entities, systems that act on entities with specific component sets
 * and factory functions to create game entities like Pac-Man, ghosts, pellets, and walls.
 */

using namespace bagel;
namespace pacman{

    /**
     * @brief Represents the type of pellet.
     */
    enum class ePelletState {
        Normal,    ///< Regular pellet
        Power      ///< Power-up pellet (enables Pac-Man to eat ghosts)
    };




    /**
     * @brief Component representing an entity's position on the grid.
     */
    using Position = struct {SDL_FPoint p; float a;};
    // struct Position {
    //     int x = 0;
    //     int y = 0;
    // };

    /**
     * @brief Component indicating movement direction.
     */
    struct Direction {
        int dx = 0;
        int dy = 0;
    };

    /**
     * @brief Component storing an entity's movement speed.
     */
    struct Speed {
        float value = 1.0f;
    };

    /**
     * @brief Component representing sprite animation state for rendering.
     */
    using Drawable = struct { SDL_FRect part[2]; SDL_FPoint size; size_t frame; };

    // struct Sprite {
    //     eAnimationState state = eAnimationState::Idle;
    // };

    /**
     * @brief Component that defines an entity's hitbox size for collision detection.
     */
    using Collider = struct { b2BodyId b; };

    // struct Hitbox {
    //     int width = 1;
    //     int height = 1;
    // };

    /**
     * @brief Component that stores the last input from a player.
     */
    using Input = struct { SDL_Scancode up, down, right, left; };
    // struct Input {
    //     int keyCode = -1;
    // };

    /**
     * @brief Component that expresses the current intended action of an entity.
     */
    using Intent = struct { bool up, down, right, left; };

    // struct Intent {
    //     eAction action = eAction::None;
    // };

    /**
     * @brief Component for pellets, including whether it's a normal or power pellet.
     */
    struct Pellet {
        ePelletState type = ePelletState::Normal;
    };

    /**
     * @brief Component tracking score and lives for a player.
     */
    struct PlayerStats {
        int score = 0;
        int lives = 3;
    };

    /**
     * @brief Tag component to identify AI-controlled entities.
     */
    struct AI { };

    /**
     * @brief Tag component to identify player-controlled entities.
     */
    struct PlayerControlled { };

    /**
     * @brief Tag component to identify ghost entities.
     */
    struct Ghost { };

    /**
     * @brief Tag component for entities that can be eaten (e.g., pellets, ghosts in vulnerable state).
     */
    struct Eatable { };

    /**
     * @brief Tag component for wall entities.
     */
    struct Wall { };


    class PacMan {
    public:
        PacMan();
        ~PacMan();

        void run();
    private:

        bool valid();

        //void InputSystem();
        void DecisionSystem();
        void MovementSystem();
        void CollisionSystem();
        void PelletSystem();
        void RenderSystem();
        void ScoreSystem();

        void createPacMan();
        void createGhost(const SDL_FRect& r1, const SDL_FRect& r2, const SDL_FPoint& p);

        void createPellet(SDL_FPoint p);

        void createScore(Position pos);

        void createWall(SDL_FPoint p);

        void createBackground(Position pos);

        bool prepareWindowAndTexture();
        void prepareBoxWorld();
        void prepareWalls() const;

        static constexpr int	WIN_WIDTH = 1280;
        static constexpr int	WIN_HEIGHT = 800;
        static constexpr int	FPS = 60;

        static constexpr float	GAME_FRAME = 1000.f/FPS;
        static constexpr float	RAD_TO_DEG = 57.2958f;

        static constexpr float	BOX_SCALE = 10;
        static constexpr float	PELLET_SCALE = 5.f;
        static constexpr float	CHARACTER_TEX_SCALE = 5.f;
        static constexpr float	PAD_TEX_SCALE = 0.25f;

    	static constexpr SDL_FRect BOARD{ 0, 0, 226, 247 };

    	static constexpr SDL_FRect PELLET{ 19, 11, 2, 2 };
    	static constexpr SDL_FRect POWER_PELLET{ 7, 23, 9,9  };

		static constexpr SDL_FRect OPEN_PACMAN{ 456, 0, 13, 14 };
		static constexpr SDL_FRect CLOSE_PACMAN{ 472, 0, 13, 14 };

		static constexpr SDL_FRect RED_GHOST_RIGHT{ 457, 65, 14, 15 };
		static constexpr SDL_FRect RED_GHOST_RIGHT_1{ 473, 65, 14, 15 };

		static constexpr SDL_FRect RED_GHOST_LEFT{ 489, 65, 14, 15 };
		static constexpr SDL_FRect RED_GHOST_LEFT_1{ 505, 65, 14, 15 };

		static constexpr SDL_FRect RED_GHOST_UP{ 521, 65, 14, 15 };
		static constexpr SDL_FRect RED_GHOST_UP_1{ 537, 65, 14, 15 };

		static constexpr SDL_FRect RED_GHOST_DDOWN{ 553, 65, 14, 15 };
		static constexpr SDL_FRect RED_GHOST_DOWN_1{ 569, 65, 14, 15 };

		static constexpr SDL_FRect PINK_GHOST_RIGHT{ 457, 81, 14, 15 };
		static constexpr SDL_FRect PINK_GHOST_RIGHT_1{ 473, 81, 14, 15 };

		static constexpr SDL_FRect PINK_GHOST_LEFT{ 489, 81, 14, 15 };
		static constexpr SDL_FRect PINK_GHOST_LEFT_1{ 505, 81, 14, 15 };

		static constexpr SDL_FRect PINK_GHOST_UP{ 521, 81, 14, 15 };
		static constexpr SDL_FRect PINK_GHOST_UP_1{ 537, 81, 14, 15 };

		static constexpr SDL_FRect PINK_GHOST_DDOWN{ 553, 81, 14, 15 };
		static constexpr SDL_FRect PINK_GHOST_DOWN_1{ 569, 81, 14, 15 };

		static constexpr SDL_FRect BLUE_GHOST_RIGHT{ 457,97 , 14, 15 };
		static constexpr SDL_FRect BLUE_GHOST_RIGHT_1{ 473, 97, 14, 15 };

		static constexpr SDL_FRect BLUE_GHOST_LEFT{ 489, 97, 14, 15 };
		static constexpr SDL_FRect BLUE_GHOST_LEFT_1{ 505, 97, 14, 15 };

		static constexpr SDL_FRect BLUE_GHOST_UP{ 521, 97, 14, 15 };
		static constexpr SDL_FRect BLUE_GHOST_UP_1{ 537, 97, 14, 15 };

		static constexpr SDL_FRect BLUE_GHOST_DDOWN{ 553, 97, 14, 15 };
		static constexpr SDL_FRect BLUE_GHOST_DOWN_1{ 569, 97, 14, 15 };

		static constexpr SDL_FRect ORANGE_GHOST_RIGHT{ 457,113, 14, 15 };
		static constexpr SDL_FRect ORANGE_GHOST_RIGHT_1{ 473, 113, 14, 15 };

		static constexpr SDL_FRect ORANGE_GHOST_LEFT{ 489, 113, 14, 15 };
		static constexpr SDL_FRect ORANGE_GHOST_LEFT_1{ 505, 113, 14, 15 };

		static constexpr SDL_FRect ORANGE_GHOST_UP{ 521, 113, 14, 15 };
		static constexpr SDL_FRect ORANGE_GHOST_UP_1{ 537, 113, 14, 15 };

		static constexpr SDL_FRect ORANGE_GHOST_DDOWN{ 553, 113, 14, 15 };
		static constexpr SDL_FRect ORANGE_GHOST_DOWN_1{ 569, 113, 14, 15 };

        SDL_Texture* tex;
        SDL_Renderer* ren;
        SDL_Window* win;

        b2WorldId boxWorld = b2_nullWorldId;

    };
} // namespace PacMan
