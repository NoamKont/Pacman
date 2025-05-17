#pragma once
#include <SDL3/SDL.h>
#include <box2d/box2d.h>

namespace pong
{
	using Transform = struct { SDL_FPoint p; float a; };
	using Drawable = struct { SDL_FRect part; SDL_FPoint size; };
	using Intent = struct { bool up, down; };
	using Keys = struct { SDL_Scancode up, down; };
	using Collider = struct { b2BodyId b; };
	using Scorer = struct { b2ShapeId s; };

	class Pong
	{
	public:
		Pong();
		~Pong();

		/// game loop
		void run();
		/// ensures initialization succeeded (ctor)
		bool valid() const;
	private:
		void input_system() const;
		void move_system() const;
		void box_system() const;
		void score_system() const;
		void draw_system() const;

		void createBall() const;
		void createPad(const SDL_FRect&, const SDL_FPoint&, const Keys&) const;

		bool prepareWindowAndTexture();
		void prepareBoxWorld();
		void prepareWalls() const;

		static constexpr int	WIN_WIDTH = 1280;
		static constexpr int	WIN_HEIGHT = 800;
		static constexpr int	FPS = 60;

		static constexpr float	GAME_FRAME = 1000.f/FPS;
		static constexpr float	RAD_TO_DEG = 57.2958f;

		static constexpr float	BOX_SCALE = 10;
		static constexpr float	BALL_TEX_SCALE = 0.5f;
		static constexpr float	PAD_TEX_SCALE = 0.25f;

		static constexpr SDL_FRect BALL_TEX = {404, 580, 76, 76};
		static constexpr SDL_FRect PAD1_TEX = {360, 4, 64, 532};
		static constexpr SDL_FRect PAD2_TEX = {456, 4, 64, 532};
		static constexpr SDL_FRect DOTS_TEX = {296, 20, 24, 24};

		SDL_Texture* tex;
		SDL_Renderer* ren;
		SDL_Window* win;

		b2WorldId boxWorld = b2_nullWorldId;
	};
}
