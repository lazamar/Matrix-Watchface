#include <pebble.h>

#define MAX_GAP 28
#define MIN_GAP 7
#define MAX_LINE 20
#define MIN_LINE 5
#define X_TOTAL 18
#define Y_TOTAL 12
#define FRAME_RATE 40
#define ANIMATION_FRAMES_TOTAL 80
#define XYTOTAL 220
#define DIGIT_H 5
#define DIGIT_W 5
  
static Window * main_window;
static Layer * main_layer;
static TextLayer * matrix_txt, *matrix_highlight_txt, *time_txt;
static char render_buffer[XYTOTAL], 
      characters_buffer[XYTOTAL],
      render_highlight_buffer[XYTOTAL],
      time_buffer[9];

static int gap[X_TOTAL]; // Expresses the number of iterations till the next vertical line of characters 
static int line[X_TOTAL]; // Expresses the number of iteration till the next gap
static int highlight_coordinates[X_TOTAL * 3];
static const uint16_t frame_time = 1000 / FRAME_RATE;
static GFont consolas_font, consolas_big_font;
static int frame_count = 0;
// 
// In the algorythm 0 means empty and 1 means random letter
// 
// 

static char randomLetter(){
  char charset[] = "QWERTqwertyuiopasdfghjklzxcvb!#$&+<>?";
  uint8_t key = rand() % (sizeof(charset) - 1);
  char letter = charset[key];
  return letter;
}


static void initialise_buffers(){
  // Iterates through the array until the one but last column
  for(int i=0; i < X_TOTAL - 1; i++){
    for(int j=0; j < Y_TOTAL; j++){
          characters_buffer[(j*X_TOTAL) + i] = randomLetter();
          render_buffer[(j*X_TOTAL) + i] = *(" ");
          render_highlight_buffer[(j*X_TOTAL) + i] = *(" ");
    }
  }

  // Fills last column with \n
  for (int j =0; j < Y_TOTAL; j++){
    render_highlight_buffer[j*(X_TOTAL) + (X_TOTAL - 1)] = *("\n");
    render_buffer[j*(X_TOTAL) + (X_TOTAL - 1)] = *("\n");
    characters_buffer[j*(X_TOTAL) + (X_TOTAL - 1)] = *("\n");
  }

  for(int i = 0; i < X_TOTAL; i++){
    gap[i] = (rand() % (MAX_GAP - MIN_GAP + 1) + MIN_GAP); // This will give a number between MAX_GAP and MIN_GAP
    line[i] = (rand() % (MAX_LINE - MIN_LINE + 1) + MIN_LINE);
  }

  for(int i=0; i < (X_TOTAL * 3); i++ ){
    highlight_coordinates[i] = -1;
  }
}


// Takes care of the loop asking main_layer to update every frame
static void do_matrix_effect(){
  
  uint16_t prev_time, after_time;
  uint8_t difference;

  prev_time = time_ms(NULL,NULL);

  //  From last horizontal line to second line
  //  Move everything in prep_buffer one line down
  for(int i= X_TOTAL - 2; i >= 0; i--){
    for(int j = Y_TOTAL - 2; j >= 0; j--){
      if(render_buffer[(j*X_TOTAL) + i] != *(" ")){
        render_buffer[((j+1)*X_TOTAL) + i] = characters_buffer[((j+1)*X_TOTAL) + i];
      }else{
        render_buffer[((j+1)*X_TOTAL) + i] = *(" ");
      }
    }
  }
  //  Move everything in highlight_buffer one line down
  for(int i=0; i < (X_TOTAL*3); i++){
    if(highlight_coordinates[i] >= (Y_TOTAL - 1)* X_TOTAL){
      render_highlight_buffer[highlight_coordinates[i]] = *(" ");
      highlight_coordinates[i] = -1;
    }else if(highlight_coordinates[i] >= 0){
      render_highlight_buffer[highlight_coordinates[i]] = *(" ");
      highlight_coordinates[i] = highlight_coordinates[i] + X_TOTAL;
      render_highlight_buffer[highlight_coordinates[i]] = characters_buffer[highlight_coordinates[i]];
    }
  }
  
    // Fill first horizontal line
  for(int i = 0; i < X_TOTAL - 1; i++){
    
    if(line[i] > 0){
      line[i] = line[i] -1;
      render_buffer[i] = characters_buffer[i];
    }else{
      if(gap[i] > 0){
        render_buffer[i] = *(" ");
        gap[i] = gap[i] -1; 
      }else{
        line[i] = (rand() % (MAX_LINE - MIN_LINE + 1)) + MIN_LINE;
        gap[i] = (rand() % (MAX_GAP - MIN_GAP + 1)) + MIN_GAP; // This will give a number between MAX_GAP and MIN_GAP
        render_buffer[i] = *(" ");
        int k = 0;
        
        // Find empty space in highlight_coordinates
        for(int j = 0; j < (X_TOTAL * 3); j++){
          if(highlight_coordinates[j]==-1){          
            k = j;
            break;
          }  
        }
        highlight_coordinates[k] = i;
        render_highlight_buffer[i] = characters_buffer[i];      
      }     
    }
  }
  
  
  text_layer_set_text(matrix_txt, render_buffer); 
  text_layer_set_text(matrix_highlight_txt, render_highlight_buffer);
    APP_LOG(APP_LOG_LEVEL_INFO, "gone through Matrix");

  after_time = time_ms(NULL,NULL);

  difference = after_time - prev_time;

  if(frame_count < ANIMATION_FRAMES_TOTAL){
    frame_count++;
    if( difference < frame_time){
      uint16_t wait_time = frame_time - difference; 
        app_timer_register(wait_time, do_matrix_effect ,NULL);  
    }
    else{
      do_matrix_effect();
    } 
  }else{
    frame_count = 0;
  }
  
}

static void update_time(struct tm *tick_time, TimeUnits units_changed){
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);
  snprintf(time_buffer, 7, "%02d\n%02d", t->tm_hour, t->tm_min);
  text_layer_set_text(time_txt, time_buffer);
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  
  switch (axis) {
  case ACCEL_AXIS_X:
    if (direction > 0) {
      APP_LOG(APP_LOG_LEVEL_INFO, "X axis positive.");
    } else {
      APP_LOG(APP_LOG_LEVEL_INFO, "X axis negative.");
    }
    break;
  case ACCEL_AXIS_Y:
    // Call all animations when flickering the wrist inwards.
    light_enable_interaction();
    do_matrix_effect();  
    break;
  case ACCEL_AXIS_Z:
    if (direction > 0) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Z axis positive.");
    } else {
      APP_LOG(APP_LOG_LEVEL_INFO, "Z axis negative.");
    }
    break;
  }
}

static void main_window_load(){
  
  initialise_buffers();
  consolas_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CONSOLAS_14));
  consolas_big_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CONSOLAS_49));
  
  // Create main_layer
  GRect bounds = layer_get_bounds(window_get_root_layer(main_window));
  main_layer = layer_create(bounds);
  layer_add_child(window_get_root_layer(main_window), main_layer);

  // Create matrix_txt
  matrix_txt = text_layer_create(bounds);
  text_layer_set_background_color(matrix_txt, GColorBlack);
  #ifdef PBL_COLOR
    text_layer_set_text_color(matrix_txt, GColorGreen);
  #else
    text_layer_set_text_color(matrix_txt, GColorWhite);
  #endif
  
  text_layer_set_font(matrix_txt, consolas_font);
  layer_add_child(main_layer,text_layer_get_layer(matrix_txt));
  //text_layer_set_text(matrix_txt, render_buffer);

    // Crete matrix_highlight_txt
  matrix_highlight_txt = text_layer_create(bounds);
  text_layer_set_background_color(matrix_highlight_txt, GColorClear);
  #ifdef PBL_COLOR
    text_layer_set_text_color(matrix_highlight_txt, GColorWhite);
  #else
    text_layer_set_text_color(matrix_highlight_txt, GColorWhite);
  #endif
  
  text_layer_set_font(matrix_highlight_txt, consolas_font);
  layer_add_child(main_layer,text_layer_get_layer(matrix_highlight_txt));
  
  time_txt = text_layer_create(GRect(0,40,148,100));
  text_layer_set_background_color(time_txt, GColorClear);
  #ifdef PBL_COLOR
    text_layer_set_text_color(time_txt, GColorGreen);
    text_layer_set_font(time_txt, fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS));
  #else
    text_layer_set_text_color(time_txt, GColorWhite);
    text_layer_set_font(time_txt, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  #endif
  
  
  text_layer_set_text_alignment(time_txt, GTextAlignmentCenter);
  layer_add_child(main_layer,text_layer_get_layer(time_txt));

  do_matrix_effect();
  tick_timer_service_subscribe(MINUTE_UNIT,  update_time);
}

static void main_window_unload(){
  layer_destroy(main_layer);
  text_layer_destroy(matrix_txt);
  text_layer_destroy(time_txt);
  text_layer_destroy(matrix_highlight_txt);
}

static void init(){
  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });

  //  Subscribe to tap service ( will call the callback when the user flicker his wrist)
  accel_tap_service_subscribe(tap_handler);

  window_stack_push(main_window, true);
}

static void deinit(){
  window_destroy(main_window);
}

int main(void){
  init();
  app_event_loop();
  deinit();
}