// Copyright 2016-2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef RESOURCE_TABLE_H_
#define RESOURCE_TABLE_H_

#include "control.h"

#pragma once

void resource_table_init(control_resid_t reserved_id);

int resource_table_add(const control_resid_t resources[MAX_RESOURCES_PER_INTERFACE],
                       unsigned num_resources, unsigned char ifnum);

int resource_table_search(control_resid_t resid, unsigned char &ifnum);

#endif // RESOURCE_TABLE_H_
