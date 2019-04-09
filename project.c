#define RESOLUTION_X 320
#define RESOLUTION_Y 240

#define COLS 3
#define ROWS 4
    
#define TILE_WIDTH  50      // think of this  -> technically make it 320/3. ~~ 106pixels
#define TILE_HEIGHT (RESOLUTION_Y / ROWS)      // 4 rows per display
#define FRACTION_TILES_SKIPPING 4              // used for animation
// define black as 1, white as 0
#define BLACK 1
#define WHITE 0

// generates 0, 1, 2, ... COLS-1 -> left ->center -> right col

// Game key press codes
#define GAME_OVER -1
#define RESET_GAME 0
#define GAME_RUNNING 1
#define GAME_INCREMENT 2

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

extern short OPENING_SCREEN[240][320];  // Game Opening Image
    
const short int PIANO_TILE_COLOR = 0x31A6;  // #333333 (dark grey)
const short int BLANK_TILE_COLOR = 0xFFFF;  // #FFFFFF (White)
const short int CURRENT_TILE_COLOR = 0xD2AF; // #D2527F (Pink)
volatile int pixel_buffer_start;                               // global variable used for display

int score;  // Game score on HEX
int gameGrid[ROWS][COLS];   // Game tile grid
int gridForDrawing[ROWS * FRACTION_TILES_SKIPPING][COLS];   // Game animated drawing grid
int correctColumn;  // Next column key to press in the game
int offset;

// Display related functions:
void waitForVsync();
void plotPixel(int x, int y, short int line_color);
void swap(int* x, int* y);
void drawLine(int x0, int y0, int x1, int y1, short int color);

void loadScreen();
void initializeScreen();
void updateScreen();
void clearScreen();
void gameOver();

void displayChar(int x, int y, char c);

// setting grid to display,
void generateRow(int rowNumber);
void generateGameGrid();
void drawDrawingGrid();
void updateGameGrid();

void animateGridForDrawing();
void updateDrawingGrid();
void drawEndScreen();

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

void waitForGameStart();

int main() {
    bool gameOn = true;
    int response; 
    // Seed random number generator
    srand(time(0));

    // Clear and set all buffers
    initializeScreen();

    // Game start
    while(gameOn) {
        score = 0;
        offset = 0;

        // Reset HEX Diplay
        HEXScoreUpdate();

        // Generate a new starting grid
        generateGameGrid();
        // Update drawing grid
        updateDrawingGrid();

         // Draw opening game image
        drawOpeningScreen();
        // Wait for KEY0 to be pressed
        waitForGameStart();

        clearScreen();
        updateScreen();
        clearScreen();

        // An instance of a game
        while(true) {
            
            if(offset == FRACTION_TILES_SKIPPING){
                // Check if the user pressed the correct key
                // in the past fractional row shifts
                response = keyPressed();

                if(response == GAME_INCREMENT) {
                    score += 1;
                    HEXScoreUpdate();
                } else if (response == GAME_OVER) {
                    gameOn = false;
                    break;
                } else if (response == RESET_GAME) {
                    break;
                }

                offset = 0;

                // Play note?

                updateGameGrid();
                updateDrawingGrid();
            }

            drawDrawingGrid();
            animateGridForDrawing();

            offset += 1;

            // Timer for delay between row shifts
            volatile int* hardwareTimePtr = (int*) 0xFFFEC600;
            // Enable counter and automatic reload
            *(hardwareTimePtr + 2) = 3;
            // Loading 50 million for a 200MHz timer
            // (initially 0.375s per fractional row)
            (*hardwareTimePtr) = 75000000;

            while(*(hardwareTimePtr + 3) == 0);
            // printf("%x\n", hardwareTimePtr + 3);

            *(hardwareTimePtr + 3) = *(hardwareTimePtr + 3);
        }
    }

    // Game Over screen
    drawEndScreen();
}

void waitForGameStart() {
    volatile int* keysEdgePtr = (int*) 0xFF20005C;

    while(true) {
        while((*keysEdgePtr) == 0);

        if((*keysEdgePtr) == 1) {
            (*keysEdgePtr) = (*keysEdgePtr);
            break;
        }
        
        (*keysEdgePtr) = (*keysEdgePtr);
    }
}

void initializeScreen(){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    
    *(pixel_ctrl_ptr + 1) = 0xC8000000;

    waitForVsync();

    pixel_buffer_start = *pixel_ctrl_ptr;
    
    clearScreen();

    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
}

void updateScreen(){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    waitForVsync(); // swap front and back buffers on VGA vertical sync
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
}

void waitForVsync(){
    volatile int* pixel_ctrl_ptr = (int*)0xFF203020; // status register
    volatile int* statusRegisterPtr = pixel_ctrl_ptr + 3;

    *(pixel_ctrl_ptr) = 1;

    while((*statusRegisterPtr & 0x1) != 0);
}

void plotPixel(int x, int y, short int line_color){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void clearScreen(){
    int x, y;

    for(x = 0; x < RESOLUTION_X; x++) {
        for(y = 0; y < RESOLUTION_Y; y++) {
            // plotPixel(x,y,0xAE23); // 0x0 is black
            plotPixel(x,y,0xAF9F); // 0x0 is black
        }
    }
}

void drawTile(int x, int y, short int line_color){
    // x and y define the TOP-LEFT corner of the tile
    int i, j;
    for(i = 0; i < TILE_WIDTH; i++)
        for (j = 0; j < TILE_HEIGHT/FRACTION_TILES_SKIPPING; j++)
            plotPixel(x+i, y+j, line_color);
}

// Function to draw line
void drawLine(int x0, int y0, int x1, int y1, short int color) {
    bool is_steep = abs(y1 - y0) > abs(x1 - x0);

    if(is_steep) {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }

    if(x0 > x1) {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }

    int deltax = x1 - x0;
    int deltay = abs(y1 - y0);
    int error = -(deltax / 2);
    int y = y0;
    int y_step;

    if(y0 < y1) {
        y_step = 1;
    } else {
        y_step = -1;
    }
    int x;
    for(x = x0; x <= x1; x++) {
        if(is_steep) {
            plotPixel(y, x, color);
        } else {
            plotPixel(x, y, color);
        }

        error += deltay;

        if(error >= 0) {
            y = y + y_step;
            error -= deltax;
        }
    }
}

void drawOpeningScreen() {
   volatile short* pixelbuf = pixel_buffer_start;
    int i, j;

    for (i=0; i<240; i++) {
        for (j=0; j<320; j++) {
            // plotPixel(j, i, 0xE000);
            // plotPixel(j, i, OPENING_SCREEN[i][j]);
           *(pixelbuf + (j<<0) + (i<<9)) = OPENING_SCREEN[i][j];
            // *(pixelbuf + (j<<0) + (i<<9)) = 0xE000;
       }
    }

    updateScreen();
}

void drawEndScreen() {
   volatile short * pixelbuf = pixel_buffer_start;
    int i, j;

    for (i=0; i<240; i++) {
        for (j=0; j<320; j++) {
            plotPixel(j, i, 0x0);
           // *(pixelbuf + (j<<0) + (i<<9)) = OPENING_SCREEN[i][j];
       }
    }

    updateScreen();
}

void generateRow(int rowNumber){
    // only one black tile per row
    int indexForBlackTile = rand()%(COLS); // must do srand
    // generates 0, 1, 2, ... COLS-1 -> left ->center -> right col
    gameGrid[rowNumber][indexForBlackTile] = BLACK;
    // need to place a white tile in the other two tiles
    int i;
    for(i = 0; i < COLS; i++)
        if(i != indexForBlackTile)
            gameGrid[rowNumber][i] = WHITE;
}

void generateGameGrid(){
    int j;

    for(j = 0; j < ROWS; j++){
        generateRow(j);
    }

    // Get correct column key to press
    for(j = 0; j < COLS; j++) {
        if(gameGrid[ROWS-1][j] == BLACK) {
            correctColumn = j;
            break;
        }
    }
}

//assume the func is called when the user presses the RIGHT key
void updateGameGrid(){
    // first must shift change
    int i,j;
    for(i = ROWS-2; i >= 0; i--){
        for(j = 0; j < COLS; j++){
            // shift everything one row below
            gameGrid[i+1][j] = gameGrid[i][j];
        }
    }
    // update the top-most row
    generateRow(0);

    // Get correct column key to press
    // shouldn't have this function here OR just do it in the first for loop
    for(j = 0; j < COLS; j++) {
        if(gameGrid[ROWS - 1][j] == BLACK) {
            correctColumn = j;
            break;
        }
    }
}

void drawDrawingGrid(){
    // Center offset
    int offset_x = (RESOLUTION_X - (TILE_WIDTH * COLS)) / 2;
    int startingIndex;
    int i, j;

    // Vertical lines on both sides of the playing grid
    drawLine(offset_x - 2, 0, offset_x - 2, RESOLUTION_Y - 1, 0x0000);
    drawLine(COLS*TILE_WIDTH + offset_x + 2, 0, COLS*TILE_WIDTH + offset_x + 2, RESOLUTION_Y - 1, 0x0000);
    drawLine(offset_x + COLS*TILE_WIDTH + 1, 0, offset_x + COLS*TILE_WIDTH + 1, RESOLUTION_Y - 1, 0x0000);
    drawLine(offset_x + COLS*TILE_WIDTH, 0, offset_x + COLS*TILE_WIDTH, RESOLUTION_Y - 1, 0x0000);
    
    for(startingIndex = 0; startingIndex < (ROWS)*FRACTION_TILES_SKIPPING; startingIndex++){
        i = startingIndex/(FRACTION_TILES_SKIPPING);

        for(j = 0; j < COLS; j++){            // tile_per_row is the total number of tiles in each row
            short int TILE_COLOR = (gridForDrawing[startingIndex][j] == BLACK) ? PIANO_TILE_COLOR : BLANK_TILE_COLOR;
            // Next key to be pressed in green colour
            TILE_COLOR = ((startingIndex-offset >= (ROWS-1)*FRACTION_TILES_SKIPPING) && j == correctColumn) ? CURRENT_TILE_COLOR : TILE_COLOR;
            
            drawTile(j*TILE_WIDTH + offset_x, (i*TILE_HEIGHT)+(startingIndex%FRACTION_TILES_SKIPPING)*(TILE_HEIGHT/FRACTION_TILES_SKIPPING), TILE_COLOR);
            // Vertical grid line
            drawLine(offset_x + j*TILE_WIDTH, 0, offset_x + j*TILE_WIDTH, RESOLUTION_Y - 1, 0x0000);
        }

        int y_placement = (i*TILE_HEIGHT)+(offset)*(TILE_HEIGHT/FRACTION_TILES_SKIPPING);

        drawLine(offset_x, y_placement, COLS*TILE_WIDTH + offset_x, y_placement, 0x0000);
    }

    drawLine(offset_x - 1, 0, offset_x - 1, RESOLUTION_Y - 1, 0x0000);
    updateScreen();
}

// Function to determine game state according to key pressed
// and update global variables accordingly
int keyPressed(){
    // Key edge capture register address
    volatile int* edgeCapture = (int *) 0xFF20005C;
    // Key pressed
    int key = -1;
    
    if((*edgeCapture) == 8) {
        key = 1;    // KEY3
    } else if ((*edgeCapture) == 4) {
        key = 2;    // KEY2
    } else if((*edgeCapture) == 2) {
        key = 3;    // KEY1
    } else if((*edgeCapture) == 1) {
        key = 0;    // KEY0
    } else if ((*edgeCapture) == 0) {
        key = 4; 
    }
    
    // Reset edge capture register
    *edgeCapture = *edgeCapture;
    
    if(key == 0) {
        // If reset key pressed
        return RESET_GAME;
    } else if(key == correctColumn + 1) {
        // If correct key pressed
        return GAME_INCREMENT;
    } else {
        // If wrong key or no key pressed
        return GAME_OVER;
    }
}

// Reset game
void resetGame() {
    int i, j;

    for(i = 0; i < ROWS; i++){
        for(j = 0; j < COLS; j++){
            gameGrid[i][j] = WHITE;
        }
    }
}

// Function to display score on HEX 3-0
void HEXScoreUpdate() {
    // HEX 3-0 location pointer
    int* HEX3_0Ptr = (int*) 0xFF200020;
    // HEX Register Value to be stored at HEX 3-0 location
    int HEXValue = 0;
    // Temporary score variable
    int temp = score;
    // Storing HEX individual digits
    int HEXDigits[4];
    // HEX Codes for decimal digits
    int HEXCodes[10] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
                        0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111};

    // Extracting HEX digits into HEX array
    int i;
    for(i = 0; i < 4; i++) {
        HEXDigits[i] = temp % 10;
        temp = temp / 10;
    }

    // Determining HEX register value using bit codes
    for(i = 3; i >= 0; i--) {
        HEXValue *= 256; // Logical shift left by 8 bits
        HEXValue += HEXCodes[HEXDigits[i]];
    }

    // Set HEX 3-0 value
    (*HEX3_0Ptr) = HEXValue;
 }

// Function to display timer on HEX 3-0
// void HEXTimerUpdate() {
//     // HEX 3-0 location pointer
//     int* HEX3_0Ptr = (int*) 0xFF200020;
//     // HEX Register Value to be stored at HEX 3-0 location
//     int HEXValue = 0;
//     // HEX Codes for decimal digits
//     int HEXCodes[10] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
//                         0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111};

//     timer[0] += 1;

//     if(timer[0] == 10) {
//         timer[0] = 0;
//         timer[1] += 1;
//     }

//     if(timer[1] == 10) {
//         timer[1] = 0;
//         timer[2] += 1;
//     }

//     if(timer[2] == 10) {
//         timer[2] = 0;
//         timer[3] += 1;
//     }

//     if(timer[3] == 6) {
//         timer[3] = 0;
//         timer[2] = 0;
//         timer[1] = 0;
//         timer[0] = 0;
//     }

//     // Determining HEX register value using bit codes
//     int i;
//     for(i = 3; i >= 0; i--) {
//         HEXValue *= 256; // Logical shift left by 8 bits
//         HEXValue += HEXCodes[timer[i]];
//     }

//     // Set HEX 3-0 value
//     (*HEX3_0Ptr) = HEXValue;
//  }

 // Function to swap values
 void swap(int* x, int* y) {
    int temp = *x;
    *x = *y;
    *y = temp;
 }

void displayChar(int x, int y, char c) {
    // VGA character buffer
    volatile char * character_buffer = (char *) (0x09000000 + (y<<7) + x);
    *character_buffer = c;
}

void initializeTimer(int loadValue){
    // Setting hardware timer for 0.01s
    // accessing load register value of private timer on ARM
    volatile int* timer_ptr = (int*) 0xFFFEC600;
    *timer_ptr = loadValue;
}

void startTimer(){
    // Setting control register
    volatile int* timer_settings_ptr = (int*) 0xFFFEC608;
    // setting auto-load and enable
    *timer_settings_ptr = 3;
}

bool timeUp(){
    volatile int* timer_settings_ptr = (int*) 0xFFFEC60C;
    return (1 == (*timer_settings_ptr & 1));
}

void resetTimer(){
    volatile int* timer_settings_ptr = (int*) 0xFFFEC60C;
    *timer_settings_ptr = 1;
}

void updateDrawingGrid(){
    int i,j;
    //always ignore the top-most row while setting it
    // (i*TILE_HEIGHT)+(startingIndex%(FRACTION_TILES_SKIPPING))*(TILE_HEIGHT/FRACTION_TILES_SKIPPING)
    for(i = 0; i < (ROWS)*FRACTION_TILES_SKIPPING; i++){
        for(j = 0; j < COLS; j++){
            // shift everything one row below
            gridForDrawing[i][j] = gameGrid[(i/(FRACTION_TILES_SKIPPING))][j];
        }
    }
}

void animateGridForDrawing(){
    // displace everything by FRACTION_TILES_SKIPPING
    int i,j;

    for(i = ((ROWS)*FRACTION_TILES_SKIPPING)-2; i > 0; i--){
        for(j = 0; j < COLS; j++){
            // shift everything one row below
            gridForDrawing[i+1][j] = gridForDrawing[i][j];
        }
    }

    // updateScreen();
}
