//
//  project.h
//  
//
//  Created by Varun Sampat on 05/04/19.
//

#ifndef project_h
#define project_h

#define RESOLUTION_X 320
#define RESOLUTION_Y 240

#define COLS 3
#define ROWS 5

#define TILE_WIDTH  50      // think of this  -> technically make it 320/3. ~~ 106pixels
#define TILE_HEIGHT (RESOLUTION_Y / ROWS)      // 4 rows per display
#define FRACTION_TILES_SKIPPING 3              // used for animation
// define black as 1, white as 0
#define BLACK 1
#define WHITE 0

// generates 0, 1, 2, ... COLS-1 -> left ->center -> right col

#define GAME_OVER -1
#define LOAD_SCREEN 0
#define GAME_RUNNING 1


extern short OPENING_SCREEN[240][320];

const short int PIANO_TILE_COLOR = 0x0821;                    // black
const short int BLANK_TILE_COLOR = 0xFFFF;                    // white
const short int CURRENT_TILE_COLOR = 0x23E1;                  // green
volatile int pixel_buffer_start;                               // global variable used for display

//bool gameOver;                                                  // global variable corresponding to the state of the game
int score;                                                      // score that will be displayed on VGA
int visibleGrid[ROWS][COLS];                 // For animation purposes, not all rows will be displayed.
int gridForDrawing[ROWS*FRACTION_TILES_SKIPPING][COLS];            // for displaying, top row of visibleGrid is never used
int correctColumn;  // Store the next column key to press in the game
int score;  // Store user's score
int timer[4];
int offset;

// Display related functions:
void wait_for_vsync();
void plot_pixel(int x, int y, short int line_color);
void draw_line(int x0, int y0, int x1, int y1, short int color);

void loadScreen();
void initializeScreen();
void updateScreen();
void clear_screen();
void gameOver();

void displayChar(int x, int y, char c);

// setting grid to display,
void generateRow(int rowNumber);
void generateGrid();
void drawGrid();
void updateGrid();

void animateGridForDrawing();
void setVisiblieGridToGridForDrawing();

// access data from KEYs
int keyPressed();

// Reset game
void resetGame();
void HEXScoreUpdate();
void HEXTimerUpdate();
void drawOpeningScreen();


//timer related functions:
void initializeTimer(int loadValue);
void startTimer();
bool timeUp();
void resetTimer();


#endif /* project_h */
