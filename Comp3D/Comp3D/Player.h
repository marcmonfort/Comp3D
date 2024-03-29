#ifndef _PLAYER_INCLUDE
#define _PLAYER_INCLUDE

#include "TileMap.h"
#include "SoundManager.h"
#include "Button.h"
#include "Switch.h"
#include "Wall.h"
#include "BallSpike.h"
#include "ParticleSystem.h"


enum orientation
{
	UP,
	LEFT,
	DOWN,
	RIGHT,
};


class Player
{
public:
	Player();
	~Player();

	void init(ShaderProgram& shaderProgram, TileMap* tileMap);
	void update(int deltaTime, vector<Wall*>* walls, vector<BallSpike*>* ballSpike, vector<Button*>* buttons, vector<Switch*>* switchs);
	void render(ShaderProgram& program, const glm::vec3& eye, float rotation);

	void setTileMap(TileMap* tileMap);
	void setPosition(const glm::vec3& pos);
	void setVelocity(const glm::vec3& vel);

	glm::vec3 getPosition();
	glm::vec3 getSize();

	void keyPressed(int key);
	void setDead(bool b);

	void setLineVolume(float lv);

private:
	bool collideWall(Wall* wall);
	bool collideBallSpike(BallSpike* blockSpike);
	bool collideButton(Button* button);
	bool collideSwitch(Switch* switx);

	void switchAllSwitchs(vector<Switch*>* switchs);
	void unpressAllButtons(vector<Button*>* buttons);

private:
	glm::vec3 posPlayer;
	glm::vec3 size;
	glm::vec3 velocity = glm::vec3(0);
	float currentTime;

	TileMap* map;
	AssimpModel* model;
	ParticleSystem* particles;
	ParticleSystem* particles_dead;


	float lastVelocity = 0;

	bool bSpace = false;
	int timeRotate = 0;
	int timeScale = 0;
	orientation eScaleDir;



	FMOD::Sound* wall_sound;
	FMOD::Sound* player_sound;
	FMOD::Sound* button_sound;
	FMOD::Sound* line_sound;
	FMOD::Sound* death_sound;
	FMOD::Sound* basic_sound;

	FMOD::Channel* channel;
	FMOD::Channel* line_channel;

	bool bDead = false;
	int numDeadRounds = 0;
};

#endif
