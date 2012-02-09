/**
 * @file pixinvader.c
 * @date 23.11.2011
 * @author matthias
 * @author nils
 * @brief Implements the core of the game.
 * This module implements the Pixinvaders.
 */

#include <stdlib.h>
#include "main.h"
#include "keys.h"
#include "display.h"
#include "spi_command.h"
#include "sound.h"

#include "pixinvaders.h"

/** Width of the display in pixel */
#define DISPLAY_SIZE_X 20
/** Height of the display in pixel */
#define DISPLAY_SIZE_Y 14

/** Width of the invader field */
#define NUM_INVADERS_X 5
/** Height of the invader field */
#define NUM_INVADERS_Y 4

/** The form of a single invader. Width. */
#define INVADER_WIDTH 2
/** The form of a single invader. Height. */
#define INVADER_HEIGHT 1

/** Horizontal space between two invaders in the field */
#define INVADER_W_SPACE 1
/** Vertical space between two invaders in the field */
#define INVADER_H_SPACE 1

/** Width of the player ship.
 * This define is only used for collision detection.
 * If you change this variable, you have to change the rendering function for the player ship.
 * @see draw()
 */
#define PLAYER_WIDTH 3
/** Height of the player ship  * This define is only used for collision detection.
 * If you change this variable, you have to change the rendering function for the player ship.
 * @see draw()
 */
#define PLAYER_HEIGHT 1

/** Block width is display width, height is not really modifiable */
#define BLOCK_HEIGHT 2

/** The top left corner of the invader field, x component */
static xdata unsigned char InvaderPosY = 0;

/** The top left corner of the invader field, y component */
static xdata signed char InvaderPosX = 0;

/** Bitfield with all invaders. If 1 the invader ist active/alive */
static xdata unsigned char InvadersAlive[(NUM_INVADERS_X*NUM_INVADERS_Y+CHAR_BIT-1)/CHAR_BIT];
/** Bitfield with the barricades. */
static xdata unsigned char Block[(DISPLAY_SIZE_X*BLOCK_HEIGHT*2+CHAR_BIT-1)/CHAR_BIT];
/** Number of alive invaders. */
static xdata unsigned char InvadersAliveCnt;

/** Barricades. From left to right, 2 bit HP */
#define BLOCK0 0x80
#define BLOCK5 0x80
#define BLOCK1 0x0A
#define BLOCK6 0x0A
#define BLOCK2 0xA8
#define BLOCK7 0xA8
#define BLOCK3 0x80
#define BLOCK8 0x80
#define BLOCK4 0x0A
#define BLOCK9 0x0A

/** Indicates the current direction of the Invader field. */
static bit InvaderMovementRight;

/** Current position of player missile, x component. */
static xdata unsigned char PlayerMissileX;
/** Current position of player missile, y component. */
static xdata unsigned char PlayerMissileY;
/** Indicates if the player missile is active. */
static bit PlayerMissileActive;

/** Every column of invader can shoot one missile. This is the x position of each invader missile. */
static xdata unsigned char InvaderMissileX[NUM_INVADERS_X];
/** Every column of invader can shoot one missile. This is the y position of each invader missile. */
static xdata unsigned char InvaderMissileY[NUM_INVADERS_X];
#if NUM_INVADERS_X > 8
#error "invader missile structure does not allow more than 8 adjacent invaders"
#endif

/** This byte, used as bitfield, indicates wich invader column has a active missile. */
static xdata unsigned char InvaderMissileActive;

/** Current playe rx position. The player is alway on the bottom of the screen.*/
static xdata unsigned char PlayerPositionX;

/** Periodically incremented by call from external timer */
data volatile unsigned char GameTimer;

/** Base speed of the invaders. The actual speed depends on on the number of invaders alive. */
#define INVADER_MOVEMENT_SPEED_BASE_MS 35UL

/** The moving speed of the missiles. (millisecond/pixel)*/
#define MISSILE_MOVEMENT_SPEED_MS 230UL
/** The Moving speed of the player. (milliseconds/pixel) */
#define PLAYER_MOVEMENT_SPEED_MS 100UL
/** This macro calculates the number of game ticks since x. */
#define GAME_TIME_DIFF(x) (((unsigned short)GameTimer+256-(unsigned short)(x))%256)

/**
 * Field macros for invaders.
 * Select byte for given invader position in field.
 * @param x horizontal position of invader in field.
 * @param y vertical position of invader in field.
 * @return byte index.
 */
#define INVADER_BYTE(x,y) (((y)*NUM_INVADERS_X+x) / CHAR_BIT)
/**
 * Field macros for invaders.
 * Select bit for given invader position in field.
 * @param x horizontal position of invader in field.
 * @param y vertical position of invader in field.
 * @return bit index.
 */
#define INVADER_BIT(x,y) (((y)*NUM_INVADERS_X+x) % CHAR_BIT)
/**
 * Field macros for invaders.
 * Select byte for given invader index in field.
 * @param i index of invader in field.
 * @return byte index.
 */
#define INVADER_BYTE_L(i) (i / CHAR_BIT)
/**
 * Field macros for invaders.
 * Select bit for given invader index in field.
 * @param i index of invader in field.
 * @return bit index.
 */
#define INVADER_BIT_L(i) (i % CHAR_BIT)
/**
 * Field macros for invaders.
 * Test if a given invader (x,y) is alive.
 * @param x horizontal position of invader in field.
 * @param y vertical position of invader in field.
 * @return true if alive
 */
#define INVADER_IS_ALIVE(x,y) (InvadersAlive[INVADER_BYTE(x,y)] & (1 << (INVADER_BIT(x,y))))
/**
 * Field macros for invaders.
 * Set a given invader (x,y) to alive.
 * @param x horizontal position of invader in field.
 * @param y vertical position of invader in field.
 */
#define INVADER_SET_ALIVE(x,y) {InvadersAlive[INVADER_BYTE(x,y)] |= (1 << (INVADER_BIT(x,y)));InvadersAliveCnt++;}
/**
 * Field macros for invaders.
 * Set a given invader (x,y) to dead.
 * @param x horizontal position of invader in field.
 * @param y vertical position of invader in field.
 */
#define INVADER_SET_DEAD(x,y) {InvadersAlive[INVADER_BYTE(x,y)] &= ~(1 << (INVADER_BIT(x,y)));InvadersAliveCnt--;}
/**
 * Field macros for invaders.
 * Set a given invader to alive by index.
 * @param i index of invader in field.
 */
#define INVADER_SET_ALIVE_L(i) {InvadersAlive[INVADER_BYTE_L(i)] |= (1 << INVADER_BIT_L(i));InvadersAliveCnt++;}
/**
 * Field macros for invaders.
 * Set a given invader to dead by index.
 * @param i index of invader in field.
 * @return true if alive
 */
#define INVADER_IS_ALIVE_L(i) (InvadersAlive[INVADER_BYTE_L(i)] & (1 << INVADER_BIT_L(i)))

/**
 * Calculates the current speed of the invaders.
 * @return invader
 */
static unsigned char getInvaderSpeed(void)
{
	return 3*(InvadersAliveCnt+1);
}

/**
 * Returns a rondom number.
 * @return number.
 */
static unsigned char getRandom(void)
{
	return rand();
}

/**
 * Returns the y position of the lowest invader.
 */
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

/**
 * Shoots a missile from one invader (under certain contitions).
 */
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

/**
 * @param x
 * @param y
 * @param hitBlock if true and collision was detected decrement the hitpoint of the block.
 * @return true if collision
 */
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

/**
 * Moves the invader fields.
 */
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
static bit checkForPlayer(unsigned char x, unsigned char y)
{
	ASSERT(x < DISPLAY_SIZE_X);
	ASSERT(y < DISPLAY_SIZE_Y);

	if((DISPLAY_SIZE_Y-y < PLAYER_HEIGHT) && (x-PlayerPositionX >=0) && (x-PlayerPositionX <PLAYER_WIDTH))
		return 1;
	return 0;

}

/**
 * Moves the player missile.
 */
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
/**
 * @return 1 if player died
 */
static unsigned char moveInvaderMissiles(void)
{
	unsigned char i = 0;
	for (i = 0; i < NUM_INVADERS_X; ++i)
	{
		if (InvaderMissileActive & (1 << i)) {
			InvaderMissileY[i] ++;
			if((InvaderMissileY[i] >= DISPLAY_ROWS) ||
			   (checkBlockCollision(InvaderMissileX[i], InvaderMissileY[i], 1)))
				InvaderMissileActive &= ~(1 << i);
			if(checkForPlayer(InvaderMissileX[i], InvaderMissileY[i])){
				return 1;
			}
		}
	}
	return 0;
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

 /**
  * Initialize all needed variables for the game.
  */
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

/**
 * Draw the game to the screen. (blocks, invaders, player, missiles)
 */
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

/** Delete all block-hitpoints */
static void clearBlocks(void)
{
	unsigned char i;
	for(i = 0; i < (DISPLAY_SIZE_X*BLOCK_HEIGHT*2+CHAR_BIT-1)/CHAR_BIT; ++i)
	{
		Block[i] = 0;
	}
}

/**
 * Check if the invader collided with the blocks.
 * If a collision is detected, all blocks get deleted.
 */
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

/**
 * Check if the invader collided with the player.
 */
static bit checkInvaderPlayerCollision()
{
	unsigned char x;
	for(x = 0; x < DISPLAY_SIZE_X; ++x)
		if(checkForInvader(x, DISPLAY_SIZE_Y - 1, 0))
			return 1;
	return 0;
}

/**
 * The game core routine.
 * All timing for the game is provided by this routine.
 * The Timing source is the @see GameTimer.
 */
unsigned char game(void)
{
	unsigned char lastInvaderMovement = GameTimer;
	unsigned char lastMissileMovement = GameTimer;
	unsigned char lastPlayerMove = GameTimer;
	unsigned char lastPlayerShot = GameTimer;
	unsigned char invaderMoveSoundNo = 0;
	unsigned char invaderSpeed = getInvaderSpeed();
	bit gameRunning = 1;
	bit redraw = 0;
	initGame();
	srand(GameTimer);
	draw();

	/* Game main loop. @see gameRunning ist set to false if the game hit a condtion to stop the game.*/
	while(gameRunning)
	{
		/* Player movements. */
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

		/* Start player missile. (shoot!) */
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


		/* Invader Movement */
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

		/* Missile movement. */
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
					PlayerMissileActive = 0;
					playSample(6,2,20);
				}
			}

			if(moveInvaderMissiles()){
				gameRunning = 0;
			}

			redraw = 1;
		}

		/* Redraw game if necessary. */
		if(redraw)
		{
			redraw = 0;
			draw();
		}
		/* Stop game if there are no more invaders */
		if(!InvadersAliveCnt)
			gameRunning=0;

	}

	/* If all invaders are dead the player won, otherwise the player lost.*/
	return !InvadersAliveCnt;
}
