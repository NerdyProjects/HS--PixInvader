/*
 * pixinvader.c
 *
 *  Created on: 23.11.2011
 *      Author: matthias
 */

#include <stdlib.h>
#include "main.h"
#include "keys.h"
#include "display.h"
#include "spi_command.h"
#include "sound.h"

#include "pixinvaders.h"

#define DISPLAY_SIZE_X 20
#define DISPLAY_SIZE_Y 14

#define NUM_INVADERS_X 5
#define NUM_INVADERS_Y 4
#define INVADER_WIDTH 2
#define INVADER_HEIGHT 1
#define INVADER_W_SPACE 1
#define INVADER_H_SPACE 1

#define PLAYER_WIDTH 3
#define PLAYER_HEIGHT 1

/* block width is display width, height is not really modifiable */
#define BLOCK_HEIGHT 2

static xdata unsigned char InvaderPosY = 0;
static xdata signed char InvaderPosX = 0;

static xdata unsigned char InvadersAlive[(NUM_INVADERS_X*NUM_INVADERS_Y+CHAR_BIT-1)/CHAR_BIT];
static xdata unsigned char Block[(DISPLAY_SIZE_X*BLOCK_HEIGHT*2+CHAR_BIT-1)/CHAR_BIT];
static xdata unsigned char InvadersAliveCnt;
/* from left to right, 2 bit HP */
#define BLOCK0 0x00
#define BLOCK5 0x00
#define BLOCK1 0x0A
#define BLOCK6 0x0A
#define BLOCK2 0x00
#define BLOCK7 0x00
#define BLOCK3 0x0A
#define BLOCK8 0x0A
#define BLOCK4 0x00
#define BLOCK9 0x00

static bit InvaderMovementRight;


static xdata unsigned char PlayerMissileX;
static xdata unsigned char PlayerMissileY;
static bit PlayerMissileActive;

static xdata unsigned char InvaderMissileX[NUM_INVADERS_X];
static xdata unsigned char InvaderMissileY[NUM_INVADERS_X];
#if NUM_INVADERS_X > 8
#error "invader missile structure does not allow more than 8 adjacent invaders"
#endif
static xdata unsigned char InvaderMissileActive;

static xdata unsigned char PlayerPositionX;

/* periodically incremented by call from external timer */
data volatile unsigned char GameTimer;

#define INVADER_MOVEMENT_SPEED_BASE_MS 35UL
#define MISSILE_MOVEMENT_SPEED_MS 230UL
#define PLAYER_MOVEMENT_SPEED_MS 100UL
#define GAME_TIME_DIFF(x) (((unsigned short)GameTimer+256-(unsigned short)(x))%256)

#define INVADER_BYTE(x,y) (((y)*NUM_INVADERS_X+x) / CHAR_BIT)
#define INVADER_BIT(x,y) (((y)*NUM_INVADERS_X+x) % CHAR_BIT)
#define INVADER_BYTE_L(i) (i / CHAR_BIT)
#define INVADER_BIT_L(i) (i % CHAR_BIT)
#define INVADER_IS_ALIVE(x,y) (InvadersAlive[INVADER_BYTE(x,y)] & (1 << (INVADER_BIT(x,y))))
#define INVADER_SET_ALIVE(x,y) {InvadersAlive[INVADER_BYTE(x,y)] |= (1 << (INVADER_BIT(x,y)));InvadersAliveCnt++;}
#define INVADER_SET_DEAD(x,y) {InvadersAlive[INVADER_BYTE(x,y)] &= ~(1 << (INVADER_BIT(x,y)));InvadersAliveCnt--;}
#define INVADER_SET_ALIVE_L(i) {InvadersAlive[INVADER_BYTE_L(i)] |= (1 << INVADER_BIT_L(i));InvadersAliveCnt++;}
#define INVADER_IS_ALIVE_L(i) (InvadersAlive[INVADER_BYTE_L(i)] & (1 << INVADER_BIT_L(i)))


static unsigned char getInvaderSpeed(void)
{
	unsigned char speed= INVADER_MOVEMENT_SPEED_BASE_MS*32*InvadersAliveCnt/32;
	return speed?speed:1;
}
static unsigned char getRandom(void)
{
	return rand();
}

static unsigned char findLowestInvaderX(unsigned char x)
{
	unsigned char i;
	for(i = NUM_INVADERS_Y; i; --i)
	{
		if(INVADER_IS_ALIVE(x, i - 1))
			return i - 1;
	}
	return NUM_INVADERS_Y;
}

static void invaderShoot(void)
{
	unsigned char invader = getRandom() % NUM_INVADERS_X;
	unsigned char invaderY = findLowestInvaderX(invader);

	if(!(InvaderMissileActive & (1 << invader)) && invaderY < NUM_INVADERS_Y) {
		InvaderMissileActive |= (1 << invader);
		InvaderMissileX[invader] = InvaderPosX + invader*(INVADER_W_SPACE+INVADER_WIDTH);
		InvaderMissileY[invader] = InvaderPosY + findLowestInvaderX(invader) * (INVADER_HEIGHT + INVADER_H_SPACE);
	}
}

static bit checkBlockCollision(unsigned char x, unsigned char y, bit hitBlock)
{
	unsigned char ind;
	ASSERT(x < DISPLAY_SIZE_X);
	ASSERT(y < DISPLAY_SIZE_Y);
	y -= DISPLAY_SIZE_Y - 1 - BLOCK_HEIGHT - PLAYER_HEIGHT;
	if(y > DISPLAY_SIZE_Y || y > (BLOCK_HEIGHT - 1))
		return 0;

	ind = x/4 + 5*y;
#ifdef _DEBUG
	if(ind >= 0x0A)
	{
		fprintf(stderr, "Block (X|Y) access is out of array: (%d|%d) %d\n", x, y, ind);
		fflush(stderr);
	}
#endif

	ASSERT((x/4+5*y) < 0x0A);
	if(Block[x/4+5*y] & (3 << (2*(x%4))))
	{
		if(hitBlock)
			Block[x/4+5*y]  -= (1 << (2*(x%4)));
		return 1;
	}

	return 0;
}

static void moveInvaders(void)
{
	unsigned char left = NUM_INVADERS_X;
	unsigned char right = 0;

	unsigned char x,i;
	for(i = 0; i < NUM_INVADERS_X * NUM_INVADERS_Y; ++i)
	{
		if(INVADER_IS_ALIVE_L(i))
		{
			x = i % NUM_INVADERS_X;
			if(left > x)
				left = x;
			if(right < x)
				right = x;
		}
	}
#ifdef _DEBUG
	fprintf(stderr, "l: %d r: %d\n", left, right);
	fprintf(stderr, "pos (%d, %d) m: %d\n", InvaderPosX, InvaderPosY, InvaderMovementRight);
#endif

	if(((InvaderPosX + left*(INVADER_WIDTH+INVADER_W_SPACE) == 0) && !InvaderMovementRight) ||
			(((InvaderPosX + (right + 1)*(INVADER_WIDTH+INVADER_W_SPACE) - INVADER_W_SPACE) >= DISPLAY_COLS) && InvaderMovementRight))
	{
		InvaderPosY++;
		InvaderMovementRight = !InvaderMovementRight;
#ifdef _DEBUG
		fprintf(stderr, "movement changed to: %d\n", InvaderMovementRight);
#endif
		/* todo: check block & player collision */
	} else {
		if(InvaderMovementRight)
				InvaderPosX++;
			else
				InvaderPosX--;
	}

}

/**
 * checks for an invader at specified (x|y) coordinate (pixels).
 * @param x, y: coordinates
 * @pre (x|y) has to be on the screen
 * returns true when there is an invader
 */
static bit checkForInvader(unsigned char x, unsigned char y, bit killInvader)
{
	unsigned char subX, subY;
	ASSERT(x < DISPLAY_SIZE_X);
	ASSERT(y < DISPLAY_SIZE_Y);
	x -= InvaderPosX;
	y -= InvaderPosY;

	if(x >= DISPLAY_SIZE_X || y >= DISPLAY_SIZE_Y)
		/* pixel above or left of invaders */
		return 0;

	subX = x % (INVADER_WIDTH + INVADER_W_SPACE);
	x /= INVADER_WIDTH + INVADER_W_SPACE;

	subY = y % (INVADER_HEIGHT + INVADER_H_SPACE);
	y /= INVADER_HEIGHT + INVADER_H_SPACE;

	if(subX >= INVADER_WIDTH || subY >= INVADER_HEIGHT)
		/* pixel in space between invaders */
		return 0;

	if(x >= NUM_INVADERS_X || y >= NUM_INVADERS_Y)
		/* pixel right of or below invaders */
		return 0;

	ASSERT(x < NUM_INVADERS_X);
	ASSERT(y < NUM_INVADERS_Y);

	if(INVADER_IS_ALIVE(x, y))
	{
		/* pixel is on invader that is still alive */
		if(killInvader)
			INVADER_SET_DEAD(x, y);
		return 1;
	}

	/* last case: pixel is on dead invader */
	return 0;

}

/**
 * checks for an player at specified (x|y) coordinate (pixels).
 * @param x, y: coordinates
 * @pre (x|y) has to be on the screen
 * returns true when there the player
 */
static bit checkForPlayer(unsigned char x, unsigned char y, bit killInvader)
{
	unsigned char subX, subY;
	ASSERT(x < DISPLAY_SIZE_X);
	ASSERT(y < DISPLAY_SIZE_Y);

	if((DISPLAY_SIZE_Y-y < PLAYER_HEIGHT) && (y-PlayerPositionX >=0) && (y-PlayerPositionX <PLAYER_WIDTH))
		return 1;
	return 0;

}

static void movePlayerMissile(void)
{
	if(PlayerMissileActive)
	{
		if(PlayerMissileY == 0)
		{
			PlayerMissileActive = 0;
		} else
		{
			PlayerMissileY--;
		}
			/* block collision is done outside (no really good idea) todo */
	}

}

static void moveInvaderMissiles(void)
{
	unsigned char i = 0;
	for (i = 0; i < NUM_INVADERS_X; ++i)
	{
		if (InvaderMissileActive & (1 << i)) {
			InvaderMissileY[i] ++;
			if((InvaderMissileY[i] >= DISPLAY_ROWS) ||
			   (checkBlockCollision(InvaderMissileX[i], InvaderMissileY[i], 1)))
				InvaderMissileActive &= ~(1 << i);
		}
	}
}

/**
 *  moves the player in given direction.
 *  @param right when true, move to the right. else to the left.
 *  */
static void movePlayer(bit right)
{
	if(!right)
	{
		if(PlayerPositionX > 0)
			PlayerPositionX -= 1;
	} else {
		if(PlayerPositionX + PLAYER_WIDTH < DISPLAY_SIZE_X)
			PlayerPositionX += 1;
	}
}

/**
 * creates a shot if there is no active one
 * */
static void shoot()
{
	if(!PlayerMissileActive)
	{
		playSample(5,0,20);
		PlayerMissileX = PlayerPositionX + PLAYER_WIDTH/2;
		PlayerMissileY = DISPLAY_SIZE_Y - 1 - PLAYER_HEIGHT;
		PlayerMissileActive = 1;
	}
}


static void initGame(void)
{
	unsigned char i;

	InvadersAliveCnt = 0;

	for(i = 0; i < NUM_INVADERS_X * NUM_INVADERS_Y; ++i)
		INVADER_SET_ALIVE_L(i);

	PlayerPositionX = (DISPLAY_SIZE_X - PLAYER_WIDTH) / 2;

	InvaderPosX = 0;
	InvaderPosY = 0;

	PlayerMissileActive = 0;
	InvaderMovementRight = 1;

	Block[0] = BLOCK0;
	Block[1] = BLOCK1;
	Block[2] = BLOCK2;
	Block[3] = BLOCK3;
	Block[4] = BLOCK4;
	Block[5] = BLOCK5;
	Block[6] = BLOCK6;
	Block[7] = BLOCK7;
	Block[8] = BLOCK8;
	Block[9] = BLOCK9;
}

static void draw()
{
	unsigned char x, y, i;
	/* draw blocks */
	for(y = 0; y < BLOCK_HEIGHT; ++y)
	{
		for(x = 0; x < DISPLAY_SIZE_X; ++x)
		{
			unsigned char blockHealth = (Block[x/4+5*y] & (3 << (2*(x%4)))) >> (2*(x%4));
				displayPixel(x,y + (DISPLAY_SIZE_Y - 1 - BLOCK_HEIGHT - PLAYER_HEIGHT),
						(blockHealth) > 1 ? COLOR_FULL : blockHealth);
		}

	}


	/* draw invaders */
	for(x = 0; x < NUM_INVADERS_X; ++x)
	{
		for(y = 0; y < NUM_INVADERS_Y; ++y)
		{
			if(INVADER_IS_ALIVE(x, y))
			{
				displayPixel(x * (INVADER_WIDTH + INVADER_W_SPACE) + InvaderPosX, y * (INVADER_HEIGHT + INVADER_H_SPACE) + InvaderPosY, COLOR_FULL);
				displayPixel(x * (INVADER_WIDTH + INVADER_W_SPACE) + InvaderPosX + 1, y * (INVADER_HEIGHT + INVADER_H_SPACE) + InvaderPosY, COLOR_FULL);
			}
		}
	}

	/* draw player */
	displayPixel(PlayerPositionX, DISPLAY_SIZE_Y - 1, COLOR_HALF);
	displayPixel(PlayerPositionX+1, DISPLAY_SIZE_Y - 1, COLOR_FULL);
	displayPixel(PlayerPositionX+2, DISPLAY_SIZE_Y - 1, COLOR_HALF);

	/* draw missile */
	if(PlayerMissileActive)
	{
		displayPixel(PlayerMissileX, PlayerMissileY, COLOR_FULL);
	}

	/* draw invader missiles */
	for(i = 0; i < NUM_INVADERS_X; ++i)
	{
		if(InvaderMissileActive & (1 << i))
		{
			displayPixel(InvaderMissileX[i], InvaderMissileY[i], COLOR_FULL);
		}
	}


	/* finally output new picture! */
	displayChangeBuffer();
}

static void clearBlocks(void)
{
	unsigned char i;
	for(i = 0; i < (DISPLAY_SIZE_X*BLOCK_HEIGHT*2+CHAR_BIT-1)/CHAR_BIT; ++i)
	{
		Block[i] = 0;
	}
}

static void checkInvaderBlockCollision()
{
	unsigned char x;
	for(x = 0; x < DISPLAY_SIZE_X; ++x)
		if(checkForInvader(x, DISPLAY_SIZE_Y - 1 - PLAYER_HEIGHT - BLOCK_HEIGHT, 0))
		{
			clearBlocks();
			return;
		}
}


static bit checkInvaderPlayerCollision()
{
	unsigned char x;
	for(x = 0; x < DISPLAY_SIZE_X; ++x)
		if(checkForInvader(x, DISPLAY_SIZE_Y - 1, 0))
			return 1;
	return 0;
}


unsigned char game(void)
{
	unsigned char lastInvaderMovement = GameTimer;
	unsigned char lastMissileMovement = GameTimer;
	unsigned char lastPlayerMove = GameTimer;
	unsigned char lastPlayerShot = GameTimer;
	unsigned char invaderMoveSoundNo = 0;
	unsigned char invaderSpeed = INVADER_MOVEMENT_BASE_SPEED_MS;
	bit gameRunning = 1;
	bit redraw = 0;
	initGame();
	srand(GameTimer);
	draw();
	while(gameRunning)
	{
		if(GAME_TIME_DIFF(lastPlayerMove) >=(PLAYER_MOVEMENT_SPEED_MS*GAME_TIMEBASE_HZ/1000))
		{
			lastPlayerMove = GameTimer;
			if(KeyIsPressed(KEY_LEFT))
			{
				movePlayer(0);
				redraw = 1;
			}
			if(KeyIsPressed(KEY_RIGHT))
			{
				movePlayer(1);
				redraw = 1;
			}

		}

		if(KeyIsPressed(KEY_ENTER))
		{
			shoot();
			/** todo code replication from below!! */
			if(checkForInvader(PlayerMissileX, PlayerMissileY, 1))
						{ /*
			 * maybe player can win here? todo
			 */
				PlayerMissileActive = 0;
			} /** end code replication */
			redraw = 1;
		}

		if(GAME_TIME_DIFF(lastInvaderMovement) >=getInvaderSpeed())
		{
#ifdef _DEBUG
			fprintf(stderr, "gt: %d move before incr: %d\n", GameTimer, nextInvaderMovement);
#endif
			lastInvaderMovement = GameTimer;

			playSample((invaderMoveSoundNo++)%4 + 7,3,20);
			moveInvaders();
			checkInvaderBlockCollision();
			if(checkInvaderPlayerCollision())
			{
				gameRunning = 0;
			}
			redraw = 1;

		}
		if(GAME_TIME_DIFF(lastMissileMovement)
				>=(MISSILE_MOVEMENT_SPEED_MS*GAME_TIMEBASE_HZ/1000))
		{
			lastMissileMovement=GameTimer;

			if(getRandom() % 8 == 0)
				invaderShoot();

			if(PlayerMissileActive)
			{
				movePlayerMissile();
				if(checkBlockCollision(PlayerMissileX, PlayerMissileY, 1))
					PlayerMissileActive = 0;
				if(checkForInvader(PlayerMissileX, PlayerMissileY, 1))
				{
					if(!InvadersAliveCnt)
						gameRunning=0;
					PlayerMissileActive = 0;
					playSample(6,2,20);
				}
			}
			moveInvaderMissiles();

			redraw = 1;
		}
		if(redraw)
		{
			redraw = 0;
			draw();
			handleSPI();
			LED_OFF();
			EA = 1;
		}
	}
	return !InvadersAliveCnt;
}
