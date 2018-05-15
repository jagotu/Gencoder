#include "gui.h"

#include "main.h"
#include "transform.h"
#include <string>
#include <algorithm>
#include <curses.h>
#include <clocale>

#define ctrl(x)           ((x) & 0x1f)

namespace gui
{
	WINDOW* main = NULL;
	WINDOW* menu = NULL;
	WINDOW* currmenuw = NULL;

	std::map < TransformType, std::vector<std::unique_ptr<Transform>>> availableTransforms;

	//Stores information of the TransformType menu that is currently visible
	TransformType opened_menu = NoneTransformType;

	int height, width;

	//Maps Fkey to a TransformType menu
	std::map<char, TransformType> menus_keymap;
	//Maps Fkey to the menu's starting x position
	std::map<char, int> menus_position;

	//Maps key presses to Transforms in the currently opened TransformType menu
	std::map<char, const Transform*> opened_menu_keymap;

	//Set when error is currently visible
	bool in_error = false;

	//Local function declarations
	std::string get_input(const std::string & message);
	void register_transforms();
	void draw_menu();
	void open_menu(char which);
	void close_menu();

	//The currently higlighted part of the multipart document
	size_t multipart_index = 0;
	//The starting offest of the multipart document
	size_t multipart_view_start = 0;

	void draw_multipart()
	{
		MultipartDocument& doc = dynamic_cast<MultipartDocument&>(get_current_document());
		while (multipart_index >= multipart_view_start + height - 1)
		{
			multipart_view_start++;
		}
		while (multipart_index < multipart_view_start)
		{
			multipart_view_start--;
		}
		
		for (size_t i = multipart_view_start; i < std::min(multipart_view_start + height - 1, doc.data.size()); i++)
		{
			std::string s;

			size_t lentoprint = std::min(doc.data[i].first.size(), (size_t)(width - 1));
			for (size_t j = 0; j < lentoprint; j++)
			{
				utf8::append(doc.data[i].first[j], std::back_inserter(s));
			}

			if (width - lentoprint > 3)
			{
				s += ": ";
				s += doc.data[i].second->generate_preview(width - lentoprint - 2, 1);
			}
			else {
				s += "\n";
			}

			if (i == multipart_index)
			{
				wattron(main, A_REVERSE);
			}

			waddstr(main, s.c_str());

			wattroff(main, A_REVERSE);

		}
	}


	void redraw()
	{
		wclear(main);
		wclear(menu);
		if (get_current_document().get_type() != MultipartDocumentType)
		{
			multipart_index = 0;
			std::string preview = get_current_document().generate_preview(width, height - 1);
			waddstr(main, preview.c_str());
		}
		else {
			draw_multipart();
		}

		draw_menu();

		wmove(main, 0, 0);
		wmove(menu, 0, 0);
		wrefresh(main);
		wrefresh(menu);

	}

	void close_menu()
	{
		if (opened_menu != NoneTransformType)
		{
			delwin(currmenuw);
			opened_menu = NoneTransformType;
			currmenuw = NULL;
		}
	}

	void show_error(const char* err)
	{
		in_error = true;
		close_menu();
		wclear(main);
		waddstr(main, "Error:\n");
		waddstr(main, err);
		waddstr(main, "\nPress any key to continue.");
		wrefresh(main);
	}

	void start()
	{
		register_transforms();
		
		std::setlocale(LC_ALL, "en_US.UTF-8"); //necessary to get UTF-8 support

		initscr();
		raw();    //Disable line buffering
		noecho(); //Don't echo user input
		curs_set(FALSE);


		getmaxyx(stdscr, height, width);

		main = newwin(height, width, 0, 0);
		menu = newwin(1, width, height - 1, 0);
		keypad(main, TRUE); //Enable special keys
		wattron(menu, A_REVERSE);

		redraw();
		while (true)
		{
			int ch = wgetch(main);

			//If we are displaying an error, accept any key
			if (in_error)
			{
				in_error = false;
				redraw();
				continue;
			}

			//If a menu is opened, attempt to call the relevant menu item
			if (opened_menu != NoneTransformType && ch < 128)
			{
				auto it = opened_menu_keymap.find((char)ch);
				if (it != opened_menu_keymap.end())
				{
					try {
						apply_transform(it->second);
					}
					catch (const TransformError& e)
					{
						show_error(e.what());
						continue;
					}

					close_menu();
					redraw();
					continue;
				}
			}

			//If we are showing a multipart document, handle arrow keys and enter
			if (get_current_document().get_type() == MultipartDocumentType && multipart_index >= 0)
			{
				switch (ch)
				{
				case KEY_UP:
					if (multipart_index > 0) multipart_index--;
					redraw();
					break;
				case KEY_DOWN:
					if ((size_t)multipart_index < dynamic_cast<MultipartDocument&>(get_current_document()).data.size() - 1) multipart_index++;
					redraw();
					break;
				case '\n':
				case '\r':
				case KEY_ENTER:
					select_part();
					multipart_index = 0;
					multipart_view_start = 0;
					redraw();
					break;
				}
			}

			switch (ch)
			{
			case KEY_RESIZE:
				resize_term(0, 0);
				getmaxyx(stdscr, height, width);
				wresize(main, height - 1, width);
				wresize(menu, 1, width);
				mvwin(menu, height - 1, 0);
				close_menu();
				redraw();
				break;
			case KEY_F(1):
				//EDIT
				try {
					run_editor();
					//restore our terminal settings
					raw();    //Disable line buffering
					noecho(); //Don't echo user input
					curs_set(FALSE);
					keypad(main, TRUE); //reenable special keys
					redraw();
				}
				catch (const std::exception& e)
				{
					show_error(e.what());
					continue;
				}

				break;
			case KEY_F(2):
				//SAVE
				try {
					std::string in;
					if (get_current_filename().empty())
					{
						in = get_input("Save to: ");
					}
					else {
						in = get_input("Save to (leave empty for " + get_current_filename() + "):");
					}
					save_current(in);
					redraw();
				}
				catch (const std::exception& e)
				{
					show_error(e.what());
					continue;
				}
				break;
			case KEY_F(3):
				//REENC
				try {
					reenc();
				}
				catch (const std::exception& e)
				{
					show_error(e.what());
					continue;
				}
				redraw();
				break;
			case KEY_F(4):
				open_menu(4);
				break;
			case KEY_F(5):
				open_menu(5);
				break;
			case KEY_F(6):
				open_menu(6);
				break;
			case KEY_F(7):
				open_menu(7);
				break;
			case KEY_F(8):
				open_menu(8);
				break; 
			case KEY_F(9):
				open_menu(9);
				break;
			case ctrl('c'):
				goto end;
				break;
			case '\b':
			case KEY_BACKSPACE:
				try {
					ret_to_parent();
				}
				catch (const std::exception& e)
				{
					show_error(e.what());
					continue;
				}
				redraw();
				break;
			default:
				close_menu();
				redraw();
				break;
			}
		}
	end:
		endwin();
	}

	size_t get_highlighted_index()
	{
		return multipart_index;
	}

	void open_menu(char which)
	{
		auto it = menus_keymap.find(which);
		if (it != menus_keymap.end())
		{
			close_menu();
			opened_menu = it->second;
			redraw();
			size_t maxlength = 0;
			size_t count = 0;
			for (auto&& a : availableTransforms[opened_menu])
			{
				if (a->accepts_type(get_current_document().get_type()))
				{
					if (a->get_description().length() > maxlength)
					{
						maxlength = a->get_description().length();
					}
					count++;
				}
			}
			

			currmenuw = newwin(count, maxlength + 4, height - count - 1, menus_position[opened_menu]);
			wattron(currmenuw, A_REVERSE);
			opened_menu_keymap.clear();
			char i = '1';
			for (auto&& a : availableTransforms[opened_menu])
			{
				if (a->accepts_type(get_current_document().get_type()))
				{
					wprintw(currmenuw, " %c %s", i, a->get_description().c_str());
					int x = getcurx(currmenuw);
					for (int j = x; j < (int)(maxlength + 4); j++)
					{
						waddch(currmenuw, ' ');
					}
					opened_menu_keymap[i] = a.get();
					i++;
				}

			}
			if (count != 0)
			{
				wrefresh(currmenuw);
			}
			
		}
	}

	void register_transforms()
	{
		availableTransforms[DecodeTransformType].push_back(std::make_unique<Base64Decode>());
		availableTransforms[DecodeTransformType].push_back(std::make_unique<UTF8Decode>());
		availableTransforms[DecodeTransformType].push_back(std::make_unique<xwwwformurlencodedDecode>());
		availableTransforms[EncodeTransformType].push_back(std::make_unique<Base64Encode>());
		availableTransforms[EncodeTransformType].push_back(std::make_unique<UTF8Encode>());
		availableTransforms[EncodeTransformType].push_back(std::make_unique<xwwwformurlencodedEncode>());

		menus_keymap[4] = DecodeTransformType;
		menus_keymap[5] = EncodeTransformType;
	}

	void draw_menu()
	{
		int x;

		if (has_parent())
		{
			waddstr(menu, "< ");
		}
		else {
			waddstr(menu, "  ");
		}
		switch (get_current_document().get_type())
		{
		case UnicodeDocumentType:
			waddstr(menu, " (unicode)  ");
			break;
		case OctetDocumentType:
			waddstr(menu, "  (octet)   ");
			break;
		case MultipartDocumentType:
			waddstr(menu, "(multipart) ");
			break;
		}

		waddstr(menu, "| F1 EDIT | F2 SAVE | F3 REENC ");
		for (auto&& a : menus_keymap)
		{
			if (a.second == opened_menu)
			{
				wattroff(menu, A_REVERSE);
			}
			x = getcurx(menu);
			menus_position[a.second] = x + 1;
			waddstr(menu, "| F");
			wprintw(menu, "%d", a.first);
			waddstr(menu, " ");
			switch (a.second)
			{
			case EncodeTransformType:
				waddstr(menu, "ENCODE");
				break;
			case DecodeTransformType:
				waddstr(menu, "DECODE");
				break;
			default:
				waddstr(menu, "ERROR");
				break;
			}
			waddstr(menu, " ");
			wattron(menu, A_REVERSE);
		}

		x = getcurx(menu);
		for (int i = x; i < width; i++)
		{
			waddch(menu, ' ');
		}

	}

	std::string get_input(const std::string& message)
	{
		echo();
		curs_set(TRUE);
		//Hardcoded size buffer. Sorry jako.
		//At least it's only local and we only use it locally with wgetnstr which *should* be secure
		char buffer[1024];

		wclear(main);
		waddstr(main, message.c_str());
		wrefresh(main);
		wgetnstr(main, buffer, 1020); //4 bytes larger just in case ncurses has some nasty off-by-ones

		buffer[1023] = 0; //More overflow protection

		noecho();
		curs_set(FALSE);
		return std::string(buffer);
	}

}
