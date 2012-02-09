/**
 * @date 26.11.2011
 * @author matthias
 * @author nils
 */

#ifndef KEYS_H_
#define KEYS_H_
#include "main.h"

/** Key bit definitions for the left-key. Must correspond to port pin numbers */
#define KEY_LEFT	(1 << 2)
/** Key bit definitions for the right-key. Must correspond to port pin numbers */
#define KEY_RIGHT	(1 << 0)
/** Key bit definitions for the enter/shoot-key. Must correspond to port pin numbers */
#define KEY_ENTER	(1 << 1)
/** Key bit definitions for the auxillary-key. Must correspond to port pin numbers */
#define KEY_AUX		(1 << 3)
/** Key bit definitions for the all keys.*/
#define KEY_ALL		(KEY_LEFT | KEY_RIGHT | KEY_ENTER | KEY_AUX)

void keyInit(void);
bit KeyIsPressed(unsigned char keyMask);


#endif /* KEYS_H_ */
