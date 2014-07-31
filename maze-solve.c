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

typedef struct node
{
  uint8_t cost;
  uint8_t exits; 
} node;

node maze[MAZE_SIZE][MAZE_SIZE]; // x, y

#define NORTH 0
#define EAST  1
#define SOUTH 2
#define WEST  3

#define flip(dir) (dir ^ 2)
#define left_of(dir) ((dir - 1) & 0xF)
#define right_of(dir) ((dir + 1) & 0xF)

/*#define dir_exit(dir)           (1 << dir)
#define exit_explored(exit_dir) (exit_dir << 4)
#define dir_explored(dir)       exit_explored(dir_exit(dir))

#define NORTH_EXIT dir_exit(NORTH)
#define EAST_EXIT  dir_exit(EAST)
#define SOUTH_EXIT dir_exit(SOUTH)
#define WEST_EXIT  dir_exit(WEST)

#define NORTH_EXIT_EXPLORED dir_explored(NORTH)
#define EAST_EXIT_EXPLORED  dir_explored(EAST)
#define SOUTH_EXIT_EXPLORED dir_explored(SOUTH)
#define WEST_EXIT_EXPLORED  dir_explored(WEST)*/

#define NORTH_LSB 0
#define EAST_LSB 2

#define NORTH_MARK (1 << NORTH_LSB)
#define EAST_MARK  (1 << EAST_LSB)

#define NORTH_MARK_MASK (0x3 << NORTH_LSB)
#define EAST_MARK_MASK  (0x3 << EAST_LSB)

#define NO_EXIT 3
#define NO_EXITS ((NO_EXIT << NORTH_LSB) | (NO_EXIT << EAST_LSB))

#define get_north_marks(x, y) ((maze[x][y].exits & NORTH_MARK_MASK) >> NORTH_LSB)
#define get_east_marks(x, y)  ((maze[x][y].exits & EAST_MARK_MASK) >> EAST_LSB)

#define zero_north_marks(x, y) (maze[x][y].exits &= ~NORTH_MARK_MASK)
#define zero_east_marks(x, y)  (maze[x][y].exits &= ~EAST_MARK_MASK)

#define add_north_mark(x, y) (maze[x][y].exits += NORTH_MARK)
#define add_east_mark(x, y)  (maze[x][y].exits += EAST_MARK)

typedef struct pos
{
  int8_t x;
  int8_t y;
} pos;

uint8_t dir;
pos start, here, finish;
bool found_finish;

bool found_left, found_straight, found_right;
uint8_t dir_marks[4];

void clear_map()
{
  for (uint8_t y = 0; y < MAZE_SIZE; y++)
  {
    for (uint8_t x = 0; x < MAZE_SIZE; x++)
      maze[x][y] = (node){ .cost = 255, .exits = NO_EXITS };
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
  
  // record this node's unexplored exits
  
  if ( ((dir == NORTH) && found_straight) || ((dir == EAST) && found_left) || ((dir == WEST) && found_right) )
  {
    // north exit
    if (get_north_marks(here.x, here.y) == NO_EXIT)
      zero_north_marks(here.x, here.y);
  }
  if ( ((dir == EAST) && found_straight) || ((dir == SOUTH) && found_left) || ((dir == NORTH) && found_right) )
  {
    // east exit
    if (get_east_marks(here.x, here.y) == NO_EXIT)
      zero_east_marks(here.x, here.y);
  }     
  if ( ((dir == SOUTH) && found_straight) || ((dir == WEST) && found_left) || ((dir == EAST) && found_right) )
  {
    // south exit (record as north exit in the node to the south)
    if (get_north_marks(here.x, here.y - 1) == NO_EXIT)
      zero_north_marks(here.x, here.y - 1);
  }       
  if ( ((dir == WEST) && found_straight) || ((dir == NORTH) && found_left) || ((dir == SOUTH) && found_right) )
  {
    // west exit (record as east exit in the node to the west)    
    if (get_east_marks(here.x - 1, here.y) == NO_EXIT)
      zero_east_marks(here.x - 1, here.y);
  }       
}  

char select_turn()
{
  unsigned char selected_turn = 'B';
  uint8_t fewest_marks = 2;
  bool junction_marked = false;

  if (found_left && (dir_marks[left_of(dir)] < fewest_marks))
  {
    junction_marked = true;
    selected_turn = 'L';
    fewest_marks = dir_marks[left_of(dir)];
  }
  if (found_straight && (dir_marks[dir] < fewest_marks))
  {
    junction_marked = true;
    selected_turn = 'S';
    fewest_marks = dir_marks[dir];
  }
  if (found_right && (dir_marks[right_of(dir)] < fewest_marks))
  {
    junction_marked = true;
    selected_turn = 'R';
    fewest_marks = dir_marks[right_of(dir)];
  }
  
  if (junction_marked && dir_marks[flip(dir)] == 1)
  {
    // we've seen this junction before, but we didn't depart in the direction we just arrived from, so we found a loop; turn around and go back
    return 'B';
  }    
  
  if ((!(found_left || found_straight || found_right) || fewest_marks == 2) && dir_marks[flip(dir)] >= 2)
  {
    // we've arrived back at the start (didn't find the finish)
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
  //maze[start.x][start.y] |= (NORTH_EXIT | NORTH_EXIT_EXPLORED);

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
      break;
    }
    
    // Intersection identification is complete.
    
    // stop motors to prevent too much untracked movement during the following computations (if they take too long)
    // might not be necessary...
    set_motors(0, 0);
    
    // empirically determined: length = (ms - 140) / 668
    uint8_t seg_length = (((end_ms - start_ms) - 140) + 334) / 668;

    update_map(seg_length);

    turn(select_turn());
  }

  set_motors(0,0);
  play(">>a32");
  while(1);

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
