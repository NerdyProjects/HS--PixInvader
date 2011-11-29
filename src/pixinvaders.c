/*
 * pixinvader.c
 *
 *  Created on: 23.11.2011
 *      Author: matthias
 */

#include "main.h"
#include "keys.h"
#include "display.h"

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


static data unsigned char InvaderPosY = 0;
static data signed char InvaderPosX = 0;

static data unsigned char InvadersAlive[(NUM_INVADERS_X*NUM_INVADERS_Y+CHAR_BIT-1)/CHAR_BIT];
static data unsigned char Block[(DISPLAY_SIZE_X*BLOCK_HEIGHT*2+CHAR_BIT-1)/CHAR_BIT];
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


static data unsigned char PlayerMissileX;
static data unsigned char PlayerMissileY;
static bit PlayerMissileActive;

static data unsigned char PlayerPositionX;

/* periodically incremented by call from external timer */
static data volatile unsigned char GameTimer;

#define INVADER_MOVEMENT_SPEED 30
#define MISSILE_MOVEMENT_SPEED 5

#define INVADER_BYTE(x,y) ((y*NUM_INVADERS_X+x) / CHAR_BIT)
#define INVADER_BIT(x,y) ((y*NUM_INVADERS_X+x) % CHAR_BIT)
#define INVADER_BYTE_L(i) (i / CHAR_BIT)
#define INVADER_BIT_L(i) (i % CHAR_BIT)
#define INVADER_IS_ALIVE(x,y) (InvadersAlive[INVADER_BYTE(x,y)] & (1 << (INVADER_BIT(x,y))))
#define INVADER_SET_ALIVE(x,y) (InvadersAlive[INVADER_BYTE(x,y)] |= (1 << (INVADER_BIT(x,y))))
#define INVADER_SET_DEAD(x,y) (InvadersAlive[INVADER_BYTE(x,y)] &= ~(1 << (INVADER_BIT(x,y))))
#define INVADER_SET_ALIVE_L(i) (InvadersAlive[INVADER_BYTE_L(i)] |= (1 << INVADER_BIT_L(i)))
#define INVADER_IS_ALIVE_L(i) (InvadersAlive[INVADER_BYTE_L(i)] & (1 << INVADER_BIT_L(i)))

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
#endif

	if(((InvaderPosX + left*(INVADER_WIDTH+INVADER_W_SPACE) == 0) && !InvaderMovementRight) ||
			((InvaderPosX + (right + 1)*(INVADER_WIDTH+INVADER_W_SPACE) - INVADER_W_SPACE) >= DISPLAY_COLS) && InvaderMovementRight)
	{
		InvaderPosY++;
		InvaderMovementRight = !InvaderMovementRight;
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


			if(checkForInvader(PlayerMissileX, PlayerMissileY, 1))
			{	/*
				 * maybe player can win here? todo
				 */
				PlayerMissileActive = 0;
			}
		}
			/* todo: collision with mothership, collision with block */
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
		PlayerMissileX = PlayerPositionX + PLAYER_WIDTH/2;
		PlayerMissileY = DISPLAY_SIZE_Y - 1 - PLAYER_HEIGHT;
		PlayerMissileActive = 1;
	}
}


static void initGame(void)
{
	unsigned char i;
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
	unsigned int x, y;
	/* draw blocks */
	for(y = 0; y < BLOCK_HEIGHT; ++y)
	{
		for(x = 0; x < DISPLAY_SIZE_X; ++x)
		{
				displayPixel(x,y + (DISPLAY_SIZE_Y - 1 - BLOCK_HEIGHT - PLAYER_HEIGHT),
						(Block[x/4+5*y] & (3 << (2*(x%4)))) >> (2*(x%4)));
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

static bit checkBlockCollision(unsigned char x, unsigned char y, bit hitBlock)
{
	unsigned char blockThere;
	ASSERT(x < DISPLAY_SIZE_X);
	ASSERT(y < DISPLAY_SIZE_Y);
	y -= DISPLAY_SIZE_Y - 1 - BLOCK_HEIGHT - PLAYER_HEIGHT;
	if(y > DISPLAY_SIZE_Y)
		return 0;

	if(Block[x/4+5*y] & (3 << (2*(x%4))))
	{
		if(hitBlock)
			Block[x/4+5*y]  -= (1 << (2*(x%4)));
		return 1;
	}

	return 0;
}


static bit checkInvaderPlayerCollision()
{
	unsigned char x;
	for(x = 0; x < DISPLAY_SIZE_X; ++x)
		if(checkForInvader(x, DISPLAY_SIZE_Y - 1, 0))
			return 1;
	return 0;
}


void game(void)
{
	unsigned char nextInvaderMovement = 0;
	unsigned char nextShotMovement = 0;
	bit gameRunning = 1;
	bit redraw = 0;
	initGame();
	draw();
	while(gameRunning)
	{
		if(keyPress(KEY_LEFT))
		{
			movePlayer(0);
			redraw = 1;
		}
		if(keyPress(KEY_RIGHT))
		{
			movePlayer(1);
			redraw = 1;
		}
		if(keyPress(KEY_ENTER))
		{
			shoot();
			redraw = 1;
		}
		if((unsigned char)(GameTimer - nextInvaderMovement) < (unsigned char)127)
		{
#ifdef _DEBUG
			fprintf(stderr, "gt: %d move before incr: %d\n", GameTimer, nextInvaderMovement);
#endif
			nextInvaderMovement += INVADER_MOVEMENT_SPEED;
			moveInvaders();
			checkInvaderBlockCollision();
			if(checkInvaderPlayerCollision())
			{
				gameRunning = 0;
			}
			redraw = 1;

		}
		if(GameTimer >= nextShotMovement)
		{
			nextShotMovement += MISSILE_MOVEMENT_SPEED;
			if(PlayerMissileActive)
			{
				movePlayerMissile();
				if(checkBlockCollision(PlayerMissileX, PlayerMissileY, 1))
					PlayerMissileActive = 0;
			}

			redraw = 1;
		}
		if(redraw)
		{
			redraw = 0;
			draw();
		}
	}

}


void gameTime(void)
{
	GameTimer++;

}
