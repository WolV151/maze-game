// MARIN DONCHEV - SDH2A
// R00192936

// #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#define WALL 'w'
#define POTION '#'
#define NEEDED_POTIONS 3
#define PLAYER '@';

struct maze{
    char **a; // 2D array supporting maze
    unsigned int w; // width
    unsigned int h; // height
    unsigned int cell_size; // number of chars per cell; walls are 1 char
};

/**
 * Represents a cell in the 2D matrix.
 */
struct cell{
    unsigned int x;
    unsigned int y;
};

/**
 * Stack structure using a list of cells.
 * At element 0 in the list we have NULL.
 * Elements start from 1 onwards.
 * top_of_stack represents the index of the top of the stack
 * in the cell_list.
 */
struct stack{
    struct cell *cell_list;
    unsigned int top_of_stack;
    unsigned int capacity;
};

/**
 * Initialises the stack by allocating memory for the internal list
 */
void init_stack(struct stack *stack, unsigned int capacity){
    stack->cell_list = (struct cell*)malloc(sizeof(struct cell)*(capacity+1));
    stack->top_of_stack = 0;
    stack->capacity = capacity;
}

void free_stack(struct stack *stack){
    free(stack->cell_list);
}

/**
 * Returns the element at the top of the stack and removes it
 * from the stack.
 * If the stack is empty, returns NULL
 */
struct cell stack_pop(struct stack *stack){
    struct cell cell = stack->cell_list[stack->top_of_stack];
    if (stack->top_of_stack > 0) stack->top_of_stack --;
    return cell;
}

/**
 * Pushes an element to the top of the stack.
 * If the stack is already full (reached capacity), returns -1.
 * Otherwise returns 0.
 */
int stack_push(struct stack *stack, struct cell cell){
    if (stack->top_of_stack == stack->capacity) return -1;
    stack->top_of_stack ++;
    stack->cell_list[stack->top_of_stack] = cell;
    return 0;
}

int stack_isempty(struct stack *stack){
    return stack->top_of_stack == 0;
}

//-----------------------------------------------------------------------------

void mark_visited(struct maze *maze, struct cell cell){
    maze->a[cell.y][cell.x] = 'v';
}

/**
 * Convert a cell coordinate to a matrix index.
 * The matrix also contains the wall elements and a cell might span
 * multiple matrix cells.
 */
int cell_to_matrix_idx(struct maze *m, int cell){
    return (m->cell_size+1)*cell+(m->cell_size/2)+1;
}

/**
 * Convert maze dimension to matrix dimension.
 */
int maze_dimension_to_matrix(struct maze *m, int dimension){
    return (m->cell_size+1)*dimension+1;
}

/**
 * Returns the index of the previous cell (cell - 1)
 */
int matrix_idx_prev_cell(struct maze *m, int cell_num){
    return cell_num - (m->cell_size+1);
}

/**
 * Returns the index of the next cell (cell + 1)
 */
int matrix_idx_next_cell(struct maze *m, int cell_num){
    return cell_num + (m->cell_size+1);
}

/**
 * Returns into neighbours the unvisited neighbour cells of the given cell.
 * Returns the number of neighbours.
 * neighbours must be able to hold 4 cells.
 */
int get_available_neighbours(struct maze *maze, struct cell cell, struct cell *neighbours){
    int num_neighbrs = 0;

    // Check above
    if ((cell.y > cell_to_matrix_idx(maze,0)) && (maze->a[matrix_idx_prev_cell(maze, cell.y)][cell.x] != 'v')){
        neighbours[num_neighbrs].x = cell.x;
        neighbours[num_neighbrs].y = matrix_idx_prev_cell(maze, cell.y);
        num_neighbrs ++;
    }

    // Check left
    if ((cell.x > cell_to_matrix_idx(maze,0)) && (maze->a[cell.y][matrix_idx_prev_cell(maze, cell.x)] != 'v')){
        neighbours[num_neighbrs].x = matrix_idx_prev_cell(maze, cell.x);
        neighbours[num_neighbrs].y = cell.y;
        num_neighbrs ++;
    }

    // Check right
    if ((cell.x < cell_to_matrix_idx(maze,maze->w-1)) && (maze->a[cell.y][matrix_idx_next_cell(maze, cell.x)] != 'v')){
        neighbours[num_neighbrs].x = matrix_idx_next_cell(maze, cell.x);
        neighbours[num_neighbrs].y = cell.y;
        num_neighbrs ++;
    }

    // Check below
    if ((cell.y < cell_to_matrix_idx(maze,maze->h-1)) && (maze->a[matrix_idx_next_cell(maze, cell.y)][cell.x] != 'v')){
        neighbours[num_neighbrs].x = cell.x;
        neighbours[num_neighbrs].y = matrix_idx_next_cell(maze, cell.y);
        num_neighbrs ++;
    }

    return num_neighbrs;
}


/**
 * Removes a wall between two cells.
 */
void remove_wall(struct maze *maze, struct cell a, struct cell b){
    int i;
    if (a.y == b.y){
        for (i=0;i<maze->cell_size;i++)
            maze->a[a.y-maze->cell_size/2+i][a.x-(((int)a.x-(int)b.x))/2] = ' ';
    }else{
        for (i=0;i<maze->cell_size;i++)
            maze->a[a.y-(((int)a.y-(int)b.y))/2][a.x-maze->cell_size/2+i] = ' ';
    }
}

/**
 * Fill all matrix elements corresponding to the cell
 */
void fill_cell(struct maze *maze, struct cell c, char value){
    int i,j;
    for (i=0;i<maze->cell_size;i++)
        for (j=0;j<maze->cell_size;j++)
            maze->a[c.y-maze->cell_size/2+i][c.x-maze->cell_size/2+j] = value;
}

/**
 * This function generates a maze of width x height cells.
 * Each cell is a square of cell_size x cell_size characters.
 * The maze is randomly generated based on the supplied rand_seed.
 * Use the same rand_seed value to obtain the same maze.
 *
 * The function returns a struct maze variable containing:
 * - the maze represented as a 2D array (field a)
 * - the width (number of columns) of the array (field w)
 * - the height (number of rows) of the array (field h).
 * In the array, walls are represented with a 'w' character, while
 * pathways are represented with spaces (' ').
 * The edges of the array consist of walls, with the exception
 * of two openings, one on the left side (column 0) and one on
 * the right (column w-1) of the array. These should be used
 * as entry and exit.
 */
struct maze generate_maze(unsigned int width, unsigned int height, unsigned int cell_size, int rand_seed){
    int row, col, i;
    struct stack stack;
    struct cell cell;
    struct cell neighbours[4];  // to hold neighbours of a cell
    int num_neighbs;
    struct maze maze;
    maze.w = width;
    maze.h = height;
    maze.cell_size = cell_size;
    maze.a = (char**)malloc(sizeof(char*)*maze_dimension_to_matrix(&maze, height));

    // Initialise RNG
    srandom(rand_seed);

    // Initialise stack
    init_stack(&stack, width*height);

    // Initialise the matrix with walls
    for (row=0;row<maze_dimension_to_matrix(&maze, height);row++){
        maze.a[row] = (char*)malloc(maze_dimension_to_matrix(&maze, width));
        memset(maze.a[row], WALL, maze_dimension_to_matrix(&maze, width));
    }

    // Select a random position on a border.
    // Border means x=0 or y=0 or x=2*width+1 or y=2*height+1
    cell.x = cell_to_matrix_idx(&maze,0);
    cell.y = cell_to_matrix_idx(&maze,random()%height);
    mark_visited(&maze, cell);
    stack_push(&stack, cell);

    while (! stack_isempty(&stack)){
        // Take the top of stack
        cell = stack_pop(&stack);
        // Get the list of non-visited neighbours
        num_neighbs = get_available_neighbours(&maze, cell, neighbours);
        if (num_neighbs > 0){
            struct cell next;
            // Push current cell on the stack
            stack_push(&stack, cell);
            // Select one random neighbour
            next = neighbours[random()%num_neighbs];
            // Mark it visited
            mark_visited(&maze, next);
            // Break down the wall between the cells
            remove_wall(&maze, cell, next);
            // Push new cell on the stack
            stack_push(&stack, next);
        }
    }

    // Finally, replace 'v' with spaces
    for (row=0;row<maze_dimension_to_matrix(&maze, height);row++)
        for (col=0;col<maze_dimension_to_matrix(&maze, width);col++)
            if (maze.a[row][col] == 'v'){
                cell.y = row;
                cell.x = col;
                fill_cell(&maze, cell, ' ');
            }

    // Select an entry point in the top right corner.
    // The first border cell that is available.
    for (row=0;row<maze_dimension_to_matrix(&maze, height);row++)
        if (maze.a[row][1] == ' ') { maze.a[row][0] = ' '; break; }

    // Select the exit point
    for (row=maze_dimension_to_matrix(&maze, height)-1;row>=0;row--)
        if (maze.a[row][cell_to_matrix_idx(&maze,width-1)] == ' ') {
            maze.a[row][maze_dimension_to_matrix(&maze, width)-1] = ' ';
            break;
        }

    maze.w = maze_dimension_to_matrix(&maze, maze.w);
    maze.h = maze_dimension_to_matrix(&maze, maze.h);

    // Add the potions inside the maze at three random locations
    for (i=0;i<NEEDED_POTIONS;i++){
        do{
            row = random()%(maze.h-1);
            col = random()%(maze.w-1);
        }while (maze.a[row][col] != ' ');
        maze.a[row][col] = POTION;
    }

    return maze;
}


void print_maze(int row, int col, struct maze my_maze, int potions){ // print the maze, ncurses only works in terminal
    initscr();                                          // i tried using terminal emulation via the IDE but it doesn't work
    cbreak();
    noecho();
    wclear(stdscr);
    for (row = 0; row < my_maze.h; row++){
        for(col = 0; col < my_maze.w; col++){
            waddch(stdscr, my_maze.a[row][col]);
        }
    	  wmove(stdscr, row+1, col*0);
//        printw("\n");
    }
    printw("Potions collected: ");
    printw("%d", potions);
    scrollok(stdscr, TRUE);
    wrefresh(stdscr);
}

// print_maze but with the fog mechanic
void print_maze_fog(int row , int col, int player_coord_x, int player_coord_y, struct maze mymaze, int potions, int fog_radius) {
    initscr();
    cbreak();
    noecho();
    wclear(stdscr);

    if ((fog_radius == 0) || ((fog_radius >= mymaze.h - 1) ||
                              (fog_radius >= mymaze.w - 1))) { // if there is no fog or the radius exceeds the maze
        print_maze(row, col, mymaze, potions);                    // don't print it
    }
    else if (player_coord_y >= fog_radius && player_coord_y < mymaze.h - fog_radius){
            if (player_coord_x >= fog_radius && player_coord_x < mymaze.w - fog_radius) { // if the player is in the middle
                for (int i = 0 - fog_radius; i <= fog_radius; i++) { //y loop
                    for(int j = 0 - fog_radius; j <= fog_radius; j++){ // x loop
                        wmove(stdscr, player_coord_y + i, player_coord_x + j);
                        waddch(stdscr, mymaze.a[player_coord_y + i][player_coord_x + j]);
                    }
                        printw("\n");

                }
            }
            else if (player_coord_x <= fog_radius) { // if the player is on the left
                for (int i = 0 - player_coord_x; i <= fog_radius; i++) {
                    for(int j = 0 - player_coord_x; j <= fog_radius; j++){
                        wmove(stdscr, player_coord_y + i, player_coord_x + j);
                        waddch(stdscr, mymaze.a[player_coord_y + i][player_coord_x + j]);
                    }
                    printw("\n");

                }
            }
            else if(player_coord_x >= mymaze.w - fog_radius){ // if the player is on the right
                refresh();
                int my_var = (int)mymaze.w - player_coord_x; // the comparison didn't work without casting it
                for (int i = 0 - fog_radius; i <= my_var - 1; i++) {
                    for(int j = 0 - fog_radius; j <= my_var - 1; j++){
                        wmove(stdscr, player_coord_y + i, player_coord_x + j);
                        waddch(stdscr, mymaze.a[player_coord_y + i][player_coord_x + j]);
                    }
                    printw("\n");

                }

            }


    }
    else if (player_coord_y <= fog_radius){ // the player is near the upper wall
        if (player_coord_x >= fog_radius && player_coord_x < mymaze.w - fog_radius) { // if the player is in the middle
            for (int i = 0 - player_coord_y; i <= fog_radius; i++) {
                for(int j = 0 - fog_radius; j <= fog_radius; j++){
                    wmove(stdscr, player_coord_y + i, player_coord_x + j);
                    waddch(stdscr, mymaze.a[player_coord_y + i][player_coord_x + j]);
                }
                printw("\n");

            }
        }
        else if (player_coord_x <= fog_radius) { // if the player is on the upper left
            for (int i = 0 - player_coord_y; i <= fog_radius; i++) {
                for(int j = 0 - player_coord_x; j <= fog_radius; j++){
                    wmove(stdscr, player_coord_y + i, player_coord_x + j);
                    waddch(stdscr, mymaze.a[player_coord_y + i][player_coord_x + j]);
                }
                printw("\n");


            }
        }
        else if(player_coord_x >= mymaze.w - fog_radius){ // if the player is on the upper right
            refresh();
            int my_var = (int)mymaze.w - player_coord_x - 1;

            for (int i = 0 - player_coord_y; i <= fog_radius; i++) {
                for(int j = 0 - fog_radius; j <= my_var; j++){
                    wmove(stdscr, player_coord_y + i, player_coord_x + j);
                    waddch(stdscr, mymaze.a[player_coord_y + i][player_coord_x + j]);
                }
                printw("\n");


            }

        }
    }
    else if (player_coord_y >= mymaze.h - fog_radius){ //player is on the lower wall
        if (player_coord_x >= fog_radius && player_coord_x < mymaze.w - fog_radius) { // if the player is in the middle
            for (int i = 0 - fog_radius; i <= (int)mymaze.h - player_coord_y - 1; i++) {
                for(int j = 0 - fog_radius; j <= fog_radius; j++){
                    wmove(stdscr, player_coord_y + i, player_coord_x + j);
                    waddch(stdscr, mymaze.a[player_coord_y + i][player_coord_x + j]);
                }
                printw("\n");


            }
        }
        else if (player_coord_x <= fog_radius) { // if the player is on the lower left
            for (int i = 0 - fog_radius; i <= (int)mymaze.h - player_coord_y - 1; i++) {
                for(int j = 0 - player_coord_x; j <= fog_radius; j++){
                    wmove(stdscr, player_coord_y + i, player_coord_x + j);
                    waddch(stdscr, mymaze.a[player_coord_y + i][player_coord_x + j]);
                }
                printw("\n");


            }
        }
        else if(player_coord_x >= mymaze.w - fog_radius){ // if the player is on the lower right
            refresh();
            int my_var = (int)mymaze.w - player_coord_x - 1;

            for (int i = 0 - fog_radius; i <= (int)mymaze.h - player_coord_y - 1; i++) {
                for(int j = 0 - fog_radius; j <= my_var; j++){
                    wmove(stdscr, player_coord_y + i, player_coord_x + j);
                    waddch(stdscr, mymaze.a[player_coord_y + i][player_coord_x + j]);
                }
                printw("\n");

            }

        }

    }

    printw("Potions collected: ");
    printw("%d", potions);
    scrollok(stdscr, TRUE);
    wrefresh(stdscr);
    wrefresh(stdscr);
}

int main() {
    unsigned int width; // vars to ask the player
    unsigned int height;
    unsigned int cell_size;
    int seed;
    int fog_radius;
    
    // movement
     enum {
         MOVE_UP = 'w',
         MOVE_DOWN = 's',
         MOVE_LEFT = 'a',
         MOVE_RIGHT = 'd'
    };

    int potions_collected = 0;

    char input = 0; // user input
    // player coordinates
    int player_coordinate_x = 0;
    int player_coordinate_y = 0;


    int row = 0; // variables to go through the 2d array maze
    int col = 0;

    // ask the user for parameters--------------------------
    printf("Press 'q' in maze to quit.\n");
    printf("Enter a width: ");
    scanf("%d", &width);
    printf("Enter a height: ");
    scanf("%d", &height);
    printf("Enter size for the cell (enter an odd number): ");
    scanf("%d", &cell_size);
    printf("Enter a value for the seed: ");
    scanf("%d", &seed);
    printf("Enter a fog radius: ");
    scanf("%d", &fog_radius);

    struct maze my_maze = generate_maze(width, height, cell_size, seed); // creat a new maze

    while (row < my_maze.h){ // spawn player at the first entrance row
        if (my_maze.a[row][col] == ' '){
            my_maze.a[row][col] = PLAYER;

            player_coordinate_x = col;
            player_coordinate_y = row;

            break;
        }
        row ++;
    }

    // print the maze for the first time
    print_maze_fog(row, col,player_coordinate_x, player_coordinate_y, my_maze, potions_collected, fog_radius);


    while (true){ // while playing

        input = getch(); // get a char (movement key)

        switch (input) {
            case MOVE_UP:
                if (my_maze.a[player_coordinate_y - 1][player_coordinate_x] != 'w'){  // get movement

                    if (my_maze.a[player_coordinate_y - 1][player_coordinate_x] == POTION) //if there is a potion on that spot
                        potions_collected += 1; // pick it up

                    my_maze.a[player_coordinate_y][player_coordinate_x] = ' '; // replace the old coordinate with blank
                    my_maze.a[player_coordinate_y - 1][player_coordinate_x] = PLAYER; // place the player at the new coordinate
                    player_coordinate_y -= 1; // update the player coordinates
                    print_maze_fog(row, col, player_coordinate_x, player_coordinate_y, my_maze, potions_collected,fog_radius); // refresh the maze
                }
                break;
            case MOVE_DOWN:
                if (my_maze.a[player_coordinate_y + 1][player_coordinate_x] != 'w'){

                    if (my_maze.a[player_coordinate_y + 1][player_coordinate_x] == POTION)
                        potions_collected += 1;

                    my_maze.a[player_coordinate_y][player_coordinate_x] = ' ';
                    my_maze.a[player_coordinate_y + 1][player_coordinate_x] = PLAYER;
                    player_coordinate_y += 1;
                    print_maze_fog(row, col, player_coordinate_x,player_coordinate_y,my_maze, potions_collected, fog_radius);

                }
                break;
            case MOVE_LEFT:
                if (my_maze.a[player_coordinate_y][player_coordinate_x - 1] != 'w'
                && (player_coordinate_x - 1) < my_maze.w){

                    if (my_maze.a[player_coordinate_y][player_coordinate_x - 1] == POTION)
                        potions_collected += 1;

                    my_maze.a[player_coordinate_y][player_coordinate_x] = ' ';
                    my_maze.a[player_coordinate_y][player_coordinate_x - 1] = PLAYER;
                    player_coordinate_x -= 1;
                    print_maze_fog(row, col, player_coordinate_x, player_coordinate_y, my_maze, potions_collected,fog_radius);

                }
                break;
            case MOVE_RIGHT:
                if (my_maze.a[player_coordinate_y][player_coordinate_x + 1] != 'w'
                && (player_coordinate_x + 1) < my_maze.w){

                    if (my_maze.a[player_coordinate_y][player_coordinate_x + 1] == POTION)
                        potions_collected += 1;

                    my_maze.a[player_coordinate_y][player_coordinate_x] = ' ';
                    my_maze.a[player_coordinate_y][player_coordinate_x + 1] = PLAYER;
                    player_coordinate_x += 1;
                    print_maze_fog(row, col,player_coordinate_x, player_coordinate_y, my_maze, potions_collected,fog_radius);

                    // check can the player exit
                    if (player_coordinate_x + 1 == my_maze.w){ // player exit
                        if (potions_collected == NEEDED_POTIONS){
                            clear();
                            refresh();
                            printw("You have escaped the maze! Press any key to exit.");
                            getch();
                            endwin();
                            return 0;
                        } else { // player cannot exit
                            refresh();
                            printw("\nYou cannot exit before collecting all the potions!");
                        }
                    }
                }
                break;
            case 'q':  // quit the game at any time
                endwin();
                return 0;
            default: // shouldn't be able to get there
                break;
        }


    }
}

