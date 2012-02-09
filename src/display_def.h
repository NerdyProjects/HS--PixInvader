/**
 * @date 23.11.2011
 * @author matthias
 * @author nils
 */


#ifndef DISPLAY_DEF_H_
#define DISPLAY_DEF_H_

/* size of display. Electrical specification of display may forbid changing these
 * definitions. We always write one complete col in one matrix at once. All same
 * numbered columns in a matrix are activated at the same time through bits 3-5
 * in SELECT register. Bits 0-2 select the matrix the next data write goes to.
 * */
#define DISPLAY_ROWS 14
#define DISPLAY_COLS 20
#define DISPLAY_COLS_PER_MATRIX 5
#define DISPLAY_ROWS_PER_MATRIX 7
#define DISPLAY_MATRICES 8

#define DISPLAY_BUFFER_BYTES_PER_COLOR (DISPLAY_COLS_PER_MATRIX * DISPLAY_MATRICES)

#define DISPLAY_BLANK (0xF8)
/* software generates multiple output colors that are saved in this many bits: */
#define DISPLAY_COLOR_BITS 2



/* sets the number of ON colors that can be generated.
 */
#define DISPLAY_COLORS 2

#define COLOR_FULL 3
#define COLOR_HALF 1
#define COLOR_OFF 0

/* change display timing after each color.
 * increase timer reload high byte by that value for first color,
 * decrease for second color.
 * that leads to a lighter first color and a brighter second color.
 * also allows triggering a third color by setting only the second one.
 */
#define COLOR_FULL_STRETCH	0x05

#endif /* DISPLAY_H_ */
