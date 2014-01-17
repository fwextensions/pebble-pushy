/*
	- create static layer, and two layers in array?
	- one pushed layer, one pusher layer
	- load all bitmaps first?  then point layer at appropriate bitmap
	- change bitmap when time changes
	- reparent digit layers to static, pushed and pusher layers
*/


#include <pebble.h>


#define ImageCount 11
#define ColonImageIndex 10
#define TimeCharCount 5
#define DigitSlotCount 4
#define DigitLayerCount 5
#define EmptySlot -1
#define DigitY 65
#define DigitW 30
#define DigitH 38


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


static Window *mainWindow;
static GBitmap *gImages[ImageCount];
static BitmapLayer *gDigitLayers[DigitLayerCount];


static void unloadDigit(
	int slotIndex)
{
	if (gDigitLayers[slotIndex]) {
		bitmap_layer_set_bitmap(gDigitLayers[slotIndex], NULL);
	}
}


static BitmapLayer* createBitmapLayer(
	int inX,
	int inY,
	int inWidth,
	int inHeight)
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
	BitmapLayer *layer = bitmap_layer_create(frame);
	Layer *windowLayer = window_get_root_layer(mainWindow);
	layer_add_child(windowLayer, bitmap_layer_get_layer(layer));

	return layer;
}


static void loadDigit(
	int slotIndex,
	int digit)
{
	if ((slotIndex < 0) || (slotIndex >= DigitSlotCount)) {
		return;
	}

	if ((digit < 0) || (digit > 9)) {
		return;
	}

		// load the image into the layer
	bitmap_layer_set_bitmap(gDigitLayers[slotIndex], gImages[digit]);
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
		int slotIndex = (group * 2) + col;

		if (!((value == 0) && (col == 0) && !showLeadingZero)) {
			loadDigit(slotIndex, value % 10);
		} else {
				// remove the leading digit, since this is a single-digit hour
			unloadDigit(slotIndex);
		}

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
	displayValue(getDisplayHour(tickTime->tm_hour), 0, false);
	displayValue(tickTime->tm_min, 1, true);
}


static void onTick(
	struct tm *tickTime,
	TimeUnits unitsChanged)
{
	displayTime(tickTime);
}


void init(void)
{
	mainWindow = window_create();
	window_stack_push(mainWindow, true);
	window_set_background_color(mainWindow, GColorBlack);

		// preload all the images
	for (int i = 0; i < ImageCount; i++) {
		gImages[i] = gbitmap_create_with_resource(ImageResourceIDs[i]);
	}

	for (int i = 0; i < DigitLayerCount; i++) {
		gDigitLayers[i] = createBitmapLayer(TimeCharX[i], TimeCharY[i], TimeCharW[i], TimeCharH[i]);
	}

		// load the colon image
	bitmap_layer_set_bitmap(gDigitLayers[4], gImages[ColonImageIndex]);

		// Avoids a blank screen on watch start.
	time_t now = time(NULL);
	struct tm *tickTime = localtime(&now);
	displayTime(tickTime);

	tick_timer_service_subscribe(MINUTE_UNIT, onTick);
}


void deinit(void)
{
	for (int i = 0; i < DigitLayerCount; i++) {
		layer_remove_from_parent(bitmap_layer_get_layer(gDigitLayers[i]));
		bitmap_layer_destroy(gDigitLayers[i]);
	}

	for (int i = 0; i < ImageCount; i++) {
		gbitmap_destroy(gImages[i]);
	}

	tick_timer_service_unsubscribe();
	window_destroy(mainWindow);
}


int main(void)
{
	init();
	app_event_loop();
	deinit();
}
