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

    /**
     * @brief Component representing sprite animation state for rendering.
     */
    using Drawable = struct { SDL_FRect part[2]; SDL_FPoint size; size_t frame; };


    /**
     * @brief Component that defines an entity's hitbox size for collision detection.
     */
    using Collider = struct { b2BodyId b; };


    /**
     * @brief Component that stores the last input from a player.
     */
    using Input = struct { SDL_Scancode up, down, right, left; };


    /**
     * @brief Component that expresses the current intended action of an entity.
     */
    using Intent = struct {
        bool up = false, down = false, left = false, right = false;
        bool blockedUp = false, blockedDown = false, blockedLeft = false, blockedRight = false;
    };

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
     * @brief Tag component to identify player-controlled entities.
     */
    struct PlayerControlled { };

    /**
     * @brief Tag component to identify ghost entities.
     */
    struct Ghost { };

    /**
     * @brief Tag component for wall entities.
     */
    struct Wall {b2ShapeId s; SDL_FPoint size;};

	/**
	* @brief Tag component for Background entities.
	*/
	struct Background { };


    class PacMan {
    public:
        PacMan();
        ~PacMan();

        void run();

        bool valid();
	private:
        void InputSystem();
        void AISystem();
        void MovementSystem();
        void CollisionSystem();
        void RenderSystem();
        void box_system();
    	void EndGameSystem();

        void createPacMan(int lives);
        void createGhost(const SDL_FRect& r1, const SDL_FRect& r2, const SDL_FPoint& p);
        void createPellet(SDL_FPoint p);
        void createScore(float n_life);
        void createWall(SDL_FPoint p, float w, float h);
        void createBackground();

        bool prepareWindowAndTexture();
        void prepareBoxWorld();
        void prepareWalls();
    	void preparePellets();

        static constexpr SDL_FRect BOARD{ 227, 0, 226, 253 };
        static constexpr SDL_FRect PELLET{ 19, 11, 2, 2 };
        static constexpr SDL_FRect POWER_PELLET{ 7, 23, 9,9  };

        static constexpr float	BOX_SCALE = 10;
        static constexpr float	CHARACTER_TEX_SCALE = 2.9f;

        static constexpr int	WIN_WIDTH = BOARD.w * CHARACTER_TEX_SCALE;
        static constexpr int	WIN_HEIGHT = BOARD.h * CHARACTER_TEX_SCALE;
        static constexpr int	FPS = 60;

        static constexpr float	GAME_FRAME = 1000.f/FPS;
        static constexpr float	RAD_TO_DEG = 57.2958f;

    	static constexpr float	PAD_TEX_SCALE = 1.f;//0.25f;

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
