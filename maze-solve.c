/*
 * This file contains the maze-solving strategy.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <pololu/3pi.h>
#include "follow-segment.h"
#include "sounds.h"

#define MAZE_SIZE 16
#define MAX_COST 255

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

#define get_dir_to_finish(x, y) ((maze[x][y].marks & DIR_TO_FINISH_MASK) >> DIR_TO_FINISH_LSB)
#define set_dir_to_finish(x, y, dir) (maze[x][y].marks = ((maze[x][y].marks & ~DIR_TO_FINISH_MASK) | ((dir) << DIR_TO_FINISH_LSB)))


// state info

typedef struct pos
{
  int8_t x;
  int8_t y;
} pos;

#define distance_between(a, b) (abs((b).x - (a).x) + abs((b).y - (a).y)) // manhattan distance

uint8_t dir;
pos start, here, prev, finish;
bool found_finish;
bool recorded_finish;

bool found_left, found_straight, found_right;
uint8_t dir_marks[4];


// used to pass info in fill_costs_from_node() recursion

uint8_t cost_here, dir_to_finish_here;


// final path

#define MAX_PATH_LENGTH 100

char path[MAX_PATH_LENGTH];
uint8_t path_seg_lengths[MAX_PATH_LENGTH];
uint8_t path_length; // the length of the path


// Displays the current path on the LCD, using two rows if necessary.
void display_path()
{
  char buf[17];
  uint8_t i;

  for (i = 0; (i < path_length) && (i < 8); i++)
  {
    buf[2*i] = '0' + path_seg_lengths[i];
    buf[2*i+1] = path[i];
  }
  buf[2*i] = 0;
  
  clear();
  print(buf);

  if(path_length > 4)
  {
    lcd_goto_xy(0,1);
    print(buf+8);
  }
}


void clear_map()
{
  for (uint8_t y = 0; y < MAZE_SIZE; y++)
  {
    for (uint8_t x = 0; x < MAZE_SIZE; x++)
      maze[x][y] = (node){ .cost = MAX_COST, .marks = 0 };
  }
}

void shift_map_north(uint8_t amt)
{
  for (int8_t y = (MAZE_SIZE - 1); y >= 0; y--) 
  {
    for (int8_t x = 0; x < MAZE_SIZE; x++)
    {
      if (y >= amt)
        maze[x][y].marks = maze[x][y - amt].marks;
      else
        maze[x][y].marks = 0;
    }
  }
  
  start.y  += amt;
  here.y   += amt;
  prev.y   += amt;
  finish.y += amt;
}

void shift_map_east(uint8_t amt)
{
  for (int8_t x = (MAZE_SIZE - 1); x >= 0; x--)
  {
    for (int8_t y = 0; y < MAZE_SIZE; y++)
    {
      if (x >= amt)
        maze[x][y].marks = maze[x - amt][y].marks;
      else
        maze[x][y].marks = 0;
    }
  }
  
   start.x  += amt;
   here.x   += amt;
   prev.x   += amt;
   finish.x += amt;
}

void shift_map_south(uint8_t amt)
{
  for (int8_t y = 0; y < MAZE_SIZE; y++)
  {
    for (int8_t x = 0; x < MAZE_SIZE; x++)
    {
      if (y < (MAZE_SIZE - amt))
        maze[x][y].marks = maze[x][y + amt].marks;
      else
        maze[x][y].marks = 0;
    }
  }
  
  start.y  -= amt;
  here.y   -= amt;
  prev.y   -= amt;
  finish.y -= amt;
}

void shift_map_west(uint8_t amt)
{
  for (int8_t x = 0; x < MAZE_SIZE; x++)
  {
    for (int8_t y = 0; y < MAZE_SIZE; y++)
    {
      if (x < (MAZE_SIZE - amt))
        maze[x][y].marks = maze[x + amt][y].marks;
      else
        maze[x][y].marks = 0;
    }
  }
  
  start.x  -= amt;
  here.x   -= amt;
  prev.x   -= amt;
  finish.x -= amt;
}

void update_map(uint8_t seg_length)
{
  prev = here;
  
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
  
  
  /*set_motors(0, 0);
  clear();
  lcd_goto_xy(1, 0);
  print_long(dir_marks[dir]);
  lcd_goto_xy(0, 1);
  print_long(dir_marks[left_of(dir)]);
  print_long(dir_marks[flip(dir)]);
  print_long(dir_marks[right_of(dir)]);
  lcd_goto_xy(3, 1);
  switch(dir)
  {
    case NORTH: print_character('N'); break;
    case EAST:  print_character('E'); break;
    case SOUTH: print_character('S'); break;
    case WEST:  print_character('W'); break;
  }
  lcd_goto_xy(5, 0);
  print_character('x');
  print_long(here.x);
  lcd_goto_xy(5, 1);
  print_character('y');
  print_long(here.y);*/
  clear();
  print_long(seg_length);
  //wait_for_button(BUTTON_A);
  //delay(200);
}  

char select_turn()
{  
  if ((dir_marks[left_of(dir)] || dir_marks[dir] || dir_marks[right_of(dir)]) && dir_marks[flip(dir)] == 1)
  {
    // we've seen this intersection before, but we didn't depart in the direction we just arrived from, so we found a loop; turn around and go back
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

// using global vars instead of function params to keep track of stuff should cut down on RAM usage at the cost of slower execution
void fill_costs_from_here()
{
  if (cost_here < maze[here.x][here.y].cost)
  {
     maze[here.x][here.y].cost = cost_here;
    set_dir_to_finish(here.x, here.y, dir_to_finish_here);
    
    if ((distance_between(start, here) + cost_here) < maze[start.x][start.y].cost)
    {
      cost_here++;
      
      if (get_north_marks(here.x, here.y) )
      {
        // north exit
        dir_to_finish_here = SOUTH;
        here.y++;
        fill_costs_from_here();
        here.y--;
      }      
      if (get_east_marks(here.x, here.y))
      {
        // east exit
        dir_to_finish_here = WEST;
        here.x++;
        fill_costs_from_here();
        here.x--;
      }      
      if (get_north_marks(here.x, here.y - 1))
      {
        // south exit
        dir_to_finish_here = NORTH;
        here.y--;
        fill_costs_from_here();
        here.y++;
      }      
      if (get_east_marks(here.x - 1, here.y))
      {
        // west exit
        dir_to_finish_here = EAST;
        here.x--;
        fill_costs_from_here();
        here.x++;
      }
      
      cost_here--;
    }
  }    
}

void fill_all_costs()
{
  cost_here = 0;
  here = finish;
  
  fill_costs_from_here(); // dir_to_finish is meaningless for finish node
}

void add_path_segment(char turn_dir, uint8_t seg_length)
{
  path[path_length] = turn_dir;
  path_seg_lengths[path_length] = seg_length;
  path_length++;
}

void build_path()
{
  prev = start;
  here = start;
  dir = NORTH;
  
  while ( !((here.x == finish.x) && (here.y == finish.y)) )
  {
    dir_to_finish_here = get_dir_to_finish(here.x, here.y);
    dir_marks[NORTH] = get_north_marks(here.x, here.y);
    dir_marks[EAST]  = get_east_marks(here.x, here.y);
    dir_marks[SOUTH] = get_north_marks(here.x, here.y - 1);
    dir_marks[WEST]  = get_east_marks(here.x - 1, here.y);
      
    // only add 'S' if there's an intersection (left or right exit)
    if ((dir_to_finish_here == dir) && (dir_marks[left_of(dir)] || dir_marks[right_of(dir)]))
    {
      add_path_segment('S', distance_between(prev, here));
      prev = here;
    }
    
    if (dir_to_finish_here == left_of(dir))
    {
      add_path_segment('L', distance_between(prev, here));
      prev = here;
    }
    
    if (dir_to_finish_here == right_of(dir))
    {
      add_path_segment('R', distance_between(prev, here));
      prev = here;
    }
    
    if (dir_to_finish_here == flip(dir))
    {
      // this should only happen as the very first turn
      add_path_segment('B', distance_between(prev, here));
      prev = here;
    }
    
    dir = dir_to_finish_here;
    
    switch (dir)
    {
    case NORTH:
      here.y++;
      break;
    case EAST:
      here.x++;
      break;
    case SOUTH:
      here.y--;
      break;
    case WEST:
      here.x--;
      break;    
    }
  }
  
  add_path_segment('X', distance_between(prev, here));
}

// This function is called once, from main.c.
void map_maze()
{
  found_finish = recorded_finish = false;
  dir = NORTH;
  start = (pos){ MAZE_SIZE / 2, MAZE_SIZE / 2 };
  here = start;
  
  clear_map();
  
  set_digital_output(IO_D0, LOW);
  
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
      play_from_program_space(done_sound);
    }
    else if (!is_playing())
    {
      play_from_program_space(map_turn_sound);
    }
    
    // Intersection identification is complete.
    
    // stop motors to prevent too much untracked movement during the following computations (if they take too long)
    // might not be necessary...
    //set_motors(0, 0);
    
    // empirically determined: length = (ms - 65) / 709
    uint8_t seg_length = (((end_ms - start_ms) - 65) + 354) / 709;

    update_map(seg_length);
    lcd_goto_xy(0, 1);
    print_long(end_ms - start_ms);

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
      turn('B');
      break;
    }      
      
    turn(select_turn());
  }


  // Solved the maze!
  
  set_motors(0, 0);
  set_digital_input(IO_D0, PULL_UP_ENABLED);
  fill_all_costs();
  build_path();  
  display_path();
}

void run_maze_conservative()
{
  // Re-run the maze.  It's not necessary to identify the
  // intersections, so this loop is really simple.
  for(uint8_t i = 0; i < (path_length - 1); i++) // path ends with 'X'
  {
    if (path_seg_lengths[i] > 0)
    {
      follow_segment();

      // Drive straight while slowing down, as before.
      set_motors(50,50);
      delay_ms(50);
      set_motors(40,40);
      delay_ms(200);
    }      

    // Make a turn according to the instruction stored in
    // path[i].
    if (path[i] != 'S')
      play_from_program_space(run_turn_sound);
    turn(path[i]);
  }
    
  // Follow the last segment up to the finish.
  follow_segment();
  set_motors(40,40);
  delay_ms(200);
  set_motors(0, 0);
  play_from_program_space(done_sound);

  // Now we should be at the finish!
}

void turn_aggressive(char turn_dir)
{
  switch(turn_dir)
  {
  case 'L':
    // Turn left.
    set_motors(-20,130);
    delay_ms(200);
    break;
  case 'R':
    // Turn right.
    set_motors(130,-20);
    delay_ms(200);
    break;
  case 'B':
    // Turn around.
    set_motors(120,-120);
    delay_ms(300);
    break;
  case 'S':
    // Don't do anything!
    break;
  }    
}

void run_maze_aggressive()
{
  uint8_t straight_seg_length = 0, intersections_to_ignore = 0;
  
  for(uint8_t i = 0; i < (path_length - 1); i++) // path ends with 'X'
  {
    if (path_seg_lengths[i] > 0)
    {
      straight_seg_length += path_seg_lengths[i];
      
      if (path[i] == 'S')
      {
        intersections_to_ignore++;
        continue;
      }
      
      follow_segment_aggressive(straight_seg_length, intersections_to_ignore);
      
      straight_seg_length = 0;
      intersections_to_ignore = 0;
    }      

    // Make a turn according to the instruction stored in
    // path[i].
    play_from_program_space(run_turn_sound);
    turn_aggressive(path[i]);
  }
    
  // Follow the last segment up to the finish.
  follow_segment_aggressive(MAZE_SIZE, intersections_to_ignore); // don't bother slowing down in anticipation
  set_motors(0, 0);
  play_from_program_space(done_sound);
}