#include <pebble.h>
Window *my_window;
static Layer *main_clock;
static char date_buffer[] = "Thu 12";
static char time_buffer[] = "23:59";
static char doy_buffer[] = "366";
static TextLayer *time_layer, *date_layer, *doy_layer;
static bool first_run = true;

static int hour; // The current hour
static int min; // The current minute

// Layout boundaries
#define DAY_RING PBL_IF_ROUND_ELSE(9, 5)
#define MIDNIGHT_INSET PBL_IF_ROUND_ELSE(17, 12)

static bool is_leap_year(int year) {
  if ( year%400 == 0)
    return true;
  else if ( year%100 == 0)
    return false;
  else if ( year%4 == 0 )
    return true;
  else
    return false;
 
  return false;
}

static bool is_night(int hour) {
  // Night begins at 6PM and ends at 5AM
  if (hour >= 18 || hour < 5) {
    return true;
  }
  return false;
}

static void set_dark () {
  window_set_background_color(my_window, GColorBlack);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_text_color(doy_layer, GColorWhite);
  text_layer_set_text_color(date_layer, GColorWhite);
}

static void set_light () {
  window_set_background_color(my_window, GColorWhite);
  text_layer_set_text_color(time_layer, GColorBlack);
  text_layer_set_text_color(doy_layer, GColorBlack);
  text_layer_set_text_color(date_layer, GColorBlack);
}



static void draw_main_clock(Layer *layer, GContext *ctx) {
  
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);

  GRect bounds = layer_get_bounds(layer);
  GRect dr = grect_inset(bounds, GEdgeInsets(DAY_RING));
  GRect mi = grect_inset(bounds, GEdgeInsets(MIDNIGHT_INSET));

  
  // Midnight Marker
  int32_t end1 = 0;
  
  if (is_night(t->tm_hour)) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
  } else {
    graphics_context_set_stroke_color(ctx, GColorBlack);
  }
	graphics_context_set_stroke_width(ctx, 1);
  GPoint p0 = gpoint_from_polar(bounds, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(end1));
	GPoint p1 = gpoint_from_polar(mi, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(end1));
  graphics_draw_line(ctx, p0, p1);


  // Day Ring
  int32_t hour_angle = (TRIG_MAX_ANGLE * ((hour * 60) + min)/1440);
  int32_t start = 0;

	graphics_context_set_stroke_width(ctx, 
                                   PBL_IF_ROUND_ELSE(12, 11));
  if (is_night(t->tm_hour)) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorWhite);
  } else {
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_fill_color(ctx, GColorBlack);
  }
  
  graphics_fill_radial(ctx,  dr,  GOvalScaleModeFitCircle, 9, start, hour_angle);
  
}

static void tick_handler(struct tm *t, TimeUnits changed) {
  
  if ((MINUTE_UNIT & changed) || first_run) {
    if (is_night(t->tm_hour)) {
      set_dark();
    } else {
      set_light();
    }
    strftime(time_buffer, sizeof(time_buffer), 
       clock_is_24h_style() ?"%H:%M" : "%I:%M", t);
    text_layer_set_text(time_layer, time_buffer);
    hour = t->tm_hour;
    min = t->tm_min;
    layer_mark_dirty(main_clock);
  }
  
  if ((DAY_UNIT & changed) || first_run) {
    int max;
    if (is_leap_year(t->tm_year)) {
      max = 366;
    } else {
      max = 365;
    }
    int diff = max - (t->tm_yday + 1);
    snprintf(doy_buffer, sizeof(doy_buffer), "%d", diff);
    strftime(date_buffer, sizeof(date_buffer), "%a %d", t); 
    text_layer_set_text(date_layer, date_buffer);
    text_layer_set_text(doy_layer, doy_buffer);
    if (first_run) {
      first_run = false;
    }
  }
}





static void main_window_load() {

  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);
  
  if (is_night(t->tm_hour)) {
    window_set_background_color(my_window, GColorBlack);
  } else {
    window_set_background_color(my_window, GColorWhite);
  }

  Layer *window_layer = window_get_root_layer(my_window);
  GRect bounds = layer_get_bounds(window_layer);

  main_clock = layer_create(bounds);
  layer_set_update_proc(main_clock, draw_main_clock);  
  layer_add_child(window_layer, main_clock); 

  
  time_layer = text_layer_create(
    GRect(PBL_IF_ROUND_ELSE(1,0), 
          PBL_IF_ROUND_ELSE(bounds.size.h/2 - 28, bounds.size.h/2 - 20), 
          bounds.size.w, 54));
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_font(time_layer, PBL_IF_ROUND_ELSE(
    fonts_load_custom_font(resource_get_handle(RESOURCE_ID_TIME_FONT_45)),
    fonts_load_custom_font(resource_get_handle(RESOURCE_ID_TIME_FONT_32))
    ));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_layer)); 
  
  doy_layer = text_layer_create(
    GRect(0, 
          PBL_IF_ROUND_ELSE(bounds.size.h/2 + 24, bounds.size.h/2 + 17), 
          bounds.size.w, 40));
  text_layer_set_background_color(doy_layer, GColorClear);
  text_layer_set_font(doy_layer, fonts_get_system_font(
    PBL_IF_ROUND_ELSE(FONT_KEY_GOTHIC_24,FONT_KEY_GOTHIC_18)));
  text_layer_set_text_alignment(doy_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(doy_layer)); 
  
  date_layer = text_layer_create(
    GRect(0, 
          PBL_IF_ROUND_ELSE(bounds.size.h/2 - 47, bounds.size.h/2 - 38),
          bounds.size.w, 40));
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  text_layer_set_font(date_layer, PBL_IF_ROUND_ELSE(
    fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DATE_FONT_18)),
    fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DATE_FONT_14))
    ));
  layer_add_child(window_layer, text_layer_get_layer(date_layer)); 

  tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT, tick_handler);
}

static void main_window_unload() {
	layer_destroy(main_clock);
  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);
}

void handle_init(void) {
  my_window = window_create();
  window_set_window_handlers(my_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  
  window_stack_push(my_window, true);
  
}

void handle_deinit(void) {
  window_destroy(my_window);
  tick_timer_service_unsubscribe();
}


int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
