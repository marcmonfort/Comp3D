#ifndef _WALL_INCLUDE
#define _WALL_INCLUDE

#include "TileMap.h"
#include "SoundManager.h"
#include "Player.h"

class Wall
{
public:
	void init(ShaderProgram& shaderProgram, bool bVertical);
	void update(int deltaTime, Player* player);
	void render(ShaderProgram& program);

	void setTileMap(TileMap* tileMap);
	void setPosition(const glm::vec3& pos);
	void setVelocity(float vel);

	glm::vec3 getPosition();
	glm::vec3 getSize();

	void keyPressed(int key);


private:
	glm::vec3 position;
	TileMap* map;

	float velocity = 0;

	glm::vec3 size;

	const SoundManager* soundManager;
	FMOD::Sound* sound;
	FMOD::Channel* channel;

	AssimpModel* model;

	bool bVertical;

	void followPlayer(glm::vec3 posPlayer);
};

#endif
