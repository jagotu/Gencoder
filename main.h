#pragma once

#include "document.h"
#include "transform.h"

Document& get_current_document();
std::string get_current_filename();

void apply_transform(const Transform* ts);

void run_editor();
void save_current(std::string filename);

bool has_parent();
void select_part();
void ret_to_parent();
void reenc();

