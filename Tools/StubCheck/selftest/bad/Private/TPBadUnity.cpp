#include "TPBadTypes.h"
static void TPHelper() {}
void TPUseHelper() { TPHelper(); FString S(TEXT("Привет")); }
// ОЖИДАЕТСЯ: а файл без UTF-8 BOM
