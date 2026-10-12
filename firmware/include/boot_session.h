#pragma once
#include "gateway_journal.h"
/* Fail closed on corruption, exhausted counter or unsuccessful commit. */
bool osg_boot_session_begin(const osg_journal_io_t *,uint32_t *session);
