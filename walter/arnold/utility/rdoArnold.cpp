#include "rdoArnold.h"

#if AI_VERSION_ARCH_NUM == 5

/* LEGACY
void RdoAiNodeSetVec(AtNode* node, const char* name, float x, float y, float z)
{
    AiNodeSetVec(node, name, x, y, z);
}
void RdoAiNodeSetVec2(AtNode* node, const char* name, float x, float y)
{
    AiNodeSetVec2(node, name, x, y);
}
*/

/* LEGACY
AtVector RdoAiNodeGetVec(const AtNode* node, const char* name)
{
    return AiNodeGetVec(node, name);
}
AtVector2 RdoAiNodeGetVec2(const AtNode* node, const char* name)
{
    return AiNodeGetVec2(node, name);
}
*/

/*
const char* RdoAiNodeGetStr(const AtNode* node, const char* name)
{
    return AiNodeGetStr(node, name).c_str();
}
*/

/* LEGACY
unsigned int RdoAiArrayGetNumElements(const AtArray* array)
{
    return AiArrayGetNumElements(array);
}
*/

/* LEGACY
unsigned char RdoAiArrayGetNumKeys(const AtArray* array)
{
    return AiArrayGetNumKeys(array);
}
*/

/* LEGACY
unsigned char RdoAiArrayGetType(const AtArray* array)
{
    return AiArrayGetType(array);
}
*/

/* LEGACY
AtVector RdoAiArrayGetVec(const AtArray* array, unsigned int i)
{
    return AiArrayGetVec(array, i);
}
AtVector2 RdoAiArrayGetVec2(const AtArray* array, unsigned int i)
{
    return AiArrayGetVec2(array, i);
}
*/

/*
const char* RdoAiArrayGetStr(const AtArray* array, unsigned int i)
{
    return AiArrayGetStr(array, i).c_str();
}
*/

/*
const uint8_t& RdoBYTE(const AtParamValue* pValue)
{
    return pValue->BYTE();
}
const int& RdoINT(const AtParamValue* pValue)
{
    return pValue->INT();
}
const unsigned int& RdoUINT(const AtParamValue* pValue)
{
    return pValue->UINT();
}
const bool& RdoBOOL(const AtParamValue* pValue)
{
    return pValue->BOOL();
}
const float& RdoFLT(const AtParamValue* pValue)
{
    return pValue->FLT();
}
const AtRGB& RdoRGB(const AtParamValue* pValue)
{
    return pValue->RGB();
}
const AtRGBA& RdoRGBA(const AtParamValue* pValue)
{
    return pValue->RGBA();
}
const AtVector& RdoVEC(const AtParamValue* pValue)
{
    return pValue->VEC();
}
const AtVector2& RdoVEC2(const AtParamValue* pValue)
{
    return pValue->VEC2();
}
const char* RdoSTR(const AtParamValue* pValue)
{
    return pValue->STR();
}
*/

#endif  // AI_VERSION
