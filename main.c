#include <pebble.h>


#define ImageCount 11
#define ColonImageIndex 10
#define ColonLayerIndex 4
#define TimeCharCount 5
#define DigitCount 4
#define DigitLayerCount 5
#define EmptySlot -1
#define DigitY 65
#define DigitW 30
#define DigitH 38
#define LayerContainerW 144
#define ContainerSpacing 7
#define PusherStartY DigitY + DigitH + ContainerSpacing
#define PusheeFinishY DigitY - DigitH - ContainerSpacing
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


GRect PusherStartFrame = ConstantGRect(0, PusherStartY, LayerContainerW, DigitH);
GRect PusherEndFrame = ConstantGRect(0, DigitY, LayerContainerW, DigitH);
GRect PusheeEndFrame = ConstantGRect(0, PusheeFinishY, LayerContainerW, DigitH);


typedef struct {
	Layer* layer;
	BitmapLayer* digits[DigitCount];
} Container;


static Window* gMainWindow;
static GBitmap* gImages[ImageCount];
static BitmapLayer* gColonLayer;
static Container gMovingContainers[2];
static Container gStaticContainer;
static PropertyAnimation* gPusherAnimation;
static PropertyAnimation* gPusheeAnimation;


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

	return layer;
}


static void createContainer(
	Container* ioContainer)
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

	ioContainer->layer = layer_create(frame);
	layer_add_child(window_get_root_layer(gMainWindow), ioContainer->layer);

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


static void displayValue(
	unsigned short value,
	unsigned short group,
	bool showLeadingZero)
{
	value = value % 100; // Maximum of two digits per row.

	// Column order is: | Column 0 | Column 1 |
	// (We process the columns in reverse order because that makes
	// extracting the digits from the value easier.)
	for (int col = 1; col >= 0; col--) {
			// default to -1 to hide the digit if it's a leading 0
		int digit = -1;
		int slotIndex = (group * 2) + col;

		if (!((value == 0) && (col == 0) && !showLeadingZero)) {
			digit = value % 10;
		}

		setContainerDigit(&gStaticContainer, slotIndex, digit);

		value /= 10;
	}
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
	struct tm *tickTime)
{
	unsigned short minutes = tickTime->tm_min;

	displayValue(getDisplayHour(tickTime->tm_hour), 0, false);

	setContainerDigit(&gStaticContainer, 2, minutes / 10);
	setContainerDigit(&gMovingContainers[0], 3, minutes % 10);
	setContainerDigit(&gMovingContainers[1], 3, (minutes + 1) % 10);

	animation_schedule((Animation*) gPusherAnimation);
	animation_schedule((Animation*) gPusheeAnimation);
}


static void onTick(
	struct tm *tickTime,
	TimeUnits unitsChanged)
{
	displayTime(tickTime);
}


void initAnimations(void)
{
	gPusherAnimation = property_animation_create_layer_frame(gMovingContainers[1].layer,
		&PusherStartFrame, &PusherEndFrame);
	animation_set_duration((Animation*) gPusherAnimation, AnimationMS);
	animation_set_curve((Animation*) gPusherAnimation, AnimationCurveLinear);

	gPusheeAnimation = property_animation_create_layer_frame(gMovingContainers[0].layer,
		&PusherEndFrame, &PusheeEndFrame);
	animation_set_duration((Animation*) gPusheeAnimation, AnimationMS);
	animation_set_curve((Animation*) gPusheeAnimation, AnimationCurveLinear);
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

	createContainer(&gStaticContainer);
	createContainer(&gMovingContainers[0]);
	createContainer(&gMovingContainers[1]);

	initAnimations();

		// load the colon image
	gColonLayer = createBitmapLayer(TimeCharX[ColonLayerIndex],
		TimeCharY[ColonLayerIndex], TimeCharW[ColonLayerIndex], TimeCharH[ColonLayerIndex],
		window_get_root_layer(gMainWindow));
	bitmap_layer_set_bitmap(gColonLayer, gImages[ColonImageIndex]);

		// Avoids a blank screen on watch start.
	time_t now = time(NULL);
	struct tm *tickTime = localtime(&now);
	displayTime(tickTime);

	tick_timer_service_subscribe(MINUTE_UNIT, onTick);
}


void deinit(void)
{
	layer_remove_from_parent(bitmap_layer_get_layer(gColonLayer));
	bitmap_layer_destroy(gColonLayer);

	destroyContainer(&gStaticContainer);
	destroyContainer(&gMovingContainers[0]);
	destroyContainer(&gMovingContainers[1]);

	property_animation_destroy(gPusherAnimation);
	property_animation_destroy(gPusheeAnimation);

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
