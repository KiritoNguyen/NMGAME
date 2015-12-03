﻿#include "Soldier.h"

Soldier::Soldier(eStatus status, GVector2 pos, int direction) : BaseEnemy(eID::SOLDIER) {
	_sprite = SpriteManager::getInstance()->getSprite(eID::SOLDIER);
	_sprite->setFrameRect(0, 0, 32.0f, 16.0f);
	GVector2 v(direction * SOLDIER_SPEED, 0);
	GVector2 a(0, 0);
	this->_listComponent.insert(pair<string, IComponent*>("Movement", new Movement(a, v, this->_sprite)));
	this->setStatus(status);
	this->setPosition(pos);
	this->setScale(SCALE_FACTOR);
	this->setScaleX(-direction * SCALE_FACTOR);
	this->_canShoot = false;
}

Soldier::Soldier(eStatus status, float x, float y, int direction) : BaseEnemy(eID::SOLDIER) {
	_sprite = SpriteManager::getInstance()->getSprite(eID::SOLDIER);
	_sprite->setFrameRect(0, 0, 32.0f, 16.0f);
	GVector2 pos(x, y);
	GVector2 v(direction * SOLDIER_SPEED, 0);
	GVector2 a(0, 0);
	this->_listComponent.insert(pair<string, IComponent*>("Movement", new Movement(a, v, this->_sprite)));
	this->setStatus(status);
	this->setPosition(pos);
	this->setScale(SCALE_FACTOR);
	this->setScaleX(-direction * SCALE_FACTOR);
	this->_canShoot = false;
}

Soldier::Soldier(eStatus status, GVector2 pos, int direction, bool shoot) : BaseEnemy(eID::SOLDIER) {
	_sprite = SpriteManager::getInstance()->getSprite(eID::SOLDIER);
	_sprite->setFrameRect(0, 0, 32.0f, 16.0f);
	GVector2 v(direction * SOLDIER_SPEED, 0);
	GVector2 a(0, 0);
	this->_listComponent.insert(pair<string, IComponent*>("Movement", new Movement(a, v, this->_sprite)));
	this->setStatus(status);
	this->setPosition(pos);
	this->setScale(SCALE_FACTOR);
	this->setScaleX(-direction * SCALE_FACTOR);
	this->_canShoot = shoot;
}

Soldier::Soldier(eStatus status, float x, float y, int direction, bool shoot) : BaseEnemy(eID::SOLDIER) {
	_sprite = SpriteManager::getInstance()->getSprite(eID::SOLDIER);
	_sprite->setFrameRect(0, 0, 32.0f, 16.0f);
	GVector2 pos(x, y);
	GVector2 v(direction * SOLDIER_SPEED, 0);
	GVector2 a(0, 0);
	this->_listComponent.insert(pair<string, IComponent*>("Movement", new Movement(a, v, this->_sprite)));
	this->setStatus(status);
	this->setPosition(pos);
	this->setScale(SCALE_FACTOR);
	this->setScaleX(-direction * SCALE_FACTOR);
	this->_canShoot = shoot;
}
 
Soldier::~Soldier() {}

void Soldier::init()
{
	this->setHitpoint(SOLDIER_HITPOINT);
	this->setScore(SOLDIER_SCORE);
	this->_listComponent.insert(pair<string, IComponent*>("Gravity", new Gravity(GVector2(0, -ENEMY_GRAVITY), (Movement*)(this->getComponent("Movement")))));

	auto collisionBody = new CollisionBody(this);
	_listComponent["CollisionBody"] = collisionBody;

	__hook(&CollisionBody::onCollisionBegin, collisionBody, &Soldier::onCollisionBegin);
	__hook(&CollisionBody::onCollisionEnd, collisionBody, &Soldier::onCollisionEnd);

	_animations[RUNNING] = new Animation(_sprite, 0.15f);
	_animations[RUNNING]->addFrameRect(eID::SOLDIER, "run_01", "run_02", "run_03", "run_04", "run_05", "run_06", NULL);

	_animations[SHOOTING] = new Animation(_sprite, 0.15f);
	_animations[SHOOTING]->addFrameRect(eID::SOLDIER, "shoot_01", "shoot_02", NULL);

	_animations[JUMPING] = new Animation(_sprite, 0.15f);
	_animations[JUMPING]->addFrameRect(eID::SOLDIER, "jump_01", NULL);

	_animations[FALLING] = new Animation(_sprite, 0.15f);
	_animations[FALLING]->addFrameRect(eID::SOLDIER, "jump_01", NULL);

	_animations[LAYING_DOWN] = new Animation(_sprite, 0.15f);
	_animations[LAYING_DOWN]->addFrameRect(eID::SOLDIER, "lay_down_01", NULL);

	_animations[DYING] = new Animation(_sprite, 0.15f);
	_animations[DYING]->addFrameRect(eID::SOLDIER, "die_01", NULL);

	_stopwatch = new StopWatch();
	_loopwatch = new StopWatch();
	_shoot = new StopWatch();
	_checkShoot = new StopWatch();
}

void Soldier::draw(LPD3DXSPRITE spritehandle, Viewport* viewport)
{
	if (_explosion != NULL)
		_explosion->draw(spritehandle, viewport);
	if (this->getStatus() == eStatus::DESTROY || this->getStatus() == eStatus::WAITING)
		return;

	// animation draw là nó lấy sprite draw nên ko cần phải render sprite nữa.
	//this->_sprite->render(spritehandle, viewport);

	_animations[this->getStatus()]->draw(spritehandle, viewport);
	for (auto it = _listBullets.begin(); it != _listBullets.end(); it++)
	{
		(*it)->draw(spritehandle, viewport);
	}
}

void Soldier::release()
{
	for (auto component : _listComponent)
	{
		delete component.second;
	}
	_listComponent.clear();
	if (this->_explosion != NULL)
		this->_explosion->release();
	SAFE_DELETE(this->_explosion);
	SAFE_DELETE(this->_sprite);
}

IComponent* Soldier::getComponent(string componentName)
{
	return _listComponent.find(componentName)->second;
}

void Soldier::update(float deltatime)
{
	if (_explosion != NULL)
		_explosion->update(deltatime);
	if (this->getStatus() == DESTROY || this->getStatus() == WAITING)
		return;
	Gravity *gravity = (Gravity*)this->getComponent("Gravity");
	Movement *movement = (Movement*)this->getComponent("Movement");
	if (this->getHitpoint() <= 0)
		this->setStatus(eStatus::DYING);
	if (this->getStatus() == eStatus::DYING) {
		this->die();
		if (_stopwatch->isStopWatch(200))
		{
			movement->setVelocity(GVector2(0, 0));
			auto pos = this->getPosition();
			_explosion = new Explosion(1);
			_explosion->init();
			_explosion->setScale(SCALE_FACTOR);
			_explosion->setPosition(pos);
			this->setStatus(eStatus::DESTROY);		
			return;
		}
	}
	if (this->_shoot->isTimeLoop(1000.0f))
	{
		if (this->_canShoot && this->getStatus() == RUNNING)
		{
			int chance = rand() % 2;
			if (chance == 1)			
			{
				this->setStatus(SHOOTING);			
				Movement *move = (Movement*)this->getComponent("Movement");
				move->setVelocity(GVector2(0, 0));
			}
		}
	}
	if (this->getStatus() == SHOOTING)
	{
		if (_checkShoot->isStopWatch(1000.0f))
		{
			Movement *move = (Movement*)this->getComponent("Movement");
			move->setVelocity(GVector2(-SOLDIER_SPEED * this->getScale().x / 2, 0));
			this->setStatus(RUNNING);
			delete _checkShoot;
			_checkShoot = new StopWatch();
		}
	}
	if (_loopwatch->isTimeLoop(SOLDIER_SHOOTING_DELAY))
		if (this->getStatus() == SHOOTING)
			shoot();
	for (auto it : _listComponent)
	{
		it.second->update(deltatime);
	}
	for (auto it = _listBullets.begin(); it != _listBullets.end(); it++)
	{
		(*it)->update(deltatime);
	}

	if (this->getStatus() != DESTROY)
		_animations[this->getStatus()]->update(deltatime);
}

void Soldier::changeDirection()
{
	_sprite->setScaleX(-this->getScale().x);
	Movement *movement = (Movement*)this->getComponent("Movement");
	movement->setVelocity(GVector2(-movement->getVelocity().x, 0));
}

void Soldier::onCollisionBegin(CollisionEventArg* collision_eventt) {
	eID objectID = collision_eventt->_otherObject->getId();
	switch (objectID)
	{
	case eID::BILL:
	{
		if (collision_eventt->_otherObject->isInStatus(eStatus::DYING) == false)
		{
			collision_eventt->_otherObject->setStatus(eStatus::DYING);
			((Bill*)collision_eventt->_otherObject)->die();
		}
		break;
	}
	default:
		break;
	}
}

void Soldier::onCollisionEnd(CollisionEventArg* collision_event) {
	if (this->getStatus() == eStatus::DESTROY)
		return;
	eID objectID = collision_event->_otherObject->getId();
	switch (objectID)
	{
	case eID::LAND:
	case eID::BRIDGE:
	{
		if (prevObject == collision_event->_otherObject)
		{
			this->setStatus(FALLING);
			prevObject = nullptr;
		}
	}
	break;
	default:
		break;
	}
}


float Soldier::checkCollision(BaseObject * object, float dt)
{
	if (this->getStatus() == eStatus::DESTROY)
		return 0.0f;
	auto collisionBody = (CollisionBody*)_listComponent["CollisionBody"];
	eID objectId = object->getId();
	eLandType land = eLandType::WATER;
	if (objectId == LAND)
		land = ((Land*)object)->getType();
	eDirection direction;

	if (objectId == eID::BRIDGE || land != eLandType::WATER)
	{
		if (collisionBody->checkCollision(object, direction, dt))
		{
			if (direction == eDirection::TOP && this->getVelocity().y < 0)
			{
				auto gravity = (Gravity*)this->_listComponent["Gravity"];
				auto movement = (Movement*)this->_listComponent["Movement"];
				movement->setVelocity(GVector2(movement->getVelocity().x, 0));
				gravity->setStatus(eGravityStatus::SHALLOWED);

				auto move = (Movement*)this->_listComponent["Movement"];
				move->setVelocity(GVector2(move->getVelocity().x, 0));

				this->setStatus(eStatus::RUNNING);
				prevObject = object;
			}
		}
		else if (prevObject == object)
		{
			prevObject = nullptr;

			auto gravity = (Gravity*)this->_listComponent["Gravity"];
			gravity->setStatus(eGravityStatus::FALLING__DOWN);	
			this->setStatus(FALLING);
		}
	}
	else
	{
		collisionBody->checkCollision(object, dt);
	}
	return 0.0f;

}
void Soldier::jump() 
{
	this->setStatus(FALLING);
	Movement *move = (Movement*)this->getComponent("Movement");
	move->setVelocity(GVector2(move->getVelocity().x, SOLDIER_JUMP_VELOCITY));
}

GVector2 Soldier::getVelocity()
{
	auto move = (Movement*)this->_listComponent["Movement"];
	return move->getVelocity();
}

void Soldier::die() {
	Gravity *gravity = (Gravity*)this->getComponent("Gravity");
	gravity->setStatus(eGravityStatus::SHALLOWED);
	Movement *movement = (Movement*)this->getComponent("Movement");
	movement->setVelocity(GVector2(0, 200));
}

void Soldier::shoot()
{
	float angle = -90 * this->getScale().x / 2;
	auto pos = this->getPosition();
	if (this->isInStatus(SHOOTING)) 
	{
		pos.x += this->getScale().x < 0 ? this->getSprite()->getFrameWidth() / 2 : -this->getSprite()->getFrameWidth() / 2;
		pos.y += this->getSprite()->getFrameHeight() / 4.5;
	}
	else if (this->isInStatus(LAYING_DOWN)) 
	{
		pos.x += this->getScale().x < 0 ? this->getSprite()->getFrameWidth() / 2 : -this->getSprite()->getFrameWidth() / 2;
		pos.y -= this->getSprite()->getFrameHeight() / 4.5;
	}
	_listBullets.push_back(new Bullet(pos, (eBulletType)(ENEMY_BULLET | NORMAL_BULLET), angle)); // normalbullet ->hardcode
	_listBullets.back()->init();
}