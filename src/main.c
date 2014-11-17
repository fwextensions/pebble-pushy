/*
	- don't display the pusher digit if seconds != 00
	- maybe have a gradient overlay on top and bottom
	- show date somehow, maybe on tap
	- clean up digit bitmaps, especially 4
	- move 1 back into center of bitmap
	- combine pusher and pushee into one container and just animate that
		container
*/


#include <pebble.h>


#define ImageCount 11
#define ColonImageIndex 10
#define ColonLayerIndex 4
#define TimeCharCount 5
#define DigitCount 4
#define DigitY 65
#define DigitW 30
#define DigitH 38
#define LayerContainerW 144
#define ContainerSpacing 7
#define PusherStartY DigitH + ContainerSpacing
#define PusheeFinishY DigitY - DigitH - ContainerSpacing
#define MovingLayerH 2 * DigitH + ContainerSpacing
#define AnimationMS 60 * 1000

	// This works around the inability to use the current GRect macro
	// for constants.
#define ConstantGRect(x, y, w, h) {{(x), (y)}, {(w), (h)}}


const int ImageResourceIDs[ImageCount] = {
	RESOURCE_ID_Img_0,
	RESOURCE_ID_Img_1,
	RESOURCE_ID_Img_2,
	RESOURCE_ID_Img_3,
	RESOURCE_ID_Img_4,
	RESOURCE_ID_Img_5,
	RESOURCE_ID_Img_6,
	RESOURCE_ID_Img_7,
	RESOURCE_ID_Img_8,
	RESOURCE_ID_Img_9,
	RESOURCE_ID_Img_Colon
};
const int TimeCharX[TimeCharCount] = {
	4,
	36,
	78,
	110,
	68
};
const int TimeCharY[TimeCharCount] = {
	DigitY,
	DigitY,
	DigitY,
	DigitY,
	72
};
const int TimeCharW[TimeCharCount] = {
	DigitW,
	DigitW,
	DigitW,
	DigitW,
	8
};
const int TimeCharH[TimeCharCount] = {
	DigitH,
	DigitH,
	DigitH,
	DigitH,
	25
};


GRect StaticFrame = ConstantGRect(0, DigitY, LayerContainerW, DigitH);
GRect PusheeFrame = ConstantGRect(0, 0, LayerContainerW, DigitH);
GRect PusherFrame = ConstantGRect(0, PusherStartY, LayerContainerW, DigitH);
GRect PusherStartFrame = ConstantGRect(0, DigitY, LayerContainerW, MovingLayerH);
GRect PusherEndFrame = ConstantGRect(0, PusheeFinishY, LayerContainerW, MovingLayerH);


typedef struct {
	Layer* layer;
	BitmapLayer* digits[DigitCount];
} Container;


static Window* gMainWindow;
static GBitmap* gImages[ImageCount];
static BitmapLayer* gColonLayer;
static Layer* gMovingLayer;
static Container gPusherContainer;
static Container gPusheeContainer;
static Container gStaticContainer;
static PropertyAnimation* gPusherAnimation;


static BitmapLayer* createBitmapLayer(
	int inX,
	int inY,
	int inWidth,
	int inHeight,
	Layer* inParent)
{
	GRect frame = (GRect) {
		.origin = {
			inX,
			inY
		},
		.size = {
			inWidth,
			inHeight
		}
	};

		// create the layer and add it to the window
	BitmapLayer* layer = bitmap_layer_create(frame);
	layer_add_child(inParent, bitmap_layer_get_layer(layer));
	bitmap_layer_set_background_color(bitmap_layer_get_layer(layer), GColorWhite);

	return layer;
}


static void createContainer(
	Container* ioContainer,
	GRect* inFrame,
	Layer* inParent)
{
	GRect frame = (GRect) {
		.origin = {
			0,
			DigitY
		},
		.size = {
			LayerContainerW,
			DigitH
		}
	};

	ioContainer->layer = layer_create(*inFrame);

	if (!inParent) {
		inParent = window_get_root_layer(gMainWindow);
	}

	layer_add_child(inParent, ioContainer->layer);

	for (int i = 0; i < DigitCount; i++) {
			// all the digits should be aligned at the top of the container, so
			// don't use TimeCharY, just 0
		ioContainer->digits[i] = createBitmapLayer(TimeCharX[i], 0,
			TimeCharW[i], TimeCharH[i], ioContainer->layer);
	}
}


static void destroyContainer(
	Container* inContainer)
{
	layer_remove_from_parent(inContainer->layer);

	for (int i = 0; i < DigitCount; i++) {
		layer_remove_from_parent(bitmap_layer_get_layer(inContainer->digits[i]));
		bitmap_layer_destroy(inContainer->digits[i]);
		inContainer->digits[i] = NULL;
	}
}


static void setContainerDigit(
	Container* inContainer,
	int slotIndex,
	int digit)
{
	GBitmap* image = NULL;

	if ((slotIndex < 0) || (slotIndex >= DigitCount)) {
		return;
	}

	if (digit >= 0 && digit <= 9) {
		image = gImages[digit];
	}

		// load the image into the layer
	bitmap_layer_set_bitmap(inContainer->digits[slotIndex], image);
}


static void setDigit(
	int inSlotIndex,
	int inDigit1,
	int inDigit2,
	int inDigit3)
{
	setContainerDigit(&gPusheeContainer, inSlotIndex, inDigit1);
	setContainerDigit(&gPusherContainer, inSlotIndex, inDigit2);
	setContainerDigit(&gStaticContainer, inSlotIndex, inDigit3);
}


static unsigned short getDisplayHour(
	unsigned short hour)
{
	if (clock_is_24h_style()) {
		return hour;
	}

	unsigned short displayHour = hour % 12;

	// Converts "0" to "12"
	return displayHour ? displayHour : 12;
}


static void displayTime(
	struct tm *tickTime,
	bool inInit)
{
	unsigned short minutes = tickTime->tm_min;
	unsigned short hours = getDisplayHour(tickTime->tm_hour);
	unsigned short onesMinutes = minutes % 10;
	unsigned short tensMinutes = minutes / 10;
	unsigned short onesHours = hours % 10;
	unsigned short tensHours = hours / 10;

	unsigned short pusherMinutes = (minutes + 1) % 60;
	unsigned short pusherHours = getDisplayHour(hours + 1);
	unsigned short pusherOnesMinutes = pusherMinutes % 10;
	unsigned short pusherTensMinutes = pusherMinutes / 10 ;
	unsigned short pusherOnesHours = pusherHours % 10;
	unsigned short pusherTensHours = pusherHours / 10;

	setContainerDigit(&gPusheeContainer, 3, onesMinutes);
	setContainerDigit(&gPusherContainer, 3, pusherOnesMinutes);

	setDigit(2, -1, -1, tensMinutes);
	setDigit(1, -1, -1, onesHours);
	setDigit(0, -1, -1, tensHours == 0 ? -1 : tensHours);

	if (pusherOnesMinutes == 0) {
		setDigit(2, tensMinutes, pusherTensMinutes, -1);

		if (pusherTensMinutes == 0) {
			setDigit(1, onesHours, pusherOnesHours, -1);

			if (pusherOnesHours == 0) {
				setDigit(0, -1, pusherTensHours, -1);
			} else if (pusherTensHours == 0 && tensHours == 1) {
					// when flipping from 12:59 to 1:00, we want to push the 1
					// off with a blank space in the pusher
				setDigit(0, tensHours, -1, -1);
			}
		}
	}

	if (!inInit) {
		animation_schedule((Animation*) gPusherAnimation);
	}
}


static void onTick(
	struct tm *tickTime,
	TimeUnits unitsChanged)
{
	displayTime(tickTime, false);
}


void initAnimations(void)
{
	gPusherAnimation = property_animation_create_layer_frame(gMovingLayer,
		&PusherStartFrame, &PusherEndFrame);
	animation_set_duration((Animation*) gPusherAnimation, AnimationMS);
	animation_set_curve((Animation*) gPusherAnimation, AnimationCurveLinear);
}


void init(void)
{
	gMainWindow = window_create();
	window_stack_push(gMainWindow, true);
	window_set_background_color(gMainWindow, GColorBlack);

		// preload all the images
	for (int i = 0; i < ImageCount; i++) {
		gImages[i] = gbitmap_create_with_resource(ImageResourceIDs[i]);
	}

	GRect frame = (GRect) {
		.origin = {
			0,
			DigitY
		},
		.size = {
			LayerContainerW,
			DigitH
		}
	};

	gMovingLayer = layer_create(frame);
	createContainer(&gPusheeContainer, &GRect(0, 0, LayerContainerW, DigitH), gMovingLayer);
	createContainer(&gPusherContainer, &GRect(0, PusherStartY, LayerContainerW, DigitH), gMovingLayer);
	createContainer(&gStaticContainer, &frame, NULL);

	initAnimations();

		// load the colon image
	gColonLayer = createBitmapLayer(TimeCharX[ColonLayerIndex],
		TimeCharY[ColonLayerIndex], TimeCharW[ColonLayerIndex], TimeCharH[ColonLayerIndex],
		window_get_root_layer(gMainWindow));
	bitmap_layer_set_bitmap(gColonLayer, gImages[ColonImageIndex]);

		// Avoids a blank screen on watch start.
	time_t now = time(NULL);
	struct tm *tickTime = localtime(&now);
	displayTime(tickTime, true);

	tick_timer_service_subscribe(MINUTE_UNIT, onTick);
}


void deinit(void)
{
	layer_remove_from_parent(bitmap_layer_get_layer(gColonLayer));
	bitmap_layer_destroy(gColonLayer);

	destroyContainer(&gStaticContainer);
	destroyContainer(&gPusherContainer);
	destroyContainer(&gPusheeContainer);
	layer_remove_from_parent(gMovingLayer);

	property_animation_destroy(gPusherAnimation);

	for (int i = 0; i < ImageCount; i++) {
		gbitmap_destroy(gImages[i]);
	}

	tick_timer_service_unsubscribe();
	window_destroy(gMainWindow);
}


int main(void)
{
	init();
	app_event_loop();
	deinit();
}
