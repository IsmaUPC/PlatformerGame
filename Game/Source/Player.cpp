#include "Player.h"
#include "Entity.h"

Player::Player() : Entity()
{
    name.Create("player");
}

Player::Player(TypeEntity pTypeEntity, fPoint pPosition, float pVelocity, SDL_Texture* pTexture) 
	: Entity(pTypeEntity, pPosition, pVelocity, pTexture)
{
	
	playerData.state = IDLE;
    name.Create("player");
}

Player::~Player()
{}

bool Player::Start() 
{
	iPoint pathInit =  app->map->WorldToMap(positionInitial.x ,positionInitial.y) ;
	app->map->ResetPath(pathInit);
	playerData.texture = app->tex->Load("Assets/textures/Dino_Green.png");
	playerData.position = positionInitial;
	playerData.state = IDLE;

	checkpointMove = false;
	endUpdate = true;
	return true;
}

bool Player::Awake(pugi::xml_node& config)
{
	LOG("Loading Player Parser");
	bool ret = true;
	win = false;
	
	playerData.velocity = 1;
	playerData.isJumped = false;
	playerData.state = IDLE;
	playerData.direction = WALK_R;

	idleAnim->loop = true;
	idleAnim->speed = 0.025f;
	walkAnim->loop = true;
	walkAnim->speed = 0.04f;
	damageAnim->loop = true;
	damageAnim->speed = 0.025f;
	runAnim->loop = true;
	runAnim->speed = 0.05f;
	
	jumpAnim->loop = true;
	jumpAnim->speed = 0.12f;

	for (int i = 0; i < 4; i++)
		idleAnim->PushBack({ 78 * i,0, 78, 78 });

	for (int i = 0; i < 6; i++)
		jumpAnim->PushBack({ 312 + (78 * i),0, 78, 78 });

	for (int i = 0; i < 6; i++)
		walkAnim->PushBack({ 312 + (78 * i),0, 78, 78 });

	for (int i = 0; i < 3; i++)
		atakAnim->PushBack({ 480 + (78 * i),0, 78, 78 });

	for (int i = 0; i < 4; i++)
		damageAnim->PushBack({ 1008 + (78 * i),0, 78, 78 });
	
	for (int i = 0; i < 4; i++)
		runAnim->PushBack({ 1319 + (78 * i),0, 78, 78 });

	playerData.currentAnimation = idleAnim;
	return ret;
}


bool Player::LoadState(pugi::xml_node& player) 
{
	bool ret=true;
		playerData.position.x = player.child("position").attribute("x").as_int(playerData.position.x);
		playerData.position.y = player.child("position").attribute("y").as_int(playerData.position.y);
	return ret;
}
bool Player::SaveState(pugi::xml_node& player) const
{
	pugi::xml_node positionPlayer = player.child("position");


	positionPlayer.attribute("x").set_value(playerData.position.x) ;
	positionPlayer.attribute("y").set_value( playerData.position.y) ;

	return true;
}


bool Player::PreUpdate() 
{

	return true;
}

bool Player::Update(float dt) 
{
	if(godMode==false)Fallings(dt);
	//playerData.velocity = (125*dt);
	playerData.velocity = 5;
	//LOG("Delta %d  <--------", velX);

	if (godMode == false)playerData.currentAnimation->Update();
	else playerData.currentAnimation = idleAnim;

	int followPositionPalyerX = (WINDOW_W / 2) + (playerData.position.x * -1);
	int followPositionPalyerY = (WINDOW_H / 2) + (playerData.position.y * -1)+200;


	// Camera delimitation x
	if (app->render->camera.x <= -1 && app->render->camera.x >= -((app->map->data.width * app->map->data.tileWidth)- WINDOW_W))
		app->render->camera.x = followPositionPalyerX;
	else if (followPositionPalyerX<-1 && followPositionPalyerX>-((app->map->data.width * app->map->data.tileWidth) - WINDOW_W))
		app->render->camera.x = followPositionPalyerX;

	// Camera delimitation x
	if (app->render->camera.y <= -48 && app->render->camera.y >= -((app->map->data.height * app->map->data.tileHeight) - (WINDOW_H+(4* app->map->data.tileHeight))))
			app->render->camera.y = followPositionPalyerY;
		else if (followPositionPalyerY<-48 && followPositionPalyerY>-((app->map->data.height * app->map->data.tileHeight)-(WINDOW_H+(4 * app->map->data.tileHeight))))
			app->render->camera.y = followPositionPalyerY;



	// Move player inputs control
	if (!checkpointMove)
	{
		PlayerControls(dt);
	}
	else // Move Between CheckPoints
	{
		if (app->input->GetKey(SDL_SCANCODE_D) == KEY_DOWN) {
			if ((lastCP + 1) >= checkPoints.Count()) lastCP = 0;
			else lastCP++;
			playerData.position = TransformIPointMapToFPointWorld(checkPoints.At(lastCP)->data);
			app->render->camera.x = cameraPosCP.At(lastCP)->data.x;
			app->render->camera.y = cameraPosCP.At(lastCP)->data.y;
		}
		if (app->input->GetKey(SDL_SCANCODE_A) == KEY_DOWN) {
			if (lastCP == 0) lastCP = checkPoints.Count() - 1;
			else lastCP--;
			playerData.position = TransformIPointMapToFPointWorld(checkPoints.At(lastCP)->data);
			app->render->camera.x = cameraPosCP.At(lastCP)->data.x;
			app->render->camera.y = cameraPosCP.At(lastCP)->data.y;
		}
	}

	if (app->input->GetKey(SDL_SCANCODE_P) == KEY_DOWN)
	{
		app->map->PropagateDijkstra();
		//app->map->PropagateAStar(1);
	}

	return true;
}



void Player::PlayerMoveAnimation()
{
	switch (playerData.state)
	{
	case IDLE:
		playerData.currentAnimation = idleAnim;
		break;

	case WALK:
		playerData.currentAnimation = walkAnim;
		break;

	case JUMP:
		playerData.currentAnimation = jumpAnim;
		break;

	case RUN:
		playerData.currentAnimation = runAnim;
		break;

	default:
		break;
	}
}

void Player::PlayerControls(float dt)
{
	//MovePlayer();
	
		// Comprobamos si las tecas est�n pulsadas al mismo tiempo
	if (!(app->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT && app->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
		&& (app->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT || app->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT))
	{
		if (playerData.state == State::IDLE)
		{
			playerData.state = State::WALK;
			velX = playerData.velocity ;

		}
		if (app->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)MovePlayer(MoveDirection::WALK_R,dt);
		if (app->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)MovePlayer(MoveDirection::WALK_L,dt);
	}	// Any key is pressed or A and D pressed in same time, set player in IDLE state
	else if(playerData.state== State::IDLE) playerData.state = State::IDLE;

	// Player Jump
	if (app->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN)Jump(dt);

	if (godMode == true)
	{
		if (app->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT)playerData.position.y -= playerData.velocity;
		if (app->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT)playerData.position.y += playerData.velocity;
	}

	// Player Run
	if (app->input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_REPEAT&& playerData.state == State::WALK)
	{
			playerData.state = State::RUN;
		velX = playerData.velocity * 1.50f;
	}

	PlayerMoveAnimation();
}


void Player::MovePlayer(MoveDirection playerDirection, float dt)
{
	tmp = playerData.position;
	playerData.direction = playerDirection;


	switch (playerData.state)
	{
	case IDLE:
		// Future conditions in state IDLE...
		break;	

	case JUMP:
		// Move in state JUMP 
		MoveToDirection(velX);
		// Future conditions in state JUMP...
		break;

	case WALK:
		// Move in state WALK 
		MoveToDirection(velX);
		// Future conditions in state WALK...
		break;

	case RUN:
		// Move in state RUN 
		MoveToDirection(velX);
		// Future conditions in state RUN...
		break;

	default:
		break;
	}

	//if (!DownY(playerData.position) && velY != 0)
	if (CollisionPlayer(playerData.position))playerData.position = tmp;
	
}

void Player::MoveToDirection(int velocity)
{
	switch (playerData.direction)
	{	
	// Move in to correct direction
	case WALK_L:
		playerData.position.x -= velocity;
		break;
	case WALK_R:
		playerData.position.x += velocity;
		break;

	default:
		break;
	}
}
iPoint Player::TransformFPoint(fPoint fpoint)
{
	iPoint transformation;

	transformation.x = fpoint.x;
	transformation.y = fpoint.y;
	
	return transformation;
}
iPoint Player::TransformIPointMapToFPointWorld(iPoint ipoint)
{
	iPoint CPos = app->map->MapToWorld(ipoint.x, ipoint.y);
	iPoint CPosF;
	CPosF.x = CPos.x;
	CPosF.y = CPos.y;
	return CPosF;
}


bool Player::PostUpdate() 
{
	SDL_Rect rectPlayer;
	rectPlayer = playerData.currentAnimation->GetCurrentFrame();
	// Draw player in correct direction
	if (playerData.direction == MoveDirection::WALK_R )
		app->render->DrawTexture(playerData.texture, playerData.position.x -15, playerData.position.y - (rectPlayer.h - 10), &rectPlayer);
	if (playerData.direction == MoveDirection::WALK_L)
		app->render->DrawTextureFlip(playerData.texture, playerData.position.x -15, playerData.position.y - (rectPlayer.h - 10), &rectPlayer);
	
	endUpdate = true;
	return true;
}
// Implements to gravity fall down
void Player::Fallings(float dt)
{
	velY += gravity;
	tmp = playerData.position;
	iPoint iPointTmp;
	iPoint PlayerCurrPos = { playerData.position.x ,playerData.position.y+ (int)velY};
	bool fallingCollision = false;
	//gravity = (dt * 10);
	//gravity = 0.0f;
	//playerData.position.y += velY;
	for (int i = 0; i < playerData.numPoints; i++)
	{

		iPointTmp = { PlayerCurrPos.x+ playerData.pointsCollision[i].x ,PlayerCurrPos.y+ playerData.pointsCollision[i].y};
		if (CollisionJumping({ PlayerCurrPos.x + playerData.pointsCollision[i].x ,PlayerCurrPos.y + playerData.pointsCollision[i].y }))
			fallingCollision = true;		
		if (fallingCollision && (i == 0 || i == 1))
		{
			playerData.isJumped = false;
			playerData.isJumpedAgain = false;
			playerData.state = State::IDLE;
		}
	}
	if (fallingCollision)
	{	
		playerData.position = tmp;
		velY = 0.0f;
		fallingCollision = false;
	}
	else playerData.position.y += velY;
	
}

bool Player::CollisionPlayer(iPoint Position) 
{
	iPoint positionMapPlayer;
	int y = Position.y ;
	int x = Position.x;
	bool xCollision = false;
	bool yCollision = false;

	for (int i = 0; i < playerData.numPoints; i++)
	{	
		// Convert position player WorldToMap 
		//positionMapPlayer = app->map->WorldToMap(Position.x + playerData.pointsCollision[i][0], y + playerData.pointsCollision[i][1]);
		//if (CheckCollision(positionMapPlayer) == COLLISION)	yCollision = true;
		
		positionMapPlayer = app->map->WorldToMap(x + playerData.pointsCollision[i].x, y + playerData.pointsCollision[i].y);
		if (CheckCollision(positionMapPlayer)== COLLISION) xCollision=true;
	}

	return yCollision || xCollision;
}


bool Player::CleanUp() 
{
	if (!active)
		return true;

	app->tex->UnLoad(playerData.texture);
	active = false;

	return true;
}


bool Player::CollisionJumping(iPoint nextPosition)
{

	iPoint positionMapPlayer;
	int y = nextPosition.y;

	positionMapPlayer = app->map->WorldToMap(nextPosition.x, nextPosition.y);
	if (CheckCollision(positionMapPlayer)== COLLISION) return true;
	//positionMapPlayer = app->map->WorldToMap((int)nextPosition.x+30, y);
	//if (CheckCollision(positionMapPlayer)== COLLISION) return true;

	return false;
}

// Comprove position player in array of tiles in mapLayer collision
int Player::CheckCollision(iPoint positionMapPlayer)
{
	uint typeTilePlayer = app->map->data.layers.At(2)->data->Get(positionMapPlayer.x, positionMapPlayer.y) ;
	uint firstgidLayerCollisions = app->map->data.tilesets.At(2)->data->firstgid;
	typeTilePlayer -= firstgidLayerCollisions;

	if (godMode == false) {
		switch (typeTilePlayer)
		{
		case VICTORY:
			//victory
			win = true;
			return 0;
			break;

		case COLLISION:
			//collision
			return 1;
			break;

		case CHECK_POINT:
			//checkpoint
			app->SaveGameRequest();
			activeCheckpoint(positionMapPlayer);
			return 2;
			break;

		default:
			return -1;
			break;
		}
	}

	return false;
}
void Player::Jump(float dt)
{
	if (playerData.isJumped && !playerData.isJumpedAgain)
	{
		// Generate second impulse
		//velY =-floor(20 *gravity);
		velY =-12;

		playerData.isJumpedAgain = true;
	}
	if (!playerData.isJumped)
	{
		// Generate first impulse
		//velY = -floor(30 * gravity);
		velY = -15;
		playerData.isJumped = true;
	}

	playerData.state = State::JUMP;
	//MovePlayer(playerData.direction,dt);
}


bool Player::CheckGameOver(int level)
{
	if (level == 1)
	{
		if (playerData.position.y > 1720)
		{
			//isDead = true;
			playerData.position = TransformIPointMapToFPointWorld(checkPoints.end->data);
			app->render->camera.x = cameraPosCP.end->data.x;
			app->render->camera.y = cameraPosCP.end->data.y;
			return true;
		}
	}
	if (level == 2)
	{
		if (playerData.position.y > 1968)
		{
			isDead = true;
			return true;
		}
	}
	
	return false;
}

void Player::activeCheckpoint(iPoint positionMapPlayer)
{
	if (app->map->data.layers.At(2)->data->Get(positionMapPlayer.x, positionMapPlayer.y) == app->map->data.tilesets.At(2)->data->firstgid + 2)
	{

		for (int i = 0; i < checkPoints.Count(); i++)
		{
			if (checkPoints.At(i)->data == positionMapPlayer) {

				lastCP = i;
				if (app->input->GetKey(SDL_SCANCODE_E) == KEY_DOWN && endUpdate) {
					endUpdate = false;
					checkpointMove = !checkpointMove;
				}
				return;
			}
		}
		app->SaveGameRequest();
		checkPoints.Add(positionMapPlayer);
		iPoint cam(app->render->camera.x, app->render->camera.y);
		cameraPosCP.Add(cam);
		LOG("CHECKPOINT pos:%d,%d", positionMapPlayer.x, positionMapPlayer.y);

	}
}