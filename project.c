#define RESOLUTION_X 320
#define RESOLUTION_Y 240

#define TILE_WIDTH  106      // think of this  -> technically make it 320/3. ~~ 106pixels
#define TILE_HEIGHT 60      // 4 rows per display

#define COLS 3
#define ROWS 4

// define black as 1, white as 0
#define BLACK 1
#define WHITE 0

// generates 0, 1, 2, ... COLS-1 -> left ->center -> right col

#include <stdlib.h>

const short int PIANO_TILE_COLOR = 0x0000;                    // black
const short int BLANK_TILE_COLOR = 0xFFFF;                    // white
volatile int pixel_buffer_start;                               // global variable

//bool gameOver;                                                  // global variable corresponding to the state of the game
int score;                                                      // score that will be displayed on VGA
int visibleGrid[ROWS][COLS];                 //
int correctColumn;  // Store the next column key to press in the game
int score;  // Stroe user's score

// Display related functions:
void wait_for_vsync();
void plot_pixel(int x, int y, short int line_color);

void drawRectangle(int x, int y, short int line_color);

void initializeScreen();
void updateScreen();
void clear_screen();

// setting grid to display,
void generateRow(int rowNumber);
void generateGrid();
void drawGrid();
void updateGrid();

// access data from KEYs
int keyPressed();

// Reset game
void resetGame();

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
    int* HEX3_0Ptr = (int*) 0xFF200020;

    int HEXDigits[10] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
                        0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111};

x:  score = 0;
    initializeScreen();
    updateScreen();
    // initialize grid
    generateGrid();
    drawGrid();
 
    while(1){
        updateGrid();
        drawGrid();
        updateScreen();
        *HEX3_0Ptr = HEXDigits[score % 10];

        while((*keys_ptr) == 0x0);
        
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
            plot_pixel(x,y,0x0000); // 0x0 is black
}

void drawTile(int x, int y, short int line_color){
    // x and y define the TOP-LEFT corner of the tile
    for(int i = 0; i < TILE_WIDTH; i++)
        for (int j = 0; j < TILE_HEIGHT; j++)
            plot_pixel(x+i, y+j, line_color);
}

void generateRow(int rowNumber){
    // only one black tile per row
    int indexForBlackTile = rand()%(COLS); // must do srand
    // generates 0, 1, 2, ... COLS-1 -> left ->center -> right col
    visibleGrid[rowNumber][indexForBlackTile] = BLACK;
    // need to place a white tile in the other two tiles
    for(int i = 0; i < COLS; i++)
        if(i != indexForBlackTile)
            visibleGrid[rowNumber][i] = WHITE;
}

void generateGrid(){
    for(int j = 0; j < ROWS; j++){
        generateRow(j);
    }

    // Get correct column key to press
    for(int j = 0; j < COLS; j++) {
        if(visibleGrid[ROWS-1][j] == BLACK) {
            correctColumn = j;
            break;
        }
    }
}

//assume the func is called when the user presses the RIGHT key
void updateGrid(){
    // first must shift change
    for(int i = ROWS-2; i >= 0; i--){
        for(int j = 0; j < COLS; j++){
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
    for(int j = 0; j < COLS; j++) {
        if(visibleGrid[ROWS-1][j] == BLACK) {
            correctColumn = j;
            break;
        }
    }
}

void drawGrid(){
    for (int i = 0; i < ROWS; i++){                // ROWS is the total number of rows
        for(int j = 0; j < COLS; j++){            // tile_per_row is the total number of tiles in each row
            short int TILE_COLOR = (visibleGrid[i][j] == BLACK) ? PIANO_TILE_COLOR : BLANK_TILE_COLOR;
            drawTile(j*TILE_WIDTH, i*TILE_HEIGHT, TILE_COLOR);
        }
    }
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

// depending on which key is pressed, 0,1,2,3 returns the corresponding integer value
int keyPressed(){
    volatile int* edgeCapture = (int *) 0xFF20005C;
    int key = -1;
    
    if((*edgeCapture) == 8) {
        key = 1;
    } else if ((*edgeCapture) == 4) {
        key = 2;
    } else if((*edgeCapture) == 2) {
        key = 3;
    } else if((*edgeCapture) == 1) {
        key = 0;
    }
    
    *edgeCapture = *edgeCapture;
    
    if(key == 0) {
        resetGame();
        return 0;
    } else if(key == correctColumn + 1) {
        score += 1;
        return 1;
    } else {
        //gameOver();
        return -1;
    }
}

void resetGame() {
    for(int i = 0; i < ROWS; i++){
        for(int j = 0; j < COLS; j++){
            visibleGrid[i][j] = WHITE;
        }
    }
    drawGrid();
    updateScreen();
}

