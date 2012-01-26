/*
 * pixinvaders.h
 *
 *  Created on: 28.11.2011
 *      Author: matthias
 */

#ifndef PIXINVADERS_H_
#define PIXINVADERS_H_

/* set increment rate of game timer */
#define GAME_TIMEBASE_HZ 50

/**
 * Game routine.
 * @return 1: player won the game.
 * 		   2: player lost the game.
 */
unsigned char game(void);

#endif /* PIXINVADERS_H_ */
