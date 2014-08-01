/*
 * This file contains the maze-solving strategy.
 */

#include <stdbool.h>
#include <pololu/3pi.h>
#include "follow-segment.h"
#include "sounds.h"

#define MAZE_SIZE 16

/*
    +y
 -x    +x 
    -y
*/

typedef struct node
{
  uint8_t cost;
  uint8_t marks;
} node;

node maze[MAZE_SIZE][MAZE_SIZE]; // x, y


// directions

#define NORTH 0
#define EAST  1
#define SOUTH 2
#define WEST  3

#define flip(dir) (dir ^ 2)
#define left_of(dir) ((dir - 1) & 0x3)
#define right_of(dir) ((dir + 1) & 0x3)


// exit info

#define NORTH_LSB 0
#define EAST_LSB 2
#define DIR_TO_FINISH_LSB 4

#define NORTH_MARK (1 << NORTH_LSB)
#define EAST_MARK  (1 << EAST_LSB)

#define NORTH_MARK_MASK (0x3 << NORTH_LSB)
#define EAST_MARK_MASK  (0x3 << EAST_LSB)
#define DIR_TO_FINISH_MASK  (0x3 << DIR_TO_FINISH_LSB)

#define get_north_marks(x, y) ((maze[x][y].marks & NORTH_MARK_MASK) >> NORTH_LSB)
#define get_east_marks(x, y)  ((maze[x][y].marks & EAST_MARK_MASK) >> EAST_LSB)

#define add_north_mark(x, y) (maze[x][y].marks += NORTH_MARK)
#define add_east_mark(x, y)  (maze[x][y].marks += EAST_MARK)

#define set_dir_to_finish(x, y, dir) (maze[x][y].marks = ((maze[x][y].marks & ~DIR_TO_FINISH_MASK) | ((dir) << DIR_TO_FINISH_LSB)))


// state info

typedef struct pos
{
  int8_t x;
  int8_t y;
} pos;

uint8_t dir;
pos start, here, finish;
bool found_finish;
bool recorded_finish;

bool found_left, found_straight, found_right;
uint8_t dir_marks[4];


pos fill_pos;
uint8_t fill_cost, fill_dir_to_finish;


// final path

#define MAX_PATH_LENGTH 100

char path[MAX_PATH_LENGTH] = "";
uint8_t path_length = 0; // the length of the path


// Displays the current path on the LCD, using two rows if necessary.
void display_path()
{
  // Set the last character of the path to a 0 so that the print()
  // function can find the end of the string.  This is how strings
  // are normally terminated in C.
  path[path_length] = 0;

  clear();
  print(path);

  if(path_length > 8)
  {
    lcd_goto_xy(0,1);
    print(path+8);
  }
}


void clear_map()
{
  for (uint8_t y = 0; y < MAZE_SIZE; y++)
  {
    for (uint8_t x = 0; x < MAZE_SIZE; x++)
      maze[x][y] = (node){ .cost = 255, .marks = 0 };
  }
}

void shift_map_north(uint8_t amt)
{
  for (uint8_t y = (MAZE_SIZE - 1); y >= 0; y--) 
  {
    for (uint8_t x = 0; x < MAZE_SIZE; x++)
    {
      if (y >= amt)
        maze[x][y] = maze[x][y - amt];
      else
        maze[x][y] = (node){ 0, 0 };
    }
  }
  
  start.y  += amt;
  here.y   += amt;
  finish.y += amt;
}

void shift_map_south(uint8_t amt)
{
  for (uint8_t y = 0; y < MAZE_SIZE; y++)
  {
    for (uint8_t x = 0; x < MAZE_SIZE; x++)
    {
      if (y < (MAZE_SIZE - amt))
        maze[x][y] = maze[x][y + amt];
      else
        maze[x][y] = (node){ 0, 0 };
    }
  }
  
  start.y  -= amt;
  here.y   -= amt;
  finish.y -= amt;
}

void shift_map_east(uint8_t amt)
{
  for (uint8_t x = (MAZE_SIZE - 1); x >= 0; x--)
  {
    for (uint8_t y = 0; y < MAZE_SIZE; y++)
    {
      if (x >= amt)
      maze[x][y] = maze[x - amt][y];
      else
      maze[x][y] = (node){ 0, 0 };
    }
  }
  
   start.x  += amt;
   here.x   += amt;
   finish.x += amt;
}

void shift_map_west(uint8_t amt)
{
  for (uint8_t x = 0; x < MAZE_SIZE; x++)
  {
    for (uint8_t y = 0; y < MAZE_SIZE; y++)
    {
      if (x < (MAZE_SIZE - amt))
      maze[x][y] = maze[x + amt][y];
      else
      maze[x][y] = (node){ 0, 0 };
    }
  }
  
  start.x  -= amt;
  here.x   -= amt;
  finish.x -= amt;
}

void update_map(uint8_t seg_length)
{
  pos prev = here;
  
  // record the most recent segment followed
  
  switch(dir)
  {
    
  case NORTH:
    here.y += seg_length;

    if (here.y >= MAZE_SIZE)
      shift_map_south(here.y - (MAZE_SIZE - 1));
      
    for (uint8_t y = prev.y; y < here.y; y++)
      add_north_mark(here.x, y);
 
    break;  


  case EAST:
  
    here.x += seg_length;
      
    if (here.x >= MAZE_SIZE)
      shift_map_west(here.x - (MAZE_SIZE - 1));
      
    for (uint8_t x = prev.x; x < here.x; x++)
      add_east_mark(x, here.y);

    break;


  case SOUTH:
  
    here.y -= seg_length;
      
    if (here.y < 1)
      shift_map_north(1 - here.y);  

    for (uint8_t y = here.y; y < prev.y; y++)
      add_north_mark(here.x, y);
        
    break;
    
    
  case WEST:
  
    here.x -= seg_length;
      
    if (here.x < 1)
      shift_map_east(1 - here.x);
      
    for (uint8_t x = here.x; x < prev.x; x++)
      add_east_mark(x, here.y);

    break;
  }   
    
      
  // store # of marks in each direction
  dir_marks[NORTH] = get_north_marks(here.x, here.y);
  dir_marks[EAST]  = get_east_marks(here.x, here.y);
  dir_marks[SOUTH] = get_north_marks(here.x, here.y - 1);
  dir_marks[WEST]  = get_east_marks(here.x - 1, here.y);     
}  

char select_turn()
{  
  if ((dir_marks[left_of(dir)] || dir_marks[dir] || dir_marks[right_of(dir)]) && dir_marks[flip(dir)] == 1)
  {
    // we've seen this junction before, but we didn't depart in the direction we just arrived from, so we found a loop; turn around and go back
    return 'B';
  }
  
  unsigned char selected_turn = 'B';
  uint8_t fewest_marks = 2;
  
  if (found_left && (dir_marks[left_of(dir)] < fewest_marks))
  {
    selected_turn = 'L';
    fewest_marks = dir_marks[left_of(dir)];
  }
  if (found_straight && (dir_marks[dir] < fewest_marks))
  {
    selected_turn = 'S';
    fewest_marks = dir_marks[dir];
  }
  if (found_right && (dir_marks[right_of(dir)] < fewest_marks))
  {
    selected_turn = 'R';
    fewest_marks = dir_marks[right_of(dir)];
  }  
  
  if ((fewest_marks == 2) && dir_marks[flip(dir)] >= 2)
  {
    // we've arrived back at the start
    // there might be 3 marks in the direction we came from if we originally started in the middle of a segment:
    //  ___________
    // /   start___)
    // \___________end
    return 'X';
  }    

  // 
  return selected_turn;
}

void turn(char turn_dir)
{
  switch(turn_dir)
  {
  case 'L':
    // Turn left.
    dir = left_of(dir);
    set_motors(-80,80);
    delay_ms(200);
    break;
  case 'R':
    // Turn right.
    dir = right_of(dir);
    set_motors(80,-80);
    delay_ms(200);
    break;
  case 'B':
    // Turn around.
    dir = flip(dir);
    set_motors(80,-80);
    delay_ms(400);
    break;
  case 'S':
    // Don't do anything!
    break;
  }
}

// returns true if found path to finish
// using global vars instead of function params to keep track of stuff should cut down on RAM usage at the cost of slower execution
void fill_costs_from_node()
{
  if (fill_cost < maze[fill_pos.x][fill_pos.y].cost)
  {
     maze[fill_pos.x][fill_pos.y].cost = fill_cost;
    set_dir_to_finish(fill_pos.x, fill_pos.y, fill_dir_to_finish);
    
    fill_cost++;
    
    if (get_north_marks(fill_pos.x, fill_pos.y))
    {
      fill_dir_to_finish = SOUTH;
      fill_pos.y++;
      fill_costs_from_node();
      fill_pos.y--;
    }      
    if (get_east_marks(fill_pos.x, fill_pos.y))
    {
      fill_dir_to_finish = WEST;
      fill_pos.x++;
      fill_costs_from_node();
      fill_pos.x--;
    }      
    if (get_north_marks(fill_pos.x, fill_pos.y - 1))
    {
      fill_dir_to_finish = NORTH;
      fill_pos.y--;
      fill_costs_from_node();
      fill_pos.y++;
    }      
    if (get_east_marks(fill_pos.x - 1, fill_pos.y))
    {
      fill_dir_to_finish = EAST;
      fill_pos.x--;
      fill_costs_from_node();
      fill_pos.x++;
    }      
      
    fill_cost--;
  }
}

void fill_all_costs()
{
  fill_cost = 0;
  fill_pos = finish;
  
  fill_costs_from_node(); // dir_to_finish is meaningless for finish node
}

void build_path()
{
  
}

// This function is called once, from main.c.
void map_maze()
{
  found_finish = recorded_finish = false;
  dir = NORTH;
  start = (pos){ MAZE_SIZE / 2, MAZE_SIZE / 2 };
  here = start;
  
  clear_map();
  
  // Loop until we have solved the maze.
  while(1)
  {
    found_left = found_straight = found_right = false;
    
    unsigned int start_ms = get_ms();
    
    follow_segment();

    // Drive straight a bit.  This helps us in case we entered the
    // intersection at an angle.
    // Note that we are slowing down - this prevents the robot
    // from tipping forward too much.
    set_motors(50,50);
    delay_ms(50);

    // Now read the sensors and check the intersection type.
    unsigned int sensors[5];
    read_line(sensors,IR_EMITTERS_ON);

    // Check for left and right exits.
    if(sensors[0] > 100)
      found_left = true;
    if(sensors[4] > 100)
      found_right = true;

    // Drive straight a bit more - this is enough to line up our
    // wheels with the intersection.
    set_motors(40,40);
    delay_ms(200);
    
    unsigned int end_ms = get_ms();

    // Check for a straight exit.
    read_line(sensors,IR_EMITTERS_ON);
    if(sensors[1] > 200 || sensors[2] > 200 || sensors[3] > 200)
      found_straight = true;

    // Check for the ending spot.
    // If all three middle sensors are on dark black, we have
    // solved the maze.
    if(sensors[1] > 600 && sensors[2] > 600 && sensors[3] > 600)
    {
      found_left = found_straight = found_right = false;
      found_finish = true;
      play_from_program_space(done);
    }
    else if (!is_playing())
    {
      play_from_program_space(wakka);
    }
    
    // Intersection identification is complete.
    
    // stop motors to prevent too much untracked movement during the following computations (if they take too long)
    // might not be necessary...
    //set_motors(0, 0);
    
    // empirically determined: length = (ms - 140) / 668
    uint8_t seg_length = (((end_ms - start_ms) - 140) + 334) / 668;

    update_map(seg_length);

    if (found_finish && !recorded_finish)
    {
      finish = here;
      recorded_finish = true;
    }
    
    char turn_dir = select_turn();
    if (turn_dir == 'X')
    {
      // Beep to show that we finished the maze.
      play("!>>a32");
      break;
    }      
      
    turn(select_turn());
  }


  // Solved the maze!
  
  fill_all_costs();
  build_path();  
  

  // Now enter an infinite loop - we can re-run the maze as many
  // times as we want to.
  while(1)
  {    
    set_motors(0,0);

    // Wait for the user to press a button, while displaying
    // the solution.
    while(!button_is_pressed(BUTTON_B))
    {
      if(get_ms() % 2000 < 1000)
      {
        clear();
        print("Solved!");
        lcd_goto_xy(0,1);
        print("Press B");
      }
      else
        display_path();
      delay_ms(30);
    }
    while(button_is_pressed(BUTTON_B));
  
    /*delay_ms(1000);

    // Re-run the maze.  It's not necessary to identify the
    // intersections, so this loop is really simple.
    int i;
    for(i=0;i<path_length;i++)
    {
      // SECOND MAIN LOOP BODY  
      follow_segment();

      // Drive straight while slowing down, as before.
      set_motors(50,50);
      delay_ms(50);
      set_motors(40,40);
      delay_ms(200);

      // Make a turn according to the instruction stored in
      // path[i].
      turn(path[i]);
    }
    
    // Follow the last segment up to the finish.
    follow_segment();*/

    // Now we should be at the finish!  Restart the loop.
  }
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: t **
// end: **
