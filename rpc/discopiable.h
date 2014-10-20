#pragma once

#define DISALLOW_EVIL_LIBRPC_COPY_AND_ASSIGN(type)\
    type(type const&);\
    type& operator=(type const&)

