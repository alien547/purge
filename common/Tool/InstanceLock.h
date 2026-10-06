#ifndef INSTANCE_LOCK_H
#define INSTANCE_LOCK_H

#include <string>

bool acquire_single_instance_lock(const std::string& name);

#endif

