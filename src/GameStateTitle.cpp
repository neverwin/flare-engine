/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2014 Henrik Andersson
Copyright © 2012-2016 Justin Jacobs

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "GameStateConfigBase.h"
#include "GameStateConfigDesktop.h"
#include "GameStateCutscene.h"
#include "GameStateLoad.h"
#include "GameStateTitle.h"
#include "InputState.h"
#include "MessageEngine.h"
#include "Platform.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "SoundManager.h"
#include "WidgetButton.h"
#include "WidgetLabel.h"
#include "WidgetScrollBox.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"
#include "Version.h"

GameStateTitle::GameStateTitle()
	: GameState()
	, logo(NULL)
	, align_logo(ALIGN_CENTER)
	, exit_game(false)
	, load_game(false)
{

	// set up buttons
	button_play = new WidgetButton();
	button_exit = new WidgetButton();
	button_cfg = new WidgetButton();
	button_credits = new WidgetButton();

	FileParser infile;
	// @CLASS GameStateTitle|Description of menus/gametitle.txt
	if (infile.open("menus/gametitle.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			// @ATTR logo|filename, int, int, alignment : Image file, X, Y, Alignment|Filename and position of the main logo image.
			if (infile.key == "logo") {
				Image *graphics = render_device->loadImage(popFirstString(infile.val), RenderDevice::ERROR_NONE);
				if (graphics) {
 				    logo = graphics->createSprite();
					graphics->unref();

					pos_logo.x = popFirstInt(infile.val);
					pos_logo.y = popFirstInt(infile.val);
					align_logo = parse_alignment(popFirstString(infile.val));
				}
			}
			// @ATTR play_pos|int, int, alignment : X, Y, Alignment|Position of the "Play Game" button.
			else if (infile.key == "play_pos") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				ALIGNMENT a = parse_alignment(popFirstString(infile.val));
				button_play->setBasePos(x, y, a);
			}
			// @ATTR config_pos|int, int, alignment : X, Y, Alignment|Position of the "Configuration" button.
			else if (infile.key == "config_pos") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				ALIGNMENT a = parse_alignment(popFirstString(infile.val));
				button_cfg->setBasePos(x, y, a);
			}
			// @ATTR credits_pos|int, int, alignment : X, Y, Alignment|Position of the "Credits" button.
			else if (infile.key == "credits_pos") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				ALIGNMENT a = parse_alignment(popFirstString(infile.val));
				button_credits->setBasePos(x, y, a);
			}
			// @ATTR exit_pos|int, int, alignment : X, Y, Alignment|Position of the "Exit Game" button.
			else if (infile.key == "exit_pos") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				ALIGNMENT a = parse_alignment(popFirstString(infile.val));
				button_exit->setBasePos(x, y, a);
			}
			else {
				infile.error("GameStateTitle: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	button_play->label = msg->get("Play Game");
	if (!eset->gameplay.enable_playgame) {
		button_play->enabled = false;
		button_play->tooltip = msg->get("Enable a core mod to continue");
	}
	button_play->refresh();

	button_cfg->label = msg->get("Configuration");
	button_cfg->refresh();

	button_credits->label = msg->get("Credits");
	button_credits->refresh();

	button_exit->label = msg->get("Exit Game");
	button_exit->refresh();

	// set up labels
	label_version = new WidgetLabel();
	label_version->set(0, 0, FontEngine::JUSTIFY_RIGHT, VALIGN_TOP, getVersionString(), font->getColor("menu_normal"));

	// Setup tab order
	tablist.add(button_play);
	tablist.add(button_cfg);
	tablist.add(button_credits);
	tablist.add(button_exit);

	refreshWidgets();

	if (eset->gameplay.enable_playgame && !LOAD_SLOT.empty()) {
		showLoading();
		setRequestedGameState(new GameStateLoad());
	}

	render_device->setBackgroundColor(Color(0,0,0,0));
}

void GameStateTitle::logic() {
	if (inpt->window_resized)
		refreshWidgets();

	button_play->enabled = eset->gameplay.enable_playgame;

	snd->logic(FPoint(0,0));

	if(inpt->pressing[Input::CANCEL] && !inpt->lock[Input::CANCEL]) {
		inpt->lock[Input::CANCEL] = true;
		exitRequested = true;
	}

	tablist.logic(true);

	if (button_play->checkClick()) {
		showLoading();
		setRequestedGameState(new GameStateLoad());
	}
	else if (button_cfg->checkClick()) {
		showLoading();
		if (PLATFORM.config_menu_type == Platform::CONFIG_MENU_TYPE_DESKTOP_NO_VIDEO)
			setRequestedGameState(new GameStateConfigDesktop(!GameStateConfigDesktop::ENABLE_VIDEO_TAB));
		else if (PLATFORM.config_menu_type == Platform::CONFIG_MENU_TYPE_DESKTOP)
			setRequestedGameState(new GameStateConfigDesktop(GameStateConfigDesktop::ENABLE_VIDEO_TAB));
		else
			setRequestedGameState(new GameStateConfigBase(GameStateConfigBase::DO_INIT));
	}
	else if (button_credits->checkClick()) {
		showLoading();
		GameStateTitle *title = new GameStateTitle();
		GameStateCutscene *credits = new GameStateCutscene(title);

		if (!credits->load("cutscenes/credits.txt")) {
			delete credits;
			delete title;
		}
		else {
			setRequestedGameState(credits);
		}
	}
	else if (PLATFORM.has_exit_button && button_exit->checkClick()) {
		exitRequested = true;
	}
}

void GameStateTitle::refreshWidgets() {
	if (logo) {
		Rect r;
		r.x = pos_logo.x;
		r.y = pos_logo.y;
		r.w = logo->getGraphicsWidth();
		r.h = logo->getGraphicsHeight();
		alignToScreenEdge(align_logo, &r);
		logo->setDestX(r.x);
		logo->setDestY(r.y);
	}

	button_play->setPos();
	button_cfg->setPos();
	button_credits->setPos();
	button_exit->setPos();

	label_version->setPos(VIEW_W, 0);
}

void GameStateTitle::render() {
	// display logo
	render_device->render(logo);

	// display buttons
	button_play->render();
	button_cfg->render();
	button_credits->render();

	if (PLATFORM.has_exit_button)
		button_exit->render();

	// version number
	label_version->render();
}

GameStateTitle::~GameStateTitle() {
	if (logo) delete logo;
	delete button_play;
	delete button_cfg;
	delete button_credits;
	delete button_exit;
	delete label_version;
}
