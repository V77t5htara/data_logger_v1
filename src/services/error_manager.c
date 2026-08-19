#include "error_manager.h"

static volatile uint32_t errors;

void ErrorManager_Set(uint32_t flag) { errors |= flag; }
void ErrorManager_Clear(uint32_t flag) { errors &= ~flag; }
uint32_t ErrorManager_Get(void) { return errors; }
uint8_t ErrorManager_Has(uint32_t flag) { return (errors & flag) != 0; }
