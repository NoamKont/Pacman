#include "Pong.h"
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <box2d/box2d.h>

#include "lib/box2d/src/body.h"
using namespace std;

#include "bagel.h"
using namespace bagel;

namespace pong
{
	bool Pong::valid() const
	{
		return tex != nullptr;
	}

	void Pong::createBall() const
	{
		b2BodyDef ballBodyDef = b2DefaultBodyDef();
		ballBodyDef.type = b2_dynamicBody;
		ballBodyDef.fixedRotation = false;
		ballBodyDef.position = {WIN_WIDTH/2/BOX_SCALE, WIN_HEIGHT/2/BOX_SCALE};

		b2ShapeDef ballShapeDef = b2DefaultShapeDef();
		ballShapeDef.enableSensorEvents = true;
		ballShapeDef.density = 1;
		ballShapeDef.material.friction = 0;
		ballShapeDef.material.restitution = 1.1f;
		b2Circle ballCircle = {0,0,BALL_TEX.w*BALL_TEX_SCALE/BOX_SCALE/2};

		b2BodyId ballBody = b2CreateBody(boxWorld, &ballBodyDef);
		b2CreateCircleShape(ballBody, &ballShapeDef, &ballCircle);

		float xs = SDL_randf()/2+.25f;
		if (SDL_rand(2))
			xs = -xs;
		float ys = SDL_sqrtf(1-xs*xs);
		if (SDL_rand(2))
			ys = -ys;
		b2Body_SetLinearVelocity(ballBody, {xs*30,ys*30});

		Entity ballEntity = Entity::create();
		ballEntity.addAll(
			Transform{{},0},
			Drawable{{BALL_TEX}, {BALL_TEX.w*BALL_TEX_SCALE, BALL_TEX.h*BALL_TEX_SCALE}},
			Collider{ballBody}
		);
		b2Body_SetUserData(ballBody, new ent_type{ballEntity.entity()});
	}
	void Pong::createPad(const SDL_FRect& r, const SDL_FPoint& p, const Keys& k) const
	{
		b2BodyDef padBodyDef = b2DefaultBodyDef();
		padBodyDef.type = b2_kinematicBody;
		padBodyDef.position = {p.x / BOX_SCALE, p.y / BOX_SCALE};
		b2BodyId padBody = b2CreateBody(boxWorld, &padBodyDef);

		b2ShapeDef padShapeDef = b2DefaultShapeDef();
		padShapeDef.density = 1;

		b2Polygon padBox = b2MakeBox(r.w*PAD_TEX_SCALE/BOX_SCALE/2, r.h*PAD_TEX_SCALE/BOX_SCALE/2);
		b2CreatePolygonShape(padBody, &padShapeDef, &padBox);

		Entity::create().addAll(
			Transform{{},0},
			Drawable{r, {r.w*PAD_TEX_SCALE, r.h*PAD_TEX_SCALE}},
			Collider{padBody},
			Intent{},
			k
		);
	}

	bool Pong::prepareWindowAndTexture()
	{
		if (!SDL_Init(SDL_INIT_VIDEO)) {
			cout << SDL_GetError() << endl;
			return false;
		}

		if (!SDL_CreateWindowAndRenderer(
			"Pong", WIN_WIDTH, WIN_HEIGHT, 0, &win, &ren)) {
			cout << SDL_GetError() << endl;
			return false;
		}
		SDL_Surface *surf = IMG_Load("res/pong.png");
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
	void Pong::prepareBoxWorld()
	{
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = {0,0};
		boxWorld = b2CreateWorld(&worldDef);
	}
	void Pong::prepareWalls() const
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

	class InputSystem
	{
	public:
		void update() {
			for (int i = 0; i < _entities.size(); ++i) {
				ent_type e = _entities[i];
				if (!World::mask(e).test(mask)) {
					_entities[i] = _entities[_entities.size()-1];
					_entities.pop();
					--i;
					continue;
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
				.set<Keys>()
				.set<Intent>()
				.build();
	};

	void Pong::input_system() const
	{
		static const Mask mask = MaskBuilder()
			.set<Keys>()
			.set<Intent>()
			.build();

		SDL_PumpEvents();
		const bool* keys = SDL_GetKeyboardState(nullptr);

		for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
			if (World::mask(e).test(mask)) {
				const auto& k = World::getComponent<Keys>(e);
				auto& i = World::getComponent<Intent>(e);

				i.up = keys[k.up];
				i.down = keys[k.down];
			}
		}
	}
	void Pong::move_system() const
	{
		static const Mask mask = MaskBuilder()
			.set<Intent>()
			.set<Collider>()
			.build();

		for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
			if (World::mask(e).test(mask)) {
				const auto& i = World::getComponent<Intent>(e);
				const auto& c = World::getComponent<Collider>(e);

				const float f = i.up ? -30 : i.down ? 30 : 0;
				b2Body_SetLinearVelocity(c.b, {0,f});
			}
		}
	}
	void Pong::box_system() const
	{
		static const Mask mask = MaskBuilder()
			.set<Collider>()
			.set<Transform>()
			.build();
		static constexpr float	BOX2D_STEP = 1.f/FPS;

		b2World_Step(boxWorld, BOX2D_STEP, 4);

		for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
			if (World::mask(e).test(mask)) {
				b2Transform t = b2Body_GetTransform(World::getComponent<Collider>(e).b);
				World::getComponent<Transform>(e) = {
					{t.p.x*BOX_SCALE, t.p.y*BOX_SCALE},
					RAD_TO_DEG * b2Rot_GetAngle(t.q)
				};
			}
		}
	}
	void Pong::score_system() const
	{
		const auto se = b2World_GetSensorEvents(boxWorld);
		for (int i = 0; i < se.endCount; ++i) {
			// score, recreate ball
			b2BodyId b = b2Shape_GetBody(se.endEvents[i].visitorShapeId);
			auto *e = static_cast<ent_type*>(b2Body_GetUserData(b));
			World::destroyEntity(*e);
			b2DestroyBody(b);

			createBall();
		}
	}
	void Pong::draw_system() const
	{
		static const Mask mask = MaskBuilder()
			.set<Transform>()
			.set<Drawable>()
			.build();

		SDL_RenderClear(ren);

		for (ent_type e{0}; e.id <= World::maxId().id; ++e.id) {
			if (World::mask(e).test(mask)) {
				const auto& d = World::getComponent<Drawable>(e);
				const auto& t = World::getComponent<Transform>(e);

				const SDL_FRect dst = {
					t.p.x-d.size.x/2,
					t.p.y-d.size.y/2,
					d.size.x, d.size.y};

				SDL_RenderTextureRotated(
					ren, tex, &d.part, &dst, t.a,
					nullptr, SDL_FLIP_NONE);
			}
		}

		SDL_RenderPresent(ren);
	}

	Pong::Pong()
	{
		if (!prepareWindowAndTexture())
			return;
		SDL_srand(time(nullptr));

		prepareBoxWorld();
		prepareWalls();
		createBall();

		createPad(PAD1_TEX, {50, WIN_HEIGHT/2}, {SDL_SCANCODE_W, SDL_SCANCODE_S});
		createPad(PAD2_TEX, {WIN_WIDTH-50, WIN_HEIGHT/2}, {SDL_SCANCODE_UP, SDL_SCANCODE_DOWN});
	}

	Pong::~Pong()
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

	void Pong::run()
	{
		SDL_SetRenderDrawColor(ren, 0,0,0,255);
		auto start = SDL_GetTicks();
		bool quit = false;

		InputSystem is;
		while (!quit) {
			is.updateEntities();
			//first updateEntities() for all systems

			is.update();
			//then update() for all systems

			World::step();
			//finally World::step() to clear added() array

			//input_system();
			move_system();
			box_system();
			score_system();

			draw_system();

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
}
