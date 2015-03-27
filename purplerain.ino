// This is a project I built for the Maplin Arduino Day competition 2015
// It uses an Adafruit Trinket and a Pimoroni UnicornHAT (Yes, I know that's a Pi device
// but there are 3 blank headers that allow you to drive the neopixels direct)
// it also uses a dual gang linear potentiometer (a sliding one), but a rotory one will
// work just as well
//
// By Carl Monk (@ForToffee)

#include <Adafruit_NeoPixel.h>

#define PIN 1
#define PIN_SLIDER 1    // select the Analog input pin for the potentiometer
#define PIN_BUTTON 0

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(64, PIN, NEO_GRB + NEO_KHZ800);

// variables
int game_state = 0;  // 0=idle, 1=running, 2=end
int game_time = 100;	// time bewteen each board/sprite movement will be multipled by 10 due to delay
int game_tick = 0;	// number of iterations round loop(), should be <= game_time 
int game_line = 1;	// location of the player	(y value)
int game_cycles = 0;// number of times the board/sprites has moved
int game_artifacts = 1;	// number of new sprites to generate at one time 
int player_pos = -1;	// location of player (x value)
int board[8];	// array to hold binary representation of sprite locations

void setup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);	// setup pin to read potentiometer
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(int(255 * 0.2));	// brightness is 0 - 255
}

void loop() {
    switch (game_state) {
    case 0:
      idle_screen();
      break;
    case 1:    
      if (check_position())  {	// return true if player has hit sprite
        game_state = 2;
        break;
      }
      if (game_tick >= game_time) {  // timeout reached to move board/sprites
        game_tick = 0;
        game_cycles++;
        switch (game_cycles % 32)  {  // check level up condition
        case 0:		// 4 cycles of the board
          if (game_artifacts < 5)  {
            game_artifacts++;		//increase number of new sprites generated 
          }
          break;
        
        case 16:	// 4 cycles of the board
          if (game_time > 10)  {
            game_time -= 10;	//reduce time between board movements (i.e. speed up) 
          }
          break;
        }
        if (move_sprites())   {	// return true if player has hit sprite
          game_state = 2;
          break;
        }
        draw_board();
      }
      game_tick++;
      break;
    case 2:    
      game_over();
      game_state = 0;
      break;
    }
  delay(10);
}

// generates a random rainbow effect
void idle_screen()  {
  clear_pixels(true);
  randomSeed(analogRead(3));
  while (digitalRead(PIN_BUTTON) == HIGH)  {
    show_pixel(random(0,8),random(0,8),random(0,256),random(0,256),random(0,256));
  }
  init_game();
}

// sets all game vars, clear "screen"
void init_game()  {
  clear_pixels(true);
  game_state = 1;
  game_cycles = 0;
  game_time = 100;
  game_artifacts = 1;
  for (int i=0;i<8;i++)  {
    board[i] = 0;
  }
}

// check if pixel is set by x,y co-ords
boolean hit_check(int line_to_test, int pos) {
  int mask = 1 << (pos);
  return (board[line_to_test] & mask);
}

// if board movement will "hit" player - return true
// move board in memory
// generate new row of sprites
boolean move_sprites()  {
    if (hit_check(game_line + 1, player_pos))  {
      return true;
    }
    else  {
      //move board down one
      for (int y=0;y<7;y++){
        int var = board[y+1];
        board[y]= var;
      }
      
      board[7] = 0;
      int val = 0;
      for (int i=0;i<game_artifacts;i++)  {
        while (true)  {
          val = 1 << random(0,8);
          if (!(board[7] & val))  {
            board[7] += val;
            break;
          }
        }
      }
    }
    return false;
}

// render board
void draw_board()  {
  clear_pixels(false);
  for (int y=0;y<8;y++)  {
    for (int x=0; x<8; x++)  {
      if (hit_check(y,x))  {
        show_pixel(x,y,255,0,255,false);
      }
    }
  }
  show_player();
}

// render the player pixel
void show_player()  {
    show_pixel(player_pos,game_line,0,0,255);
}

// read potentiometer position
// if sprite collision -  return true
// redraw player pixel
boolean check_position()  {
  int x = map(analogRead(PIN_SLIDER), 0, 1023, 0, 8);

  if (player_pos != x) {
    if (player_pos >= 0) {
      show_pixel(player_pos,game_line,0,0,0);
    }
    player_pos = x;
    show_player();
  }
  return hit_check(game_line,x);
}

// game over animation
void game_over()  {
  for (int i=0; i<10; i++)  {
    show_pixel(player_pos,game_line,random(0,256),random(0,256),random(0,256));
    delay(500);
  }
}

// clear the screen
void clear_pixels(boolean show){
  for (int pixel = 0; pixel < 65; pixel++){
    strip.setPixelColor(pixel, 0,0,0);
  }
  if (show)  {
    strip.show();
  }
}

// manage drawing pixels on screen
void show_pixel(int x, int y, int r, int g, int b)  {
  show_pixel(x,y,r,g,b,true);
}

// on Pimoroni UnicornHAT pixels are in l-r/r-l ordering i.e.
//
// .. .. .. .. .. .. .. ..
// 16 17 18 19 20 21 22 23
// 15 14 13 12 11 10 09 08
// 00 01 02 03 04 05 06 07 

void show_pixel(int x, int y, int r, int g, int b, boolean show)  {
  if (y % 2 == 1){
    x = 7 - x;    //pixels in l-r/r-l order
  }
  int pixel = (y * 8) +  x;
  strip.setPixelColor(pixel, r,g,b);
  if (show)  {
    strip.show();
  }
}
