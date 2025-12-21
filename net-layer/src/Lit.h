#ifndef WEBC_H_
#define WEBC_H_
#include "DD_JSON.h"

void WC_SetStateSize(size_t size);

// Widgets
//----------
enum WC_WidgetKind {
    WC_WidgetKind_Undefined,
    WC_WidgetKind_Button,
    WC_WidgetKind_Input,
};

typedef JS_JSON* WC_Element;

WC_Element WC_Container();
WC_Element WC_PushContainer();
WC_Element WC_PopContainer();

bool WC_Button(const char* label);
bool WC_Button(WC_Element jContainer, const char* label);

#endif // WEBC_H_
