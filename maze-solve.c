/*
 * This file contains the maze-solving strategy.
 */

#include <stdbool.h>
#include <avr/pgmspace.h>
#include <pololu/3pi.h>
#include "follow-segment.h"

#define MAZE_SIZE 16

/*
    +y
 -x    +x 
    -y
*/

uint8_t maze[MAZE_SIZE][MAZE_SIZE]; // x, y

#define NORTH 0
#define EAST  1
#define SOUTH 2
#define WEST  3

#define flip(dir) (dir ^ 2)
#define left_of(dir) ((dir - 1) & 0xF)
#define right_of(dir) ((dir + 1) & 0xF)

#define dir_exit(dir)           (1 << dir)
#define exit_explored(exit_dir) (exit_dir << 4)
#define dir_explored(dir)       exit_explored(dir_exit(dir))

#define NORTH_EXIT dir_exit(NORTH)
#define EAST_EXIT  dir_exit(EAST)
#define SOUTH_EXIT dir_exit(SOUTH)
#define WEST_EXIT  dir_exit(WEST)

#define NORTH_EXIT_EXPLORED dir_explored(NORTH)
#define EAST_EXIT_EXPLORED  dir_explored(EAST)
#define SOUTH_EXIT_EXPLORED dir_explored(SOUTH)
#define WEST_EXIT_EXPLORED  dir_explored(WEST)

typedef struct pos
{
  int8_t x;
  int8_t y;
} pos;

uint8_t dir;
pos start, here, finish;
bool found_finish;

void clear_map()
{
  for (uint8_t y = 0; y < MAZE_SIZE; y++)
  {
    for (uint8_t x = 0; x < MAZE_SIZE; x++)
      maze[x][y] = 0;
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
        maze[x][y] = 0;
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
        maze[x][y] = 0;
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
      maze[x][y] = 0;
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
      maze[x][y] = 0;
    }
  }
  
  start.x  -= amt;
  here.x   -= amt;
  finish.x -= amt;
}

void update_map(uint8_t seg_length, bool found_left, bool found_straight, bool found_right)
{
  pos prev = here;
  
  switch(dir)
  {
    
  case NORTH:
    here.y -= seg_length;
      
    if (here.y < 0)
      shift_map_south(-here.y);
      
    for (uint8_t y = (prev.y + 1); y < here.y; y++)
      maze[here.x][y] |= (SOUTH_EXIT | SOUTH_EXIT_EXPLORED | NORTH_EXIT | NORTH_EXIT_EXPLORED);
        
    maze[here.x][here.y] |= (SOUTH_EXIT | SOUTH_EXIT_EXPLORED);
      
    if (found_left)
      maze[here.x][here.y] |= WEST_EXIT;
    if (found_straight)
      maze[here.x][here.y] |= NORTH_EXIT;
    if (found_right)
      maze[here.x][here.y] |= EAST_EXIT;
      
    break;  


  case SOUTH:
  
    here.y += seg_length;
      
    if (here.y >= MAZE_SIZE)
      shift_map_north(here.y - (MAZE_SIZE - 1));

    for (uint8_t y = (prev.y - 1); y > here.y; y--)
      maze[here.x][y] |= (NORTH_EXIT | NORTH_EXIT_EXPLORED | SOUTH_EXIT | SOUTH_EXIT_EXPLORED);
      
    maze[here.x][here.y] |= (NORTH_EXIT | NORTH_EXIT_EXPLORED);
      
    if (found_left)
      maze[here.x][here.y] |= EAST_EXIT;
    if (found_straight)
      maze[here.x][here.y] |= SOUTH_EXIT;
    if (found_right)
      maze[here.x][here.y] |= WEST_EXIT;
        
    break;


  case EAST:
  
    here.x += seg_length;
      
    if (here.x >= MAZE_SIZE)
      shift_map_west(here.x - (MAZE_SIZE - 1));
      
    for (uint8_t x = (prev.x + 1); x < here.x; x++)
      maze[x][here.y] |= (WEST_EXIT | WEST_EXIT_EXPLORED | EAST_EXIT | EAST_EXIT_EXPLORED);
      
    maze[here.x][here.y] |= (WEST_EXIT | WEST_EXIT_EXPLORED);
      
    if (found_left)
      maze[here.x][here.y] |= NORTH_EXIT;
    if (found_straight)
      maze[here.x][here.y] |= EAST_EXIT;
    if (found_right)
      maze[here.x][here.y] |= SOUTH_EXIT;
	  
    break;


  case WEST:
  
    here.x -= seg_length;
      
    if (here.x < 0)
      shift_map_east(-here.x);
      
    for (uint8_t x = (prev.x - 1); x > here.x; x--)
      maze[x][here.y] |= (EAST_EXIT | EAST_EXIT_EXPLORED | WEST_EXIT | WEST_EXIT_EXPLORED);
      
    maze[here.x][here.y] |= (EAST_EXIT | EAST_EXIT_EXPLORED);
      
    if (found_left)
      maze[here.x][here.y] |= SOUTH_EXIT;
    if (found_straight)
      maze[here.x][here.y] |= WEST_EXIT;
    if (found_right)
      maze[here.x][here.y] |= NORTH_EXIT;

    break;
  }    
}  

// returns first unexplored exit of { S, L, R }; 0 if no unexplored exits
char select_turn(bool found_left, bool found_straight, bool found_right)
{
  if (found_straight && !(maze[here.x][here.y] & dir_explored(dir)))
    return 'S';
  else if (found_left && !(maze[here.x][here.y] & dir_explored(left_of(dir))))
    return 'L';
  else if (found_right && !(maze[here.x][here.y] & dir_explored(right_of(dir))))
    return 'R';
  else
    return 0;
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

bool go_to_nearest_unexplored_exit()
{
  return false;
}

// This function is called once, from main.c.
void map_maze()
{
  found_finish = 0;
  
  dir = NORTH;
  start = (pos){ MAZE_SIZE / 2, MAZE_SIZE / 2 };
  here = start;
  
  clear_map();
  maze[start.x][start.y] |= (NORTH_EXIT | NORTH_EXIT_EXPLORED);

  // Loop until we have solved the maze.
  while(1)
  {
    unsigned int start_ms = get_ms();
    
    follow_segment();

    // Drive straight a bit.  This helps us in case we entered the
    // intersection at an angle.
    // Note that we are slowing down - this prevents the robot
    // from tipping forward too much.
    set_motors(50,50);
    delay_ms(50);

    // These variables record whether the robot has seen a line to the
    // left, straight ahead, and right, whil examining the current
    // intersection.
    uint8_t found_left=0;
    uint8_t found_straight=0;
    uint8_t found_right=0;

    // Now read the sensors and check the intersection type.
    unsigned int sensors[5];
    read_line(sensors,IR_EMITTERS_ON);

    // Check for left and right exits.
    if(sensors[0] > 100)
      found_left = 1;
    if(sensors[4] > 100)
      found_right = 1;

    // Drive straight a bit more - this is enough to line up our
    // wheels with the intersection.
    set_motors(40,40);
    delay_ms(200);
    
    unsigned int end_ms = get_ms();

    // Check for a straight exit.
    read_line(sensors,IR_EMITTERS_ON);
    if(sensors[1] > 200 || sensors[2] > 200 || sensors[3] > 200)
      found_straight = 1;

    // Check for the ending spot.
    // If all three middle sensors are on dark black, we have
    // solved the maze.
    if(sensors[1] > 600 && sensors[2] > 600 && sensors[3] > 600)
    {
      found_left = 0;
      found_straight = 0;
      found_right = 0;
    }
    
    // Intersection identification is complete.
    
    // stop motors to prevent too much untracked movement during the following computations (if they take too long)
    // might not be necessary...
    set_motors(0, 0);
    
    // empirically determined: length = (ms - 140) / 668
    uint8_t seg_length = (((end_ms - start_ms) - 140) + 334) / 668;

    update_map(seg_length, found_left, found_straight, found_right);

    char turn_dir = select_turn(found_left, found_straight, found_right);
    
    if (turn_dir)
    {
      // there is an unexplored exit, so go that way
      turn(turn_dir);
    }      
    else
    {
      if (!go_to_nearest_unexplored_exit())
      {
        return;
      }
    }
  }

  /*// Solved the maze!

  // Now enter an infinite loop - we can re-run the maze as many
  // times as we want to.
  while(1)
  {
    // Beep to show that we finished the maze.
    set_motors(0,0);
    play(">>a32");

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
  
    delay_ms(1000);

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
    follow_segment();

    // Now we should be at the finish!  Restart the loop.
  }*/
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: t **
// end: **
