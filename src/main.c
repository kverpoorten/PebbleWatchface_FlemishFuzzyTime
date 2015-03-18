#include <pebble.h>
#include <pebble_fonts.h>

// If this is defined, the face will use minutes and seconds instead of hours and minutes
// to make debugging faster.
//#define DEBUG_FAST
#define CUSTOM_FONTS

static Window *window;
static GFont font_small;
static GFont font_big;
static InverterLayer *inverter;
static InverterLayer *btConnectionInverter;
static PropertyAnimation *inverter_anim;

typedef struct {
  GRect frame;
	TextLayer * layer;
	PropertyAnimation *anim;
	const char * text;
	const char * old_text;
} word_t;

static word_t first_word;
static word_t first_word_between;
static word_t second_word;
static word_t second_word_between;
static word_t third_word;

static const char *hours[] = {"twaalf","een","twee","drie","vier","vijf","zes","zeven","acht","negen","tien","elf","twaalf"};

TextLayer *text_layer_setup(Window * window, GRect frame, GFont font, GTextAlignment alignment) {
	TextLayer *layer = text_layer_create(frame);
	text_layer_set_text(layer, "");
	text_layer_set_text_color(layer, GColorWhite);
	text_layer_set_background_color(layer, GColorClear);
	text_layer_set_text_alignment(layer, alignment);
	text_layer_set_font(layer, font);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(layer));
	return layer;
}

static void recreate_word_animation(word_t * word) {
  if (word->anim != NULL) {
    property_animation_destroy(word->anim);    // destroy the animation if it already exists
  }
  
  // and recreate it
  GRect frame_right = word->frame;
	frame_right.origin.x = 150;
  
  word->anim = property_animation_create_layer_frame(text_layer_get_layer(word->layer), &frame_right, &word->frame);
	animation_set_duration(&word->anim->animation, 500);
	animation_set_curve(&word->anim->animation, AnimationCurveEaseIn);
}

static void recreate_inverter_layer_animation(GRect *from, GRect *to) {
  if (inverter_anim != NULL) {
    property_animation_destroy(inverter_anim);    // destroy the animation if it already exists
  }
  
  inverter_anim = property_animation_create_layer_frame(inverter_layer_get_layer(inverter), from, to);
	animation_set_duration(&inverter_anim->animation, 500);
	animation_set_curve(&inverter_anim->animation, AnimationCurveEaseIn);
}

static const char *hour_string(int h) {
	if(h > 11) {
		h -= 12;
	}
	return hours[h];
}
static const char *min_string(int m) {
	if(m == 5) {
		return "vijf";
	} else {
		return "tien";
	}
}

static void update_word(word_t * const word) {
	if(word->text != word->old_text) {
    // recreate the wordt animation, otherwise we cannot run it more than once
    recreate_word_animation(word);
    
		// Move the layer offscreen before changing text.
		// Without this, the new text will be visible briefly before the animation starts.
    layer_set_frame(text_layer_get_layer(word->layer), word->anim->values.from.grect);
    animation_schedule(&word->anim->animation);
	}
	text_layer_set_text(word->layer, word->text);
}

static void nederlands_format(int h, int m) {
	first_word.text = "";
	first_word_between.text = "";
	second_word.text = "";
	second_word_between.text = "";
	third_word.text = "";
  
	int unrounded = m;
	float temp_m = m;
	temp_m = temp_m / 5;
	m = temp_m + 0.5;
	m = m * 5;

	if(m == 0 || m == 60) {
		if(m == 60) {
			h++;
		}
		first_word_between.text = hour_string(h);
		second_word_between.text = "uur";
	} else if(m == 30) {
		first_word_between.text = "half";
		second_word_between.text = hour_string(h + 1);
	} else if(m == 15) {
		first_word.text = "kwart";
		second_word.text = "na";
		third_word.text = hour_string(h);
  } else if(m == 20) {
		first_word.text = "twintig";
		second_word.text = "na";
		third_word.text = hour_string(h);
  } else if(m == 40) {
		first_word.text = "twintig";
		second_word.text = "voor";
		third_word.text = hour_string(h + 1);
	} else if(m == 45) {
		first_word.text = "kwart";
		second_word.text = "voor";
		third_word.text = hour_string(h + 1);
	} else if(m > 30) {
		if(m < 45) {
			first_word.text = min_string(m - 30);
      second_word.text = "na half";
			third_word.text = hour_string(h + 1);
		} else {
			first_word.text = min_string(60 - m);
			second_word.text = "voor";
			third_word.text = hour_string(h + 1);
		}
	} else {
		if(m > 15) {
			first_word.text = min_string(30 - m);
      second_word.text = "voor half";
			third_word.text = hour_string(h + 1);
		} else {
			first_word.text = min_string(m);
			second_word.text = "na";
			third_word.text = hour_string(h);
		}
	}

	GRect frame;
	GRect frame_right;
	switch(unrounded - m) {
		case -2:
		frame = GRect(116, 167, 28, 1);
		frame_right = frame;
		frame_right.origin.x = 0;
		break;
		case -1:
		frame = GRect(0, 167, 28, 1);
		frame_right = frame;
		frame_right.origin.x = 28;
		break;
		case 0:
		frame = GRect(28, 167, 28, 1);
		frame_right = frame;
		frame_right.origin.x = 56;
		frame_right.size.w = 32;
		break;
		case 1:
		frame = GRect(56, 167, 32, 1);
		frame_right = frame;
		frame_right.origin.x = 84;
		frame_right.size.w = 28;
		break;
		case 2:
		frame = GRect(84, 167, 28, 1);
		frame_right = frame;
		frame_right.origin.x = 116;
		break;
	}
	
  recreate_inverter_layer_animation(&frame, &frame_right);
	animation_schedule(&inverter_anim->animation);
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
	first_word.old_text = first_word.text;
	first_word_between.old_text = first_word_between.text;
	second_word.old_text = second_word.text;
	second_word_between.old_text = second_word_between.text;
	third_word.old_text = third_word.text;

#ifdef DEBUG_FAST
	nederlands_format(tick_time->tm_min % 24,  tick_time->tm_sec);
#else
	nederlands_format(tick_time->tm_hour,  tick_time->tm_min);
#endif

	update_word(&first_word);
	update_word(&first_word_between);
	update_word(&second_word);
	update_word(&second_word_between);
	update_word(&third_word);
}

static void handle_bluetooth_connection(bool connected) {
  //show inverted when BT not connected
  layer_set_hidden(inverter_layer_get_layer(btConnectionInverter), connected);
  
  if (!connected) {
    vibes_double_pulse();
  }
}

static void text_layer(word_t * word, GRect frame, GFont font, GTextAlignment alignment) {
	word->layer = text_layer_setup(window, frame, font, alignment);
  word->frame = frame;
}

static void word_destroy(word_t * word) {
	property_animation_destroy(word->anim);
	text_layer_destroy(word->layer);
}

static void init() {

	window = window_create();
	window_stack_push(window, false);
	window_set_background_color(window, GColorBlack);

#ifdef CUSTOM_FONTS
	font_big = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_44));
	font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_30));
#else
  font_big = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
	font_small = fonts_get_system_font(FONT_KEY_GOTHIC_28);
#endif

	text_layer(&first_word, GRect(0, 10, 143, 51), font_big, GTextAlignmentCenter);
	text_layer(&second_word, GRect(0, 62, 143, 42), font_small, GTextAlignmentCenter);
	text_layer(&third_word, GRect(0, 94, 143, 51), font_big, GTextAlignmentCenter);
  
	text_layer(&first_word_between, GRect(0, 25, 143, 51), font_big, GTextAlignmentCenter);
	text_layer(&second_word_between, GRect(0, 81, 143, 51), font_big, GTextAlignmentCenter);

	inverter = inverter_layer_create(GRect(0, 166, 36, 1));
	layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(inverter));

#ifdef DEBUG_FAST
	tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
#else
	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
#endif
  
  //create inverted layer to display BT status
  btConnectionInverter = inverter_layer_create(GRect(0, 0, 144, 168));
  layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(btConnectionInverter));
  
  //also subsctibe to bluetooth status
  bluetooth_connection_service_subscribe(handle_bluetooth_connection);
  
  //intialize to correct bluetooth status at init?
  handle_bluetooth_connection(bluetooth_connection_service_peek());
}

static void deinit() {
	tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
	property_animation_destroy(inverter_anim);
	inverter_layer_destroy(inverter);
	word_destroy(&second_word_between);
	word_destroy(&first_word_between);
  word_destroy(&third_word);
	word_destroy(&second_word);
	word_destroy(&first_word);
#ifdef CUSTOM_FONTS
	fonts_unload_custom_font(font_small);
	fonts_unload_custom_font(font_big);
#endif
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
