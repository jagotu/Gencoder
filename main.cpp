#include "main.h"

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <sstream>
#include <stack>

#include <cstdlib>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif


#include "gui.h"
#include "document.h"
#include "transform.h"


//local functions declarations
void usage(const char * arg0);
bool is_valid_utf8_file(const std::istream& file_name);

std::stack<const Transform*> transformation_history;

//Contains a hierarchy of current document's multipart parents
//The std::vector<utf8::uint32_t> is the string key under which the current document was known to the parent
std::stack < std::pair<std::unique_ptr<Document>, std::vector<utf8::uint32_t>>> parents;

std::unique_ptr<Document> current;
std::string current_filename = "";

//Implements pushing a (potentially modified) document back to its multipart parent
class PushbackPart : public Transform {
public:
	bool accepts_type(DocType type) const final { return true; };
	bool reverse_transform() const final { return false; };
	std::unique_ptr<Transform> get_reverse_transform() const final { throw std::logic_error("Can't reverse pushback"); };
	std::unique_ptr<Document> transform(const Document& input) const final {
		std::unique_ptr<Document> doc = std::move(parents.top().first);
		MultipartDocument& multidoc = dynamic_cast<MultipartDocument &>(*doc);
		multidoc.data.push_back(make_pair(parents.top().second, std::move(current)));
		parents.pop();
		return doc;
	};
	const std::string get_description() const final { return "PushbackPart"; };
};

//Implements getting a single document from a multipart document
class SelectPart : public Transform {
public:
	bool accepts_type(DocType type) const final { return type == MultipartDocumentType; };
	bool reverse_transform() const final { return true; };
	std::unique_ptr<Transform> get_reverse_transform() const final { return std::make_unique<PushbackPart>(); };
	std::unique_ptr<Document> transform(const Document& input) const final {
		if (current->get_type() != MultipartDocumentType)
		{
			throw TransformError("SelectPart only accepts multipart documents");
		}
		MultipartDocument& multidoc = dynamic_cast<MultipartDocument&>(*current);
		size_t index = gui::get_highlighted_index();
		std::unique_ptr<Document> selected = std::move(multidoc.data[index].second);
		parents.push(make_pair(std::move(current), std::move(multidoc.data[index].first)));
		multidoc.data.erase(multidoc.data.begin() + index);
		return selected;
	};
	const std::string get_description() const final { return "SelectPart"; };
};

PushbackPart pushback_transform;
SelectPart select_transform;

int main(int argc, char* argv[])
{
	if (argc > 2)
	{
		usage(argv[0]);
		return 0;
	}

	if (argc == 1)
	{
		current = std::make_unique<UnicodeDocument>();
	}
	else {
		current_filename = std::string(argv[1]);
#ifdef __linux__ 
		if (current_filename == "-")
		{
			current = std::make_unique<OctetDocument>();
			current->do_import(std::cin);

			//Hack to reopen stdin even though we were piped
			//Copied from vim source code, so it should be pretty good
			close(0);
			int dummy = dup(2);
			if (dummy != 0)
			{
				std::cerr << "Reading from std-in not supported on this platform";
				return 1;
			}
			current_filename = "";
			goto start;
		}
#endif
		std::ifstream input(current_filename, std::ios::binary);
		if (!input)
		{
			std::cerr << "Failed to open file " << argv[1] << "!";
			return 1;
		}
		if (is_valid_utf8_file(input))
		{
			current = std::make_unique<UnicodeDocument>();
		}
		else {
			current = std::make_unique<OctetDocument>();
		}
		input.seekg(0, std::ios_base::beg);
		current->do_import(input);
		input.close();
	}
start:
	gui::start();
	return 0;
}

void usage(const char* arg0)
{
	std::cout << "Usage: " << arg0 << " [filename]" << std::endl;
}

Document& get_current_document()
{
	return *current;
}

void run_editor()
{
	char* editor = getenv("EDITOR");
	if (editor == NULL)
	{
		throw std::logic_error("No editor specified. Try setting the EDITOR environment variable.");
	}

	std::ostringstream tmpname;

	tmpname << ".gencoder." << getpid();

	std::ostringstream ss;
	ss << editor << " " << tmpname.str();

	std::ofstream out(tmpname.str(), std::ostream::out | std::ostream::binary);
	if (!out)
	{
		throw std::logic_error("Failed to create TMP file");
	}
	current->do_export(out);
	out.close();

	if (!system(ss.str().c_str()))
	{
	};

	std::ifstream in(tmpname.str(), std::ios::binary);
	if (!in)
	{
		throw std::logic_error("Failed to read back TMP file");
	}
	current->do_import(in);
	in.close();

	std::remove(tmpname.str().c_str());

}

std::string get_current_filename()
{
	return current_filename;
}

void apply_transform(const Transform* ts)
{
	current = ts->transform(*current);
	transformation_history.push(ts);
}

bool is_valid_utf8_file(const std::istream& in)
{
	std::istreambuf_iterator<char> it(in.rdbuf());
	std::istreambuf_iterator<char> eos;

	return utf8::is_valid(it, eos);
}

void save_current(std::string filename)
{
	if (filename.empty() && !current_filename.empty())
	{
		filename = current_filename;
	}

	std::ofstream out(filename, std::ostream::out | std::ostream::binary);
	if (!out)
	{
		throw std::logic_error("Failed to create output file");
	}
	current->do_export(out);
	out.close();
}

bool has_parent()
{
	return !parents.empty();
}

//Selects a single part from a multipart document based on UI
void select_part()
{
	apply_transform(&select_transform);
}

void pop_history()
{
	if (!transformation_history.empty())
	{
		if (transformation_history.top()->reverse_transform())
		{
			std::unique_ptr<Transform> t = transformation_history.top()->get_reverse_transform();
			current = t->transform(*current);
		}
		transformation_history.pop();
	}
}

void ret_to_parent()
{
	if (!parents.empty())
	{
		size_t startsize = parents.size();
		while (parents.size() == startsize)
		{
			pop_history();
		}
	}
}

void reenc()
{
	while (!transformation_history.empty())
	{
		pop_history();
	}
}