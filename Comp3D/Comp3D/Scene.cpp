#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include "Scene.h"
#include "Game.h"


#define PI 3.14159f
#define NUM_LEVELS 5


Scene::Scene()
{
	map = NULL;
	player = NULL;
	crown = NULL;
	channel = NULL;
	fireworks_channel = NULL;
	
}

Scene::~Scene()
{
	if (crown != NULL)
		delete map;
	if (map != NULL)
		delete map;
	if (player != NULL)
		delete player;
	for (Wall* obj : walls)
	{
		delete obj;
	}
	for (BallSpike* obj : ballSpikes)
	{
		delete obj;
	}
	for (Button* obj : buttons)
	{
		delete obj;
	}
	for (Switch* obj : switchs)
	{
		delete obj;
	}
	if (channel != NULL)
	{
		channel->stop();
	}
	if (fireworks_channel != NULL)
	{
		channel->stop();
	}
}


void Scene::init(int numLevel)
{
	initShaders();

	// Initialize TileMap
	string pathLevel = "levels/level0" + to_string(numLevel) + ".txt";
	map = TileMap::createTileMap(pathLevel, glm::vec2(0, 0), texProgram);
	roomSize = map->getRoomSize();
	glm::vec3 rgb = map->getColorBackground();
	glClearColor(rgb.x, rgb.y, rgb.z, 1.0f);
	lastLevel = numLevel == NUM_LEVELS + 1;

	//Init Music
	if (lastLevel)
	{
		fireworks = SoundManager::instance().loadSound("sounds/fireworks.mp3", FMOD_LOOP_NORMAL);
		fireworks_channel = SoundManager::instance().playSound(fireworks);
		fireworks_channel->setVolume(0.f);

		music = SoundManager::instance().loadSound("sounds/ending.mp3", FMOD_DEFAULT);
		channel = SoundManager::instance().playSound(music);
		channel->setVolume(0.f);
	}
	else {
		style = map->getStyle();
		string theme = themes[style];
		music = SoundManager::instance().loadSound(theme, FMOD_LOOP_NORMAL);
		channel = SoundManager::instance().playSound(music);
		channel->setVolume(0.f);
	}
	

	if (lastLevel)
		maxMusicVolume = 0.8f;
	else
		maxMusicVolume = 0.5f;
	

	// Init Camera.
	camera.position = map->getCenterCamera();
	camera.movement = map->getMovementCamera();

	// Init Player
	player = new Player();
	player->init(texProgram, map);
	player->setPosition(map->getCheckPointPlayer());

	// Init CheckPoint (player/camera)
	checkpoint.posCamera = map->getCenterCamera();
	checkpoint.posPlayer = map->getCheckPointPlayer();

	// Init Walls
	vector<TileMap::Wall> pos_walls = map->getWalls();
	for (int i = 0; i < pos_walls.size(); ++i)
	{ 
		Wall* wall = new Wall();
		wall->init(texProgram, pos_walls[i].bVertical, static_cast<Wall::Type>(pos_walls[i].type), map);
		wall->setPosition(glm::vec3(pos_walls[i].position,0));
		walls.push_back(wall);
	}

	// Init BallSpikes
	vector<pair<bool, glm::vec2>> pos_ballSpikes = map->getBallSpikes();
	for (int i = 0; i < pos_ballSpikes.size(); ++i)
	{
		BallSpike* ballSpike = new BallSpike();
		ballSpike->init(texProgram, pos_ballSpikes[i].first, map);
		ballSpike->setPosition(glm::vec3(pos_ballSpikes[i].second, 0));
		ballSpikes.push_back(ballSpike);
	}

	// Init Buttons
	vector<tuple<bool, glm::vec2, int>> pos_buttons = map->getButtons();
	for (int i = 0; i < pos_buttons.size(); ++i)
	{
		Button* button = new Button();
		button->init(texProgram, get<0>(pos_buttons[i]));
		button->setPosition(glm::vec3(get<1>(pos_buttons[i]), 0));
		button->setOrientation(get<2>(pos_buttons[i]));
		button->setTileMap(map);
		buttons.push_back(button);
	}

	// Init Switchs
	vector<pair<bool, glm::vec2>> pos_switchs = map->getSwitchs();
	for (int i = 0; i < pos_switchs.size(); ++i)
	{
		Switch* switx = new Switch();
		switx->init(texProgram, pos_switchs[i].first, map);
		switx->setPosition(glm::vec3(pos_switchs[i].second, 0));
		switchs.push_back(switx);
	}

	// Init God Mode Sprite
	godMode_spritesheet.loadFromFile("images/godmode.png", TEXTURE_PIXEL_FORMAT_RGBA);
	godMode_sprite = Sprite::createSprite(glm::ivec2(128, 16), glm::vec2(1.f, 1.f), &godMode_spritesheet, &texProgram);
	godMode_sprite->setPosition(glm::vec2(50, 690));

	// Init Fade
	fade_spritesheet.loadFromFile("images/fade.png", TEXTURE_PIXEL_FORMAT_RGBA);
	fade_sprite = Sprite::createSprite(glm::ivec2(12800, 12800), glm::vec2(1.f, 1.f), &fade_spritesheet, &texProgram);
	fade_sprite->setPosition(glm::vec2(0, 0));
	totalFadeTime = 750;
	fadeTime = 0;
	fadeIn = true;
	fadeOut = false;

	// End Inits
	projection = glm::perspective(glm::radians(45.f), float(CAMERA_WIDTH) / float(CAMERA_HEIGHT), 0.1f, 1000.f);
	currentTime = 0.0f;

	firstUpdate = true;

	if (lastLevel)
	{
		// Init Crown
		crown = new AssimpModel();
		crown->loadFromFile("models/crown.obj", texProgram);
	}
	
	bDead = false;

	rotation = 0.f;
	victoryTime = 0.f;

	escape = false;
}

void Scene::update(int deltaTime)
{
	currentTime += deltaTime;

	if (bDead)
	{
		timeDead -= deltaTime;
		if (timeDead <= 0)
		{
			bDead = false;
			player->setDead(false);

			player->setPosition(checkpoint.posPlayer);
			camera.position = checkpoint.posCamera;
			eCamMove = CamMove::STATIC;
		}

	}

	if (firstUpdate)
	{
		deltaTime = 0;
		firstUpdate = false;
	}

	if (fadeIn) {
		fadeTime += deltaTime;
		channel->setVolume((fadeTime / totalFadeTime)*maxMusicVolume);
		if (lastLevel) fireworks_channel->setVolume(fadeTime / totalFadeTime);

		if (fadeTime >= totalFadeTime) {
			fadeIn = false;
			fadeTime = 0;
			channel->setVolume(maxMusicVolume);
			if (lastLevel) fireworks_channel->setVolume(1.0f);
		}
	}
	else if (fadeOut) {
		fadeTime += deltaTime;
		channel->setVolume(maxMusicVolume*(1.0f - fadeTime / totalFadeTime));
		player->setLineVolume(maxMusicVolume * (1.0f - fadeTime / totalFadeTime));
		if (lastLevel) fireworks_channel->setVolume(1.0f - fadeTime / totalFadeTime);

		if (fadeTime >= totalFadeTime) {
			channel->stop();
			player->setLineVolume(0.f);

			if (lastLevel)
				fireworks_channel->stop();

			if (escape)
				Game::instance().goBackToMenu();
			else
				PlayGameState::instance().finalBlockTaken();
		}
	}
	
	if (!fadeIn && !fadeOut || lastLevel)
	{
		if (map->getNewCheckPoint() && eCamMove == CamMove::STATIC)
		{
			checkpoint.posPlayer = map->getCheckPointPlayer();
			checkpoint.posCamera = camera.position;
			map->setNewCheckPoint(false);
		}

		if (map->getPlayerDead())
		{
			bDead = true;
			timeDead = 1000;
			player->setDead(true);
			map->setPlayerDead(false);
		}

		glm::vec3 posPlayer = player->getPosition();
		glm::vec3 sizePlayer = player->getSize();

		if (lastLevel)
		{
			eCamMove = CamMove::FOLLOW;
		}
		else if (posPlayer.x + sizePlayer.x - camera.position.x > (roomSize.x / 2) && eCamMove == CamMove::STATIC)
		{
			timeCamMove = camera.movement.x;
			eCamMove = CamMove::RIGHT;
		}
		else if (camera.position.x - posPlayer.x > (roomSize.x / 2) && eCamMove == CamMove::STATIC)
		{
			timeCamMove = camera.movement.x;
			eCamMove = CamMove::LEFT;
		}

		if (camera.position.y + (posPlayer.y + sizePlayer.y) > (roomSize.y / 2) && eCamMove == CamMove::STATIC)
		{
			timeCamMove = camera.movement.y;
			eCamMove = CamMove::DOWN;
		}
		else if (-posPlayer.y - camera.position.y > (roomSize.y / 2) && eCamMove == CamMove::STATIC)
		{
			timeCamMove = camera.movement.y;
			eCamMove = CamMove::UP;
		}

		switch (eCamMove)
		{
		case Scene::CamMove::STATIC:
			break;
		case Scene::CamMove::RIGHT:
			camera.position.x += camera.velocity * deltaTime;
			timeCamMove -= camera.velocity * deltaTime;
			if (timeCamMove <= 0.f) {
				camera.position.x += timeCamMove;
				eCamMove = CamMove::STATIC;
			}
			break;
		case Scene::CamMove::LEFT:
			camera.position.x -= camera.velocity * deltaTime;
			timeCamMove -= camera.velocity * deltaTime;
			if (timeCamMove <= 0.f) {
				camera.position.x -= timeCamMove;
				eCamMove = CamMove::STATIC;
			}
			break;
		case Scene::CamMove::UP:
			camera.position.y += camera.velocity * deltaTime;
			timeCamMove -= camera.velocity * deltaTime;
			if (timeCamMove <= 0.f) {
				camera.position.y += timeCamMove;
				eCamMove = CamMove::STATIC;
			}
			break;
		case Scene::CamMove::DOWN:
			camera.position.y -= camera.velocity * deltaTime;
			timeCamMove -= camera.velocity * deltaTime;
			if (timeCamMove <= 0.f) {
				camera.position.y -= timeCamMove;
				eCamMove = CamMove::STATIC;
			}
			break;
		case Scene::CamMove::FOLLOW:
			camera.position.x = posPlayer.x;
			camera.position.y = -posPlayer.y;
			break;
		}

		player->update(deltaTime, &walls, &ballSpikes, &buttons, &switchs);

		for (int i = 0; i < walls.size(); ++i)
		{
			walls[i]->update(deltaTime, player->getPosition(), player->getSize(), &switchs);
		}

		//update BallSpikes
		for (int i = 0; i < ballSpikes.size(); ++i)
		{
			ballSpikes[i]->update(deltaTime, player->getPosition());
		}

	}
	map->update(deltaTime);

	if (lastLevel) {
		rotation += deltaTime / 2.0f;
		if (rotation > 2 * PI)
			rotation -= 2 * PI;

		victoryTime += deltaTime;
		if (victoryTime > 21250)
			fadeOut = true;
	}
}

void Scene::render()
{
	glm::mat4 modelMatrix, viewMatrix;
	glm::mat3 normalMatrix;


	texProgram.use();
	texProgram.setUniform1b("bLighting", true);	//si es fals, no se ven sombras
	texProgram.setUniformMatrix4f("projection", projection);
	texProgram.setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);


	// Camera position
	viewMatrix = glm::mat4(1.0f);
	viewMatrix = glm::translate(viewMatrix, -camera.position);
	/*if (lastLevel) viewMatrix = glm::rotate(viewMatrix, glm::radians(15.f), glm::vec3(0, 1, 0));*/
	texProgram.setUniformMatrix4f("view", viewMatrix);

	// Init matrix
	modelMatrix = glm::mat4(1.0f);
	normalMatrix = glm::transpose(glm::inverse(glm::mat3(viewMatrix * modelMatrix)));
	texProgram.setUniformMatrix3f("normalmatrix", normalMatrix);



	// Render TileMap
	map->render(texProgram, player->getPosition());

	// Render Player
	if (PlayGameState::instance().getGodMode()) {
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		texProgram.setUniform1f("alpha", 0.3);
	}
	player->render(texProgram, camera.position, rotation);
	if (PlayGameState::instance().getGodMode()) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}

	// Render Walls
	for (int i = 0; i < walls.size(); ++i)
	{
		walls[i]->render(texProgram, player->getPosition());
	}

	// Render BlockSpikes
	for (int i = 0; i < ballSpikes.size(); ++i)
	{
		ballSpikes[i]->render(texProgram, player->getPosition(), viewMatrix);

		//se podria poner al final...
		normalMatrix = glm::transpose(glm::inverse(glm::mat3(viewMatrix * modelMatrix)));
		texProgram.setUniformMatrix3f("normalmatrix", normalMatrix);
		//se podria poner al final...
	}

	// Render Buttons
	for (int i = 0; i < buttons.size(); ++i)
	{
		buttons[i]->render(texProgram);
	}

	// Render Switchs
	for (int i = 0; i < switchs.size(); ++i)
	{
		switchs[i]->render(texProgram);
	}

	// Render crown
	if (lastLevel) {
		glm::vec3 playerPos = player->getPosition();
		modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(playerPos.x, -playerPos.y + 1, 0.f));
		modelMatrix = glm::translate(modelMatrix, crown->getCenter());
		modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation), glm::vec3(0, -1, 0));
		modelMatrix = glm::translate(modelMatrix, -crown->getCenter());
		texProgram.setUniformMatrix4f("model", modelMatrix);
		crown->render(texProgram);
	}


	// LO ULTIMO (2D)

	if (PlayGameState::instance().getGodMode()) {
		texProgram.setUniformMatrix4f("projection", glm::ortho(0.f, float(SCREEN_WIDTH - 1), float(SCREEN_HEIGHT - 1), 0.f));
		texProgram.setUniformMatrix4f("view", glm::mat4(1.0f));
		texProgram.setUniform2f("texCoordDispl", 0.f, 0.f);
		texProgram.setUniform1b("bLighting", false);
		godMode_sprite->render();
	}

	if (fadeIn)
	{
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		texProgram.setUniformMatrix4f("projection", glm::ortho(0.f, float(SCREEN_WIDTH - 1), float(SCREEN_HEIGHT - 1), 0.f));
		texProgram.setUniformMatrix4f("view", glm::mat4(1.0f));
		texProgram.setUniform2f("texCoordDispl", 0.f, 0.f);
		texProgram.setUniform1b("bLighting", false);
		float alpha = min(1.0f, fadeTime / totalFadeTime);
		texProgram.setUniform1f("alpha", 1 - alpha);
		fade_sprite->render();

		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}

	else if (fadeOut)
	{
		if (!lastLevel) player->setVelocity(glm::vec3(0.f, 0.f, 0.f));

		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		texProgram.setUniformMatrix4f("projection", glm::ortho(0.f, float(SCREEN_WIDTH - 1), float(SCREEN_HEIGHT - 1), 0.f));
		texProgram.setUniformMatrix4f("view", glm::mat4(1.0f));
		texProgram.setUniform2f("texCoordDispl", 0.f, 0.f);
		texProgram.setUniform1b("bLighting", false);
		float alpha = min(1.0f, fadeTime / totalFadeTime);
		texProgram.setUniform1f("alpha", alpha);
		fade_sprite->render();

		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
}

void Scene::keyPressed(int key)
{
	player->keyPressed(key);
}

// Not used
void Scene::reshape(int width, int height)
{
	float FOV = glm::radians(90.f);
	float rav = float(width) / float(height);
	if (rav < 1.f)
		FOV = 2.0 * atan( tan(glm::radians(90.f/2)) / rav);

	projection = glm::perspective(FOV, rav, 0.1f, 1000.f);
}


void Scene::setFade(bool b) {
	fadeOut = b;
}


void Scene::setEscape(bool b) {
	escape = b;
}


void Scene::initShaders()
{
	Shader vShader, fShader;

	vShader.initFromFile(VERTEX_SHADER, "shaders/texture.vert");
	if(!vShader.isCompiled())
	{
		cout << "Vertex Shader Error" << endl;
		cout << "" << vShader.log() << endl << endl;
	}
	fShader.initFromFile(FRAGMENT_SHADER, "shaders/texture.frag");
	if(!fShader.isCompiled())
	{
		cout << "Fragment Shader Error" << endl;
		cout << "" << fShader.log() << endl << endl;
	}
	texProgram.init();
	texProgram.addShader(vShader);
	texProgram.addShader(fShader);
	texProgram.link();
	if(!texProgram.isLinked())
	{
		cout << "Shader Linking Error" << endl;
		cout << "" << texProgram.log() << endl << endl;
	}
	texProgram.bindFragmentOutput("outColor");
	vShader.free();
	fShader.free();
}



