#define RESOLUTION_X 320
#define RESOLUTION_Y 240

#define COLS 3
#define ROWS 5
    
#define TILE_WIDTH  50      // think of this  -> technically make it 320/3. ~~ 106pixels
#define TILE_HEIGHT (RESOLUTION_Y / ROWS)      // 4 rows per display

// define black as 1, white as 0
#define BLACK 1
#define WHITE 0

// generates 0, 1, 2, ... COLS-1 -> left ->center -> right col

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

const short int PIANO_TILE_COLOR = 0x0821;                    // black
const short int BLANK_TILE_COLOR = 0xFFFF;                    // white
volatile int pixel_buffer_start;                               // global variable

//bool gameOver;                                                  // global variable corresponding to the state of the game
int score;                                                      // score that will be displayed on VGA
int visibleGrid[ROWS][COLS];                 //
int correctColumn;  // Store the next column key to press in the game
int score;  // Stroe user's score
int timer[4];

// Display related functions:
void wait_for_vsync();
void plot_pixel(int x, int y, short int line_color);
void draw_line(int x0, int y0, int x1, int y1, short int color);

void initializeScreen();
void updateScreen();
void clear_screen();

void displayChar(int x, int y, char c);

// setting grid to display,
void generateRow(int rowNumber);
void generateGrid();
void drawGrid();
void updateGrid();

// access data from KEYs
int keyPressed();

// Reset game
void resetGame();
void HEXScoreUpdate();
void HEXTimerUpdate();

int main(void){
    // game starts with start page which says "press any key to start"
    // then the timer starts when the first (correct) key is pressed
    // at ANY point in the game if an incorrect tile is pressed -> game over
    // KEY0 is reset
    // KEY1 is right
    // KEY2 is center
    // KEY3 is left
    
    // score on HEX
    // timer on VGA
    // game is user-pace defined
    
    // for now assume only three tiles per row
    // must ENSURE that when grid is updated, it looks smooth
    volatile int* keys_ptr = (int*) 0xFF20005C;

x:  score = 0;
    // Must clear front display buffer and its back buffer
    // and also set globalPtr to pixel_buffer_start
    // do that in initialize screen
    initializeScreen();
    updateScreen();
    // initialize grid
    generateGrid();
 
    while(1) {
        updateGrid();
        drawGrid();
        updateScreen();
        HEXScoreUpdate();
        HEXTimerUpdate();
        displayChar(0, 0, 'V');
        int i;
        for(i = 0; i < 4; i++) {
            timer[i] = 0;
        }

        // Setting hardware timer for 0.01s
        volatile int* timer_ptr = (int*) 0xFFFEC600;
        *timer_ptr = 2000000;
        volatile int* timer_settings_ptr = (int*) 0xFFFEC608;
        *timer_settings_ptr = 3;

        while((*keys_ptr) == 0x0) {
            volatile int* timer_value_ptr = (int*) 0xFFFEC60C;

            if(*timer_value_ptr != 0) {
                *timer_value_ptr = 1;
                HEXTimerUpdate();
                HEXScoreUpdate();
            }           
        }
        
        int response = keyPressed();

        if(response == 0) {
            goto x;
        } else if(response == -1) {
            break;
        }
    }
    //wait_for_vsync();
}

void initializeScreen(){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    // declare other variables(not shown)
    // initialize location and direction of rectangles(not shown)
    
    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the
    // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    
    pixel_buffer_start = *(pixel_ctrl_ptr+1); // we draw on the back buffer
    clear_screen(); // pixel_buffer_start points to the pixel buffer
}

void updateScreen(){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    wait_for_vsync(); // swap front and back buffers on VGA vertical sync
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
}

void wait_for_vsync(){
    // place 1 in front buffer (to start swap)
    volatile int* pixel_ctrl_ptr = (int*)0xFF203020; // status register
    int registerValue;
    *(pixel_ctrl_ptr) = 1;  // swaps the two registers -> what happens to the address stored in the register
    registerValue = *(pixel_ctrl_ptr+3);
    while((registerValue & 0x01) != 0){
        registerValue = *(pixel_ctrl_ptr+3);                // AND it with 1 to obtain the last bit
    }
}

void plot_pixel(int x, int y, short int line_color){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void clear_screen(){
    int x, y;
    for(x = 0; x < RESOLUTION_X; x++)
        for(y = 0; y < RESOLUTION_Y; y++)
            plot_pixel(x,y,0xFeA4); // 0x0 is black
}

void drawTile(int x, int y, short int line_color){
    // x and y define the TOP-LEFT corner of the tile
    int i, j;
    for(i = 0; i < TILE_WIDTH; i++)
        for (j = 0; j < TILE_HEIGHT; j++)
            plot_pixel(x+i, y+j, line_color);
}

void generateRow(int rowNumber){
    // only one black tile per row
    int indexForBlackTile = rand()%(COLS); // must do srand
    // generates 0, 1, 2, ... COLS-1 -> left ->center -> right col
    visibleGrid[rowNumber][indexForBlackTile] = BLACK;
    // need to place a white tile in the other two tiles
    int i;
    for(i = 0; i < COLS; i++)
        if(i != indexForBlackTile)
            visibleGrid[rowNumber][i] = WHITE;
}

void generateGrid(){
    int j;
    for(j = 0; j < ROWS; j++){
        generateRow(j);
    }

    // Get correct column key to press
    for(j = 0; j < COLS; j++) {
        if(visibleGrid[ROWS-1][j] == BLACK) {
            correctColumn = j;
            break;
        }
    }
}

//assume the func is called when the user presses the RIGHT key
void updateGrid(){
    // first must shift change
    int i,j;
    for(i = ROWS-2; i >= 0; i--){
        for(j = 0; j < COLS; j++){
            // shift everything one row below
            visibleGrid[i+1][j] = visibleGrid[i][j];
        }
    }
    // update the top-most row
    generateRow(0);
    // remove the last row
    // shift everything downwards
    // generate row for first row and place

    // Get correct column key to press
    for(j = 0; j < COLS; j++) {
        if(visibleGrid[ROWS-1][j] == BLACK) {
            correctColumn = j;
            break;
        }
    }
}

void drawGrid(){
    // Center offset
    int offset_x = (RESOLUTION_X - (TILE_WIDTH * COLS)) / 2;
    // Vertical lines on both sides of the playing grid
    draw_line(offset_x - 2, 0, offset_x - 2, RESOLUTION_Y - 1, 0x0000);
    draw_line(COLS*TILE_WIDTH + offset_x + 2, 0, COLS*TILE_WIDTH + offset_x + 2, RESOLUTION_Y - 1, 0x0000);
    draw_line(offset_x + COLS*TILE_WIDTH + 1, 0, offset_x + COLS*TILE_WIDTH + 1, RESOLUTION_Y - 1, 0x0000);
    draw_line(offset_x + COLS*TILE_WIDTH, 0, offset_x + COLS*TILE_WIDTH, RESOLUTION_Y - 1, 0x0000);
    int i, j;
    for (i = 0; i < ROWS; i++){                // ROWS is the total number of rows
        for(j = 0; j < COLS; j++){            // tile_per_row is the total number of tiles in each row
            short int TILE_COLOR = (visibleGrid[i][j] == BLACK) ? PIANO_TILE_COLOR : BLANK_TILE_COLOR;
            // Next key to be pressed in green colour
            TILE_COLOR = ((i == ROWS - 1) && j == correctColumn) ? 0x23E1 : TILE_COLOR;

            drawTile(j*TILE_WIDTH + offset_x, i*TILE_HEIGHT, TILE_COLOR);
            // Vertical grid line
            draw_line(offset_x + j*TILE_WIDTH, 0, offset_x + j*TILE_WIDTH, RESOLUTION_Y - 1, 0x0000);
        }

        // Horizontal grid line
        draw_line(offset_x, i*TILE_HEIGHT, COLS*TILE_WIDTH + offset_x, i*TILE_HEIGHT, 0x0000);
    }

    // Last vertical grid line
    draw_line(offset_x - 1, 0, offset_x - 1, RESOLUTION_Y - 1, 0x0000);
}
// bool checkValidMove(int key_value){
//     if(visibleGrid[0][key_value] == BLACK)            // hit the right key
//         return true;
//     else return false;
// }

// need a function that places a high value in the timer initially
// this value is used to move the screen upwards
// this value gets smaller over time
// therefore, the screen seems to move more quickly
// this value can be access constantly to determine how much the screen should be shifted upwards
// to give smooth animation feel, instead of a discrete movement -> moving one tile at a time


// for sound need to play a chord/note everytime a black key is pressed
// idea for that: load separate mp3 files for each chord,
// store all ptrs to these files in an array in order that's supposed to be played
// have a global variable that is used to access the next note to play
// when a key is pressed that note is played
// and global variable is incremented
// when reached last note, reset varialble to 0

// Function to determine game state according to key pressed
// and updat eglobal variables accordingly
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
    }
    
    // Reset edge capture register
    *edgeCapture = *edgeCapture;
    
    if(key == 0) {
        // If reset key pressed
        resetGame();
        return 0;
    } else if(key == correctColumn + 1) {
        // If correct key pressed
        score += 1;
        return 1;
    } else {
        // If wrong key or illegal key combination pressed
        //gameOver();
        return -1;
    }
}

void resetGame() {
    int i, j;
    for(i = 0; i < ROWS; i++){
        for(j = 0; j < COLS; j++){
            visibleGrid[i][j] = WHITE;
        }
    }
    drawGrid();
    updateScreen();
}

// Function to display score on HEX 3-0
void HEXScoreUpdate() {
    // HEX 3-0 location pointer
    int* HEX5_4Ptr = (int*) 0xFF200030;
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
    for(i = 1; i >= 0; i--) {
        HEXValue *= 256; // Logical shift left by 8 bits
        HEXValue += HEXCodes[HEXDigits[i]];
    }

    // Set HEX 3-0 value
    (*HEX5_4Ptr) = HEXValue;
 }

 // Function to display timer on HEX 3-0
void HEXTimerUpdate() {
    // HEX 3-0 location pointer
    int* HEX3_0Ptr = (int*) 0xFF200020;
    // HEX Register Value to be stored at HEX 3-0 location
    int HEXValue = 0;
    // HEX Codes for decimal digits
    int HEXCodes[10] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
                        0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111};

    timer[0] += 1;

    if(timer[0] == 10) {
        timer[0] = 0;
        timer[1] += 1;
    }

    if(timer[1] == 10) {
        timer[1] = 0;
        timer[2] += 1;
    }

    if(timer[2] == 10) {
        timer[2] = 0;
        timer[3] += 1;
    }

    if(timer[3] == 6) {
        timer[3] = 0;
        timer[2] = 0;
        timer[1] = 0;
        timer[0] = 0;
    }

    // Determining HEX register value using bit codes
    int i;
    for(i = 3; i >= 0; i--) {
        HEXValue *= 256; // Logical shift left by 8 bits
        HEXValue += HEXCodes[timer[i]];
    }

    // Set HEX 3-0 value
    (*HEX3_0Ptr) = HEXValue;
 }

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

// Function to draw line
void draw_line(int x0, int y0, int x1, int y1, short int color) {
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
            plot_pixel(y, x, color);
        } else {
            plot_pixel(x, y, color);
        }

        error += deltay;

        if(error >= 0) {
            y = y + y_step;
            error -= deltax;
        }
    }
}
