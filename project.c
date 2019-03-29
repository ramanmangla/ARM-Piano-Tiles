#define RESOLUTION_X 320
#define RESOLUTION_Y 240

#define TILE_WIDTH 	xxx	  // think of this  -> technically make it 320/3. ~~ 106pixels
#define TILE_HEIGHT 60	  // 4 rows per display

#define TILES_PER_ROW 3
#define ROWS_PER_COL 4

// define black as 1, white as 0
#define BLACK 1
#define WHITE 0

// generates 0, 1, 2, ... TILES_PER_ROW-1 -> left ->center -> right col

const short int BLANK_TILE_COLOR = 0x0000;                    // black
const short int PIANO_TILE_COLOR = 0xFFFF;  				  // white
bool gameOver;												  // global variable corresponding to the state of the game 	
int visibleGrid[ROWS_PER_COL][TILES_PER_ROW];

// Display related functions:
void wait_for_vsync();
void plot_pixel(int x, int y, short int line_color);
void clear_screen();
void drawRectangle(int x, int y, short int line_color);

// setting grid,
void generateRow(int rowNumber);

int main(void){

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
    // x and y define the TOP-LEFT corner of the square
    for(int i = 0; i < TILE_WIDTH; i++)
        for (int j = 0; j < TILE_HEIGHT; j++)
            plot_pixel(x+i, y+j, line_color);
}

void generateRow(int rowNumber){
	// only one black tile per row 
	int indexForBlackTile = rand()%(TILES_PER_ROW); 	
	// generates 0, 1, 2, ... TILES_PER_ROW-1 -> left ->center -> right col
	visibleGrid[rowNumber][indexForBlackTile] = BLACK;
	// need to place a white tile in the other two tiles
	for(int i = 0; i < TILES_PER_ROW; i++)
		if(i != indexForBlackTile)
			visibleGrid[rowNumber][i] = WHITE;
}

bool checkValidMove(int key_value){
	if(visibleGrid[0][key_value] == BLACK)			// hit the right key
		return true;
	else return false;
}

// need a function that places a high value in the timer initially
// this value is used to move the screen upwards
// this value gets smaller over time
// therefore, the screen seems to move more quickly
// this value can be access constantly to determine how much the screen should be shifted upwards
// to give smooth animation feel, instead of a discrete movement -> moving one tile at a time